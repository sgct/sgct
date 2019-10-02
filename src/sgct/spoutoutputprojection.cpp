/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/spoutoutputprojection.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/settings.h>
#include <sgct/window.h>
#include <sgct/helpers/stringfunctions.h>
#include <sgct/shaders/internalfisheyeshaders.h>
#include <sgct/shaders/internalfisheyeshaders_cubic.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>

#ifdef SGCT_HAS_SPOUT
#define WIN32_LEAN_AND_MEAN
#include <SpoutLibrary.h>
#endif

namespace sgct::core {

//#define DebugCubemap

SpoutOutputProjection::~SpoutOutputProjection() {
    for (int i = 0; i < NFaces; i++) {
        if (mSpout[i].handle) {
#ifdef SGCT_HAS_SPOUT
            reinterpret_cast<SPOUTHANDLE>(mSpout[i].handle)->ReleaseSender();
            reinterpret_cast<SPOUTHANDLE>(mSpout[i].handle)->Release();
#endif
        }
    }

    if (mappingHandle) {
#ifdef SGCT_HAS_SPOUT
        reinterpret_cast<SPOUTHANDLE>(mappingHandle)->ReleaseSender();
        reinterpret_cast<SPOUTHANDLE>(mappingHandle)->Release();
#endif
    }

    glDeleteTextures(1, &mappingTexture);

    if (mSpoutFBO) {
        mSpoutFBO->destroy();
        mSpoutFBO = nullptr;
    }
}

void SpoutOutputProjection::update(glm::vec2) {
    mVerts[0] = 0.f;
    mVerts[1] = 0.f;
    mVerts[2] = -1.f;
    mVerts[3] = -1.f;
    mVerts[4] = -1.f;

    mVerts[5] = 0.f;
    mVerts[6] = 1.f;
    mVerts[7] = -1.f;
    mVerts[8] = 1.f;
    mVerts[9] = -1.f;

    mVerts[10] = 1.f;
    mVerts[11] = 0.f;
    mVerts[12] = 1.f;
    mVerts[13] = -1.f;
    mVerts[14] = -1.f;

    mVerts[15] = 1.f;
    mVerts[16] = 1.f;
    mVerts[17] = 1.f;
    mVerts[18] = 1.f;
    mVerts[19] = -1.f;

    // update VBO
    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);

    GLvoid* PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    memcpy(PositionBuffer, mVerts.data(), 20 * sizeof(float));
    glUnmapBuffer(GL_ARRAY_BUFFER);

    glBindVertexArray(0);
}

void SpoutOutputProjection::render() {
    renderInternal();
}

void SpoutOutputProjection::renderCubemap(size_t* subViewPortIndex) {
    renderCubemapInternal(subViewPortIndex);
}

void SpoutOutputProjection::setSpoutChannels(const bool channels[NFaces]) {
    for (size_t i = 0; i < NFaces; i++) {
        mSpout[i].enabled = channels[i];
    }
}

void SpoutOutputProjection::setSpoutMappingName(std::string name) {
    mappingName = std::move(name);
}

void SpoutOutputProjection::setSpoutMapping(Mapping type) {
    mappingType = type;
}

void SpoutOutputProjection::setSpoutRigOrientation(glm::vec3 orientation) {
    rigOrientation = std::move(orientation);
}

