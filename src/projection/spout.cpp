/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/spout.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/format.h>
#include <sgct/internalshaders.h>
#include <sgct/log.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/window.h>
#include <algorithm>
#include <bitset>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifdef SGCT_HAS_SPOUT
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <SpoutLibrary.h>
#endif // SGCT_HAS_SPOUT

#ifdef SGCT_HAS_SPOUT
namespace {
    constexpr int NFaces = 6;
    constexpr std::array<const char*, NFaces> CubeMapFaceName = {
        "Right", "zLeft", "Bottom", "Top", "Left", "zRight"
    };
} // namespace
#endif // SGCT_HAS_SPOUT

namespace {
    struct Vertex {
        float x;
        float y;
        float z;
        float s;
        float t;
    };

    sgct::SpoutOutputProjection::Mapping convert(
                                           sgct::config::SpoutOutputProjection::Mapping m)
    {
        switch (m) {
            case sgct::config::SpoutOutputProjection::Mapping::Fisheye:
                return sgct::SpoutOutputProjection::Mapping::Fisheye;
            case sgct::config::SpoutOutputProjection::Mapping::Equirectangular:
                return sgct::SpoutOutputProjection::Mapping::Equirectangular;
            case sgct::config::SpoutOutputProjection::Mapping::Cubemap:
                return sgct::SpoutOutputProjection::Mapping::Cubemap;
            default:
                throw std::logic_error("Unhandled case label");
        }
    }
} // namespace

namespace sgct {

SpoutOutputProjection::SpoutOutputProjection(const Window* parent, User* user,
                                              const config::SpoutOutputProjection& config)
    : NonLinearProjection(parent)
    , _mainViewport(parent)
    , _mappingType(
        convert(config.mapping.value_or(config::SpoutOutputProjection::Mapping::Cubemap))
    )
    , _mappingName(config.mappingSpoutName)
    , _spout {
        SpoutInfo { config.channels ? config.channels->right : true, nullptr, 0 },
        SpoutInfo { config.channels ? config.channels->left : true, nullptr, 0 },
        SpoutInfo { config.channels ? config.channels->bottom : true, nullptr, 0 },
        SpoutInfo { config.channels ? config.channels->top : true, nullptr, 0 },
        SpoutInfo { config.channels ? config.channels->zLeft : true, nullptr, 0 },
        SpoutInfo { config.channels ? config.channels->zRight : true, nullptr, 0 },
        SpoutInfo { config.drawMain.value_or(true), nullptr, 0 }
    }
    , _rigOrientation(config.orientation.value_or(vec3{ 0.f, 0.f, 0.f }))
{
    setUser(*user);
    if (config.quality) {
        setCubemapResolution(*config.quality);
    }
    _clearColor = config.background.value_or(_clearColor);
}

SpoutOutputProjection::~SpoutOutputProjection() {
#ifdef SGCT_HAS_SPOUT
    for (int i = 0; i < NTextures; i++) {
        if (_spout[i].handle) {
            reinterpret_cast<SPOUTHANDLE>(_spout[i].handle)->ReleaseSender();
            reinterpret_cast<SPOUTHANDLE>(_spout[i].handle)->Release();
            glDeleteTextures(1, &_spout[i].texture);
        }
    }

    if (_mappingHandle) {
        reinterpret_cast<SPOUTHANDLE>(_mappingHandle)->ReleaseSender();
        reinterpret_cast<SPOUTHANDLE>(_mappingHandle)->Release();
    }
#else // ^^^^ SGCT_HAS_SPOUT // !SGCT_HAS_SPOUT vvvv
    // Prevent an unused variable warning
    (void)_mappingHandle;
#endif // SGCT_HAS_SPOUT

    glDeleteTextures(1, &_mappingTexture);

    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
    _shader.deleteProgram();
    _depthCorrectionShader.deleteProgram();

    glDeleteFramebuffers(1, &_blitFbo);
}

void SpoutOutputProjection::update(const vec2&) const {}

void SpoutOutputProjection::render(const BaseViewport& viewport,
                                   FrustumMode frustumMode) const
{
    ZoneScoped;

    glEnable(GL_SCISSOR_TEST);
    viewport.setupViewport(frustumMode);
    glClearColor(_clearColor.x, _clearColor.y, _clearColor.z, _clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    if (_spout[6].enabled) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _spout[6].texture);
        _flatShader.bind();
        viewport.window().renderScreenQuad();
        ShaderProgram::unbind();
    }

