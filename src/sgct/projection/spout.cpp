/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/spout.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/log.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/settings.h>
#include <sgct/window.h>
#include <sgct/shaders/internalfisheyeshaders.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifdef SGCT_HAS_SPOUT
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <SpoutLibrary.h>
#endif

#ifdef SGCT_HAS_SPOUT
namespace {
    constexpr const int NFaces = 6;
    constexpr const std::array<const char*, NFaces> CubeMapFaceName = {
        "Right", "zLeft", "Bottom", "Top", "Left", "zRight"
    };
} // namespace
#endif // SGCT_HAS_SPOUT

namespace sgct {

SpoutOutputProjection::SpoutOutputProjection(const Window* parent)
    : NonLinearProjection(parent)
{
    setUseDepthTransformation(true);
}

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
#else
    // Prevent an unused variable warning
    (void)_mappingHandle;
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

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void SpoutOutputProjection::render(const Window& window, const BaseViewport& viewport,
                                   Frustum::Mode frustumMode)
{
    ZoneScoped

    glEnable(GL_SCISSOR_TEST);
    Engine::instance().setupViewport(window, viewport, frustumMode);
    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    if (_mappingType != Mapping::Cubemap) {
        GLint saveBuffer = {};
        glGetIntegerv(GL_DRAW_BUFFER0, &saveBuffer);
        GLint saveTexture = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &saveTexture);
        GLint saveFrameBuffer = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &saveFrameBuffer);


        GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
        _spoutFBO->bind(false, 1, buffers); // bind no multi-sampled
        _spoutFBO->attachColorTexture(_mappingTexture, GL_COLOR_ATTACHMENT0);

        _shader.bind();

        glViewport(0, 0, _mappingWidth, _mappingHeight);
        glScissor(0, 0, _mappingWidth, _mappingHeight);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        // if for some reson the active texture has been reset
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapColor);

        glDisable(GL_CULL_FACE);
        const bool hasAlpha = window.hasAlpha();
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
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &saveTexture);
        GLint saveFrameBuffer = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &saveFrameBuffer);

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