void SpoutOutputProjection::initTextures() {
    NonLinearProjection::initTextures();

    MessageHandler::instance()->print("SpoutOutputProjection initTextures");

    switch (mappingType) {
        case Mapping::Cubemap:
            mappingWidth = mCubemapResolution;
            mappingHeight = mCubemapResolution;

            for (int i = 0; i < NFaces; ++i) {
#ifdef SGCT_HAS_SPOUT
                MessageHandler::instance()->print(
                    "SpoutOutputProjection initTextures %d", i
                );
                if (!mSpout[i].enabled) {
                    continue;
                }
                mSpout[i].handle = GetSpout();
                if (mSpout[i].handle) {
                    SPOUTHANDLE h = reinterpret_cast<SPOUTHANDLE>(mSpout[i].handle);
                    h->CreateSender(CubeMapFaceName[i], mappingWidth, mappingHeight);
                }
#endif
                glGenTextures(1, &mSpout[i].texture);
                glBindTexture(GL_TEXTURE_2D, mSpout[i].texture);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    mTexInternalFormat,
                    mappingWidth,
                    mappingHeight,
                    0,
                    mTexFormat,
                    mTexType,
                    nullptr
                );
            }
            break;
        case Mapping::Equirectangular:
            mappingWidth = mCubemapResolution * 4;
            mappingHeight = mCubemapResolution * 2;
#ifdef SGCT_HAS_SPOUT
            MessageHandler::instance()->print(
                "SpoutOutputProjection initTextures Equirectangular"
            );
            mappingHandle = GetSpout();
            if (mappingHandle) {
                SPOUTHANDLE h = reinterpret_cast<SPOUTHANDLE>(mappingHandle);
                h->CreateSender(mappingName.c_str(), mappingWidth, mappingHeight);
            }
#endif
            glGenTextures(1, &mappingTexture);
            glBindTexture(GL_TEXTURE_2D, mappingTexture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                mTexInternalFormat,
                mappingWidth,
                mappingHeight,
                0,
                mTexFormat,
                mTexType,
                nullptr
            );
            break;
        case Mapping::Fisheye:
            mappingWidth = mCubemapResolution * 2;
            mappingHeight = mCubemapResolution * 2;
#ifdef SGCT_HAS_SPOUT
            MessageHandler::instance()->print(
                "SpoutOutputProjection initTextures Fisheye"
            );
            mappingHandle = GetSpout();
            if (mappingHandle) {
                SPOUTHANDLE h = reinterpret_cast<SPOUTHANDLE>(mappingHandle);
                h->CreateSender(mappingName.c_str(), mappingWidth, mappingHeight);
            }
#endif
            glGenTextures(1, &mappingTexture);
            glBindTexture(GL_TEXTURE_2D, mappingTexture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                mTexInternalFormat,
                mappingWidth,
                mappingHeight,
                0,
                mTexFormat,
                mTexType,
                nullptr
            );
            break;
        }
}

