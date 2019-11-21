/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/spoutoutputprojection.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/ogl_headers.h>
#include <sgct/window.h>
#include <sgct/helpers/stringfunctions.h>
#include <sgct/shaders/internalfisheyeshaders.h>
#include <sgct/shaders/internalfisheyeshaders_cubic.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifdef SGCT_HAS_SPOUT
#define WIN32_LEAN_AND_MEAN
#include <SpoutLibrary.h>
#endif

namespace {
    constexpr const int NFaces = 6;
    constexpr const std::array<const char*, NFaces> CubeMapFaceName = {
        "Right", "zLeft", "Bottom", "Top", "Left", "zRight"
    };
}

namespace sgct::core {

SpoutOutputProjection::~SpoutOutputProjection() {
#ifdef SGCT_HAS_SPOUT
    for (int i = 0; i < NFaces; i++) {
        if (_spout[i].handle) {
            reinterpret_cast<SPOUTHANDLE>(_spout[i].handle)->ReleaseSender();
            reinterpret_cast<SPOUTHANDLE>(_spout[i].handle)->Release();
        }
    }

    if (_mappingHandle) {
        reinterpret_cast<SPOUTHANDLE>(_mappingHandle)->ReleaseSender();
        reinterpret_cast<SPOUTHANDLE>(_mappingHandle)->Release();
    }
#endif

    glDeleteTextures(1, &_mappingTexture);
}

void SpoutOutputProjection::update(glm::vec2) {
    constexpr const std::array<const float, 20> v = {
        0.f, 0.f, -1.f, -1.f, -1.f,
        0.f, 1.f, -1.f,  1.f, -1.f,
        1.f, 0.f,  1.f, -1.f, -1.f,
        1.f, 1.f,  1.f,  1.f, -1.f
    };

    // update VBO
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void SpoutOutputProjection::render() {
    glEnable(GL_SCISSOR_TEST);
    Engine::instance().enterCurrentViewport();
    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    if (_mappingType != Mapping::Cubemap) {
        GLenum saveBuffer = {};
        GLint saveTexture = 0;
        GLint saveFrameBuffer = 0;
        glGetIntegerv(GL_DRAW_BUFFER0, &saveBuffer);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &saveTexture);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &saveFrameBuffer);


        GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
        _spoutFBO->bind(false, 1, buffers); // bind no multi-sampled
        _spoutFBO->attachColorTexture(_mappingTexture);

        _shader.bind();

        glViewport(0, 0, _mappingWidth, _mappingHeight);
        glScissor(0, 0, _mappingWidth, _mappingHeight);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        // if for some reson the active texture has been reset
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapColor);

        glDisable(GL_CULL_FACE);
        bool hasAlpha = Engine::instance().getCurrentWindow().hasAlpha();
        if (hasAlpha) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        else {
            glDisable(GL_BLEND);
        }
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);

        glUniform1i(_cubemapLoc, 0);
        if (_mappingType == Mapping::Fisheye) {
            glUniform1f(_halfFovLoc, glm::half_pi<float>());
        }
        else if (_mappingType == Mapping::Equirectangular) {
            glUniform1f(_halfFovLoc, glm::pi<float>());
        }

        glBindVertexArray(_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        ShaderProgram::unbind();

        glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        glDisable(GL_DEPTH_TEST);

        if (hasAlpha) {
            glDisable(GL_BLEND);
        }

        // restore depth func
        glDepthFunc(GL_LESS);

        _spoutFBO->unbind();

        glBindTexture(GL_TEXTURE_2D, _mappingTexture);
#ifdef SGCT_HAS_SPOUT
        reinterpret_cast<SPOUTHANDLE>(_mappingHandle)->SendTexture(
            _mappingTexture,
            static_cast<GLuint>(GL_TEXTURE_2D),
            _mappingWidth,
            _mappingHeight
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
            if (!_spout[i].enabled) {
                continue;
            }
            glBindTexture(GL_TEXTURE_2D, _spout[i].texture);
            reinterpret_cast<SPOUTHANDLE>(_spout[i].handle)->SendTexture(
                _spout[i].texture,
                static_cast<GLuint>(GL_TEXTURE_2D),
                _mappingWidth,
                _mappingHeight
            );
        }
#endif

        glBindTexture(GL_TEXTURE_2D, saveTexture);
        glBindFramebuffer(GL_FRAMEBUFFER, saveFrameBuffer);
    }
}

