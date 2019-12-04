/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/settings.h>

#include <sgct/config.h>
#include <sgct/logger.h>

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
            [](config::Settings::BufferFloatPrecision pr) {
            switch (pr) {
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

int Settings::swapInterval() const {
    return _swapInterval;
}

void Settings::setRefreshRateHint(int freq) {
    _refreshRate = freq;
}

int Settings::refreshRateHint() const {
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

int Settings::numberCaptureThreads() const {
    return _nCaptureThreads;
}

Settings::DrawBufferType Settings::drawBufferType() const {
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

const std::string& Settings::capturePath(CapturePath cpi) const {
    switch (cpi) {
        case CapturePath::Mono: return _capturePath.mono;
        case CapturePath::LeftStereo: return _capturePath.left;
        case CapturePath::RightStereo: return _capturePath.right;
        default: throw std::logic_error("Unhandled case label");
    }
}

Settings::CaptureFormat Settings::captureFormat() const {
    return _captureFormat;
}

void Settings::setCaptureFromBackBuffer(bool state) {
    _captureBackBuffer = state;
}

void Settings::setExportWarpingMeshes(bool state) {
    _exportWarpingMeshes = state;
}

bool Settings::tryKeepAspectRatio() const {
    return _tryKeepAspectRatio;
}

bool Settings::exportWarpingMeshes() const {
    return _exportWarpingMeshes;
}

bool Settings::captureFromBackBuffer() const {
    return _captureBackBuffer;
}

void Settings::setTryKeepAspectRatio(bool state) {
    _tryKeepAspectRatio = state;
}

GLenum Settings::bufferFloatPrecision() const {
    return
        _bufferFloatPrecision == BufferFloatPrecision::Float16Bit ? GL_RGB16F :GL_RGB32F;
}

} // namespace sgct