void SpoutOutputProjection::initViewports() {
    enum class CubeFaces { PosX = 0, NegX, PosY, NegY, PosZ, NegZ };

    // radius is needed to calculate the distance to all view planes
    const float radius = 1.f;

    // setup base viewport that will be rotated to create the other cubemap views
    // +Z face
    const glm::vec4 lowerLeftBase(-radius, -radius, radius, 1.f);
    const glm::vec4 upperLeftBase(-radius, radius, radius, 1.f);
    const glm::vec4 upperRightBase(radius, radius, radius, 1.f);

    const glm::mat4 pitchRot = glm::rotate(
        glm::mat4(1.f),
        glm::radians(-rigOrientation.x),
        glm::vec3(0.f, 1.f, 0.f)
    );
    const glm::mat4 yawRot = glm::rotate(
        pitchRot,
        glm::radians(rigOrientation.y),
        glm::vec3(1.f, 0.f, 0.f)
    );
    const glm::mat4 rollRot = glm::rotate(
        yawRot,
        glm::radians(-rigOrientation.z),
        glm::vec3(0.f, 0.f, 1.f)
    );

    // right, left, bottom, top, front, back
    {
        mSubViewports.right.setName("SpoutOutput 0");

        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(-90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );
        upperRight.x = radius;
        mSubViewports.right.setSize(glm::vec2(1.f, 1.f));

        mSubViewports.right.getProjectionPlane().setCoordinateLowerLeft(
            glm::vec3(rotMat * lowerLeft)
        );
        mSubViewports.right.getProjectionPlane().setCoordinateUpperLeft(
            glm::vec3(rotMat * upperLeft)
        );
        mSubViewports.right.getProjectionPlane().setCoordinateUpperRight(
            glm::vec3(rotMat * upperRight)
        );
    }

    // left
    {
        mSubViewports.left.setName("SpoutOutput 1");

        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );
        lowerLeft.x = -radius;
        upperLeft.x = -radius;
        mSubViewports.left.setPos(glm::vec2(0.f, 0.f));
        mSubViewports.left.setSize(glm::vec2(1.f, 1.f));

        mSubViewports.left.getProjectionPlane().setCoordinateLowerLeft(
            glm::vec3(rotMat * lowerLeft)
        );
        mSubViewports.left.getProjectionPlane().setCoordinateUpperLeft(
            glm::vec3(rotMat * upperLeft)
        );
        mSubViewports.left.getProjectionPlane().setCoordinateUpperRight(
            glm::vec3(rotMat * upperRight)
        );
    }

    // bottom
    {
        mSubViewports.bottom.setName("SpoutOutput 2");

        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(-90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );
        lowerLeft.y = -radius;
        mSubViewports.bottom.setPos(glm::vec2(0.f, 0.f));
        mSubViewports.bottom.setSize(glm::vec2(1.f, 1.f));

        mSubViewports.bottom.getProjectionPlane().setCoordinateLowerLeft(
            glm::vec3(rotMat * lowerLeft)
        );
        mSubViewports.bottom.getProjectionPlane().setCoordinateUpperLeft(
            glm::vec3(rotMat * upperLeft)
        );
        mSubViewports.bottom.getProjectionPlane().setCoordinateUpperRight(
            glm::vec3(rotMat * upperRight)
        );
    }

    // top
    {
        mSubViewports.top.setName("SpoutOutput 3");

        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 rotMat = glm::rotate(
            rollRot,
            glm::radians(90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );
        upperLeft.y = radius;
        upperRight.y = radius;
        mSubViewports.top.setSize(glm::vec2(1.f, 1.f));

        mSubViewports.top.getProjectionPlane().setCoordinateLowerLeft(
            glm::vec3(rotMat * lowerLeft)
        );
        mSubViewports.top.getProjectionPlane().setCoordinateUpperLeft(
            glm::vec3(rotMat * upperLeft)
        );
        mSubViewports.top.getProjectionPlane().setCoordinateUpperRight(
            glm::vec3(rotMat * upperRight)
        );
    }

    // front
    {
        mSubViewports.front.setName("SpoutOutput 4");

        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 rotMat = rollRot;

        mSubViewports.front.getProjectionPlane().setCoordinateLowerLeft(
            glm::vec3(rotMat * lowerLeft)
        );
        mSubViewports.front.getProjectionPlane().setCoordinateUpperLeft(
            glm::vec3(rotMat * upperLeft)
        );
        mSubViewports.front.getProjectionPlane().setCoordinateUpperRight(
            glm::vec3(rotMat * upperRight)
        );
    }

    // back
    {
        mSubViewports.back.setName("SpoutOutput 5");

        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 rotMat = glm::rotate(rollRot,
            glm::radians(180.f),
            glm::vec3(0.f, 1.f, 0.f)
        );

        mSubViewports.back.getProjectionPlane().setCoordinateLowerLeft(
            glm::vec3(rotMat * lowerLeft)
        );
        mSubViewports.back.getProjectionPlane().setCoordinateUpperLeft(
            glm::vec3(rotMat * upperLeft)
        );
        mSubViewports.back.getProjectionPlane().setCoordinateUpperRight(
            glm::vec3(rotMat * upperRight)
        );
    }
}