void SpoutOutputProjection::renderCubemap() {
    for (int i = 0; i < 6; i++) {
        BaseViewport& vp = [this](int face) -> BaseViewport& {
            switch (face) {
            default:
            case 0: return _subViewports.right;
            case 1: return _subViewports.left;
            case 2: return _subViewports.bottom;
            case 3: return _subViewports.top;
            case 4: return _subViewports.front;
            case 5: return _subViewports.back;
            }
        }(i);
        unsigned int idx = static_cast<unsigned int>(i);

        if (!_spout[i].enabled || !vp.isEnabled()) {
            continue;
        }

        // bind & attach buffer
        _cubeMapFbo->bind();
        if (!_cubeMapFbo->isMultiSampled()) {
            attachTextures(idx);
        }

        Engine::instance().getCurrentWindow().setCurrentViewport(&vp);
        drawCubeFace(i);

        // blit MSAA fbo to texture
        if (_cubeMapFbo->isMultiSampled()) {
            blitCubeFace(idx);
        }

        // re-calculate depth values from a cube to spherical model
        if (Settings::instance().useDepthTexture()) {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            _cubeMapFbo->bind(false, 1, buffers); //bind no multi-sampled

            _cubeMapFbo->attachCubeMapTexture(_textures.cubeMapColor, idx);
            _cubeMapFbo->attachCubeMapDepthTexture(_textures.cubeMapDepth, idx);

            glViewport(0, 0, _mappingWidth, _mappingHeight);
            glScissor(0, 0, _mappingWidth, _mappingHeight);
            glEnable(GL_SCISSOR_TEST);

            const glm::vec4 color = Engine::instance().getClearColor();
            const bool hasAlpha = Engine::instance().getCurrentWindow().hasAlpha();
            glClearColor(color.r, color.g, color.b, hasAlpha ? 0.f : color.a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glDisable(GL_CULL_FACE);
            if (hasAlpha) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            else {
                glDisable(GL_BLEND);
            }

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_ALWAYS);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, _textures.colorSwap);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, _textures.depthSwap);

            // bind shader
            _depthCorrectionShader.bind();
            glUniform1i(_swapColorLoc, 0);
            glUniform1i(_swapDepthLoc, 1);
            glUniform1f(_swapNearLoc, Engine::instance().getNearClipPlane());
            glUniform1f(_swapFarLoc, Engine::instance().getFarClipPlane());

            Engine::instance().getCurrentWindow().bindVAO();
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            Engine::instance().getCurrentWindow().unbindVAO();

            // unbind shader
            ShaderProgram::unbind();

            glDisable(GL_DEPTH_TEST);

            if (hasAlpha) {
                glDisable(GL_BLEND);
            }

            // restore depth func
            glDepthFunc(GL_LESS);
            glDisable(GL_SCISSOR_TEST);
        }

        if (_mappingType == Mapping::Cubemap) {
            _cubeMapFbo->unbind();

            if (_spout[i].handle) {
                glBindTexture(GL_TEXTURE_2D, 0);
                glCopyImageSubData(
                    _textures.cubeMapColor,
                    GL_TEXTURE_CUBE_MAP,
                    0,
                    0,
                    0,
                    idx,
                    _spout[i].texture,
                    GL_TEXTURE_2D,
                    0,
                    0,
                    0,
                    0,
                    _mappingWidth,
                    _mappingHeight,
                    1
                );
            }
        }
    }
}

