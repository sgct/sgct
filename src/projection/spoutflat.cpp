/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/projection/spoutflat.h>

#include <sgct/callbackdata.h>
#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/internalshaders.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/settings.h>
#include <algorithm>
#include <array>
#include <cmath>
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

namespace sgct {

namespace {
    struct Vertex {
        float x;
        float y;
        float s;
        float t;
        float c0;
        float c1;
        float c2;
        float c3;
    };
} // namespace

SpoutFlatProjection::SpoutFlatProjection(const Window* parent)
    : NonLinearProjection(parent)
{}

SpoutFlatProjection::~SpoutFlatProjection() {
#ifdef SGCT_HAS_SPOUT
    if (_mappingHandle) {
        _mappingHandle->ReleaseSender();
        _mappingHandle->Release();
    }
#else // SGCT_HAS_SPOUT
    // Prevent an unused variable warning
    (void)_mappingHandle;
#endif // SGCT_HAS_SPOUT

    glDeleteTextures(1, &_textureIdentifiers.spoutColor);
    glDeleteTextures(1, &_textureIdentifiers.spoutDepth);
    glDeleteTextures(1, &_textureIdentifiers.spoutNormals);
    glDeleteTextures(1, &_textureIdentifiers.spoutPositions);
    glDeleteTextures(1, &_textureIdentifiers.colorSwap);
    glDeleteTextures(1, &_textureIdentifiers.depthSwap);

    _shader.deleteProgram();
}

void SpoutFlatProjection::setResolutionWidth(int resolutionX) {
    _resolutionX = resolutionX;
    _cubemapResolution.x = resolutionX;
}

void SpoutFlatProjection::setResolutionHeight(int resolutionY) {
    _resolutionY = resolutionY;
    _cubemapResolution.y = resolutionY;
}

void SpoutFlatProjection::setSpoutMappingName(std::string name) {
    _mappingName = std::move(name);
}

void SpoutFlatProjection::setSpoutFov(float up, float down, float left, float right,
                                      quat orientation, float distance)
{
    _spoutFov.up = up;
    _spoutFov.down = down;
    _spoutFov.left = left;
    _spoutFov.right = right;
    _spoutFov.distance = distance;
    _spoutOrientation = std::move(orientation);
}

void SpoutFlatProjection::setSpoutOffset(vec3 offset) {
    _spoutOffset = std::move(offset);
}

void SpoutFlatProjection::setSpoutDrawMain(bool drawMain) {
    _spoutDrawMain = drawMain;
}

void SpoutFlatProjection::initTextures() {
    generateMap(_textureIdentifiers.spoutColor, _texInternalFormat, _texFormat, _texType);
    Log::Debug(fmt::format(
        "{0}x{1} color spout texture (id: {2}) generated",
        _resolutionX, _resolutionY, _textureIdentifiers.spoutColor
    ));

    if (Settings::instance().useDepthTexture()) {
        generateMap(
            _textureIdentifiers.spoutDepth,
            GL_DEPTH_COMPONENT32,
            GL_DEPTH_COMPONENT,
            GL_FLOAT
        );
        Log::Debug(fmt::format(
            "{0}x{1} depth spout texture (id: {2}) generated",
            _resolutionX, _resolutionY, _textureIdentifiers.spoutDepth
        ));

        if (_useDepthTransformation) {
            // generate swap textures
            generateMap(
                _textureIdentifiers.depthSwap,
                GL_DEPTH_COMPONENT32,
                GL_DEPTH_COMPONENT,
                GL_FLOAT
            );
            Log::Debug(fmt::format(
                "{0}x{1} depth swap map texture (id: {2}) generated",
                _resolutionX, _resolutionY, _textureIdentifiers.depthSwap
            ));

            generateMap(
                _textureIdentifiers.colorSwap,
                _texInternalFormat,
                _texFormat,
                _texType
            );
            Log::Debug(fmt::format(
                "{0}x{1} color swap map texture (id: {2}) generated",
                _resolutionX, _resolutionY, _textureIdentifiers.colorSwap
            ));
        }
    }

    if (Settings::instance().useNormalTexture()) {
        generateMap(
            _textureIdentifiers.spoutNormals,
            Settings::instance().bufferFloatPrecision(),
            GL_RGB,
            GL_FLOAT
        );
        Log::Debug(fmt::format(
            "{0}x{1} normal spout texture (id: {2}) generated",
            _resolutionX, _resolutionY, _textureIdentifiers.spoutNormals
        ));
    }

    if (Settings::instance().usePositionTexture()) {
        generateMap(
            _textureIdentifiers.spoutPositions,
            Settings::instance().bufferFloatPrecision(),
            GL_RGB,
            GL_FLOAT
        );
        Log::Debug(fmt::format(
            "{0}x{1} position spout texture ({2}) generated",
            _resolutionX, _resolutionY, _textureIdentifiers.spoutPositions
        ));
    }


#ifdef SGCT_HAS_SPOUT
    Log::Debug(fmt::format("SpoutFlat initTextures"));
    _mappingHandle = GetSpout();
    if (_mappingHandle) {
        bool success = _mappingHandle->CreateSender(
            _mappingName.c_str(),
            _resolutionX,
            _resolutionY
        );
        if (!success) {
            Log::Error(fmt::format("Error creating SPOUT handle for {}", _mappingName));
        }
    }
#endif // SGCT_HAS_SPOUT
}

void SpoutFlatProjection::initFBO() {
    _spoutFbo = std::make_unique<OffScreenBuffer>();
    _spoutFbo->setInternalColorFormat(_texInternalFormat);
    _spoutFbo->createFBO(_resolutionX, _resolutionY, _samples);
}

void SpoutFlatProjection::setupViewport(BaseViewport& vp) {
    _vpCoords = ivec4{
        static_cast<int>(floor(vp.position().x * _resolutionX + 0.5f)),
        static_cast<int>(floor(vp.position().y * _resolutionY + 0.5f)),
        static_cast<int>(floor(vp.size().x * _resolutionX + 0.5f)),
        static_cast<int>(floor(vp.size().y * _resolutionY + 0.5f))
    };

    glViewport(_vpCoords.x, _vpCoords.y, _vpCoords.z, _vpCoords.w);
    glScissor(_vpCoords.x, _vpCoords.y, _vpCoords.z, _vpCoords.w);
}

void SpoutFlatProjection::generateMap(unsigned int& texture, unsigned int internalFormat,
                                      unsigned int format, unsigned int type)
{
    glDeleteTextures(1, &texture);

    GLint maxMapRes;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxMapRes);
    if (_resolutionX > maxMapRes || _resolutionY > maxMapRes) {
        Log::Error(fmt::format(
            "Requested size is too big ({} > {}) || ({} > {})",
            _resolutionX, maxMapRes, _resolutionY, maxMapRes
        ));
    }

