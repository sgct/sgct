/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/SpoutOutputProjection.h>

#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
#include <sgct/OffScreenBuffer.h>
#include <sgct/SGCTSettings.h>
#include <sgct/SGCTWindow.h>
#include <sgct/helpers/SGCTStringFunctions.h>
#include <sgct/shaders/SGCTInternalFisheyeShaders.h>
#include <sgct/shaders/SGCTInternalFisheyeShaders_modern.h>
#include <sgct/shaders/SGCTInternalFisheyeShaders_cubic.h>
#include <sgct/shaders/SGCTInternalFisheyeShaders_modern_cubic.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>

#ifdef SGCT_HAS_SPOUT
#include <SpoutLibrary.h>
#endif

namespace sgct_core {

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
    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(mVAO);
    }
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);

    GLvoid* PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    memcpy(PositionBuffer, mVerts.data(), 20 * sizeof(float));
    glUnmapBuffer(GL_ARRAY_BUFFER);

    if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(0);
    }
    else {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void SpoutOutputProjection::render() {
    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        renderInternalFixedPipeline();
    }
    else {
        renderInternal();
    }
}

void SpoutOutputProjection::renderCubemap(size_t* subViewPortIndex) {
    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        renderCubemapInternalFixedPipeline(subViewPortIndex);
    }
    else {
        renderCubemapInternal(subViewPortIndex);
    }
}