    if (_mappingType != Mapping::Cubemap) {
        GLint saveBuffer = 0;
        glGetIntegerv(GL_DRAW_BUFFER0, &saveBuffer);
        GLint saveTexture = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &saveTexture);
        GLint saveFrameBuffer = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &saveFrameBuffer);

        std::array<GLenum, 1> buffers = { GL_COLOR_ATTACHMENT0 };
        _spoutFBO->bind(false, 1, buffers.data()); // bind no multi-sampled
        _spoutFBO->attachColorTexture(_mappingTexture, GL_COLOR_ATTACHMENT0);

        _shader.bind();

        glViewport(0, 0, _mappingWidth, _mappingHeight);
        glScissor(0, 0, _mappingWidth, _mappingHeight);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        // if for some reson the active texture has been reset
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapColor);

        if (Engine::instance().settings().useDepthTexture) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapDepth);
            glUniform1i(_shaderLoc.depthCubemap, 1);
        }

        if (Engine::instance().settings().useNormalTexture) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapNormals);
            glUniform1i(_shaderLoc.normalCubemap, 2);
        }

        if (Engine::instance().settings().usePositionTexture) {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapPositions);
            glUniform1i(_shaderLoc.positionCubemap, 3);
        }

        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);

        glUniform1i(_shaderLoc.cubemap, 0);
        if (_mappingType == Mapping::Fisheye) {
            glUniform1f(_shaderLoc.halfFov, glm::half_pi<float>());
        }
        else if (_mappingType == Mapping::Equirectangular) {
            glUniform1f(_shaderLoc.halfFov, glm::pi<float>());
        }

        glBindVertexArray(_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        ShaderProgram::unbind();

        glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        glDisable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        OffScreenBuffer::unbind();

        glBindTexture(GL_TEXTURE_2D, _mappingTexture);
#ifdef SGCT_HAS_SPOUT
        reinterpret_cast<SPOUTHANDLE>(_mappingHandle)->SendTexture(
            _mappingTexture,
            static_cast<GLuint>(GL_TEXTURE_2D),
            _mappingWidth,
            _mappingHeight
        );
#endif // SGCT_HAS_SPOUT
        glBindTexture(GL_TEXTURE_2D, 0);

        buffers[0] = saveBuffer;
        glBindTexture(GL_TEXTURE_2D, saveTexture);
        glBindFramebuffer(GL_FRAMEBUFFER, saveFrameBuffer);
        glDrawBuffers(1, buffers.data());
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
            const bool s = reinterpret_cast<SPOUTHANDLE>(_spout[i].handle)->SendTexture(
                _spout[i].texture,
                static_cast<GLuint>(GL_TEXTURE_2D),
                _mappingWidth,
                _mappingHeight
            );
            if (!s) {
                Log::Error(std::format(
                    "Error sending texture '{}' for face {}", _spout[i].texture, i
                ));
            }
        }
#endif // SGCT_HAS_SPOUT

        glBindTexture(GL_TEXTURE_2D, saveTexture);
        glBindFramebuffer(GL_FRAMEBUFFER, saveFrameBuffer);
    }
}