void SpoutOutputProjection::initShaders() {
    // reload shader program if it exists
    if (mShader.isLinked()) {
        mShader.deleteProgram();
    }

    std::string fisheyeFragShader;
    std::string fisheyeVertShader;

    fisheyeVertShader = shaders_fisheye::FisheyeVert;

    if (Settings::instance()->useDepthTexture()) {
        switch (Settings::instance()->getCurrentDrawBufferType()) {
            case Settings::DrawBufferType::Diffuse:
            default:
                fisheyeFragShader = shaders_fisheye::FisheyeFragDepth;
                break;
            case Settings::DrawBufferType::DiffuseNormal:
                fisheyeFragShader = shaders_fisheye::FisheyeFragDepthNormal;
                break;
            case Settings::DrawBufferType::DiffusePosition:
                fisheyeFragShader = shaders_fisheye::FisheyeFragDepthPosition;
                break;
            case Settings::DrawBufferType::DiffuseNormalPosition:
                fisheyeFragShader =
                    shaders_fisheye::FisheyeFragDepthNormalPosition;
                break;
        }
    }
    else {
        //no depth
        switch (Settings::instance()->getCurrentDrawBufferType()) {
            case Settings::DrawBufferType::Diffuse:
            default:
                fisheyeFragShader = shaders_fisheye::FisheyeFrag;
                break;
            case Settings::DrawBufferType::DiffuseNormal:
                fisheyeFragShader = shaders_fisheye::FisheyeFragNormal;
                break;
            case Settings::DrawBufferType::DiffusePosition:
                fisheyeFragShader = shaders_fisheye::FisheyeFragPosition;
                break;
            case Settings::DrawBufferType::DiffuseNormalPosition:
                fisheyeFragShader = shaders_fisheye::FisheyeFragNormalPosition;
                break;
        }
    }

    //depth correction shader only
    if (Settings::instance()->useDepthTexture()) {
        std::string depthCorrFrag = shaders_fisheye::BaseVert;
        std::string depthCorrVert = shaders_fisheye::FisheyeDepthCorrectionFrag;

        //replace glsl version
        helpers::findAndReplace(
            depthCorrFrag,
            "**glsl_version**",
            Engine::instance()->getGLSLVersion()
        );
        bool fragShader = mDepthCorrectionShader.addShaderSrc(
            depthCorrFrag,
            GL_VERTEX_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!fragShader) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Failed to load fisheye depth correction vertex shader\n"
            );
        }

        helpers::findAndReplace(
            depthCorrVert,
            "**glsl_version**",
            Engine::instance()->getGLSLVersion()
        );
        bool vertShader = mDepthCorrectionShader.addShaderSrc(
            depthCorrVert,
            GL_FRAGMENT_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!vertShader) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Failed to load fisheye depth correction fragment shader\n"
            );
        }
    }

    // add functions to shader
    switch (mappingType) {
        case Mapping::Fisheye:
            helpers::findAndReplace(
                fisheyeFragShader,
                "**sample_fun**",
                shaders_fisheye::SampleFun
            );
            break;
        case Mapping::Equirectangular:
            helpers::findAndReplace(
                fisheyeFragShader,
                "**sample_fun**",
                shaders_fisheye::SampleLatlonFun
            );
            break;
        default:
            helpers::findAndReplace(
                fisheyeFragShader,
                "**sample_fun**",
                shaders_fisheye::SampleFun
            );
            break;
    }

    glm::mat4 pitchRot = glm::rotate(
        glm::mat4(1.f),
        glm::radians(rigOrientation.x),
        glm::vec3(0.f, 1.f, 0.f)
    );
    glm::mat4 yawRot = glm::rotate(
        pitchRot,
        glm::radians(rigOrientation.y),
        glm::vec3(1.f, 0.f, 0.f)
    );
    glm::mat4 rollRot = glm::rotate(
        yawRot,
        glm::radians(rigOrientation.z),
        glm::vec3(0.f, 0.f, 1.f)
    );

    std::stringstream ssRot;
    ssRot.precision(5);
    ssRot << "vec3 rotVec = vec3(" <<
        rollRot[0].x << "f*x + " << rollRot[0].y << "f*y + " << rollRot[0].z << "f*z, " <<
        rollRot[1].x << "f*x + " << rollRot[1].y << "f*y + " << rollRot[1].z << "f*z, " <<
        rollRot[2].x << "f*x + " << rollRot[2].y << "f*y + " << rollRot[2].z << "f*z)";

    // replace add correct transform in the fragment shader
    helpers::findAndReplace(fisheyeFragShader, "**rotVec**", ssRot.str());

    // replace glsl version
    helpers::findAndReplace(
        fisheyeVertShader,
        "**glsl_version**",
        Engine::instance()->getGLSLVersion()
    );
    helpers::findAndReplace(
        fisheyeFragShader,
        "**glsl_version**",
        Engine::instance()->getGLSLVersion()
    );

    // replace color
    std::stringstream ssColor;
    ssColor.precision(2);
    ssColor << std::fixed << "vec4(" << mClearColor.r << ", " << mClearColor.g
            << ", " << mClearColor.b << ", " << mClearColor.a << ")";
    helpers::findAndReplace(fisheyeFragShader, "**bgColor**", ssColor.str());

    bool vertShader = mShader.addShaderSrc(
        fisheyeVertShader,
        GL_VERTEX_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!vertShader) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Failed to load fisheye vertex shader:\n%s\n", fisheyeVertShader.c_str()
        );
    }
    bool fragShader = mShader.addShaderSrc(
        fisheyeFragShader,
        GL_FRAGMENT_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!fragShader) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Failed to load fisheye fragment shader\n%s\n", fisheyeFragShader.c_str()
        );
    }


    switch (mappingType) {
        case Mapping::Fisheye:
            mShader.setName("FisheyeShader");
            break;
        case Mapping::Equirectangular:
            mShader.setName("EquirectangularShader");
            break;
        case Mapping::Cubemap:
            mShader.setName("None");
            break;
    }
    mShader.createAndLinkProgram();
    mShader.bind();

    mCubemapLoc = mShader.getUniformLocation("cubemap");
    glUniform1i(mCubemapLoc, 0);

    mHalfFovLoc = mShader.getUniformLocation("halfFov");
    glUniform1f(mHalfFovLoc, glm::half_pi<float>());

    ShaderProgram::unbind();

    if (Settings::instance()->useDepthTexture()) {
        mDepthCorrectionShader.setName("FisheyeDepthCorrectionShader");
        mDepthCorrectionShader.createAndLinkProgram();
        mDepthCorrectionShader.bind();

        mSwapColorLoc = mDepthCorrectionShader.getUniformLocation("cTex");
        glUniform1i(mSwapColorLoc, 0);
        mSwapDepthLoc = mDepthCorrectionShader.getUniformLocation("dTex");
        glUniform1i(mSwapDepthLoc, 1);
        mSwapNearLoc = mDepthCorrectionShader.getUniformLocation("near");
        mSwapFarLoc = mDepthCorrectionShader.getUniformLocation("far");

        ShaderProgram::unbind();
    }
}

