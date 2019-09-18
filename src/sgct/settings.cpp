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

void Settings::configure(tinyxml2::XMLElement* element) {
    using namespace tinyxml2;

    XMLElement* elem = element->FirstChildElement();
    while (elem) {
        std::string_view val = elem->Value();

        if (val == "DepthBufferTexture") {
            if (elem->Attribute("value")) {
                setUseDepthTexture(strcmp(elem->Attribute("value"), "true") == 0);
            }
        }
        else if (val == "NormalTexture") {
            if (elem->Attribute("value")) {
                setUseNormalTexture(strcmp(elem->Attribute("value"), "true") == 0);
            }
        }
        else if (val == "PositionTexture") {
            if (elem->Attribute("value")) {
                setUsePositionTexture(strcmp(elem->Attribute("value"), "true") == 0);
            }
        }
        else if (val == "PBO") {
            if (elem->Attribute("value")) {
                setUsePBO(strcmp(elem->Attribute("value"), "true") == 0);
            }
        }
        else if (val == "Precision") {
            int fprec = 0;
            if (elem->QueryIntAttribute("float", &fprec) == XML_NO_ERROR) {
                if (fprec == 16) {
                    setBufferFloatPrecision(BufferFloatPrecision::Float16Bit);
                }
                else if (fprec == 32) {
                    setBufferFloatPrecision(BufferFloatPrecision::Float32Bit);
                }
                else {
                    MessageHandler::instance()->print(
                        MessageHandler::Level::Warning,
                        "ReadConfig: Invalid precision value (%d)! Must be 16 or 32\n",
                        fprec
                    );
                }
            }
            else {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Warning,
                    "ReadConfig: Invalid precision value! Must be 16 or 32\n"
                );
            }
        }
        else if (val == "Display") {
            int interval = 0;
            if (elem->QueryIntAttribute("swapInterval", &interval) == XML_NO_ERROR) {
                setSwapInterval(interval);
                MessageHandler::instance()->print(
                    MessageHandler::Level::Info,
                    "ReadConfig: Display swap interval is set to %d\n", interval
                );
            }

            int rate = 0;
            if (elem->QueryIntAttribute("refreshRate", &rate) == XML_NO_ERROR) {
                setRefreshRateHint(rate);
                MessageHandler::instance()->print(
                    MessageHandler::Level::Info,
                    "ReadConfig: Display refresh rate hint is set to %d Hz\n", rate
                );
            }

            const char* maintainAspect = elem->Attribute("tryMaintainAspectRatio");
            if (maintainAspect != nullptr) {
                setTryMaintainAspectRatio(strcmp(maintainAspect, "true") == 0);
            }

            const char* exportMeshes = elem->Attribute("exportWarpingMeshes");
            if (exportMeshes) {
                setExportWarpingMeshes(strcmp(exportMeshes, "true") == 0);
            }
        }
        else if (val == "OSDText") {
            float x = 0.f;
            float y = 0.f;

            if (elem->Attribute("name")) {
                setOSDTextFontName(elem->Attribute("name"));
                MessageHandler::instance()->print(
                    MessageHandler::Level::Debug,
                    "ReadConfig: Setting font name to %s\n", elem->Attribute("name")
                );
            }

            if (elem->Attribute("path")) {
                setOSDTextFontPath(elem->Attribute("path"));
                MessageHandler::instance()->print(
                    MessageHandler::Level::Debug,
                    "ReadConfig: Setting font path to %s\n", elem->Attribute("path")
                );
            }

            if (elem->Attribute("size")) {
                unsigned int size;
                XMLError err = elem->QueryUnsignedAttribute("size", &size);
                if (err == tinyxml2::XML_NO_ERROR) {
                    setOSDTextFontSize(size);
                    MessageHandler::instance()->print(
                        MessageHandler::Level::Debug,
                        "ReadConfig: Setting font size to %u\n", size
                    );
                }
                else {
                    MessageHandler::instance()->print(
                        MessageHandler::Level::Warning,
                        "ReadConfig: Font size not specified. Setting to size=10\n"
                    );
                }
            }

            if (elem->QueryFloatAttribute("xOffset", &x) == XML_NO_ERROR) {
                mOSDTextOffset.x = x;
                MessageHandler::instance()->print(
                    MessageHandler::Level::Debug,
                    "ReadConfig: Setting font x offset to %f\n", x
                );
            }

            if (elem->QueryFloatAttribute("yOffset", &y) == XML_NO_ERROR) {
                mOSDTextOffset.y = y;
                MessageHandler::instance()->print(
                    MessageHandler::Level::Debug,
                    "ReadConfig: Setting font y offset to %f\n", y
                );
            }
        }
        else if (val == "FXAA") {
            float offset = 0.f;
            if (elem->QueryFloatAttribute("offset", &offset) == XML_NO_ERROR) {
                setFXAASubPixOffset(offset);
                MessageHandler::instance()->print(
                    MessageHandler::Level::Debug,
                    "ReadConfig: Setting FXAA sub-pixel offset to %f\n", offset
                );
            }

            float trim = 0.f;
            if (elem->QueryFloatAttribute("trim", &trim) == XML_NO_ERROR) {
                if (trim > 0.f) {
                    setFXAASubPixTrim(1.f / trim);
                    MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Debug,
                        "ReadConfig: Setting FXAA sub-pixel trim to %f\n", 1.f / trim
                    );
                }
                else {
                    setFXAASubPixTrim(0.f);
                    MessageHandler::instance()->print(
                        MessageHandler::Level::Debug,
                        "ReadConfig: Setting FXAA sub-pixel trim to %f\n", 0.f
                    );
                }
            }
        }

        elem = elem->NextSiblingElement();
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
