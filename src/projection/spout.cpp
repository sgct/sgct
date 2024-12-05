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
} // namespace

namespace sgct {

SpoutOutputProjection::SpoutOutputProjection(const Window* parent, User* user,
                                              const config::SpoutOutputProjection& config)
    : NonLinearProjection(parent)
    , _spoutName(config.spoutName)
    , _spout {
        SpoutInfo { config.channels ? config.channels->right : true, nullptr, 0 },
        SpoutInfo { config.channels ? config.channels->left : true, nullptr, 0 },
        SpoutInfo { config.channels ? config.channels->bottom : true, nullptr, 0 },
        SpoutInfo { config.channels ? config.channels->top : true, nullptr, 0 },
        SpoutInfo { config.channels ? config.channels->zLeft : true, nullptr, 0 },
        SpoutInfo { config.channels ? config.channels->zRight : true, nullptr, 0 },
    }
    , _rigOrientation(config.orientation.value_or(vec3{ 0.f, 0.f, 0.f }))
{
    setUser(*user);
    if (config.quality) {
        setCubemapResolution(*config.quality);
    }
}

SpoutOutputProjection::~SpoutOutputProjection() {
#ifdef SGCT_HAS_SPOUT
    for (const SpoutInfo& info : _spout) {
        if (info.handle) {
            reinterpret_cast<SPOUTHANDLE>(info.handle)->ReleaseSender();
            reinterpret_cast<SPOUTHANDLE>(info.handle)->Release();
            glDeleteTextures(1, &info.texture);
        }
    }
#endif // SGCT_HAS_SPOUT

    glDeleteTextures(1, &_mappingTexture);

    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
    _shader.deleteProgram();

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

void SpoutOutputProjection::renderCubemap(FrustumMode frustumMode) const {
    ZoneScoped;

    auto render = [this](const BaseViewport& vp, int idx, FrustumMode mode) {
        if (!_spout[idx].enabled || !vp.isEnabled()) {
            return;
        }

        const int safeIdx = idx % 6;
        renderCubeFace(vp, safeIdx, mode);

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
    };

    render(_subViewports.right, 0, frustumMode);
    render(_subViewports.left, 1, frustumMode);
    render(_subViewports.bottom, 2, frustumMode);
    render(_subViewports.top, 3, frustumMode);
    render(_subViewports.front, 4, frustumMode);
    render(_subViewports.back, 5, frustumMode);
}

void SpoutOutputProjection::setSpoutRigOrientation(vec3 orientation) {
    _rigOrientation = std::move(orientation);
}

void SpoutOutputProjection::initTextures(unsigned int internalFormat, unsigned int format,
                                         unsigned int type)
{
    NonLinearProjection::initTextures(internalFormat, format, type);

    Log::Debug("SpoutOutputProjection initTextures");

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


    _shader = ShaderProgram("Spout");
    _shader.addVertexShader(shaders_fisheye::BaseVert);
    _shader.addFragmentShader(frag);
    _shader.addFragmentShader(shaders_fisheye::SampleFun);
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
}

void SpoutOutputProjection::initFBO(unsigned int internalFormat) {
    NonLinearProjection::initFBO(internalFormat);

    _spoutFBO = std::make_unique<OffScreenBuffer>(internalFormat);
    _spoutFBO->createFBO(_mappingWidth, _mappingHeight, 1);
    glGenFramebuffers(1, &_blitFbo);
}

} // namespace sgct