void SpoutOutputProjection::initFBO() {
    NonLinearProjection::initFBO();

    mSpoutFBO = std::make_unique<core::OffScreenBuffer>();
    mSpoutFBO->setInternalColorFormat(mTexInternalFormat);
    mSpoutFBO->createFBO(mappingWidth, mappingHeight, 1);

    if (mSpoutFBO->checkForErrors()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "Spout FBO created\n"
        );
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Spout FBO created with errors\n"
        );
    }

    OffScreenBuffer::unBind();
}

void SpoutOutputProjection::drawCubeFace(int face) {
    BaseViewport& vp = [this](int face) -> BaseViewport& {
        switch (face) {
            default:
            case 0: return mSubViewports.right;
            case 1: return mSubViewports.left;
            case 2: return mSubViewports.bottom;
            case 3: return mSubViewports.top;
            case 4: return mSubViewports.front;
            case 5: return mSubViewports.back;
        }
    }(face);

    glLineWidth(1.0);
    if (Engine::instance()->getWireframe()) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // reset depth function (to opengl default)
    glDepthFunc(GL_LESS);

    // run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);
    setupViewport(vp);

#ifdef DebugCubemap
    glm::vec4 color;
    switch (face) {
        case 0:
            color.r = 0.5f;
            color.g = 0.f;
            color.b = 0.f;
            color.a = 1.0f;
            break;
        case 1:
            color.r = 0.5f;
            color.g = 0.5f;
            color.b = 0.f;
            color.a = 1.f;
            break;
        case 2:
            color.r = 0.f;
            color.g = 0.5f;
            color.b = 0.f;
            color.a = 1.f;
            break;
        case 3:
            color.r = 0.f;
            color.g = 0.5f;
            color.b = 0.5f;
            color.a = 1.f;
            break;
        case 4:
            color.r = 0.f;
            color.g = 0.f;
            color.b = 0.5f;
            color.a = 1.f;
            break;
        case 5:
            color.r = 0.5f;
            color.g = 0.f;
            color.b = 0.5f;
            color.a = 1.f;
            break;
    }
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
    if (Engine::instance()->mClearBufferFnPtr) {
        Engine::instance()->mClearBufferFnPtr();
    }
    else {
        glm::vec4 color = Engine::instance()->getClearColor();
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
#endif

    glDisable(GL_SCISSOR_TEST);

    Engine::instance()->mDrawFnPtr();

    // restore polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void SpoutOutputProjection::blitCubeFace(int face) {
    // copy AA-buffer to "regular"/non-AA buffer

    // bind separate read and draw buffers to prepare blit operation
    mCubeMapFbo->bindBlit();
    attachTextures(face);
    mCubeMapFbo->blit();
}

void SpoutOutputProjection::attachTextures(int face) {
    if (Settings::instance()->useDepthTexture()) {
        mCubeMapFbo->attachDepthTexture(mTextures.depthSwap);
        mCubeMapFbo->attachColorTexture(mTextures.colorSwap);
    }
    else {
        mCubeMapFbo->attachCubeMapTexture(mTextures.cubeMapColor, face);
    }

    if (Settings::instance()->useNormalTexture()) {
        mCubeMapFbo->attachCubeMapTexture(
            mTextures.cubeMapNormals,
            face,
            GL_COLOR_ATTACHMENT1
        );
    }

    if (Settings::instance()->usePositionTexture()) {
        mCubeMapFbo->attachCubeMapTexture(
            mTextures.cubeMapPositions,
            face,
            GL_COLOR_ATTACHMENT2
        );
    }
}

void SpoutOutputProjection::renderInternal() {
    glEnable(GL_SCISSOR_TEST);
    Engine::instance()->enterCurrentViewport();
    glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    if (mappingType != Mapping::Cubemap) {
        GLenum saveBuffer = {};
        GLint saveTexture = 0;
        GLint saveFrameBuffer = 0;
        glGetIntegerv(GL_DRAW_BUFFER0, &saveBuffer);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &saveTexture);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &saveFrameBuffer);


        GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
        mSpoutFBO->bind(false, 1, buffers); //bind no multi-sampled
        mSpoutFBO->attachColorTexture(mappingTexture);

        mShader.bind();

        glViewport(0, 0, mappingWidth, mappingHeight);
        glScissor(0, 0, mappingWidth, mappingHeight);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        //if for some reson the active texture has been reset
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures.cubeMapColor);

        glDisable(GL_CULL_FACE);
        bool alpha = Engine::instance()->getCurrentWindow().getAlpha();
        if (alpha) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        else {
            glDisable(GL_BLEND);
        }
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);

        glUniform1i(mCubemapLoc, 0);
        if (mappingType == Mapping::Fisheye) {
            glUniform1f(mHalfFovLoc, glm::radians<float>(180.f / 2.f));
        }
        else if (mappingType == Mapping::Equirectangular) {
            glUniform1f(mHalfFovLoc, glm::radians<float>(360.f / 2.f));
        }

        glBindVertexArray(mVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        ShaderProgram::unbind();

        glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        glDisable(GL_DEPTH_TEST);

        if (alpha) {
            glDisable(GL_BLEND);
        }

        // restore depth func
        glDepthFunc(GL_LESS);

        mSpoutFBO->unBind();

        glBindTexture(GL_TEXTURE_2D, mappingTexture);
#ifdef SGCT_HAS_SPOUT
        reinterpret_cast<SPOUTHANDLE>(mappingHandle)->SendTexture(
            mappingTexture,
            static_cast<GLuint>(GL_TEXTURE_2D),
            mappingWidth,
            mappingHeight
        );
#endif
        glBindTexture(GL_TEXTURE_2D, 0);

        buffers[0] = saveBuffer;
        glDrawBuffers(1, buffers);
        glBindTexture(GL_TEXTURE_2D, saveTexture);
        glBindFramebuffer(GL_FRAMEBUFFER, saveFrameBuffer);
    }
    else {
        GLint saveTexture = 0;
        GLint saveFrameBuffer = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &saveTexture);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &saveFrameBuffer);