void SpoutOutputProjection::setSpoutChannels(bool channels[NFaces]) {
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

    sgct::MessageHandler::instance()->print("SpoutOutputProjection initTextures");

    const bool compat = sgct::Engine::instance()->isOpenGLCompatibilityMode();
    if (compat) {
        glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT);
        glEnable(GL_TEXTURE_2D);
    }

    switch (mappingType) {
        case Mapping::Cubemap:
            mappingWidth = mCubemapResolution;
            mappingHeight = mCubemapResolution;

            for (int i = 0; i < NFaces; ++i) {
#ifdef SGCT_HAS_SPOUT
                sgct::MessageHandler::instance()->print(
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
            sgct::MessageHandler::instance()->print(
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
            sgct::MessageHandler::instance()->print(
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

    if (compat) {
        glPopAttrib();
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

    // add viewports
    for (int i = 0; i < 6; i++) {
        mSubViewports[i].setName("SpoutOutput " + std::to_string(i));

        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 rotMat(1.f);

        // Rotate and clamp the half height viewports
        switch (static_cast<CubeFaces>(i)) {
            case CubeFaces::PosX: //+X face
                rotMat = glm::rotate(
                    rollRot,
                    glm::radians(-90.f),
                    glm::vec3(0.f, 1.f, 0.f)
                );
                upperRight.x = radius;
                mSubViewports[i].setSize(glm::vec2(1.f, 1.f));
                break;
            case CubeFaces::NegX: //-X face
                rotMat = glm::rotate(
                    rollRot,
                    glm::radians(90.f),
                    glm::vec3(0.f, 1.f, 0.f)
                );
                lowerLeft.x = -radius;
                upperLeft.x = -radius;
                mSubViewports[i].setPos(glm::vec2(0.f, 0.f));
                mSubViewports[i].setSize(glm::vec2(1.f, 1.f));
                break;
            case CubeFaces::PosY: //+Y face
                rotMat = glm::rotate(
                    rollRot,
                    glm::radians(-90.f),
                    glm::vec3(1.f, 0.f, 0.f)
                );
                lowerLeft.y = -radius;
                mSubViewports[i].setPos(glm::vec2(0.f, 0.f));
                mSubViewports[i].setSize(glm::vec2(1.f, 1.f));
                break;
            case CubeFaces::NegY: //-Y face
                rotMat = glm::rotate(
                    rollRot,
                    glm::radians(90.f),
                    glm::vec3(1.f, 0.f, 0.f)
                );
                upperLeft.y = radius;
                upperRight.y = radius;
                mSubViewports[i].setSize(glm::vec2(1.f, 1.f));
                break;
            case CubeFaces::PosZ: //+Z face
                rotMat = rollRot;
                break;
            case CubeFaces::NegZ: //-Z face
                rotMat = glm::rotate(rollRot,
                    glm::radians(180.f),
                    glm::vec3(0.f, 1.f, 0.f)
                );
                break;
        }

        mSubViewports[i].getProjectionPlane().setCoordinateLowerLeft(
            glm::vec3(rotMat * lowerLeft)
        );
        mSubViewports[i].getProjectionPlane().setCoordinateUpperLeft(
            glm::vec3(rotMat * upperLeft)
        );
        mSubViewports[i].getProjectionPlane().setCoordinateUpperRight(
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

    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        fisheyeVertShader = sgct_core::shaders_fisheye::FisheyeVert;

        if (sgct::SGCTSettings::instance()->useDepthTexture()) {
            switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType()) {
            case sgct::SGCTSettings::DrawBufferType::Diffuse:
                case sgct::SGCTSettings::DrawBufferType::DiffuseNormal:
                    fisheyeFragShader = shaders_fisheye::FisheyeFragDepthNormal;
                    break;
                case sgct::SGCTSettings::DrawBufferType::DiffusePosition:
                    fisheyeFragShader = shaders_fisheye::FisheyeFragDepthPosition;
                    break;
                case sgct::SGCTSettings::DrawBufferType::DiffuseNormalPosition:
                    fisheyeFragShader = shaders_fisheye::FisheyeFragDepthNormalPosition;
                    break;
                default:
                    fisheyeFragShader = shaders_fisheye::FisheyeFragDepth;
                    break;
            }
        }
        else  {
            // no depth
            switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType()) {
                case sgct::SGCTSettings::DrawBufferType::Diffuse:
                default:
                    fisheyeFragShader = shaders_fisheye::FisheyeFrag;
                    break;

                case sgct::SGCTSettings::DrawBufferType::DiffuseNormal:
                    fisheyeFragShader = shaders_fisheye::FisheyeFragNormal;
                    break;

                case sgct::SGCTSettings::DrawBufferType::DiffusePosition:
                    fisheyeFragShader = shaders_fisheye::FisheyeFragPosition;
                    break;

                case sgct::SGCTSettings::DrawBufferType::DiffuseNormalPosition:
                    fisheyeFragShader = shaders_fisheye::FisheyeFragNormalPosition;
                    break;
            }
        }

        //depth correction shader only
        if (sgct::SGCTSettings::instance()->useDepthTexture()) {
            std::string depthCorrFrag = shaders_fisheye::BaseVert;
            std::string depthCorrVert = shaders_fisheye::FisheyeDepthCorrectionFrag;

            //replace glsl version
            sgct_helpers::findAndReplace(
                depthCorrFrag,
                "**glsl_version**",
                sgct::Engine::instance()->getGLSLVersion()
            );
            sgct_helpers::findAndReplace(
                depthCorrVert,
                "**glsl_version**",
                sgct::Engine::instance()->getGLSLVersion()
            );

            bool fragShader = mDepthCorrectionShader.addShaderSrc(
                depthCorrFrag,
                GL_VERTEX_SHADER,
                sgct::ShaderProgram::ShaderSourceType::String
            );
            if (!fragShader) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "Failed to load fisheye depth correction vertex shader\n"
                );
            }
            bool vertShader = mDepthCorrectionShader.addShaderSrc(
                depthCorrVert,
                GL_FRAGMENT_SHADER,
                sgct::ShaderProgram::ShaderSourceType::String
            );
            if (!vertShader) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "Failed to load fisheye depth correction fragment shader\n"
                );
            }
        }
    }
    else {
        // modern pipeline
        fisheyeVertShader = shaders_modern_fisheye::FisheyeVert;

        if (sgct::SGCTSettings::instance()->useDepthTexture()) {
            switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType()) {
                case sgct::SGCTSettings::DrawBufferType::Diffuse:
                default:
                    fisheyeFragShader = shaders_modern_fisheye::FisheyeFragDepth;
                    break;
                case sgct::SGCTSettings::DrawBufferType::DiffuseNormal:
                    fisheyeFragShader = shaders_modern_fisheye::FisheyeFragDepthNormal;
                    break;
                case sgct::SGCTSettings::DrawBufferType::DiffusePosition:
                    fisheyeFragShader = shaders_modern_fisheye::FisheyeFragDepthPosition;
                    break;
                case sgct::SGCTSettings::DrawBufferType::DiffuseNormalPosition:
                    fisheyeFragShader =
                        shaders_modern_fisheye::FisheyeFragDepthNormalPosition;
                    break;
            }
        }
        else {
            //no depth
            switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType()) {
                case sgct::SGCTSettings::DrawBufferType::Diffuse:
                default:
                    fisheyeFragShader = shaders_modern_fisheye::FisheyeFrag;
                    break;
                case sgct::SGCTSettings::DrawBufferType::DiffuseNormal:
                    fisheyeFragShader = shaders_modern_fisheye::FisheyeFragNormal;
                    break;
                case sgct::SGCTSettings::DrawBufferType::DiffusePosition:
                    fisheyeFragShader = shaders_modern_fisheye::FisheyeFragPosition;
                    break;
                case sgct::SGCTSettings::DrawBufferType::DiffuseNormalPosition:
                    fisheyeFragShader = shaders_modern_fisheye::FisheyeFragNormalPosition;
                    break;
            }
        }

        //depth correction shader only
        if (sgct::SGCTSettings::instance()->useDepthTexture()) {
            std::string depthCorrFrag = shaders_modern_fisheye::BaseVert;
            std::string depthCorrVert =
                shaders_modern_fisheye::FisheyeDepthCorrectionFrag;

            //replace glsl version
            sgct_helpers::findAndReplace(
                depthCorrFrag,
                "**glsl_version**",
                sgct::Engine::instance()->getGLSLVersion()
            );
            bool fragShader = mDepthCorrectionShader.addShaderSrc(
                depthCorrFrag,
                GL_VERTEX_SHADER,
                sgct::ShaderProgram::ShaderSourceType::String
            );
            if (!fragShader) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "Failed to load fisheye depth correction vertex shader\n"
                );
            }

            sgct_helpers::findAndReplace(
                depthCorrVert,
                "**glsl_version**",
                sgct::Engine::instance()->getGLSLVersion()
            );
            bool vertShader = mDepthCorrectionShader.addShaderSrc(
                depthCorrVert,
                GL_FRAGMENT_SHADER,
                sgct::ShaderProgram::ShaderSourceType::String
            );
            if (!vertShader) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "Failed to load fisheye depth correction fragment shader\n"
                );
            }
        }
    }

    // add functions to shader
    switch (mappingType) {
        case Mapping::Fisheye:
            sgct_helpers::findAndReplace(
                fisheyeFragShader,
                "**sample_fun**",
                sgct::Engine::instance()->isOGLPipelineFixed() ?
                    shaders_fisheye::SampleFun :
                    shaders_modern_fisheye::SampleFun
            );
            break;
        case Mapping::Equirectangular:
            sgct_helpers::findAndReplace(
                fisheyeFragShader,
                "**sample_fun**",
                sgct::Engine::instance()->isOGLPipelineFixed() ?
                    shaders_fisheye::SampleLatlonFun :
                    shaders_modern_fisheye::SampleLatlonFun
            );
            break;
        default:
            sgct_helpers::findAndReplace(
                fisheyeFragShader,
                "**sample_fun**",
                sgct::Engine::instance()->isOGLPipelineFixed() ?
                    shaders_fisheye::SampleFun :
                    shaders_modern_fisheye::SampleFun
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

    //replace add correct transform in the fragment shader
    sgct_helpers::findAndReplace(fisheyeFragShader, "**rotVec**", ssRot.str());

    //replace glsl version
    sgct_helpers::findAndReplace(
        fisheyeVertShader,
        "**glsl_version**",
        sgct::Engine::instance()->getGLSLVersion()
    );
    sgct_helpers::findAndReplace(
        fisheyeFragShader,
        "**glsl_version**",
        sgct::Engine::instance()->getGLSLVersion()
    );

    //replace color
    std::stringstream ssColor;
    ssColor.precision(2);
    ssColor << std::fixed << "vec4(" << mClearColor.r << ", " << mClearColor.g
            << ", " << mClearColor.b << ", " << mClearColor.a << ")";
    sgct_helpers::findAndReplace(fisheyeFragShader, "**bgColor**", ssColor.str());

    bool vertShader = mShader.addShaderSrc(
        fisheyeVertShader,
        GL_VERTEX_SHADER,
        sgct::ShaderProgram::ShaderSourceType::String
    );
    if (!vertShader) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Failed to load fisheye vertex shader:\n%s\n",
            fisheyeVertShader.c_str()
        );
    }
    bool fragShader = mShader.addShaderSrc(
        fisheyeFragShader,
        GL_FRAGMENT_SHADER,
        sgct::ShaderProgram::ShaderSourceType::String
    );
    if (!fragShader) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Failed to load fisheye fragment shader\n%s\n",
            fisheyeFragShader.c_str()
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

    sgct::ShaderProgram::unbind();

    if (sgct::SGCTSettings::instance()->useDepthTexture()) {
        mDepthCorrectionShader.setName("FisheyeDepthCorrectionShader");
        mDepthCorrectionShader.createAndLinkProgram();
        mDepthCorrectionShader.bind();

        mSwapColorLoc = mDepthCorrectionShader.getUniformLocation("cTex");
        glUniform1i(mSwapColorLoc, 0);
        mSwapDepthLoc = mDepthCorrectionShader.getUniformLocation("dTex");
        glUniform1i(mSwapDepthLoc, 1);
        mSwapNearLoc = mDepthCorrectionShader.getUniformLocation("near");
        mSwapFarLoc = mDepthCorrectionShader.getUniformLocation("far");

        sgct::ShaderProgram::unbind();
    }
}