void SpoutOutputProjection::setSpoutChannels(bool right, bool zLeft, bool bottom,
                                             bool top, bool left, bool zRight)
{
    _spout[0].enabled = right;
    _spout[1].enabled = zLeft;
    _spout[2].enabled = bottom;
    _spout[3].enabled = top;
    _spout[4].enabled = left;
    _spout[5].enabled = zRight;
}

void SpoutOutputProjection::setSpoutMappingName(std::string name) {
    _mappingName = std::move(name);
}

void SpoutOutputProjection::setSpoutMapping(Mapping type) {
    _mappingType = type;
}

void SpoutOutputProjection::setSpoutRigOrientation(glm::vec3 orientation) {
    _rigOrientation = std::move(orientation);
}

void SpoutOutputProjection::initTextures() {
    NonLinearProjection::initTextures();
    MessageHandler::printDebug("SpoutOutputProjection initTextures");

    switch (_mappingType) {
        case Mapping::Cubemap:
            _mappingWidth = _cubemapResolution;
            _mappingHeight = _cubemapResolution;

            for (int i = 0; i < NFaces; ++i) {
#ifdef SGCT_HAS_SPOUT
                MessageHandler::printDebug("SpoutOutputProjection initTextures %d", i);
                if (!_spout[i].enabled) {
                    continue;
                }
                _spout[i].handle = GetSpout();
                if (_spout[i].handle) {
                    SPOUTHANDLE h = reinterpret_cast<SPOUTHANDLE>(_spout[i].handle);
                    h->CreateSender(CubeMapFaceName[i], _mappingWidth, _mappingHeight);
                }
#endif
                glGenTextures(1, &_spout[i].texture);
                glBindTexture(GL_TEXTURE_2D, _spout[i].texture);

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
                    _texInternalFormat,
                    _mappingWidth,
                    _mappingHeight,
                    0,
                    _texFormat,
                    _texType,
                    nullptr
                );
            }
            break;
        case Mapping::Equirectangular:
            _mappingWidth = _cubemapResolution * 4;
            _mappingHeight = _cubemapResolution * 2;
#ifdef SGCT_HAS_SPOUT
            MessageHandler::printDebug(
                "SpoutOutputProjection initTextures Equirectangular"
            );
            _mappingHandle = GetSpout();
            if (_mappingHandle) {
                SPOUTHANDLE h = reinterpret_cast<SPOUTHANDLE>(_mappingHandle);
                h->CreateSender(_mappingName.c_str(), _mappingWidth, _mappingHeight);
            }
#endif
            glGenTextures(1, &_mappingTexture);
            glBindTexture(GL_TEXTURE_2D, _mappingTexture);

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
                _texInternalFormat,
                _mappingWidth,
                _mappingHeight,
                0,
                _texFormat,
                _texType,
                nullptr
            );
            break;
        case Mapping::Fisheye:
            _mappingWidth = _cubemapResolution * 2;
            _mappingHeight = _cubemapResolution * 2;
#ifdef SGCT_HAS_SPOUT
            MessageHandler::printDebug("SpoutOutputProjection initTextures Fisheye");
            _mappingHandle = GetSpout();
            if (_mappingHandle) {
                SPOUTHANDLE h = reinterpret_cast<SPOUTHANDLE>(_mappingHandle);
                h->CreateSender(_mappingName.c_str(), _mappingWidth, _mappingHeight);
            }