#ifdef SGCT_HAS_SPOUT
        for (int i = 0; i < NFaces; i++) {
            if (!mSpout[i].enabled) {
                continue;
            }
            glBindTexture(GL_TEXTURE_2D, mSpout[i].texture);
            reinterpret_cast<SPOUTHANDLE>(mSpout[i].handle)->SendTexture(
                mSpout[i].texture,
                static_cast<GLuint>(GL_TEXTURE_2D),
                mappingWidth,
                mappingHeight
            );
        }
#endif

        glBindTexture(GL_TEXTURE_2D, saveTexture);
        glBindFramebuffer(GL_FRAMEBUFFER, saveFrameBuffer);
    }
}

void SpoutOutputProjection::renderInternalFixedPipeline() {
    glEnable(GL_SCISSOR_TEST);
    Engine::instance()->enterCurrentViewport();
    glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    if (mappingType != Mapping::Cubemap) {
        GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
        mSpoutFBO->bind(false, 1, buffers); //bind no multi-sampled
        mSpoutFBO->attachColorTexture(mappingTexture);

        mShader.bind();

        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        //if for some reson the active texture has been reset
        glActiveTexture(GL_TEXTURE0);

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW); //restore

        glEnable(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextures.cubeMapColor);

        glDisable(GL_CULL_FACE);
        bool alpha = Engine::instance()->getCurrentWindow().getAlpha();
        if (alpha) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        else {
            glDisable(GL_BLEND);
        }
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);

        glUniform1i(mCubemapLoc, 0);
        if (mappingType == Mapping::Fisheye) {
            glUniform1f(mHalfFovLoc, glm::radians<float>(180.f / 2.f));
        } else if (mappingType == Mapping::Equirectangular) {
            glUniform1f(mHalfFovLoc, glm::radians<float>(360.f / 2.f));
        }

        glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
        // make sure that VBOs are unbound, to not mess up the vertex array
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glClientActiveTexture(GL_TEXTURE0);

        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(0));

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(8));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        ShaderProgram::unbind();
        mSpoutFBO->unBind();

        glBindTexture(GL_TEXTURE_2D, mappingTexture);
