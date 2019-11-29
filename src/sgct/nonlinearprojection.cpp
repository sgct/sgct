/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/nonlinearprojection.h>

#include <sgct/logger.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/settings.h>
#include <algorithm>

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
    _isStereo = state;
}

void NonLinearProjection::setClearColor(glm::vec4 color) {
    _clearColor = std::move(color);
}

void NonLinearProjection::setAlpha(float alpha) {
    _clearColor.a = alpha;
}

void NonLinearProjection::setUser(User* user) {
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

glm::ivec4 NonLinearProjection::getViewportCoords() {
    return _vpCoords;
}

void NonLinearProjection::initTextures() {
    generateCubeMap(_textures.cubeMapColor, _texInternalFormat);
    Logger::Debug(
        "%dx%d color cube map texture (id: %d) generated",
        _cubemapResolution, _cubemapResolution, _textures.cubeMapColor
    );
    
    if (Settings::instance().useDepthTexture()) {
        generateCubeMap(_textures.cubeMapDepth, GL_DEPTH_COMPONENT32);
        Logger::Debug(
            "%dx%d depth cube map texture (id: %d) generated",
            _cubemapResolution, _cubemapResolution, _textures.cubeMapDepth
        );

        if (_useDepthTransformation) {
            // generate swap textures
            generateMap(_textures.depthSwap, GL_DEPTH_COMPONENT32);
            Logger::Debug(
                "%dx%d depth swap map texture (id: %d) generated",
                _cubemapResolution, _cubemapResolution, _textures.depthSwap
            );

            generateMap(_textures.colorSwap, _texInternalFormat);
            Logger::Debug(
                "%dx%d color swap map texture (id: %d) generated",
                _cubemapResolution, _cubemapResolution, _textures.colorSwap
            );
        }
    }

    if (Settings::instance().useNormalTexture()) {
        generateCubeMap(
            _textures.cubeMapNormals,
            Settings::instance().getBufferFloatPrecision()
        );
        Logger::Debug(
            "%dx%d normal cube map texture (id: %d) generated",
            _cubemapResolution, _cubemapResolution, _textures.cubeMapNormals
        );
    }

    if (Settings::instance().usePositionTexture()) {
        generateCubeMap(
            _textures.cubeMapPositions,
            Settings::instance().getBufferFloatPrecision()
        );
        Logger::Debug(
            "%dx%d position cube map texture (%d) generated",
            _cubemapResolution, _cubemapResolution, _textures.cubeMapPositions
        );
    }
}

void NonLinearProjection::initFBO() {
    _cubeMapFbo = std::make_unique<core::OffScreenBuffer>();
    _cubeMapFbo->setInternalColorFormat(_texInternalFormat);
    _cubeMapFbo->createFBO(_cubemapResolution, _cubemapResolution, _samples);
}

void NonLinearProjection::initVBO() {
    std::array<float, 20> vertices;
    glGenVertexArrays(1, &_vao);
    Logger::Debug("Generating VAO: %d", _vao);

    glGenBuffers(1, &_vbo);
    Logger::Debug("Generating VBO: %d", _vbo);
    
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    // 2TF + 3VF = 2*4 + 3*4 = 20
    glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), vertices.data(), GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

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

void NonLinearProjection::generateMap(unsigned int& texture, GLenum internalFormat) {
    glDeleteTextures(1, &texture);

    GLint maxMapRes;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxMapRes);
    if (_cubemapResolution > maxMapRes) {
        Logger::Error(
            "Requested map size is too big (%d > %d)", _cubemapResolution, maxMapRes
        );
    }

    // set up texture target
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Disable mipmaps
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    glTexStorage2D(
        GL_TEXTURE_2D,
        1,
        internalFormat,
        _cubemapResolution,
        _cubemapResolution
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void NonLinearProjection::generateCubeMap(unsigned int& texture, GLenum internalFormat) {
    glDeleteTextures(1, &texture);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    GLint maxCubeMapRes;
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &maxCubeMapRes);
    if (_cubemapResolution > maxCubeMapRes) {
        _cubemapResolution = maxCubeMapRes;
        Logger::Debug("Cubemap size set to max size: %d", maxCubeMapRes);
    }

    // set up texture target
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    // Disable mipmaps
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    glTexStorage2D(
        GL_TEXTURE_CUBE_MAP,
        1,
        internalFormat,
        _cubemapResolution,
        _cubemapResolution
    );

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

} // namespace sgct::core
