/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2022                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/spout.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/fmt.h>
#include <sgct/internalshaders.h>
#include <sgct/log.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/settings.h>
#include <sgct/window.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifdef SGCT_HAS_SPOUT
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
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

namespace {
    template <typename From, typename To>
    To fromGLM(From v) {
        To r;
        std::memcpy(&r, glm::value_ptr(v), sizeof(To));
        return r;
    }

    struct Vertex {
        float x;
        float y;
        float z;
        float s;
        float t;
    };
} // namespace

namespace sgct {

SpoutOutputProjection::SpoutOutputProjection(const Window* parent)
    : NonLinearProjection(parent)
{}

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

    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
    _shader.deleteProgram();
    _depthCorrectionShader.deleteProgram();

    glDeleteFramebuffers(1, &_blitFbo);
}

void SpoutOutputProjection::update(vec2) {
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    const std::array<const Vertex, 4> v = {
        Vertex{ -1.f, -1.f, -1.f, 0.f, 0.f },
        Vertex{ -1.f,  1.f, -1.f, 0.f, 1.f },
        Vertex{  1.f, -1.f, -1.f, 1.f, 0.f },
        Vertex{  1.f,  1.f, -1.f, 1.f, 1.f }
    };
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(Vertex), v.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void SpoutOutputProjection::render(const Window& window, const BaseViewport& viewport,
                                   Frustum::Mode frustumMode)
{
    ZoneScoped

    glEnable(GL_SCISSOR_TEST);
    Engine::instance().setupViewport(window, viewport, frustumMode);
    glClearColor(_clearColor.x, _clearColor.y, _clearColor.z, _clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    if (_mappingType != Mapping::Cubemap) {
        GLint saveBuffer = 0;
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

        if (Settings::instance().useDepthTexture()) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapDepth);
            glUniform1i(_shaderLoc.depthCubemap, 1);
        }

        if (Settings::instance().useNormalTexture()) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapNormals);
            glUniform1i(_shaderLoc.normalCubemap, 2);
        }

        if (Settings::instance().usePositionTexture()) {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_CUBE_MAP, _textures.cubeMapPositions);
            glUniform1i(_shaderLoc.positionCubemap, 3);
        }

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
            const bool s = reinterpret_cast<SPOUTHANDLE>(_spout[i].handle)->SendTexture(
                _spout[i].texture,
                static_cast<GLuint>(GL_TEXTURE_2D),
                _mappingWidth,
                _mappingHeight
            );
            if (!s) {
                Log::Error(fmt::format(
                    "Error sending texture '{}' for face {}", _spout[i].texture, i
                ));
            }
        }
#endif

        glBindTexture(GL_TEXTURE_2D, saveTexture);
        glBindFramebuffer(GL_FRAMEBUFFER, saveFrameBuffer);
    }
}