    // set up texture target
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Disable mipmaps
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internalFormat,
        _resolutionX,
        _resolutionY,
        0,
        format,
        type,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void SpoutFlatProjection::attachTextures(int) {
    if (Settings::instance().useDepthTexture()) {
        _spoutFbo->attachDepthTexture(_textureIdentifiers.depthSwap);
        _spoutFbo->attachColorTexture(
            _textureIdentifiers.colorSwap,
            GL_COLOR_ATTACHMENT0
        );
    }
    else {
        _spoutFbo->attachColorTexture(
            _textureIdentifiers.spoutColor,
            GL_COLOR_ATTACHMENT0
        );
    }

    if (Settings::instance().useNormalTexture()) {
        _spoutFbo->attachColorTexture(
            _textureIdentifiers.spoutNormals,
            GL_COLOR_ATTACHMENT1
        );
    }

    if (Settings::instance().usePositionTexture()) {
        _spoutFbo->attachColorTexture(
            _textureIdentifiers.spoutPositions,
            GL_COLOR_ATTACHMENT2
        );
    }
}

void SpoutFlatProjection::blitCubeFace(int face) {
    // copy AA-buffer to "regular"/non-AA buffer
    _spoutFbo->bindBlit();
    attachTextures(face);
    _spoutFbo->blit();
}

void SpoutFlatProjection::render(const Window& window, const BaseViewport& viewport,
                                 Frustum::Mode frustumMode)
{
    ZoneScoped;

    if (!_mappingHandle) {
        return;
    }

    int vec[4];
    glGetIntegerv(GL_VIEWPORT, vec);
    GLint saveTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &saveTexture);
    GLint saveFrameBuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &saveFrameBuffer);


    if (!_spoutDrawMain) {
        glEnable(GL_SCISSOR_TEST);
        Engine::instance().setupViewport(window, viewport, frustumMode);
        glClearColor(_clearColor.x, _clearColor.y, _clearColor.z, _clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
    }

#ifdef SGCT_HAS_SPOUT
    glBindTexture(GL_TEXTURE_2D, _textureIdentifiers.spoutColor);
    const bool s = _mappingHandle->SendTexture(
        _textureIdentifiers.spoutColor,
        static_cast<GLuint>(GL_TEXTURE_2D),
        _resolutionX,
        _resolutionY
    );
    if (!s) {
        Log::Error(fmt::format(
            "Error sending texture '{}'", _textureIdentifiers.spoutColor
        ));
    }
#endif

    glBindFramebuffer(GL_FRAMEBUFFER, saveFrameBuffer);

    if (_spoutDrawMain) {
        Engine::instance().setupViewport(window, viewport, frustumMode);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _textureIdentifiers.spoutColor);
        _shader.bind();
        window.renderScreenQuad();
        ShaderProgram::unbind();
    }


    glBindFramebuffer(GL_FRAMEBUFFER, saveFrameBuffer);
    glBindTexture(GL_TEXTURE_2D, saveTexture);
    glViewport(vec[0], vec[1], vec[2], vec[3]);
    glScissor(vec[0], vec[1], vec[2], vec[3]);
}

