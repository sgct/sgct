/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/settings.h>

#include <sgct/config.h>
#include <sgct/engine.h>
#include <sgct/log.h>
#include <sgct/opengl.h>

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
                default: throw std::logic_error("Unhandled case label");
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
    }
}

void Settings::applyCapture(const config::Capture& capture) {
    if (capture.path) {
        setCapturePath(*capture.path);
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

    if (capture.range.has_value()) {
        _screenshot.limits = Capture::Limits();
        _screenshot.limits->begin = capture.range->first;
        _screenshot.limits->end = capture.range->last;
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
    if (count <= 0) {
        Log::Error("Only positive number of capture threads allowed");
    }
    else {
        _nCaptureThreads = count;
    }
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

void Settings::setCapturePath(std::filesystem::path path) {
    if (path != _screenshot.capturePath) {
        // If we set a new path, we want to start counting from 0 again
        Engine::instance().setScreenshotNumber(0);
    }
    _screenshot.capturePath = std::move(path);
}

void Settings::setCaptureFormat(CaptureFormat format) {
    _captureFormat = format;
}

void Settings::setScreenshotPrefix(std::string prefix) {
    _screenshot.prefix = std::move(prefix);
}

const std::filesystem::path& Settings::capturePath() const {
    return _screenshot.capturePath;
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

void Settings::setAddNodeNameToScreenshot(bool state) {
    _screenshot.addNodeName = state;
}

void Settings::setAddWindowNameToScreenshot(bool state) {
    _screenshot.addWindowName = state;
}

bool Settings::exportWarpingMeshes() const {
    return _exportWarpingMeshes;
}

bool Settings::captureFromBackBuffer() const {
    return _captureBackBuffer;
}

unsigned int Settings::bufferFloatPrecision() const {
    return
        _bufferFloatPrecision == BufferFloatPrecision::Float16Bit ? GL_RGB16F : GL_RGB32F;
}

bool Settings::addNodeNameToScreenshot() const {
    return _screenshot.addNodeName;
}

bool Settings::addWindowNameToScreenshot() const {
    return _screenshot.addWindowName;
}

const std::string& Settings::prefixScreenshot() const {
    return _screenshot.prefix;
}

bool Settings::hasScreenshotLimit() const {
    return _screenshot.limits.has_value();
}

uint64_t Settings::screenshotLimitBegin() const {
    return _screenshot.limits.has_value() ?
        _screenshot.limits->begin :
        std::numeric_limits<uint64_t>::min();
}

uint64_t Settings::screenshotLimitEnd() const {
    return _screenshot.limits.has_value() ?
        _screenshot.limits->end :
        std::numeric_limits<uint64_t>::max();
}

} // namespace sgct