void SpoutOutputProjection::renderCubemap(FrustumMode frustumMode) const {
    ZoneScoped;

    auto render = [this](const BaseViewport& vp, int idx, FrustumMode mode) {
        if (!_spout[idx].enabled || !vp.isEnabled()) {
            return;
        }

        const int safeIdx = idx % 6;
        renderCubeFace(vp, safeIdx, mode);


        // re-calculate depth values from a cube to spherical model
        if (Engine::instance().settings().useDepthTexture) {
            std::array<GLenum, 1> buffers = { GL_COLOR_ATTACHMENT0 };
            _cubeMapFbo->bind(false, 1, buffers.data()); // bind no multi-sampled

            _cubeMapFbo->attachCubeMapTexture(
                _textures.cubeMapColor,
                safeIdx,
                GL_COLOR_ATTACHMENT0
            );
            _cubeMapFbo->attachCubeMapDepthTexture(_textures.cubeMapDepth, safeIdx);

            glViewport(0, 0, _cubemapResolution.x, _cubemapResolution.y);
            glScissor(0, 0, _cubemapResolution.x, _cubemapResolution.y);
            glEnable(GL_SCISSOR_TEST);

            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glDisable(GL_CULL_FACE);
            glDisable(GL_BLEND);

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_ALWAYS);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, _textures.colorSwap);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, _textures.depthSwap);

            _depthCorrectionShader.bind();
            glUniform1i(_shaderLoc.swapColor, 0);
            glUniform1i(_shaderLoc.swapDepth, 1);
            glUniform1f(_shaderLoc.swapNear, Engine::instance().nearClipPlane());
            glUniform1f(_shaderLoc.swapFar, Engine::instance().farClipPlane());

            vp.window().renderScreenQuad();
            ShaderProgram::unbind();

            glDisable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDisable(GL_SCISSOR_TEST);
        }

        if (_mappingType == Mapping::Cubemap || idx == 6) {
            OffScreenBuffer::unbind();

            if (_spout[idx].handle) {
                glBindTexture(GL_TEXTURE_2D, 0);

                glBindFramebuffer(GL_FRAMEBUFFER, _blitFbo);
                glFramebufferTexture2D(
                    GL_READ_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + safeIdx,
                    _textures.cubeMapColor,
                    0
                );
                glFramebufferTexture2D(
                    GL_DRAW_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT1,
                    GL_TEXTURE_2D,
                    _spout[idx].texture,
                    0
                );
                glDrawBuffer(GL_COLOR_ATTACHMENT1);
                glBlitFramebuffer(
                    0,
                    0,
                    _mappingWidth,
                    _mappingHeight,
                    0,
                    0,
                    _mappingWidth,
                    _mappingHeight,
                    GL_COLOR_BUFFER_BIT,
                    GL_NEAREST
                );
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
        }
    };

    render(_mainViewport, 6, frustumMode);
    render(_subViewports.right, 0, frustumMode);
    render(_subViewports.left, 1, frustumMode);
    render(_subViewports.bottom, 2, frustumMode);
    render(_subViewports.top, 3, frustumMode);
    render(_subViewports.front, 4, frustumMode);
    render(_subViewports.back, 5, frustumMode);
}

void SpoutOutputProjection::setSpoutChannels(bool right, bool zLeft, bool bottom,
                                             bool top, bool left, bool zRight)
{
    _spout[0].enabled = right;
    _spout[1].enabled = left;
    _spout[2].enabled = bottom;
    _spout[3].enabled = top;
    _spout[4].enabled = zLeft;
    _spout[5].enabled = zRight;
}

void SpoutOutputProjection::setSpoutDrawMain(bool drawMain) {
    _spout[6].enabled = drawMain;
}

void SpoutOutputProjection::setSpoutMappingName(std::string name) {
    _mappingName = std::move(name);
}

void SpoutOutputProjection::setSpoutMapping(Mapping type) {
    _mappingType = type;
}

void SpoutOutputProjection::setSpoutRigOrientation(vec3 orientation) {
    _rigOrientation = std::move(orientation);
}

void SpoutOutputProjection::initTextures(unsigned int internalFormat, unsigned int format,
                                         unsigned int type)
{
    NonLinearProjection::initTextures(internalFormat, format, type);

    Log::Debug("SpoutOutputProjection initTextures");

    switch (_mappingType) {
    case Mapping::Cubemap:
        _mappingWidth = _cubemapResolution.x;
        _mappingHeight = _cubemapResolution.y;

        for (int i = 0; i < NFaces; i++) {
#ifdef SGCT_HAS_SPOUT
            Log::Debug(std::format("SpoutOutputProjection initTextures {}", i));
            if (!_spout[i].enabled) {
                continue;
            }
            _spout[i].handle = GetSpout();
            if (_spout[i].handle) {
                SPOUTHANDLE h = reinterpret_cast<SPOUTHANDLE>(_spout[i].handle);
                bool success = h->CreateSender(
                    CubeMapFaceName[i],
                    _mappingWidth,
                    _mappingHeight
                );
                if (!success) {
                    Log::Error(std::format(
                        "Error creating SPOUT handle for {}", CubeMapFaceName[i]
                    ));
                }
            }
#endif // SGCT_HAS_SPOUT
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
                internalFormat,
                _mappingWidth,
                _mappingHeight,
                0,
                format,
                type,
                nullptr
            );
        }
        break;
    case Mapping::Equirectangular:
        _mappingWidth = _cubemapResolution.x * 4;
        _mappingHeight = _cubemapResolution.y * 2;
#ifdef SGCT_HAS_SPOUT
        Log::Debug("Spout Projection initTextures Equirectangular");
        _mappingHandle = GetSpout();
        if (_mappingHandle) {
            SPOUTHANDLE h = reinterpret_cast<SPOUTHANDLE>(_mappingHandle);
            const bool success = h->CreateSender(
                _mappingName.c_str(),
                _mappingWidth,
                _mappingHeight
            );
            if (!success) {
                Log::Error(std::format(
                    "Error creating SPOUT sender for '{}'", _mappingName
                ));
            }
        }
#endif // SGCT_HAS_SPOUT
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
            internalFormat,
            _mappingWidth,
            _mappingHeight,
            0,
            format,
            type,
            nullptr
        );
        break;
    case Mapping::Fisheye:
        _mappingWidth = _cubemapResolution.x * 2;
        _mappingHeight = _cubemapResolution.y * 2;
