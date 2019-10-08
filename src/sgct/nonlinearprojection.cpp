/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/nonlinearprojection.h>

#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/settings.h>
#include <algorithm>
#include <unordered_map>

namespace sgct::core {

NonLinearProjection::~NonLinearProjection() {
    glDeleteTextures(1, &_textures.cubeMapColor);
    glDeleteTextures(1, &_textures.cubeMapDepth);
    glDeleteTextures(1, &_textures.cubeMapNormals);
    glDeleteTextures(1, &_textures.cubeMapPositions);
    glDeleteTextures(1, &_textures.colorSwap);
    glDeleteTextures(1, &_textures.depthSwap);
    glDeleteTextures(1, &_textures.cubeFaceRight);
    glDeleteTextures(1, &_textures.cubeFaceLeft);
    glDeleteTextures(1, &_textures.cubeFaceBottom);
    glDeleteTextures(1, &_textures.cubeFaceTop);
    glDeleteTextures(1, &_textures.cubeFaceFront);
    glDeleteTextures(1, &_textures.cubeFaceBack);

    if (_cubeMapFbo) {
        _cubeMapFbo->destroy();
    }

    glDeleteBuffers(1, &_vbo);
    glDeleteVertexArrays(1, &_vao);
    _shader.deleteProgram();
    _depthCorrectionShader.deleteProgram();
}

void NonLinearProjection::init(GLenum internalTextureFormat, GLenum textureFormat,
                               GLenum textureType, int samples)
{
    _texInternalFormat = internalTextureFormat;
    _texFormat = textureFormat;
    _texType = textureType;
    _samples = samples;

    initViewports();
    initTextures();
    initFBO();
    initVBO();
    initShaders();
}

void NonLinearProjection::updateFrustums(const Frustum::Mode& mode, float nearClip,
                                         float farClip)
{
    if (_subViewports.right.isEnabled()) {
        _subViewports.right.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
    if (_subViewports.left.isEnabled()) {
        _subViewports.left.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
    if (_subViewports.bottom.isEnabled()) {
        _subViewports.bottom.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
    if (_subViewports.top.isEnabled()) {
        _subViewports.top.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
    if (_subViewports.front.isEnabled()) {
        _subViewports.front.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
    if (_subViewports.back.isEnabled()) {
        _subViewports.back.calculateNonLinearFrustum(mode, nearClip, farClip);
    }
}

void NonLinearProjection::setCubemapResolution(int resolution) {
    _cubemapResolution = resolution;
}

void NonLinearProjection::setInterpolationMode(InterpolationMode im) {
    _interpolationMode = im;
}

void NonLinearProjection::setUseDepthTransformation(bool state) {
    _useDepthTransformation = state;
}

void NonLinearProjection::setStereo(bool state) {
    _stereo = state;
}

void NonLinearProjection::setClearColor(glm::vec4 color) {
    _clearColor = std::move(color);
}

void NonLinearProjection::setAlpha(float alpha) {
    _clearColor.a = alpha;
}

void NonLinearProjection::setPreferedMonoFrustumMode(Frustum::Mode fm) {
    _preferedMonoFrustumMode = fm;
}

void NonLinearProjection::setUser(User& user) {
    _subViewports.right.setUser(user);
    _subViewports.left.setUser(user);
    _subViewports.bottom.setUser(user);
    _subViewports.top.setUser(user);
    _subViewports.front.setUser(user);
    _subViewports.back.setUser(user);
}

int NonLinearProjection::getCubemapResolution() const {
    return _cubemapResolution;
}

NonLinearProjection::InterpolationMode NonLinearProjection::getInterpolationMode() const {
    return _interpolationMode;
}

OffScreenBuffer* NonLinearProjection::getOffScreenBuffer() {
    return _cubeMapFbo.get();
}

glm::ivec4 NonLinearProjection::getViewportCoords() {
    return _vpCoords;
}

void NonLinearProjection::initTextures() {    
    generateCubeMap(_textures.cubeMapColor, _texInternalFormat, _texFormat, _texType);
    if (Engine::checkForOGLErrors()) {
        MessageHandler::instance()->printDebug(
            "NonLinearProjection: %dx%d color cube map texture (id: %d) generated",
            _cubemapResolution, _cubemapResolution, _textures.cubeMapColor
        );
    }
    else {
        MessageHandler::instance()->printError(
            "NonLinearProjection: Error occured while generating %dx%d color "
            "cube texture (id: %d)",
            _cubemapResolution, _cubemapResolution, _textures.cubeMapColor
        );
    }
    
    if (Settings::instance()->useDepthTexture()) {
        generateCubeMap(
            _textures.cubeMapDepth,
            GL_DEPTH_COMPONENT32,
            GL_DEPTH_COMPONENT,
            GL_FLOAT
        );
        if (Engine::checkForOGLErrors()) {
            MessageHandler::instance()->printDebug(
                "NonLinearProjection: %dx%d depth cube map texture (id: %d) generated",
                _cubemapResolution, _cubemapResolution, _textures.cubeMapDepth
            );
        }
        else {
            MessageHandler::instance()->printError(
                "NonLinearProjection: Error occured while generating %dx%d depth "
                "cube texture (id: %d)",
                _cubemapResolution, _cubemapResolution, _textures.cubeMapDepth
            );
        }

        if (_useDepthTransformation) {
            // generate swap textures
            generateMap(
                _textures.depthSwap,
                GL_DEPTH_COMPONENT32,
                GL_DEPTH_COMPONENT,
                GL_FLOAT
            );
            if (Engine::checkForOGLErrors()) {
                MessageHandler::instance()->printDebug(
                    "NonLinearProjection: %dx%d depth swap map texture (id: %d) generated",
                    _cubemapResolution, _cubemapResolution, _textures.depthSwap
                );
            }
            else {
                MessageHandler::instance()->printError(
                    "NonLinearProjection: Error occured while generating %dx%d depth "
                    "swap texture (id: %d)",
                    _cubemapResolution, _cubemapResolution, _textures.depthSwap
                );
            }

            generateMap(_textures.colorSwap, _texInternalFormat, _texFormat, _texType);
            if (Engine::checkForOGLErrors()) {
                MessageHandler::instance()->printDebug(
                    "NonLinearProjection: %dx%d color swap map texture (id: %d) generated",
                    _cubemapResolution, _cubemapResolution, _textures.colorSwap
                );
            }
            else {
                MessageHandler::instance()->printError(
                    "NonLinearProjection: Error occured while generating %dx%d color "
                    "swap texture (id: %d)",
                    _cubemapResolution, _cubemapResolution, _textures.colorSwap
                );
            }
        }
    }

    if (Settings::instance()->useNormalTexture()) {
        generateCubeMap(
            _textures.cubeMapNormals,
            Settings::instance()->getBufferFloatPrecisionAsGLint(),
            GL_BGR,
            GL_FLOAT
        );
        if (Engine::checkForOGLErrors()) {
            MessageHandler::instance()->printDebug(
                "NonLinearProjection: %dx%d normal cube map texture (id: %d) generated",
                _cubemapResolution, _cubemapResolution, _textures.cubeMapNormals
            );
        }
        else {
            MessageHandler::instance()->printError(
                "NonLinearProjection: Error occured while generating %dx%d normal "
                "cube texture (id: %d)",
                _cubemapResolution, _cubemapResolution, _textures.cubeMapNormals
            );
        }
    }

    if (Settings::instance()->usePositionTexture()) {
        generateCubeMap(
            _textures.cubeMapPositions,
            Settings::instance()->getBufferFloatPrecisionAsGLint(),
            GL_BGR,
            GL_FLOAT
        );
        if (Engine::checkForOGLErrors()) {
            MessageHandler::instance()->printDebug(
                "NonLinearProjection: %dx%d position cube map texture (%d) generated",
                _cubemapResolution, _cubemapResolution, _textures.cubeMapPositions
            );
        }
        else {
            MessageHandler::instance()->printError(
                "NonLinearProjection: Error occured while generating %dx%d position "
                "cube texture (id: %d)",
                _cubemapResolution, _cubemapResolution, _textures.cubeMapPositions
            );
        }
    }
}

void NonLinearProjection::initFBO() {
    _cubeMapFbo = std::make_unique<core::OffScreenBuffer>();
    _cubeMapFbo->setInternalColorFormat(_texInternalFormat);
    _cubeMapFbo->createFBO(_cubemapResolution, _cubemapResolution, _samples);

    if (_cubeMapFbo->checkForErrors()) {
        MessageHandler::instance()->printDebug(
            "NonLinearProjection: Cube map FBO created"
        );
    }
    else {
        MessageHandler::instance()->printError(
            "NonLinearProjection: Cube map FBO created with errors"
        );
    }

    OffScreenBuffer::unBind();
}

void NonLinearProjection::initVBO() {
    _vertices.resize(20);
    std::fill(_vertices.begin(), _vertices.end(), 0.f);
    
    glGenVertexArrays(1, &_vao);
    MessageHandler::instance()->printDebug(
        "NonLinearProjection: Generating VAO: %d", _vao
    );

    glGenBuffers(1, &_vbo);
    MessageHandler::instance()->printDebug(
        "NonLinearProjection: Generating VBO: %d", _vbo
    );
    
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    // 2TF + 3VF = 2*4 + 3*4 = 20
    glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), _vertices.data(), GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        reinterpret_cast<void*>(0)
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        reinterpret_cast<void*>(8)
    );

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void NonLinearProjection::generateCubeMap(unsigned int& texture, GLenum internalFormat,
                                          GLenum format, GLenum type)
{
    glDeleteTextures(1, &texture);
    texture = 0;

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    GLint MaxCubeMapRes;
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &MaxCubeMapRes);
    if (_cubemapResolution > MaxCubeMapRes) {
        _cubemapResolution = MaxCubeMapRes;
        MessageHandler::instance()->printDebug(
            "NonLinearProjection: Cubemap size set to max size: %d", MaxCubeMapRes
        );
    }

    // set up texture target
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    // Disable mipmaps
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    if (Settings::instance()->getForceGlTexImage2D()) {
        for (int side = 0; side < 6; ++side) {
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + side,
                0,
                internalFormat,
                _cubemapResolution,
                _cubemapResolution,
                0,
                format,
                type,
                nullptr
            );
        }
    }
    else {
        glTexStorage2D(
            GL_TEXTURE_CUBE_MAP,
            1,
            internalFormat,
            _cubemapResolution,
            _cubemapResolution
        );
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void NonLinearProjection::setupViewport(BaseViewport& vp) {
    const float cmRes = static_cast<float>(_cubemapResolution);

    _vpCoords = glm::ivec4(
        static_cast<int>(floor(vp.getPosition().x * cmRes + 0.5f)),
        static_cast<int>(floor(vp.getPosition().y * cmRes + 0.5f)),
        static_cast<int>(floor(vp.getSize().x * cmRes + 0.5f)),
        static_cast<int>(floor(vp.getSize().y * cmRes + 0.5f))
    );

    glViewport(_vpCoords.x, _vpCoords.y, _vpCoords.z, _vpCoords.w);
    glScissor(_vpCoords.x, _vpCoords.y, _vpCoords.z, _vpCoords.w);
}

void NonLinearProjection::generateMap(unsigned int& texture, GLenum internalFormat,
                                      GLenum format, GLenum type)
{
    glDeleteTextures(1, &texture);
    texture = 0;

    GLint MaxMapRes;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxMapRes);
    if (_cubemapResolution > MaxMapRes) {
        MessageHandler::instance()->printError(
            "NonLinearProjection: Requested map size is too big (%d > %d)",
            _cubemapResolution, MaxMapRes
        );
    }

    // set up texture target
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Disable mipmaps
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    if (Settings::instance()->getForceGlTexImage2D()) {
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            internalFormat,
            _cubemapResolution,
            _cubemapResolution,
            0,
            format,
            type,
            nullptr
        );
    }
    else {
        glTexStorage2D(
            GL_TEXTURE_2D,
            1,
            internalFormat,
            _cubemapResolution,
            _cubemapResolution
        );
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

} // namespace sgct::core