void SpoutOutputProjection::initFBO() {
    NonLinearProjection::initFBO();

    mSpoutFBO = std::make_unique<sgct_core::OffScreenBuffer>();
    mSpoutFBO->setInternalColorFormat(mTexInternalFormat);
    mSpoutFBO->createFBO(mappingWidth, mappingHeight, 1);

    if (mSpoutFBO->checkForErrors()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "Spout FBO created\n"
        );
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Spout FBO created with errors\n"
        );
    }

    OffScreenBuffer::unBind();
}

void SpoutOutputProjection::drawCubeFace(int face) {
    glLineWidth(1.0);
    if (sgct::Engine::instance()->getWireframe()) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // reset depth function (to opengl default)
    glDepthFunc(GL_LESS);

    // run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);
    setupViewport(mSubViewports[face]);

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
    if (sgct::Engine::mInstance->mClearBufferFnPtr != nullptr) {
        sgct::Engine::mInstance->mClearBufferFnPtr();
    }
    else {
        glm::vec4 color = sgct::Engine::instance()->getClearColor();
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
#endif

    glDisable(GL_SCISSOR_TEST);

    if (sgct::Engine::instance()->isOGLPipelineFixed()) {
        glMatrixMode(GL_PROJECTION);
        SGCTProjection& proj = mSubViewports[face].getProjection(
            sgct::Engine::instance()->getCurrentFrustumMode()
        );
        glLoadMatrixf(glm::value_ptr(proj.getProjectionMatrix()));
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(glm::value_ptr(
            proj.getViewMatrix() * sgct::Engine::instance()->getModelMatrix()
        ));
    }

    // render
    sgct::Engine::mInstance->mDrawFnPtr();

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
    if (sgct::SGCTSettings::instance()->useDepthTexture()) {
        mCubeMapFbo->attachDepthTexture(mTextures.depthSwap);
        mCubeMapFbo->attachColorTexture(mTextures.colorSwap);
    }
    else {
        mCubeMapFbo->attachCubeMapTexture(mTextures.cubeMapColor, face);
    }

    if (sgct::SGCTSettings::instance()->useNormalTexture()) {
        mCubeMapFbo->attachCubeMapTexture(
            mTextures.cubeMapNormals,
            face,
            GL_COLOR_ATTACHMENT1
        );
    }

    if (sgct::SGCTSettings::instance()->usePositionTexture()) {
        mCubeMapFbo->attachCubeMapTexture(
            mTextures.cubeMapPositions,
            face,
            GL_COLOR_ATTACHMENT2
        );
    }
}

void SpoutOutputProjection::renderInternal() {
    glEnable(GL_SCISSOR_TEST);
    sgct::Engine::mInstance->enterCurrentViewport();
    glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    if (mappingType != Mapping::Cubemap) {
        GLint saveBuffer = 0;
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
        bool alpha = sgct::Engine::mInstance->getCurrentWindow().getAlpha();
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
        glBindVertexArray(GL_FALSE);

        sgct::ShaderProgram::unbind();

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
            GL_TEXTURE_2D,
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
                GL_TEXTURE_2D,
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
    sgct::Engine::mInstance->enterCurrentViewport();
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
        bool alpha = sgct::Engine::mInstance->getCurrentWindow().getAlpha();
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
        //make sure that VBO:s are unbinded, to not mess up the vertex array
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glClientActiveTexture(GL_TEXTURE0);

        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(0));

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(8));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);

        sgct::ShaderProgram::unbind();
        mSpoutFBO->unBind();

        glBindTexture(GL_TEXTURE_2D, mappingTexture);
