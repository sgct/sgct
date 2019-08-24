/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/SGCTSettings.h>

#include <sgct/MessageHandler.h>
#include <sgct/ScreenCapture.h>
#ifndef SGCT_DONT_USE_EXTERNAL
#include <external/tinyxml2.h>
#else // SGCT_DONT_USE_EXTERNAL
#include <tinyxml2.h>
#endif // SGCT_DONT_USE_EXTERNAL

namespace sgct {

SGCTSettings* SGCTSettings::mInstance = nullptr;

SGCTSettings* SGCTSettings::instance() {
    if (mInstance == nullptr) {
        mInstance = new SGCTSettings();
    }

    return mInstance;
}

void SGCTSettings::destroy() {
    delete mInstance;
    mInstance = nullptr;
}

void SGCTSettings::configure(tinyxml2::XMLElement* element) {
    using namespace tinyxml2;

    const char* val;
    XMLElement* elem = element->FirstChildElement();

    while (elem) {
        val = elem->Value();

        if (strcmp("DepthBufferTexture", val) == 0) {
            if (elem->Attribute("value")) {
                setUseDepthTexture(strcmp(elem->Attribute("value"), "true") == 0);
            }
        }
        else if (strcmp("NormalTexture", val) == 0) {
            if (elem->Attribute("value")) {
                setUseNormalTexture(strcmp(elem->Attribute("value"), "true") == 0);
            }
        }
        else if (strcmp("PositionTexture", val) == 0) {
            if (elem->Attribute("value")) {
                setUsePositionTexture(strcmp(elem->Attribute("value"), "true") == 0);
            }
        }
        else if (strcmp("PBO", val) == 0) {
            if (elem->Attribute("value")) {
                setUsePBO(strcmp(elem->Attribute("value"), "true") == 0);
            }
        }
        else if (strcmp("Precision", val) == 0) {
            int fprec = 0;
            if (elem->QueryIntAttribute("float", &fprec) == XML_NO_ERROR) {
                if (fprec == 16) {
                    setBufferFloatPrecision(BufferFloatPrecision::Float_16Bit);
                }
                else if (fprec == 32) {
                    setBufferFloatPrecision(BufferFloatPrecision::Float_32Bit);
                }
                else {
                    MessageHandler::instance()->print(
                        MessageHandler::Level::Warning,
                        "ReadConfig: Invalid precition value (%d)! Must be 16 or 32\n",
                        fprec
                    );
                }
            }
            else {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Warning,
                    "ReadConfig: Invalid precition value! Must be 16 or 32\n"
                );
            }
        }
        else if (strcmp("Display", val) == 0) {
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
            if (exportMeshes != nullptr) {
                setExportWarpingMeshes(strcmp(exportMeshes, "true") == 0);
            }
        }
        else if (strcmp("OSDText", val) == 0) {
            float x = 0.f;
            float y = 0.f;

            if (elem->Attribute("name") != nullptr) {
                setOSDTextFontName(elem->Attribute("name"));
                MessageHandler::instance()->print(
                    MessageHandler::Level::Debug,
                    "ReadConfig: Setting font name to %s\n", elem->Attribute("name")
                );
            }

            if (elem->Attribute("path") != nullptr) {
                setOSDTextFontPath(elem->Attribute("path"));
                MessageHandler::instance()->print(
                    MessageHandler::Level::Debug,
                    "ReadConfig: Setting font path to %s\n", elem->Attribute("path")
                );
            }

            if (elem->Attribute("size") != nullptr) {
                unsigned int tmpi;
                XMLError err = elem->QueryUnsignedAttribute("size", &tmpi);
                if (err == tinyxml2::XML_NO_ERROR && tmpi > 0) {
                    setOSDTextFontSize(tmpi);
                    MessageHandler::instance()->print(
                        MessageHandler::Level::Debug,
                        "ReadConfig: Setting font size to %u\n", tmpi
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
        else if (strcmp("FXAA", val) == 0) {
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
                        "ReadConfig: Setting FXAA sub-pixel trim to %f\n", 1.0f / trim
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

void SGCTSettings::setSwapInterval(int val) {
    mSwapInterval = val;
}

int SGCTSettings::getSwapInterval() const {
    return mSwapInterval;
}

void SGCTSettings::setRefreshRateHint(int freq) {
    mRefreshRate = freq;
}

int SGCTSettings::getRefreshRateHint() const {
    return mRefreshRate;
}

void SGCTSettings::setUseDepthTexture(bool state) {
    mUseDepthTexture = state;
}

void SGCTSettings::setUseNormalTexture(bool state) {
    mUseNormalTexture = state;
}

void SGCTSettings::setUsePositionTexture(bool state) {
    mUsePositionTexture = state;
}

void SGCTSettings::setBufferFloatPrecision(BufferFloatPrecision bfp) {
    mCurrentBufferFloatPrecision = bfp;
}

void SGCTSettings::setUseFBO(bool state) {
    mUseFBO = state;
}

void SGCTSettings::setNumberOfCaptureThreads(int count) {
    mNumberOfCaptureThreads = count;
}

void SGCTSettings::setPNGCompressionLevel(int level) {
    mPNGCompressionLevel = level;
}

void SGCTSettings::setJPEGQuality(int quality) {
    mJPEGQuality = quality;
}

int SGCTSettings::getPNGCompressionLevel() { 
    return mPNGCompressionLevel;
}

int SGCTSettings::getJPEGQuality() {
    return mJPEGQuality;
}

bool SGCTSettings::useDepthTexture() const {
    return mUseDepthTexture;
}

bool SGCTSettings::useNormalTexture() const {
    return mUseNormalTexture;
}

bool SGCTSettings::usePositionTexture() const {
    return mUsePositionTexture;
}

bool SGCTSettings::useFBO() const {
    return mUseFBO;
}

int SGCTSettings::getNumberOfCaptureThreads() const {
    return mNumberOfCaptureThreads;
}

glm::vec2 SGCTSettings::getOSDTextOffset() const {
    return mOSDTextOffset;
}

float SGCTSettings::getFXAASubPixTrim() const {
    return mFXAASubPixTrim;
}

float SGCTSettings::getFXAASubPixOffset() const {
    return mFXAASubPixOffset;
}

SGCTSettings::DrawBufferType SGCTSettings::getCurrentDrawBufferType() const {
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

void SGCTSettings::setCapturePath(std::string path, CapturePath cpi) {
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

void SGCTSettings::setCaptureFormat(CaptureFormat format) {
    mCaptureFormat = format;
}

const std::string& SGCTSettings::getCapturePath(CapturePath cpi) const {
    switch (cpi) {
        case CapturePath::Mono:
            return mCapturePath.mono;
        case CapturePath::LeftStereo:
            return mCapturePath.left;
        case CapturePath::RightStereo:
            return mCapturePath.right;
    }
}

SGCTSettings::CaptureFormat SGCTSettings::getCaptureFormat() {
    return mCaptureFormat;
}

void SGCTSettings::setFXAASubPixTrim(float val) {
    mFXAASubPixTrim = val;
}

void SGCTSettings::setFXAASubPixOffset(float val) {
    mFXAASubPixOffset = val;
}

void SGCTSettings::setOSDTextOffset(glm::vec2 val) {
    mOSDTextOffset = std::move(val);
}

void SGCTSettings::setOSDTextFontSize(unsigned int size) {
    mFontSize = size;
}

void SGCTSettings::setOSDTextFontName(std::string name) {
    mFontName = std::move(name);
}

void SGCTSettings::setOSDTextFontPath(std::string path) {
    mFontPath = std::move(path);
}

void SGCTSettings::setDefaultNumberOfAASamples(int samples) {
    if ((samples != 0) && ((samples & (samples - 1)) == 0)) {
        // if power of two
        mDefaultNumberOfAASamples = samples;
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "SGCTSettings: Number of default MSAA samples must be a power of two\n",
            samples
        );
    }
}

void SGCTSettings::setDefaultFXAAState(bool state) {
    mDefaultFXAA = state;
}

void SGCTSettings::setForceGlTexImage2D(bool state) {
    mForceGlTexImage2D = state;
}

bool SGCTSettings::getForceGlTexImage2D() const {
    return mForceGlTexImage2D;
}

void SGCTSettings::setUsePBO(bool state) {
    mUsePBO = state;
}

bool SGCTSettings::getUsePBO() const {
    return mUsePBO;
}

void SGCTSettings::setUseRLE(bool state) {
    mUseRLE = state;
}

void SGCTSettings::setUseWarping(bool state) {
    mUseWarping = state;
}

void SGCTSettings::setShowWarpingWireframe(bool state) {
    mShowWarpingWireframe = state;
}

void SGCTSettings::setCaptureFromBackBuffer(bool state) {
    mCaptureBackBuffer = state;
}

void SGCTSettings::setExportWarpingMeshes(bool state) {
    mExportWarpingMeshes = state;
}

bool SGCTSettings::getUseRLE() {
    return mUseRLE;
}

bool SGCTSettings::getTryMaintainAspectRatio() const {
    return mTryMaintainAspectRatio;
}

bool SGCTSettings::getExportWarpingMeshes() const {
    return mExportWarpingMeshes;
}

bool SGCTSettings::getUseWarping() const {
    return mUseWarping;
}

bool SGCTSettings::getShowWarpingWireframe() const {
    return mShowWarpingWireframe;
}

bool SGCTSettings::getCaptureFromBackBuffer() const {
    return mCaptureBackBuffer;
}

void SGCTSettings::setTryMaintainAspectRatio(bool state) {
    mTryMaintainAspectRatio = state;
}

unsigned int SGCTSettings::getOSDTextFontSize() const {
    return mFontSize;
}

const std::string& SGCTSettings::getOSDTextFontName() const {
    return mFontName;
}

const std::string& SGCTSettings::getOSDTextFontPath() const {
    return mFontPath;
}

int SGCTSettings::getBufferFloatPrecisionAsGLint() const {
    return mCurrentBufferFloatPrecision == BufferFloatPrecision::Float_16Bit ?
        GL_RGB16F :
        GL_RGB32F;
}

int SGCTSettings::getDefaultNumberOfAASamples() const {
    return mDefaultNumberOfAASamples;
}

bool SGCTSettings::getDefaultFXAAState() const {
    return mDefaultFXAA;
}

} // namespace sgct