void SpoutOutputProjection::renderCubemap(Window& window, Frustum::Mode frustumMode) {
    ZoneScoped

    auto render = [this](const Window& win, BaseViewport& vp, int idx, Frustum::Mode mode)
    {
        if (!_spout[idx].enabled || !vp.isEnabled()) {
            return;
        }

        renderCubeFace(win, vp, idx, mode);


        // re-calculate depth values from a cube to spherical model
        if (Settings::instance().useDepthTexture()) {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            _cubeMapFbo->bind(false, 1, buffers); // bind no multi-sampled

            _cubeMapFbo->attachCubeMapTexture(
                _textures.cubeMapColor,
                idx,
                GL_COLOR_ATTACHMENT0
            );
            _cubeMapFbo->attachCubeMapDepthTexture(_textures.cubeMapDepth, idx);

            glViewport(0, 0, _cubemapResolution, _cubemapResolution);
            glScissor(0, 0, _cubemapResolution, _cubemapResolution);
            glEnable(GL_SCISSOR_TEST);

            const vec4 color = Engine::instance().clearColor();
            const bool hasAlpha = win.hasAlpha();
            glClearColor(color.x, color.y, color.z, hasAlpha ? 0.f : color.w);
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

            _depthCorrectionShader.bind();
            glUniform1i(_shaderLoc.swapColor, 0);
            glUniform1i(_shaderLoc.swapDepth, 1);
            glUniform1f(_shaderLoc.swapNear, Engine::instance().nearClipPlane());
            glUniform1f(_shaderLoc.swapFar, Engine::instance().farClipPlane());

            win.renderScreenQuad();
            ShaderProgram::unbind();

            glDisable(GL_DEPTH_TEST);

            if (hasAlpha) {
                glDisable(GL_BLEND);
            }

            glDepthFunc(GL_LESS);
            glDisable(GL_SCISSOR_TEST);
        }

        if (_mappingType == Mapping::Cubemap) {
            _cubeMapFbo->unbind();

            if (_spout[idx].handle) {
                glBindTexture(GL_TEXTURE_2D, 0);
                
                glBindFramebuffer(GL_FRAMEBUFFER, _blitFbo);
                glFramebufferTexture2D(
                    GL_READ_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + idx,
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

    render(window, _subViewports.right, 0, frustumMode);
    render(window, _subViewports.left, 1, frustumMode);
    render(window, _subViewports.bottom, 2, frustumMode);
    render(window, _subViewports.top, 3, frustumMode);
    render(window, _subViewports.front, 4, frustumMode);
    render(window, _subViewports.back, 5, frustumMode);
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

void SpoutOutputProjection::setSpoutRigOrientation(vec3 orientation) {
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
            Log::Debug(fmt::format("SpoutOutputProjection initTextures {}", i));
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
                    Log::Error(fmt::format(
                        "Error creating SPOUT handle for {}", CubeMapFaceName[i]
                    ));
                }
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
            const bool success = h->CreateSender(
                _mappingName.c_str(),
                _mappingWidth,
                _mappingHeight
            );
            if (!success) {
                Log::Error(fmt::format(
                    "Error creating SPOUT sender for '{}'", _mappingName
                ));
            }
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
            const bool success = h->CreateSender(
                _mappingName.c_str(),
                _mappingWidth,
                _mappingHeight
            );
            if (!success) {
                Log::Error(fmt::format(
                    "Error creating SPOUT handle for '{}'", _mappingName
                ));
            }
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

void SpoutOutputProjection::initVBO() {
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

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
        _subViewports.right.setSize(vec2{ 1.f, 1.f });

        glm::vec4 upperRight = upperRightBase;
        upperRight.x = Distance;

        glm::mat4 r = glm::rotate(rollRot, glm::radians(-90.f), glm::vec3(0.f, 1.f, 0.f));
        _subViewports.right.projectionPlane().setCoordinates(
            fromGLM<glm::vec3, vec3>(glm::vec3(r * lowerLeftBase)),
            fromGLM<glm::vec3, vec3>(glm::vec3(r * upperLeftBase)),
            fromGLM<glm::vec3, vec3>(glm::vec3(r * upperRight))
        );
    }

    // left
    {
        _subViewports.left.setPos(vec2{ 0.f, 0.f });
        _subViewports.left.setSize(vec2{ 1.f, 1.f });

        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.x = -Distance;
        glm::vec4 upperLeft = upperLeftBase;
        upperLeft.x = -Distance;

        glm::mat4 r = glm::rotate(rollRot, glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
        _subViewports.left.projectionPlane().setCoordinates(
            fromGLM<glm::vec3, vec3>(glm::vec3(r * lowerLeft)),
            fromGLM<glm::vec3, vec3>(glm::vec3(r * upperLeft)),
            fromGLM<glm::vec3, vec3>(glm::vec3(r * upperRightBase))
        );
    }

    // bottom
    {
        _subViewports.bottom.setPos(vec2{ 0.f, 0.f });
        _subViewports.bottom.setSize(vec2{ 1.f, 1.f });

        glm::vec4 lowerLeft = lowerLeftBase;
        lowerLeft.y = -Distance;

        glm::mat4 r = glm::rotate(rollRot, glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f));
        _subViewports.bottom.projectionPlane().setCoordinates(
            fromGLM<glm::vec3, vec3>(glm::vec3(r * lowerLeft)),
            fromGLM<glm::vec3, vec3>(glm::vec3(r * upperLeftBase)),
            fromGLM<glm::vec3, vec3>(glm::vec3(r * upperRightBase))
        );
    }

    // top
    {
        glm::vec4 upperLeft = upperLeftBase;
        upperLeft.y = Distance;
        glm::vec4 upperRight = upperRightBase;
        upperRight.y = Distance;

        _subViewports.top.setSize(vec2{ 1.f, 1.f });

        glm::mat4 r = glm::rotate(rollRot, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
        _subViewports.top.projectionPlane().setCoordinates(
            fromGLM<glm::vec3, vec3>(glm::vec3(r * lowerLeftBase)),
            fromGLM<glm::vec3, vec3>(glm::vec3(r * upperLeft)),
            fromGLM<glm::vec3, vec3>(glm::vec3(r * upperRight))
        );
    }

    // front
    _subViewports.front.projectionPlane().setCoordinates(
        fromGLM<glm::vec3, vec3>(glm::vec3(rollRot * lowerLeftBase)),
        fromGLM<glm::vec3, vec3>(glm::vec3(rollRot * upperLeftBase)),
        fromGLM<glm::vec3, vec3>(glm::vec3(rollRot * upperRightBase))
    );

    // back
    {
        glm::mat4 r = glm::rotate(rollRot, glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f));
        _subViewports.back.projectionPlane().setCoordinates(
            fromGLM<glm::vec3, vec3>(glm::vec3(r * lowerLeftBase)),
            fromGLM<glm::vec3, vec3>(glm::vec3(r * upperLeftBase)),
            fromGLM<glm::vec3, vec3>(glm::vec3(r * upperRightBase))
        );
    }
}

void SpoutOutputProjection::initShaders() {
    // reload shader program if it exists
    _shader.deleteProgram();

    const bool isCubic = (_interpolationMode == InterpolationMode::Cubic);
    std::string fragmentShader = [](bool useDepth, Settings::DrawBufferType type) {
        // It would be nice to do a multidimensional switch statement -.-

        constexpr auto tuple = [](bool depth, Settings::DrawBufferType t) -> uint16_t {
            // Injective mapping from <bool, DrawBufferType> to uint16_t
            uint16_t res = 0;
            res += static_cast<uint8_t>(t);
            if (depth) {
                res += 1 << 11;
            }
            return res;
        };

        using DrawBufferType = Settings::DrawBufferType;
        switch (tuple(useDepth, type)) {
            case tuple(true, DrawBufferType::Diffuse):
                return shaders_fisheye::FisheyeFragDepth;
            case tuple(true, DrawBufferType::DiffuseNormal):
                return shaders_fisheye::FisheyeFragDepthNormal;
            case tuple(true, DrawBufferType::DiffusePosition):
                return shaders_fisheye::FisheyeFragDepthPosition;
            case tuple(true, DrawBufferType::DiffuseNormalPosition):
                return shaders_fisheye::FisheyeFragDepthNormalPosition;

            case tuple(false, DrawBufferType::Diffuse):
                return shaders_fisheye::FisheyeFrag;
            case tuple(false, DrawBufferType::DiffuseNormal):
                return shaders_fisheye::FisheyeFragNormal;
            case tuple(false, DrawBufferType::DiffusePosition):
                return shaders_fisheye::FisheyeFragPosition;
            case tuple(false, DrawBufferType::DiffuseNormalPosition):
                return shaders_fisheye::FisheyeFragNormalPosition;

            default:
                throw std::logic_error("Unhandled case label");
        }
    }(Settings::instance().useDepthTexture(), Settings::instance().drawBufferType());


    std::string name = [](Mapping mapping) {
        switch (mapping) {
            case Mapping::Fisheye:         return "FisheyeShader";
            case Mapping::Equirectangular: return "EquirectangularShader";
            case Mapping::Cubemap:         return "None";
            default:                       throw std::logic_error("Unhandled case label");
        }
    }(_mappingType);

    _shader = ShaderProgram(std::move(name));
    _shader.addShaderSource(shaders_fisheye::BaseVert, fragmentShader);

    std::string samplerShaderCode = [](Mapping mappingType) {
        switch (mappingType) {
            case Mapping::Fisheye:         return shaders_fisheye::SampleFun;
            case Mapping::Equirectangular: return shaders_fisheye::SampleLatlonFun;
            default:                       return shaders_fisheye::SampleFun;
        }
    }(_mappingType);
    _shader.addShaderSource(std::move(samplerShaderCode), GL_FRAGMENT_SHADER);
    _shader.addShaderSource(shaders_fisheye::RotationFun, GL_FRAGMENT_SHADER);
    _shader.addShaderSource(
        _interpolationMode == InterpolationMode::Cubic ?
        shaders_fisheye::InterpolateCubicFun :
        shaders_fisheye::InterpolateLinearFun, GL_FRAGMENT_SHADER
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
        GLint rotMat = glGetUniformLocation(_shader.id(), "rotMatrix");
        glUniformMatrix4fv(rotMat, 1, GL_FALSE, value_ptr(rollRot));
    }

    glUniform4fv(glGetUniformLocation(_shader.id(), "bgColor"), 1, &_clearColor.x);
    if (isCubic) {
        glUniform1f(
            glGetUniformLocation(_shader.id(), "size"),
            static_cast<float>(_cubemapResolution)
        );
    }

    _shaderLoc.cubemap = glGetUniformLocation(_shader.id(), "cubemap");
    glUniform1i(_shaderLoc.cubemap, 0);

    if (Settings::instance().useDepthTexture()) {
        _shaderLoc.depthCubemap = glGetUniformLocation(_shader.id(), "depthmap");
        glUniform1i(_shaderLoc.depthCubemap, 1);
    }

    if (Settings::instance().useNormalTexture()) {
        _shaderLoc.normalCubemap = glGetUniformLocation(_shader.id(), "normalmap");
        glUniform1i(_shaderLoc.normalCubemap, 2);
    }

    if (Settings::instance().usePositionTexture()) {
        _shaderLoc.positionCubemap = glGetUniformLocation(_shader.id(), "positionmap");
        glUniform1i(_shaderLoc.positionCubemap, 3);
    }

    _shaderLoc.halfFov = glGetUniformLocation(_shader.id(), "halfFov");
    glUniform1f(_shaderLoc.halfFov, glm::half_pi<float>());

    ShaderProgram::unbind();

    if (Settings::instance().useDepthTexture()) {
        _depthCorrectionShader = ShaderProgram("FisheyeDepthCorrectionShader");
        _depthCorrectionShader.addShaderSource(
            shaders_fisheye::BaseVert,
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

void SpoutOutputProjection::initFBO() {
    NonLinearProjection::initFBO();

    _spoutFBO = std::make_unique<OffScreenBuffer>();
    _spoutFBO->setInternalColorFormat(_texInternalFormat);
    _spoutFBO->createFBO(_mappingWidth, _mappingHeight, 1);
    glGenFramebuffers(1, &_blitFbo);
}

} // namespace sgct