#ifdef SGCT_HAS_SPOUT
        reinterpret_cast<SPOUTHANDLE>(mappingHandle)->SendTexture(
            mappingTexture,
            GL_TEXTURE_2D,
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
                GL_TEXTURE_2D,
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
        BaseViewport& vp = mSubViewports[i];
        *subViewPortIndex = i;
        unsigned int idx = static_cast<unsigned int>(i);

        if (!mSpout[i].enabled || !vp.isEnabled()) {
            continue;
        }

        //bind & attach buffer
        mCubeMapFbo->bind();
        if (!mCubeMapFbo->isMultiSampled()) {
            attachTextures(idx);
        }

        sgct::Engine::mInstance->getCurrentWindow().setCurrentViewport(&vp);
        drawCubeFace(i);

        //blit MSAA fbo to texture
        if (mCubeMapFbo->isMultiSampled()) {
            blitCubeFace(idx);
        }

        //re-calculate depth values from a cube to spherical model
        if (sgct::SGCTSettings::instance()->useDepthTexture()) {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            mCubeMapFbo->bind(false, 1, buffers); //bind no multi-sampled

            mCubeMapFbo->attachCubeMapTexture(mTextures.cubeMapColor, idx);
            mCubeMapFbo->attachCubeMapDepthTexture(mTextures.cubeMapDepth, idx);

            glViewport(0, 0, mappingWidth, mappingHeight);
            glScissor(0, 0, mappingWidth, mappingHeight);
            glEnable(GL_SCISSOR_TEST);

            sgct::Engine::mInstance->mClearBufferFnPtr();

            glDisable(GL_CULL_FACE);
            bool alpha = sgct::Engine::mInstance->getCurrentWindow().getAlpha();
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

            sgct::Engine* engine = sgct::Engine::instance();

            //bind shader
            mDepthCorrectionShader.bind();
            glUniform1i(mSwapColorLoc, 0);
            glUniform1i(mSwapDepthLoc, 1);
            glUniform1f(mSwapNearLoc, engine->mNearClippingPlaneDist);
            glUniform1f(mSwapFarLoc, engine->mFarClippingPlaneDist);

            engine->getCurrentWindow().bindVAO();
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            engine->getCurrentWindow().unbindVAO();

            // unbind shader
            sgct::ShaderProgram::unbind();

            glDisable(GL_DEPTH_TEST);

            if (alpha) {
                glDisable(GL_BLEND);
            }

            // restore depth func
            glDepthFunc(GL_LESS);
            glDisable(GL_SCISSOR_TEST);
        } // end if depthmap

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
        BaseViewport& vp = mSubViewports[i];
        *subViewPortIndex = i;
        unsigned int idx = static_cast<unsigned int>(i);

        if (!mSpout[i].enabled || !vp.isEnabled()) {
            continue;
        }

        //bind & attach buffer
        mCubeMapFbo->bind();
        if (!mCubeMapFbo->isMultiSampled()) {
            attachTextures(idx);
        }

        sgct::Engine::mInstance->getCurrentWindow().setCurrentViewport(&vp);
        drawCubeFace(i);

        //blit MSAA fbo to texture
        if (mCubeMapFbo->isMultiSampled()) {
            blitCubeFace(idx);
        }

        // re-calculate depth values from a cube to spherical model
        if (sgct::SGCTSettings::instance()->useDepthTexture()) {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            mCubeMapFbo->bind(false, 1, buffers); // bind no multi-sampled

            mCubeMapFbo->attachCubeMapTexture(mTextures.cubeMapColor, idx);
            mCubeMapFbo->attachCubeMapDepthTexture(mTextures.cubeMapDepth, idx);

            glViewport(0, 0, mappingWidth, mappingHeight);
            glScissor(0, 0, mappingWidth, mappingHeight);

            glPushAttrib(GL_ALL_ATTRIB_BITS);
            glEnable(GL_SCISSOR_TEST);

            sgct::Engine::mInstance->mClearBufferFnPtr();

            glActiveTexture(GL_TEXTURE0);
            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();

            glMatrixMode(GL_MODELVIEW); //restore

            // bind shader
            mDepthCorrectionShader.bind();
            glUniform1i(mSwapColorLoc, 0);
            glUniform1i(mSwapDepthLoc, 1);
            glUniform1f(mSwapNearLoc, sgct::Engine::mInstance->mNearClippingPlaneDist);
            glUniform1f(mSwapFarLoc, sgct::Engine::mInstance->mFarClippingPlaneDist);

            glDisable(GL_CULL_FACE);
            bool alpha = sgct::Engine::mInstance->getCurrentWindow().getAlpha();
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
            sgct::Engine::mInstance->getCurrentWindow().bindVBO();
            glClientActiveTexture(GL_TEXTURE0);

            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(0));

            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(8));
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            sgct::Engine::mInstance->getCurrentWindow().unbindVBO();
            glPopClientAttrib();

            // unbind shader
            sgct::ShaderProgram::unbind();
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

} // namespace sgct_core