#endif
            glGenTextures(1, &_mappingTexture);
            glBindTexture(GL_TEXTURE_2D, _mappingTexture);

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
                _texInternalFormat,
                _mappingWidth,
                _mappingHeight,
                0,
                _texFormat,
                _texType,
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
        glm::radians(-_rigOrientation.x),
        glm::vec3(0.f, 1.f, 0.f)
    );
    const glm::mat4 yawRot = glm::rotate(
        pitchRot,
        glm::radians(_rigOrientation.y),
        glm::vec3(1.f, 0.f, 0.f)
    );
    const glm::mat4 rollRot = glm::rotate(
        yawRot,
        glm::radians(-_rigOrientation.z),
        glm::vec3(0.f, 0.f, 1.f)
    );

    // right, left, bottom, top, front, back
    {
        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 r = glm::rotate(rollRot, glm::radians(-90.f), glm::vec3(0.f, 1.f, 0.f));
        upperRight.x = radius;
        _subViewports.right.setSize(glm::vec2(1.f, 1.f));

        _subViewports.right.getProjectionPlane().setCoordinates(
            glm::vec3(r * lowerLeft),
            glm::vec3(r * upperLeft),
            glm::vec3(r * upperRight)
        );
    }

    // left
    {
        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 r = glm::rotate(rollRot, glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
        lowerLeft.x = -radius;
        upperLeft.x = -radius;
        _subViewports.left.setPos(glm::vec2(0.f, 0.f));
        _subViewports.left.setSize(glm::vec2(1.f, 1.f));

        _subViewports.left.getProjectionPlane().setCoordinates(
            glm::vec3(r * lowerLeft),
            glm::vec3(r * upperLeft),
            glm::vec3(r * upperRight)
        );
    }

    // bottom
    {
        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 r = glm::rotate(rollRot, glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f));
        lowerLeft.y = -radius;
        _subViewports.bottom.setPos(glm::vec2(0.f, 0.f));
        _subViewports.bottom.setSize(glm::vec2(1.f, 1.f));

        _subViewports.bottom.getProjectionPlane().setCoordinates(
            glm::vec3(r * lowerLeft),
            glm::vec3(r * upperLeft),
            glm::vec3(r * upperRight)
        );
    }

    // top
    {
        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 r = glm::rotate(rollRot, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
        upperLeft.y = radius;
        upperRight.y = radius;
        _subViewports.top.setSize(glm::vec2(1.f, 1.f));

        _subViewports.top.getProjectionPlane().setCoordinates(
            glm::vec3(r * lowerLeft),
            glm::vec3(r * upperLeft),
            glm::vec3(r * upperRight)
        );
    }

    // front
    {
        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 rotMat = rollRot;

        _subViewports.front.getProjectionPlane().setCoordinates(
            glm::vec3(rotMat * lowerLeft),
            glm::vec3(rotMat * upperLeft),
            glm::vec3(rotMat * upperRight)
        );
    }

    // back
    {
        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 r = glm::rotate(rollRot, glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f));

        _subViewports.back.getProjectionPlane().setCoordinates(
            glm::vec3(r * lowerLeft),
            glm::vec3(r * upperLeft),
            glm::vec3(r * upperRight)
        );
    }
}