void SpoutFlatProjection::renderCubemap(Window& window, Frustum::Mode frustumMode) {
    ZoneScoped;

    auto render = [this](const Window& win, BaseViewport& vp, int idx, Frustum::Mode mode)
    {
        if (!_mappingHandle || !vp.isEnabled()) {
            return;
        }

        _spoutFbo->bind();
        if (!_spoutFbo->isMultiSampled()) {
            attachTextures(idx);
        }

        RenderData renderData(
            win,
            vp,
            mode,
            ClusterManager::instance().sceneTransform(),
            vp.projection(mode).viewMatrix(),
            vp.projection(mode).projectionMatrix(),
            vp.projection(mode).viewProjectionMatrix() *
            ClusterManager::instance().sceneTransform(),
            ivec2(_resolutionX, _resolutionY)
        );
        glLineWidth(1.f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glDepthFunc(GL_LESS);

        glEnable(GL_SCISSOR_TEST);
        setupViewport(vp);

        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_SCISSOR_TEST);
        Engine::instance().drawFunction()(renderData);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // blit MSAA fbo to texture
        if (_spoutFbo->isMultiSampled()) {
            blitCubeFace(idx);
        }
        OffScreenBuffer::unbind();
    };

    int vec[4];
    glGetIntegerv(GL_VIEWPORT, vec);
    render(window, _subViewports.front, 0, frustumMode);
    glViewport(vec[0], vec[1], vec[2], vec[3]);
    glScissor(vec[0], vec[1], vec[2], vec[3]);
}

void SpoutFlatProjection::update(vec2) {}

void SpoutFlatProjection::initVBO() {}

void SpoutFlatProjection::initViewports() {
    _subViewports.front.setViewPlaneCoordsUsingFOVs(
        _spoutFov.up,
        _spoutFov.down,
        _spoutFov.left,
        _spoutFov.right,
        _spoutOrientation,
        _spoutFov.distance
    );
}

void SpoutFlatProjection::initShaders() {
    _shader.deleteProgram();
    _shader = ShaderProgram("SpoutShader");
    _shader.addShaderSource(shaders::BaseVert, GL_VERTEX_SHADER);
    _shader.addShaderSource(shaders::BaseFrag, GL_FRAGMENT_SHADER);
    _shader.createAndLinkProgram();
    _shader.bind();
    glUniform1i(glGetUniformLocation(_shader.id(), "tex"), 0);
    ShaderProgram::unbind();
}

} // namespace sgct
