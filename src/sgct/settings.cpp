/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/settings.h>

#include <sgct/config.h>
#include <sgct/messagehandler.h>

namespace sgct {

Settings* Settings::_instance = nullptr;

Settings& Settings::instance() {
    if (!_instance) {
        _instance = new Settings;
    }
    return *_instance;
}

void Settings::destroy() {
    delete _instance;
    _instance = nullptr;
}

void Settings::applySettings(const config::Settings& settings) {
    if (settings.useDepthTexture) {
        setUseDepthTexture(*settings.useDepthTexture);
    }
    if (settings.useNormalTexture) {
        setUseNormalTexture(*settings.useNormalTexture);
    }
    if (settings.usePositionTexture) {
        setUsePositionTexture(*settings.usePositionTexture);
    }
    if (settings.bufferFloatPrecision) {
        BufferFloatPrecision p =
            [](config::Settings::BufferFloatPrecision p) {
            switch (p) {
                case config::Settings::BufferFloatPrecision::Float16Bit:
                    return BufferFloatPrecision::Float16Bit;
                case config::Settings::BufferFloatPrecision::Float32Bit:
                    return BufferFloatPrecision::Float32Bit;
                default:
                    throw std::logic_error("Unhandled case label");
            }
        }(*settings.bufferFloatPrecision);
        setBufferFloatPrecision(p);
    }
    if (settings.display) {
        if (settings.display->swapInterval) {
            setSwapInterval(*settings.display->swapInterval);
        }
        if (settings.display->refreshRate) {
            setRefreshRateHint(*settings.display->refreshRate);
        }
        if (settings.display->keepAspectRatio) {
            setTryKeepAspectRatio(*settings.display->keepAspectRatio);
        }
        if (settings.display->exportWarpingMeshes) {
            setExportWarpingMeshes(*settings.display->exportWarpingMeshes);
        }
    }
    if (settings.osdText) {
        if (settings.osdText->name) {
            setOSDTextFontName(*settings.osdText->name);
        }
        if (settings.osdText->path) {
            setOSDTextFontPath(*settings.osdText->path);
        }
        if (settings.osdText->size) {
            setOSDTextFontSize(*settings.osdText->size);
        }
        if (settings.osdText->xOffset) {
            const glm::vec2& curr = getOSDTextOffset();
            setOSDTextOffset(glm::vec2(*settings.osdText->xOffset, curr.y));
        }
        if (settings.osdText->yOffset) {
            const glm::vec2& curr = getOSDTextOffset();
            setOSDTextOffset(glm::vec2(curr.x, *settings.osdText->yOffset));
        }
    }
    if (settings.fxaa) {
        if (settings.fxaa->offset) {
            setFXAASubPixOffset(*settings.fxaa->offset);
        }
        if (settings.fxaa->trim) {
            setFXAASubPixTrim(1.f / *settings.fxaa->trim);
        }
    }
}

void Settings::applyCapture(const config::Capture& capture) {
    if (capture.monoPath) {
        setCapturePath(*capture.monoPath, CapturePath::Mono);
    }
    if (capture.leftPath) {
        setCapturePath(*capture.leftPath, CapturePath::LeftStereo);
    }
    if (capture.rightPath) {
        setCapturePath(*capture.rightPath, CapturePath::RightStereo);
    }
    if (capture.format) {
        CaptureFormat f = [](config::Capture::Format format) {
            switch (format) {
                case config::Capture::Format::PNG: return CaptureFormat::PNG;
                case config::Capture::Format::JPG: return CaptureFormat::JPG;
                case config::Capture::Format::TGA: return CaptureFormat::TGA;
                default:      throw std::logic_error("Unhandled case label");
            }
        }(*capture.format);
        setCaptureFormat(f);
    }
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
    _bufferFloatPrecision = bfp;
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

Settings::DrawBufferType Settings::getDrawBufferType() const {
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
        default:
            throw std::logic_error("Unhandled case label");
    }
}

void Settings::setCaptureFormat(CaptureFormat format) {
    _captureFormat = format;
}

const std::string& Settings::getCapturePath(CapturePath cpi) const {
    switch (cpi) {
        case CapturePath::Mono: return _capturePath.mono;
        case CapturePath::LeftStereo: return _capturePath.left;
        case CapturePath::RightStereo: return _capturePath.right;
        default: throw std::logic_error("Unhandled case label");
    }
}

Settings::CaptureFormat Settings::getCaptureFormat() const {
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
        MessageHandler::printWarning("Number of MSAA samples must be power of two");
    }
}

void Settings::setDefaultFXAAState(bool state) {
    _defaultFXAA = state;
}

void Settings::setUseWarping(bool state) {
    _useWarping = state;
}

void Settings::setCaptureFromBackBuffer(bool state) {
    _captureBackBuffer = state;
}

void Settings::setExportWarpingMeshes(bool state) {
    _exportWarpingMeshes = state;
}

bool Settings::getTryKeepAspectRatio() const {
    return _tryKeepAspectRatio;
}

bool Settings::getExportWarpingMeshes() const {
    return _exportWarpingMeshes;
}

bool Settings::getUseWarping() const {
    return _useWarping;
}

bool Settings::getCaptureFromBackBuffer() const {
    return _captureBackBuffer;
}

void Settings::setTryKeepAspectRatio(bool state) {
    _tryKeepAspectRatio = state;
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

GLenum Settings::getBufferFloatPrecision() const {
    return
        _bufferFloatPrecision == BufferFloatPrecision::Float16Bit ? GL_RGB16F :GL_RGB32F;
}

int Settings::getDefaultNumberOfAASamples() const {
    return _defaultNumberOfAASamples;
}

bool Settings::getDefaultFXAAState() const {
    return _defaultFXAA;
}

} // namespace sgct
