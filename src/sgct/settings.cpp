/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/settings.h>

#include <sgct/messagehandler.h>
#include <sgct/screencapture.h>

namespace sgct {

Settings* Settings::_instance = nullptr;

Settings* Settings::instance() {
    if (_instance == nullptr) {
        _instance = new Settings();
    }

    return _instance;
}

void Settings::destroy() {
    delete _instance;
    _instance = nullptr;
}

void Settings::setSwapInterval(int val) {
    _swapInterval = val;
}

int Settings::getSwapInterval() const {
    return _swapInterval;
}

void Settings::setRefreshRateHint(int freq) {
    _refreshRate = freq;
}

int Settings::getRefreshRateHint() const {
    return _refreshRate;
}

void Settings::setUseDepthTexture(bool state) {
    _useDepthTexture = state;
}

void Settings::setUseNormalTexture(bool state) {
    _useNormalTexture = state;
}

void Settings::setUsePositionTexture(bool state) {
    _usePositionTexture = state;
}

void Settings::setBufferFloatPrecision(BufferFloatPrecision bfp) {
    _currentBufferFloatPrecision = bfp;
}

void Settings::setUseFBO(bool state) {
    _useFBO = state;
}

void Settings::setNumberOfCaptureThreads(int count) {
    _nCaptureThreads = count;
}

bool Settings::useDepthTexture() const {
    return _useDepthTexture;
}

bool Settings::useNormalTexture() const {
    return _useNormalTexture;
}

bool Settings::usePositionTexture() const {
    return _usePositionTexture;
}

bool Settings::useFBO() const {
    return _useFBO;
}

int Settings::getNumberOfCaptureThreads() const {
    return _nCaptureThreads;
}

glm::vec2 Settings::getOSDTextOffset() const {
    return _osdTextOffset;
}

float Settings::getFXAASubPixTrim() const {
    return _fxaaSubPixTrim;
}

float Settings::getFXAASubPixOffset() const {
    return _fxaaSubPixOffset;
}

Settings::DrawBufferType Settings::getCurrentDrawBufferType() const {
    if (_usePositionTexture) {
        if (_useNormalTexture) {
            return DrawBufferType::DiffuseNormalPosition;
        }
        else {
            return DrawBufferType::DiffusePosition;
        }
    }
    else {
        if (_useNormalTexture) {
            return DrawBufferType::DiffuseNormal;
        }
        else {
            return DrawBufferType::Diffuse;
        }
    }
}

void Settings::setCapturePath(std::string path, CapturePath cpi) {
    switch (cpi) {
        case CapturePath::Mono:
            _capturePath.mono = std::move(path);
            break;
        case CapturePath::LeftStereo:
            _capturePath.left = std::move(path);
            break;
        case CapturePath::RightStereo:
            _capturePath.right = std::move(path);
            break;
    }
}

void Settings::setCaptureFormat(CaptureFormat format) {
    _captureFormat = format;
}

const std::string& Settings::getCapturePath(CapturePath cpi) const {
    switch (cpi) {
        default:
        case CapturePath::Mono:
            return _capturePath.mono;
        case CapturePath::LeftStereo:
            return _capturePath.left;
        case CapturePath::RightStereo:
            return _capturePath.right;
    }
}

Settings::CaptureFormat Settings::getCaptureFormat() {
    return _captureFormat;
}

void Settings::setFXAASubPixTrim(float val) {
    _fxaaSubPixTrim = val;
}

void Settings::setFXAASubPixOffset(float val) {
    _fxaaSubPixOffset = val;
}

void Settings::setOSDTextOffset(glm::vec2 val) {
    _osdTextOffset = std::move(val);
}

void Settings::setOSDTextFontSize(unsigned int size) {
    _fontSize = size;
}

void Settings::setOSDTextFontName(std::string name) {
    _fontName = std::move(name);
}

void Settings::setOSDTextFontPath(std::string path) {
    _fontPath = std::move(path);
}

void Settings::setDefaultNumberOfAASamples(int samples) {
    if ((samples != 0) && ((samples & (samples - 1)) == 0)) {
        // if power of two
        _defaultNumberOfAASamples = samples;
    }
    else {
        MessageHandler::instance()->printWarning(
            "Settings: Number of default MSAA samples must be a power of two", samples
        );
    }
}

void Settings::setDefaultFXAAState(bool state) {
    _defaultFXAA = state;
}

void Settings::setForceGlTexImage2D(bool state) {
    _forceGlTexImage2D = state;
}

bool Settings::getForceGlTexImage2D() const {
    return _forceGlTexImage2D;
}

void Settings::setUseWarping(bool state) {
    _useWarping = state;
}

void Settings::setShowWarpingWireframe(bool state) {
    _showWarpingWireframe = state;
}

void Settings::setCaptureFromBackBuffer(bool state) {
    _captureBackBuffer = state;
}

void Settings::setExportWarpingMeshes(bool state) {
    _exportWarpingMeshes = state;
}

bool Settings::getTryMaintainAspectRatio() const {
    return _tryMaintainAspectRatio;
}

bool Settings::getExportWarpingMeshes() const {
    return _exportWarpingMeshes;
}

bool Settings::getUseWarping() const {
    return _useWarping;
}

bool Settings::getShowWarpingWireframe() const {
    return _showWarpingWireframe;
}

bool Settings::getCaptureFromBackBuffer() const {
    return _captureBackBuffer;
}

void Settings::setTryMaintainAspectRatio(bool state) {
    _tryMaintainAspectRatio = state;
}

unsigned int Settings::getOSDTextFontSize() const {
    return _fontSize;
}

const std::string& Settings::getOSDTextFontName() const {
    return _fontName;
}

const std::string& Settings::getOSDTextFontPath() const {
    return _fontPath;
}

GLenum Settings::getBufferFloatPrecisionAsGLint() const {
    return _currentBufferFloatPrecision == BufferFloatPrecision::Float16Bit ?
        GL_RGB16F :
        GL_RGB32F;
}

int Settings::getDefaultNumberOfAASamples() const {
    return _defaultNumberOfAASamples;
}

bool Settings::getDefaultFXAAState() const {
    return _defaultFXAA;
}

} // namespace sgct