void SpoutOutputProjection::initShaders() {
    // reload shader program if it exists
    if (_shader.isLinked()) {
        _shader.deleteProgram();
    }

    std::string fisheyeFragShader;
    std::string fisheyeVertShader;

    fisheyeVertShader = shaders_fisheye::FisheyeVert;

    if (Settings::instance().useDepthTexture()) {
        switch (Settings::instance().getDrawBufferType()) {
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
                fisheyeFragShader = shaders_fisheye::FisheyeFragDepthNormalPosition;
                break;
        }
    }
    else {
        // no depth
        switch (Settings::instance().getDrawBufferType()) {
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
    if (Settings::instance().useDepthTexture()) {
        _depthCorrectionShader.addShaderSource(
            shaders_fisheye::BaseVert,
            shaders_fisheye::FisheyeDepthCorrectionFrag
        );
    }

    // add functions to shader
    switch (_mappingType) {
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
        glm::radians(_rigOrientation.x),
        glm::vec3(0.f, 1.f, 0.f)
    );
    glm::mat4 yawRot = glm::rotate(
        pitchRot,
        glm::radians(_rigOrientation.y),
        glm::vec3(1.f, 0.f, 0.f)
    );
    glm::mat4 rollRot = glm::rotate(
        yawRot,
        glm::radians(_rigOrientation.z),
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

    // replace color
    std::string color = "vec4(" + std::to_string(_clearColor.r) + ',' +
        std::to_string(_clearColor.g) + ',' + std::to_string(_clearColor.b) + ',' +
        std::to_string(_clearColor.a) + ')';
    helpers::findAndReplace(fisheyeFragShader, "**bgColor**", color);

    std::string name;
    switch (_mappingType) {
        case Mapping::Fisheye:
            name = "FisheyeShader";
            break;
        case Mapping::Equirectangular:
            name = "EquirectangularShader";
            break;
        case Mapping::Cubemap:
            name = "None";
            break;
    }
    _shader = ShaderProgram(std::move(name));
    _shader.addShaderSource(fisheyeVertShader, fisheyeFragShader);
    _shader.createAndLinkProgram();
    _shader.bind();

    _cubemapLoc = glGetUniformLocation(_shader.getId(), "cubemap");
    glUniform1i(_cubemapLoc, 0);

    _halfFovLoc = glGetUniformLocation(_shader.getId(), "halfFov");
    glUniform1f(_halfFovLoc, glm::half_pi<float>());

    ShaderProgram::unbind();

    if (Settings::instance().useDepthTexture()) {
        _depthCorrectionShader = ShaderProgram("FisheyeDepthCorrectionShader");
        _depthCorrectionShader.createAndLinkProgram();
        _depthCorrectionShader.bind();

        _swapColorLoc = glGetUniformLocation(_depthCorrectionShader.getId(), "cTex");
        glUniform1i(_swapColorLoc, 0);
        _swapDepthLoc = glGetUniformLocation(_depthCorrectionShader.getId(), "dTex");
        glUniform1i(_swapDepthLoc, 1);
        _swapNearLoc = glGetUniformLocation(_depthCorrectionShader.getId(), "near");
        _swapFarLoc = glGetUniformLocation(_depthCorrectionShader.getId(), "far");

        ShaderProgram::unbind();
    }
}

void SpoutOutputProjection::initFBO() {
    NonLinearProjection::initFBO();

    _spoutFBO = std::make_unique<core::OffScreenBuffer>();
    _spoutFBO->setInternalColorFormat(_texInternalFormat);
    _spoutFBO->createFBO(_mappingWidth, _mappingHeight, 1);
}

void SpoutOutputProjection::drawCubeFace(int face) {
    BaseViewport& vp = [this](int face) -> BaseViewport& {
        switch (face) {
            default:
            case 0: return _subViewports.right;
            case 1: return _subViewports.left;
            case 2: return _subViewports.bottom;
            case 3: return _subViewports.top;
            case 4: return _subViewports.front;
            case 5: return _subViewports.back;
        }
    }(face);

    glLineWidth(1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
    const glm::vec4 color = Engine::instance().getClearColor();
    const float alpha = Engine::instance().getCurrentWindow().hasAlpha() ? 0.f : color.a;
    glClearColor(color.r, color.g, color.b, alpha);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

    glDisable(GL_SCISSOR_TEST);

    Engine::instance().getDrawFunction()();

    // restore polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void SpoutOutputProjection::blitCubeFace(int face) {
    // bind separate read and draw buffers to prepare blit operation
    _cubeMapFbo->bindBlit();
    attachTextures(face);
    _cubeMapFbo->blit();
}

void SpoutOutputProjection::attachTextures(int face) {
    if (Settings::instance().useDepthTexture()) {
        _cubeMapFbo->attachDepthTexture(_textures.depthSwap);
        _cubeMapFbo->attachColorTexture(_textures.colorSwap);
    }
    else {
        _cubeMapFbo->attachCubeMapTexture(_textures.cubeMapColor, face);
    }

    if (Settings::instance().useNormalTexture()) {
        _cubeMapFbo->attachCubeMapTexture(
            _textures.cubeMapNormals,
            face,
            GL_COLOR_ATTACHMENT1
        );
    }

    if (Settings::instance().usePositionTexture()) {
        _cubeMapFbo->attachCubeMapTexture(
            _textures.cubeMapPositions,
            face,
            GL_COLOR_ATTACHMENT2
        );
    }
}

} // namespace sgct::core