#ifdef SGCT_HAS_SPOUT
        reinterpret_cast<SPOUTHANDLE>(mappingHandle)->SendTexture(
            mappingTexture,
            static_cast<GLuint>(GL_TEXTURE_2D),
            mappingWidth,
            mappingHeight
        );
#endif
        glBindTexture(GL_TEXTURE_2D, 0);

        glPopClientAttrib();
        glPopAttrib();
    }
    else {
#ifdef SGCT_HAS_SPOUT
        for (int i = 0; i < NFaces; i++) {
            if (!mSpout[i].enabled) {
                continue;
            }
            glBindTexture(GL_TEXTURE_2D, mSpout[i].texture);
            reinterpret_cast<SPOUTHANDLE>(mSpout[i].handle)->SendTexture(
                mSpout[i].texture,
                static_cast<GLuint>(GL_TEXTURE_2D),
                mappingWidth,
                mappingHeight
            );
        }
#endif

        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void SpoutOutputProjection::renderCubemapInternal(size_t* subViewPortIndex) {
    for (int i = 0; i < 6; i++) {
        BaseViewport& vp = [this](int face) -> BaseViewport& {
            switch (face) {
                default:
                case 0: return mSubViewports.right;
                case 1: return mSubViewports.left;
                case 2: return mSubViewports.bottom;
                case 3: return mSubViewports.top;
                case 4: return mSubViewports.front;
                case 5: return mSubViewports.back;
            }
        }(i);
        *subViewPortIndex = i;
        unsigned int idx = static_cast<unsigned int>(i);

        if (!mSpout[i].enabled || !vp.isEnabled()) {
            continue;
        }

        // bind & attach buffer
        mCubeMapFbo->bind();
        if (!mCubeMapFbo->isMultiSampled()) {
            attachTextures(idx);
        }

        Engine::instance()->getCurrentWindow().setCurrentViewport(&vp);
        drawCubeFace(i);

        // blit MSAA fbo to texture
        if (mCubeMapFbo->isMultiSampled()) {
            blitCubeFace(idx);
        }

        // re-calculate depth values from a cube to spherical model
        if (Settings::instance()->useDepthTexture()) {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            mCubeMapFbo->bind(false, 1, buffers); //bind no multi-sampled

            mCubeMapFbo->attachCubeMapTexture(mTextures.cubeMapColor, idx);
            mCubeMapFbo->attachCubeMapDepthTexture(mTextures.cubeMapDepth, idx);

            glViewport(0, 0, mappingWidth, mappingHeight);
            glScissor(0, 0, mappingWidth, mappingHeight);
            glEnable(GL_SCISSOR_TEST);

            Engine::instance()->mClearBufferFnPtr();

            glDisable(GL_CULL_FACE);
            const bool alpha = Engine::instance()->getCurrentWindow().getAlpha();
            if (alpha) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            else {
                glDisable(GL_BLEND);
            }

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_ALWAYS);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mTextures.colorSwap);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, mTextures.depthSwap);

            // bind shader
            mDepthCorrectionShader.bind();
            glUniform1i(mSwapColorLoc, 0);
            glUniform1i(mSwapDepthLoc, 1);
            glUniform1f(mSwapNearLoc, Engine::instance()->mNearClippingPlaneDist);
            glUniform1f(mSwapFarLoc, Engine::instance()->mFarClippingPlaneDist);

            Engine::instance()->getCurrentWindow().bindVAO();
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            Engine::instance()->getCurrentWindow().unbindVAO();

            // unbind shader
            ShaderProgram::unbind();

            glDisable(GL_DEPTH_TEST);

            if (alpha) {
                glDisable(GL_BLEND);
            }

            // restore depth func
            glDepthFunc(GL_LESS);
            glDisable(GL_SCISSOR_TEST);
        }

        if (mappingType == Mapping::Cubemap) {
            mCubeMapFbo->unBind();

            if (mSpout[i].handle) {
                glBindTexture(GL_TEXTURE_2D, 0);
                glCopyImageSubData(
                    mTextures.cubeMapColor,
                    GL_TEXTURE_CUBE_MAP,
                    0,
                    0,
                    0,
                    idx,
                    mSpout[i].texture,
                    GL_TEXTURE_2D,
                    0,
                    0,
                    0,
                    0,
                    mappingWidth,
                    mappingHeight,
                    1
                );
            }
        }
    }
}