void SpoutOutputProjection::renderCubemap(Window& window, Frustum::Mode frustumMode) {
    ZoneScoped

    auto renderFace = [this](const Window& window, BaseViewport& vp, unsigned int idx,
                             Frustum::Mode frustumMode)
    {
        if (!_spout[idx].enabled || !vp.isEnabled()) {
            return;
        }

        _cubeMapFbo->bind();
        if (!_cubeMapFbo->isMultiSampled()) {
            attachTextures(idx);
        }

        RenderData renderData(
            window,
            vp,
            frustumMode,
            ClusterManager::instance().sceneTransform(),
            vp.projection(frustumMode).viewMatrix(),
            vp.projection(frustumMode).projectionMatrix(),
            vp.projection(frustumMode).viewProjectionMatrix() *
                ClusterManager::instance().sceneTransform()
        );
        drawCubeFace(vp, renderData);

        // blit MSAA fbo to texture
        if (_cubeMapFbo->isMultiSampled()) {
            blitCubeFace(idx);
        }

        // re-calculate depth values from a cube to spherical model
        if (Settings::instance().useDepthTexture()) {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            _cubeMapFbo->bind(false, 1, buffers); // obind no multi-sampled

            _cubeMapFbo->attachCubeMapTexture(
                _textures.cubeMapColor,
                idx,
                GL_COLOR_ATTACHMENT0
            );
            _cubeMapFbo->attachCubeMapDepthTexture(_textures.cubeMapDepth, idx);

            glViewport(0, 0, _mappingWidth, _mappingHeight);
            glScissor(0, 0, _mappingWidth, _mappingHeight);
            glEnable(GL_SCISSOR_TEST);

            const glm::vec4 color = Engine::instance().clearColor();
            const bool hasAlpha = window.hasAlpha();
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
            glUniform1f(_swapNearLoc, Engine::instance().nearClipPlane());
            glUniform1f(_swapFarLoc, Engine::instance().farClipPlane());

            window.renderScreenQuad();

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

            if (_spout[idx].handle) {
                glBindTexture(GL_TEXTURE_2D, 0);
                // @TODO (abock, 2020-01-09) This function is only available in OpenGL 4.3
                // but we never check whether we are running a 4.3 context or not, so we
                // should replace this with a framebuffer-based blitting instead.
                // Something along the lines of;
                // glBindFramebuffer(GL_FRAMEBUFFER, fbo);
                // glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                //   GL_TEXTURE_2D, tex1, 0);
                // glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                //   GL_TEXTURE_2D, tex2, 0);
                // glDrawBuffer(GL_COLOR_ATTACHMENT1);
                // glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                //   GL_COLOR_BUFFER_BIT, GL_NEAREST);
                glCopyImageSubData(
                    _textures.cubeMapColor,
                    GL_TEXTURE_CUBE_MAP,
                    0,
                    0,
                    0,
                    idx,
                    _spout[idx].texture,
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
    };

    renderFace(window, _subViewports.right, 0, frustumMode);
    renderFace(window, _subViewports.left, 1, frustumMode);
    renderFace(window, _subViewports.bottom, 2, frustumMode);
    renderFace(window, _subViewports.top, 3, frustumMode);
    renderFace(window, _subViewports.front, 4, frustumMode);
    renderFace(window, _subViewports.back, 5, frustumMode);
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
    Log::Debug("SpoutOutputProjection initTextures");

    switch (_mappingType) {
        case Mapping::Cubemap:
            _mappingWidth = _cubemapResolution;
            _mappingHeight = _cubemapResolution;

            for (int i = 0; i < NFaces; ++i) {
#ifdef SGCT_HAS_SPOUT
                Log::Debug("SpoutOutputProjection initTextures %d", i);
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
            Log::Debug("Spout Projection initTextures Equirectangular");
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
            Log::Debug("SpoutOutputProjection initTextures Fisheye");
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
    // distance is needed to calculate the distance to all view planes
    constexpr const float Distance = 1.f;

    // setup base viewport that will be rotated to create the other cubemap views
    // +Z face
    const glm::vec4 lowerLeftBase(-Distance, -Distance, Distance, 1.f);
    const glm::vec4 upperLeftBase(-Distance, Distance, Distance, 1.f);
    const glm::vec4 upperRightBase(Distance, Distance, Distance, 1.f);

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

    // right
    {
        _subViewports.right.setSize(glm::vec2(1.f, 1.f));

        glm::vec4 upperRight = upperRightBase;
        upperRight.x = Distance;

        glm::mat4 r = glm::rotate(rollRot, glm::radians(-90.f), glm::vec3(0.f, 1.f, 0.f));
        _subViewports.right.projectionPlane().setCoordinates(
            glm::vec3(r * lowerLeftBase),
            glm::vec3(r * upperLeftBase),
            glm::vec3(r * upperRight)
        );
    }

    // left
    {
        _subViewports.left.setPos(glm::vec2(0.f, 0.f));
        _subViewports.left.setSize(glm::vec2(1.f, 1.f));

        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.x = -Distance;
        glm::vec4 upperLeft = upperLeftBase;
        upperLeft.x = -Distance;

        glm::mat4 r = glm::rotate(rollRot, glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
        _subViewports.left.projectionPlane().setCoordinates(
            glm::vec3(r * lowerLeft),
            glm::vec3(r * upperLeft),
            glm::vec3(r * upperRightBase)
        );
    }

    // bottom
    {
        _subViewports.bottom.setPos(glm::vec2(0.f, 0.f));
        _subViewports.bottom.setSize(glm::vec2(1.f, 1.f));

        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.y = -Distance;

        glm::mat4 r = glm::rotate(rollRot, glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f));
        _subViewports.bottom.projectionPlane().setCoordinates(
            glm::vec3(r * lowerLeft),
            glm::vec3(r * upperLeftBase),
            glm::vec3(r * upperRightBase)
        );
    }

    // top
    {
        glm::vec4 upperLeft = upperLeftBase;
        upperLeft.y = Distance;
        glm::vec4 upperRight = upperRightBase;
        upperRight.y = Distance;

        _subViewports.top.setSize(glm::vec2(1.f, 1.f));

        glm::mat4 r = glm::rotate(rollRot, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
        _subViewports.top.projectionPlane().setCoordinates(
            glm::vec3(r * lowerLeftBase),
            glm::vec3(r * upperLeft),
            glm::vec3(r * upperRight)
        );
    }

    // front
    _subViewports.front.projectionPlane().setCoordinates(
        glm::vec3(rollRot * lowerLeftBase),
        glm::vec3(rollRot * upperLeftBase),
        glm::vec3(rollRot * upperRightBase)
    );

    // back
    {
        glm::mat4 r = glm::rotate(rollRot, glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f));
        _subViewports.back.projectionPlane().setCoordinates(
            glm::vec3(r * lowerLeftBase),
            glm::vec3(r * upperLeftBase),
            glm::vec3(r * upperRightBase)
        );
    }
}

void SpoutOutputProjection::initShaders() {
    // reload shader program if it exists
    _shader.deleteProgram();

    std::string fisheyeVertShader = shaders_fisheye::BaseVert;
    std::string fisheyeFragShader;

    if (Settings::instance().useDepthTexture()) {
        switch (Settings::instance().drawBufferType()) {
            case Settings::DrawBufferType::Diffuse:
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
            default: throw std::logic_error("Unhandled case label");
        }
    }
    else {
        // no depth
        switch (Settings::instance().drawBufferType()) {
            case Settings::DrawBufferType::Diffuse:
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
            default: throw std::logic_error("Unhandled case label");
        }
    }

    // depth correction shader only
    if (Settings::instance().useDepthTexture()) {
        _depthCorrectionShader.addShaderSource(
            shaders_fisheye::BaseVert,
            shaders_fisheye::FisheyeDepthCorrectionFrag
        );
    }

    const std::string name = [](Mapping mapping) {
        switch (mapping) {
            case Mapping::Fisheye: return "FisheyeShader";
            case Mapping::Equirectangular: return "EquirectangularShader";
            case Mapping::Cubemap: return "None";
            default: throw std::logic_error("Unhandled case label");
        }
    }(_mappingType);

    _shader = ShaderProgram(name);
    _shader.addShaderSource(fisheyeVertShader, fisheyeFragShader);

    std::string samplerShaderCode = [](Mapping mappingType){
        switch (mappingType) {
            case Mapping::Fisheye: return shaders_fisheye::SampleFun;
            case Mapping::Equirectangular: return shaders_fisheye::SampleLatlonFun;
            default: return shaders_fisheye::SampleFun;
        }
    }(_mappingType);
    _shader.addShaderSource(samplerShaderCode, GL_FRAGMENT_SHADER);
    _shader.addShaderSource(shaders_fisheye::RotationFun, GL_FRAGMENT_SHADER);
    _shader.createAndLinkProgram();
    _shader.bind();

    glUniform4fv(
        glGetUniformLocation(_shader.id(), "bgColor"),
        1,
        glm::value_ptr(_clearColor)
    );

    {
        const glm::mat4 pitchRot = glm::rotate(
            glm::mat4(1.f),
            glm::radians(_rigOrientation.x),
            glm::vec3(0.f, 1.f, 0.f)
        );
        const glm::mat4 yawRot = glm::rotate(
            pitchRot,
            glm::radians(_rigOrientation.y),
            glm::vec3(1.f, 0.f, 0.f)
        );
        const glm::mat4 rollRot = glm::rotate(
            yawRot,
            glm::radians(_rigOrientation.z),
            glm::vec3(0.f, 0.f, 1.f)
        );
        glUniformMatrix4fv(
            glGetUniformLocation(_shader.id(), "rotMatrix"),
            1,
            GL_FALSE,
            value_ptr(rollRot)
        );
    }

    _cubemapLoc = glGetUniformLocation(_shader.id(), "cubemap");
    glUniform1i(_cubemapLoc, 0);

    _halfFovLoc = glGetUniformLocation(_shader.id(), "halfFov");
    glUniform1f(_halfFovLoc, glm::half_pi<float>());

    ShaderProgram::unbind();

    if (Settings::instance().useDepthTexture()) {
        _depthCorrectionShader = ShaderProgram("FisheyeDepthCorrectionShader");
        _depthCorrectionShader.createAndLinkProgram();
        _depthCorrectionShader.bind();

        _swapColorLoc = glGetUniformLocation(_depthCorrectionShader.id(), "cTex");
        glUniform1i(_swapColorLoc, 0);
        _swapDepthLoc = glGetUniformLocation(_depthCorrectionShader.id(), "dTex");
        glUniform1i(_swapDepthLoc, 1);
        _swapNearLoc = glGetUniformLocation(_depthCorrectionShader.id(), "near");
        _swapFarLoc = glGetUniformLocation(_depthCorrectionShader.id(), "far");

        ShaderProgram::unbind();
    }
}

void SpoutOutputProjection::initFBO() {
    NonLinearProjection::initFBO();

    _spoutFBO = std::make_unique<OffScreenBuffer>();
    _spoutFBO->setInternalColorFormat(_texInternalFormat);
    _spoutFBO->createFBO(_mappingWidth, _mappingHeight, 1);
}

void SpoutOutputProjection::drawCubeFace(BaseViewport& vp, RenderData renderData) {
    glLineWidth(1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDepthFunc(GL_LESS);

    // run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);
    setupViewport(vp);

    const glm::vec4 color = Engine::instance().clearColor();
    const float alpha = renderData.window.hasAlpha() ? 0.f : color.a;
    glClearColor(color.r, color.g, color.b, alpha);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_SCISSOR_TEST);

    Engine::instance().drawFunction()(renderData);

    // restore polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void SpoutOutputProjection::blitCubeFace(int face) {
    _cubeMapFbo->bindBlit();
    attachTextures(face);
    _cubeMapFbo->blit();
}

void SpoutOutputProjection::attachTextures(int face) {
    if (Settings::instance().useDepthTexture()) {
        _cubeMapFbo->attachDepthTexture(_textures.depthSwap);
        _cubeMapFbo->attachColorTexture(_textures.colorSwap, GL_COLOR_ATTACHMENT0);
    }
    else {
        _cubeMapFbo->attachCubeMapTexture(
            _textures.cubeMapColor,
            face,
            GL_COLOR_ATTACHMENT0
        );
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

} // namespace sgct
