/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/spout.h>

#include <sgct/log.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <glm/gtc/matrix_transform.hpp>

#ifdef SGCT_HAS_SPOUT
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <SpoutLibrary.h>
#endif // SGCT_HAS_SPOUT

namespace sgct {

SpoutOutputProjection::SpoutOutputProjection(const Window* parent, User* user,
                                              const config::SpoutOutputProjection& config)
    : NonLinearProjection(parent)
    , _spoutName(config.spoutName.value_or(""))
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

    _clearColor = vec4(0.f, 0.f, 0.f, 1.f);
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

    glDeleteFramebuffers(1, &_blitFbo);
}

void SpoutOutputProjection::update(const vec2&) const {}

void SpoutOutputProjection::render(const BaseViewport& viewport,
                                   FrustumMode frustumMode) const
{
    ZoneScoped;

#ifdef SGCT_HAS_SPOUT
    glEnable(GL_SCISSOR_TEST);
    viewport.setupViewport(frustumMode);
    glClearColor(_clearColor.x, _clearColor.y, _clearColor.z, _clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    GLint saveTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &saveTexture);
    GLint saveFrameBuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &saveFrameBuffer);

    for (int i = 0; i < 6; i++) {
        if (!_spout[i].enabled) {
            continue;
        }
        glBindTexture(GL_TEXTURE_2D, _spout[i].texture);
        const bool s = reinterpret_cast<SPOUTHANDLE>(_spout[i].handle)->SendTexture(
            _spout[i].texture,
            static_cast<GLuint>(GL_TEXTURE_2D),
            _cubemapResolution.x,
            _cubemapResolution.y
        );
        if (!s) {
            Log::Error(std::format(
                "Error sending texture '{}' for face {}", _spout[i].texture, i
            ));
        }
    }

    glBindTexture(GL_TEXTURE_2D, saveTexture);
    glBindFramebuffer(GL_FRAMEBUFFER, saveFrameBuffer);
#endif // SGCT_HAS_SPOUT
}

void SpoutOutputProjection::setSpoutRigOrientation(vec3 orientation) {
    _rigOrientation = std::move(orientation);
}

void SpoutOutputProjection::initTextures(unsigned int internalFormat, unsigned int format,
                                         unsigned int type)
{
    NonLinearProjection::initTextures(internalFormat, format, type);

#ifdef SGCT_HAS_SPOUT
    Log::Debug("SpoutOutputProjection initTextures");

    for (int i = 0; i < 6; i++) {
        Log::Debug(std::format("SpoutOutputProjection initTextures {}", i));
        if (!_spout[i].enabled) {
            continue;
        }
        _spout[i].handle = GetSpout();
        if (_spout[i].handle) {
            constexpr std::array<const char*, 6> CubeMapFaceName = {
                "Right", "zLeft", "Bottom", "Top", "Left", "zRight"
            };

            SPOUTHANDLE h = reinterpret_cast<SPOUTHANDLE>(_spout[i].handle);
            const std::string fullName =
                _spoutName.empty() ?
                CubeMapFaceName[i] :
                std::format("{}-{}", _spoutName, CubeMapFaceName[i]);
            bool success = h->CreateSender(
                fullName.c_str(),
                _cubemapResolution.x,
                _cubemapResolution.y
            );
            if (!success) {
                Log::Error(std::format(
                    "Error creating SPOUT handle for {}", CubeMapFaceName[i]
                ));
            }
        }
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
            _cubemapResolution.x,
            _cubemapResolution.y,
            0,
            format,
            type,
            nullptr
        );
    }
#endif // SGCT_HAS_SPOUT
}

void SpoutOutputProjection::initVBO() {}

void SpoutOutputProjection::initViewports() {
    // distance is needed to calculate the distance to all view planes
    constexpr float Distance = 1.f;

    // setup base viewport that will be rotated to create the other cubemap views
    // +Z face
    const glm::vec4 lowerLeftBase = glm::vec4(-Distance, -Distance, Distance, 1.f);
    const glm::vec4 upperLeftBase = glm::vec4(-Distance, Distance, Distance, 1.f);
    const glm::vec4 upperRightBase = glm::vec4(Distance, Distance, Distance, 1.f);

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

void SpoutOutputProjection::initShaders() {}

void SpoutOutputProjection::initFBO(unsigned int internalFormat) {
    NonLinearProjection::initFBO(internalFormat);

    _spoutFBO = std::make_unique<OffScreenBuffer>(internalFormat);
    _spoutFBO->createFBO(_cubemapResolution.x, _cubemapResolution.y, 1);
    glGenFramebuffers(1, &_blitFbo);
}

void SpoutOutputProjection::renderCubemap(FrustumMode frustumMode) const {
    ZoneScoped;

    auto render = [this](const BaseViewport& vp, int index, FrustumMode mode) {
        if (!_spout[index].enabled) {
            return;
        }

        renderCubeFace(vp, index, mode);

        if (_spout[index].handle) {
            glBindTexture(GL_TEXTURE_2D, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, _blitFbo);
            glFramebufferTexture2D(
                GL_READ_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + index,
                _textures.cubeMapColor,
                0
            );
            glFramebufferTexture2D(
                GL_DRAW_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT1,
                GL_TEXTURE_2D,
                _spout[index].texture,
                0
            );
            glDrawBuffer(GL_COLOR_ATTACHMENT1);
            glBlitFramebuffer(
                0,
                0,
                _cubemapResolution.x,
                _cubemapResolution.y,
                0,
                0,
                _cubemapResolution.x,
                _cubemapResolution.y,
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

} // namespace sgct
