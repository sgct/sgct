/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/settings.h>

#include <sgct/messagehandler.h>
#include <sgct/screencapture.h>
#include <tinyxml2.h>

namespace sgct {

Settings* Settings::mInstance = nullptr;

Settings* Settings::instance() {
    if (mInstance == nullptr) {
        mInstance = new Settings();
    }

    return mInstance;
}

void Settings::destroy() {
    delete mInstance;
    mInstance = nullptr;
}

void Settings::applySettings(const sgct::config::Settings& settings) {
    if (settings.useDepthTexture) {
        setUseDepthTexture(*settings.useDepthTexture);
    }
    if (settings.useNormalTexture) {
        setUseNormalTexture(*settings.useNormalTexture);
    }
    if (settings.usePositionTexture) {
        setUsePositionTexture(*settings.usePositionTexture);
    }
    if (settings.usePBO) {
        setUsePBO(*settings.usePBO);
    }
    if (settings.bufferFloatPrecision) {
        BufferFloatPrecision p = [](sgct::config::Settings::BufferFloatPrecision p) {
            switch (p) {
                case sgct::config::Settings::BufferFloatPrecision::Float16Bit:
                    return BufferFloatPrecision::Float16Bit;
                case sgct::config::Settings::BufferFloatPrecision::Float32Bit:
                    return BufferFloatPrecision::Float32Bit;
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
        if (settings.display->maintainAspectRatio) {
            setTryMaintainAspectRatio(*settings.display->maintainAspectRatio);
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
            mOSDTextOffset.x = *settings.osdText->xOffset;
        }
        if (settings.osdText->yOffset) {
            mOSDTextOffset.y = *settings.osdText->yOffset;
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

void Settings::setSwapInterval(int val) {
    mSwapInterval = val;
}

int Settings::getSwapInterval() const {
    return mSwapInterval;
}

void Settings::setRefreshRateHint(int freq) {
    mRefreshRate = freq;
}

int Settings::getRefreshRateHint() const {
    return mRefreshRate;
}

void Settings::setUseDepthTexture(bool state) {
    mUseDepthTexture = state;
}

void Settings::setUseNormalTexture(bool state) {
    mUseNormalTexture = state;
}

void Settings::setUsePositionTexture(bool state) {
    mUsePositionTexture = state;
}

void Settings::setBufferFloatPrecision(BufferFloatPrecision bfp) {
    mCurrentBufferFloatPrecision = bfp;
}

void Settings::setUseFBO(bool state) {
    mUseFBO = state;
}

void Settings::setNumberOfCaptureThreads(int count) {
    mNumberOfCaptureThreads = count;
}

void Settings::setPNGCompressionLevel(int level) {
    mPNGCompressionLevel = level;
}

void Settings::setJPEGQuality(int quality) {
    mJPEGQuality = quality;
}

int Settings::getPNGCompressionLevel() { 
    return mPNGCompressionLevel;
}

int Settings::getJPEGQuality() {
    return mJPEGQuality;
}

bool Settings::useDepthTexture() const {
    return mUseDepthTexture;
}

bool Settings::useNormalTexture() const {
    return mUseNormalTexture;
}

bool Settings::usePositionTexture() const {
    return mUsePositionTexture;
}

bool Settings::useFBO() const {
    return mUseFBO;
}

int Settings::getNumberOfCaptureThreads() const {
    return mNumberOfCaptureThreads;
}

glm::vec2 Settings::getOSDTextOffset() const {
    return mOSDTextOffset;
}

float Settings::getFXAASubPixTrim() const {
    return mFXAASubPixTrim;
}

float Settings::getFXAASubPixOffset() const {
    return mFXAASubPixOffset;
}

Settings::DrawBufferType Settings::getCurrentDrawBufferType() const {
    if (mUsePositionTexture) {
        if (mUseNormalTexture) {
            return DrawBufferType::DiffuseNormalPosition;
        }
        else {
            return DrawBufferType::DiffusePosition;
        }
    }
    else {
        if (mUseNormalTexture) {
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
            mCapturePath.mono = std::move(path);
            break;
        case CapturePath::LeftStereo:
            mCapturePath.left = std::move(path);
            break;
        case CapturePath::RightStereo:
            mCapturePath.right = std::move(path);
            break;
    }
}

void Settings::setCaptureFormat(CaptureFormat format) {
    mCaptureFormat = format;
}

const std::string& Settings::getCapturePath(CapturePath cpi) const {
    switch (cpi) {
        default:
        case CapturePath::Mono:
            return mCapturePath.mono;
        case CapturePath::LeftStereo:
            return mCapturePath.left;
        case CapturePath::RightStereo:
            return mCapturePath.right;
    }
}

Settings::CaptureFormat Settings::getCaptureFormat() {
    return mCaptureFormat;
}

void Settings::setFXAASubPixTrim(float val) {
    mFXAASubPixTrim = val;
}

void Settings::setFXAASubPixOffset(float val) {
    mFXAASubPixOffset = val;
}

void Settings::setOSDTextOffset(glm::vec2 val) {
    mOSDTextOffset = std::move(val);
}

void Settings::setOSDTextFontSize(unsigned int size) {
    mFontSize = size;
}

void Settings::setOSDTextFontName(std::string name) {
    mFontName = std::move(name);
}

void Settings::setOSDTextFontPath(std::string path) {
    mFontPath = std::move(path);
}

void Settings::setDefaultNumberOfAASamples(int samples) {
    if ((samples != 0) && ((samples & (samples - 1)) == 0)) {
        // if power of two
        mDefaultNumberOfAASamples = samples;
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Settings: Number of default MSAA samples must be a power of two\n",
            samples
        );
    }
}

void Settings::setDefaultFXAAState(bool state) {
    mDefaultFXAA = state;
}

void Settings::setForceGlTexImage2D(bool state) {
    mForceGlTexImage2D = state;
}

bool Settings::getForceGlTexImage2D() const {
    return mForceGlTexImage2D;
}

void Settings::setUsePBO(bool state) {
    mUsePBO = state;
}

bool Settings::getUsePBO() const {
    return mUsePBO;
}

void Settings::setUseRLE(bool state) {
    mUseRLE = state;
}

void Settings::setUseWarping(bool state) {
    mUseWarping = state;
}

void Settings::setShowWarpingWireframe(bool state) {
    mShowWarpingWireframe = state;
}

void Settings::setCaptureFromBackBuffer(bool state) {
    mCaptureBackBuffer = state;
}

void Settings::setExportWarpingMeshes(bool state) {
    mExportWarpingMeshes = state;
}

bool Settings::getUseRLE() {
    return mUseRLE;
}

bool Settings::getTryMaintainAspectRatio() const {
    return mTryMaintainAspectRatio;
}

bool Settings::getExportWarpingMeshes() const {
    return mExportWarpingMeshes;
}

bool Settings::getUseWarping() const {
    return mUseWarping;
}

bool Settings::getShowWarpingWireframe() const {
    return mShowWarpingWireframe;
}

bool Settings::getCaptureFromBackBuffer() const {
    return mCaptureBackBuffer;
}

void Settings::setTryMaintainAspectRatio(bool state) {
    mTryMaintainAspectRatio = state;
}

unsigned int Settings::getOSDTextFontSize() const {
    return mFontSize;
}

const std::string& Settings::getOSDTextFontName() const {
    return mFontName;
}

const std::string& Settings::getOSDTextFontPath() const {
    return mFontPath;
}

int Settings::getBufferFloatPrecisionAsGLint() const {
    return mCurrentBufferFloatPrecision == BufferFloatPrecision::Float16Bit ?
        GL_RGB16F :
        GL_RGB32F;
}

int Settings::getDefaultNumberOfAASamples() const {
    return mDefaultNumberOfAASamples;
}

bool Settings::getDefaultFXAAState() const {
    return mDefaultFXAA;
}

} // namespace sgct