#ifdef SGCT_HAS_SPOUT
        Log::Debug("SpoutOutputProjection initTextures Fisheye");
        _mappingHandle = GetSpout();
        if (_mappingHandle) {
            SPOUTHANDLE h = reinterpret_cast<SPOUTHANDLE>(_mappingHandle);
            const bool success = h->CreateSender(
                _mappingName.c_str(),
                _mappingWidth,
                _mappingHeight
            );
            if (!success) {
                Log::Error(std::format(
                    "Error creating SPOUT handle for '{}'", _mappingName
                ));
            }
        }
#endif // SGCT_HAS_SPOUT
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
            internalFormat,
            _mappingWidth,
            _mappingHeight,
            0,
            format,
            type,
            nullptr
        );
        break;
    }

    if (_spout[6].enabled) {
#ifdef SGCT_HAS_SPOUT
        _spout[6].handle = GetSpout();
#endif // SGCT_HAS_SPOUT
        if (_spout[6].handle) {
            glGenTextures(1, &_spout[6].texture);
            glBindTexture(GL_TEXTURE_2D, _spout[6].texture);

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
                internalFormat,
                _cubemapResolution.x,
                _cubemapResolution.y,
                0,
                format,
                type,
                nullptr
            );
        }
    }
}

void SpoutOutputProjection::initVBO() {
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    const std::array<const Vertex, 4> v = {
        Vertex{ -1.f, -1.f, -1.f, 0.f, 0.f },
        Vertex{ -1.f,  1.f, -1.f, 0.f, 1.f },
        Vertex{  1.f, -1.f, -1.f, 1.f, 0.f },
        Vertex{  1.f,  1.f, -1.f, 1.f, 1.f }
    };
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(Vertex), v.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, s))
    );

    glBindVertexArray(0);
}