void SpoutOutputProjection::renderCubemapInternalFixedPipeline(size_t* subViewPortIndex) {
    for (int i = 0; i < 6; i++) {
        BaseViewport& vp = [this](int face) -> BaseViewport& {
            switch (face) {
                default:
                case 0: return mSubViewports.right;
                case 1: return mSubViewports.left;
                case 2: return mSubViewports.bottom;
                case 3: return mSubViewports.top;
                case 4: return mSubViewports.front;
                case 5: return mSubViewports.back;
            }
        }(i);
        *subViewPortIndex = i;
        unsigned int idx = static_cast<unsigned int>(i);

        if (!mSpout[i].enabled || !vp.isEnabled()) {
            continue;
        }

        // bind & attach buffer
        mCubeMapFbo->bind();
        if (!mCubeMapFbo->isMultiSampled()) {
            attachTextures(idx);
        }

        Engine::instance()->getCurrentWindow().setCurrentViewport(&vp);
        drawCubeFace(i);

        // blit MSAA fbo to texture
        if (mCubeMapFbo->isMultiSampled()) {
            blitCubeFace(idx);
        }

        // re-calculate depth values from a cube to spherical model
        if (Settings::instance()->useDepthTexture()) {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            mCubeMapFbo->bind(false, 1, buffers); // bind no multi-sampled

            mCubeMapFbo->attachCubeMapTexture(mTextures.cubeMapColor, idx);
            mCubeMapFbo->attachCubeMapDepthTexture(mTextures.cubeMapDepth, idx);

            glViewport(0, 0, mappingWidth, mappingHeight);
            glScissor(0, 0, mappingWidth, mappingHeight);

            glPushAttrib(GL_ALL_ATTRIB_BITS);
            glEnable(GL_SCISSOR_TEST);

            Engine::instance()->mClearBufferFnPtr();

            glActiveTexture(GL_TEXTURE0);
            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();

            glMatrixMode(GL_MODELVIEW); //restore

            // bind shader
            mDepthCorrectionShader.bind();
            glUniform1i(mSwapColorLoc, 0);
            glUniform1i(mSwapDepthLoc, 1);
            glUniform1f(mSwapNearLoc, Engine::instance()->mNearClippingPlaneDist);
            glUniform1f(mSwapFarLoc, Engine::instance()->mFarClippingPlaneDist);

            glDisable(GL_CULL_FACE);
            bool alpha = Engine::instance()->getCurrentWindow().getAlpha();
            if (alpha) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            else {
                glDisable(GL_BLEND);
            }

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_ALWAYS);

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, mTextures.colorSwap);

            glActiveTexture(GL_TEXTURE1);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, mTextures.depthSwap);

            glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
            Engine::instance()->getCurrentWindow().bindVBO();
            glClientActiveTexture(GL_TEXTURE0);

            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(0));

            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(8));
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            Engine::instance()->getCurrentWindow().unbindVBO();
            glPopClientAttrib();

            // unbind shader
            ShaderProgram::unbind();
            glPopAttrib();
        }

        if (mappingType == Mapping::Cubemap) {
            mCubeMapFbo->unBind();

            if (mSpout[i].handle) {
                glBindTexture(GL_TEXTURE_2D, 0);
                glCopyImageSubData(
                    mTextures.cubeMapColor,
                    GL_TEXTURE_CUBE_MAP,
                    0,
                    0,
                    0,
                    idx,
                    mSpout[i].texture,
                    GL_TEXTURE_2D,
                    0,
                    0,
                    0,
                    0,
                    mappingWidth,
                    mappingHeight,
                    1
                );
            }
        }
    }
}

} // namespace sgct::core