void SpoutOutputProjection::initViewports() {
    // distance is needed to calculate the distance to all view planes
    constexpr float Distance = 1.f;

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

    // main
    {
        const float angleCorrection = _mappingType == Mapping::Fisheye ? 0.f : -90.f;
        _mainViewport.setPosition(vec2{ 0.f, 0.f });
        _mainViewport.setSize(vec2{ 1.f, 1.f });

        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.y = -Distance;

        const glm::mat4 r = glm::rotate(
            glm::mat4(1.f),
            glm::radians(angleCorrection),
            glm::vec3(1.f, 0.f, 0.f)
        );
        const glm::vec3 ll = glm::vec3(r * lowerLeft);
        const glm::vec3 ul = glm::vec3(r * upperLeftBase);
        const glm::vec3 ur = glm::vec3(r * upperRightBase);
        _mainViewport.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // right
    {
        _subViewports.right.setSize(vec2{ 1.f, 1.f });

        glm::vec4 upperRight = upperRightBase;
        upperRight.x = Distance;

        const glm::mat4 r = glm::rotate(
            rollRot,
            glm::radians(-90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );
        const glm::vec3 ll = glm::vec3(r * lowerLeftBase);
        const glm::vec3 ul = glm::vec3(r * upperLeftBase);
        const glm::vec3 ur = glm::vec3(r * upperRight);
        _subViewports.right.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // left
    {
        _subViewports.left.setPosition(vec2{ 0.f, 0.f });
        _subViewports.left.setSize(vec2{ 1.f, 1.f });

        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.x = -Distance;
        glm::vec4 upperLeft = upperLeftBase;
        upperLeft.x = -Distance;

        const glm::mat4 r = glm::rotate(
            rollRot,
            glm::radians(90.f),
            glm::vec3(0.f, 1.f, 0.f)
        );
        const glm::vec3 ll = glm::vec3(r * lowerLeft);
        const glm::vec3 ul = glm::vec3(r * upperLeft);
        const glm::vec3 ur = glm::vec3(r * upperRightBase);
        _subViewports.left.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // bottom
    {
        _subViewports.bottom.setPosition(vec2{ 0.f, 0.f });
        _subViewports.bottom.setSize(vec2{ 1.f, 1.f });

        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.y = -Distance;

        const glm::mat4 r = glm::rotate(
            rollRot,
            glm::radians(-90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );
        const glm::vec3 ll = glm::vec3(r * lowerLeft);
        const glm::vec3 ul = glm::vec3(r * upperLeftBase);
        const glm::vec3 ur = glm::vec3(r * upperRightBase);
        _subViewports.bottom.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // top
    {
        glm::vec4 upperLeft = upperLeftBase;
        upperLeft.y = Distance;
        glm::vec4 upperRight = upperRightBase;
        upperRight.y = Distance;

        _subViewports.top.setSize(vec2{ 1.f, 1.f });

        const glm::mat4 r = glm::rotate(
            rollRot,
            glm::radians(90.f),
            glm::vec3(1.f, 0.f, 0.f)
        );
        const glm::vec3 ll = glm::vec3(r * lowerLeftBase);
        const glm::vec3 ul = glm::vec3(r * upperLeft);
        const glm::vec3 ur = glm::vec3(r * upperRight);
        _subViewports.top.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // front
    {
        const glm::vec3 ll = glm::vec3(rollRot * lowerLeftBase);
        const glm::vec3 ul = glm::vec3(rollRot * upperLeftBase);
        const glm::vec3 ur = glm::vec3(rollRot * upperRightBase);
        _subViewports.front.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }

    // back
    {
        const glm::mat4 r = glm::rotate(
            rollRot,
            glm::radians(180.f),
            glm::vec3(0.f, 1.f, 0.f)
        );
        const glm::vec3 ll = glm::vec3(r * lowerLeftBase);
        const glm::vec3 ul = glm::vec3(r * upperLeftBase);
        const glm::vec3 ur = glm::vec3(r * upperRightBase);
        _subViewports.back.projectionPlane().setCoordinates(
            vec3(ll.x, ll.y, ll.z),
            vec3(ul.x, ul.y, ul.z),
            vec3(ur.x, ur.y, ur.z)
        );
    }
}

void SpoutOutputProjection::updateFrustums(FrustumMode mode, float nearClip,
                                           float farClip)
{
    if (_mainViewport.isEnabled()) {
        _mainViewport.calculateNonLinearFrustum(mode, nearClip, farClip);
    }

    NonLinearProjection::updateFrustums(mode, nearClip, farClip);
}

void SpoutOutputProjection::setUser(User& user) {
    _mainViewport.setUser(user);
    NonLinearProjection::setUser(user);
}

void SpoutOutputProjection::initShaders() {
    // reload shader program if it exists
    _shader.deleteProgram();

    const std::string_view frag = [](bool useDepth, bool useNormal, bool usePosition) {
        // It would be nice to do a multidimensional switch statement -.-

        constexpr auto tuple = [](bool depth, bool normal, bool position) {
            // Injective mapping from <bool, bool, bool> to uint8_t
            uint8_t value = 0;
            value |= depth ? 0b000 : 0b001;
            value |= normal ? 0b000 : 0b010;
            value |= position ? 0b000 : 0b100;
            return value;
        };

        switch (tuple(useDepth, useNormal, usePosition)) {
            case tuple(true, false, false):
                return shaders_fisheye::FisheyeFragDepth;
            case tuple(true, true, false):
                return shaders_fisheye::FisheyeFragDepthNormal;
            case tuple(true, false, true):
                return shaders_fisheye::FisheyeFragDepthPosition;
            case tuple(true, true, true):
                return shaders_fisheye::FisheyeFragDepthNormalPosition;

            case tuple(false, false, false):
                return shaders_fisheye::FisheyeFrag;
            case tuple(false, true, false):
                return shaders_fisheye::FisheyeFragNormal;
            case tuple(false, false, true):
                return shaders_fisheye::FisheyeFragPosition;
            case tuple(false, true, true):
                return shaders_fisheye::FisheyeFragNormalPosition;

            default:
                throw std::logic_error("Unhandled case label");
        }
    }(
        Engine::instance().settings().useDepthTexture,
        Engine::instance().settings().useNormalTexture,
        Engine::instance().settings().usePositionTexture
    );


    std::string name = [](Mapping mapping) {
        switch (mapping) {
            case Mapping::Fisheye:         return "FisheyeShader";
            case Mapping::Equirectangular: return "EquirectangularShader";
            case Mapping::Cubemap:         return "None";
            default:                       throw std::logic_error("Unhandled case label");
        }
    }(_mappingType);

    _shader = ShaderProgram(std::move(name));
    _shader.addVertexShader(shaders_fisheye::BaseVert);
    _shader.addFragmentShader(frag);

    const std::string_view samplerShaderCode = [](Mapping mappingType) {
        switch (mappingType) {
            case Mapping::Fisheye:         return shaders_fisheye::SampleFun;
            case Mapping::Equirectangular: return shaders_fisheye::SampleLatlonFun;
            default:                       return shaders_fisheye::SampleFun;
        }
    }(_mappingType);
    _shader.addFragmentShader(samplerShaderCode);
    _shader.addFragmentShader(shaders_fisheye::RotationFun);
    _shader.addFragmentShader(
        _interpolationMode == InterpolationMode::Cubic ?
        shaders_fisheye::InterpolateCubicFun :
        shaders_fisheye::InterpolateLinearFun
    );
    _shader.createAndLinkProgram();
    _shader.bind();


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
        const GLint rotMat = glGetUniformLocation(_shader.id(), "rotMatrix");
        glUniformMatrix4fv(rotMat, 1, GL_FALSE, glm::value_ptr(rollRot));
    }

    glUniform4fv(glGetUniformLocation(_shader.id(), "bgColor"), 1, &_clearColor.x);
    if (_interpolationMode == InterpolationMode::Cubic) {
        glUniform1f(
            glGetUniformLocation(_shader.id(), "size"),
            static_cast<float>(_cubemapResolution.x)
        );
    }

    _shaderLoc.cubemap = glGetUniformLocation(_shader.id(), "cubemap");
    glUniform1i(_shaderLoc.cubemap, 0);

    if (Engine::instance().settings().useDepthTexture) {
        _shaderLoc.depthCubemap = glGetUniformLocation(_shader.id(), "depthmap");
        glUniform1i(_shaderLoc.depthCubemap, 1);
    }

    if (Engine::instance().settings().useNormalTexture) {
        _shaderLoc.normalCubemap = glGetUniformLocation(_shader.id(), "normalmap");
        glUniform1i(_shaderLoc.normalCubemap, 2);
    }

    if (Engine::instance().settings().usePositionTexture) {
        _shaderLoc.positionCubemap = glGetUniformLocation(_shader.id(), "positionmap");
        glUniform1i(_shaderLoc.positionCubemap, 3);
    }

    _shaderLoc.halfFov = glGetUniformLocation(_shader.id(), "halfFov");
    glUniform1f(_shaderLoc.halfFov, glm::half_pi<float>());

    ShaderProgram::unbind();

    _flatShader.deleteProgram();
    _flatShader = ShaderProgram("SpoutShader");
    _flatShader.addVertexShader(shaders::BaseVert);
    _flatShader.addFragmentShader(shaders::BaseFrag);
    _flatShader.createAndLinkProgram();
    _flatShader.bind();
    glUniform1i(glGetUniformLocation(_flatShader.id(), "tex"), 0);

    ShaderProgram::unbind();

    if (Engine::instance().settings().useDepthTexture) {
        _depthCorrectionShader = ShaderProgram("FisheyeDepthCorrectionShader");
        _depthCorrectionShader.addVertexShader(shaders_fisheye::BaseVert);
        _depthCorrectionShader.addFragmentShader(
            shaders_fisheye::FisheyeDepthCorrectionFrag
        );
        _depthCorrectionShader.createAndLinkProgram();
        _depthCorrectionShader.bind();

        _shaderLoc.swapColor = glGetUniformLocation(_depthCorrectionShader.id(), "cTex");
        glUniform1i(_shaderLoc.swapColor, 0);
        _shaderLoc.swapDepth = glGetUniformLocation(_depthCorrectionShader.id(), "dTex");
        glUniform1i(_shaderLoc.swapDepth, 1);
        _shaderLoc.swapNear = glGetUniformLocation(_depthCorrectionShader.id(), "near");
        _shaderLoc.swapFar = glGetUniformLocation(_depthCorrectionShader.id(), "far");

        ShaderProgram::unbind();
    }
}

void SpoutOutputProjection::initFBO(unsigned int internalFormat) {
    NonLinearProjection::initFBO(internalFormat);

    _spoutFBO = std::make_unique<OffScreenBuffer>(internalFormat);
    _spoutFBO->createFBO(_mappingWidth, _mappingHeight, 1);
    glGenFramebuffers(1, &_blitFbo);
}

} // namespace sgct
