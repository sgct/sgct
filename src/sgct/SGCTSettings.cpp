/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/SGCTSettings.h>

#include <sgct/MessageHandler.h>
#include <sgct/ScreenCapture.h>

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

SGCTSettings::SGCTSettings()
    : mCaptureFormat(sgct_core::ScreenCapture::CaptureFormat::NotSet)
{}


void SGCTSettings::configure(tinyxml2::XMLElement* element) {
    using namespace tinyxml2;

    const char* val;
    XMLElement* subElement = element->FirstChildElement();

    while (subElement != nullptr) {
        val = subElement->Value();

        if (strcmp("DepthBufferTexture", val) == 0) {
            if (subElement->Attribute("value") != nullptr) {
                instance()->setUseDepthTexture(
                    strcmp(subElement->Attribute("value"), "true") == 0 ? true : false
                );
            }
        }
        else if (strcmp("NormalTexture", val) == 0) {
            if (subElement->Attribute("value") != nullptr) {
                instance()->setUseNormalTexture(
                    strcmp(subElement->Attribute("value"), "true") == 0 ? true : false
                );
            }
        }
        else if (strcmp("PositionTexture", val) == 0) {
            if (subElement->Attribute("value") != nullptr) {
                instance()->setUsePositionTexture(
                    strcmp(subElement->Attribute("value"), "true") == 0 ? true : false
                );
            }
        }
        else if (strcmp("PBO", val) == 0) {
            if (subElement->Attribute("value") != nullptr) {
                instance()->setUsePBO(
                    strcmp(subElement->Attribute("value"), "true") == 0 ? true : false
                );
            }
        }
        else if (strcmp("Precision", val) == 0) {
            int fprec = 0;
            if (subElement->QueryIntAttribute("float", &fprec) == XML_NO_ERROR) {
                if (fprec == 16) {
                    instance()->setBufferFloatPrecision(BufferFloatPrecision::Float_16Bit);
                }
                else if (fprec == 32) {
                    instance()->setBufferFloatPrecision(BufferFloatPrecision::Float_32Bit);
                }
                else {
                    MessageHandler::instance()->print(
                        MessageHandler::Level::Warning,
                        "ReadConfig: Invalid float precition value (%d)! Must be 16 or 32.\n",
                        fprec
                    );
                }
            }
            else {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Warning,
                    "ReadConfig: Invalid float precition value! Must be 16 or 32.\n"
                );
            }
        }
        else if (strcmp("Display", val) == 0) {
            int interval = 0;
            if (subElement->QueryIntAttribute("swapInterval", &interval) == XML_NO_ERROR) {
                instance()->setSwapInterval(interval);
                MessageHandler::instance()->print(
                    MessageHandler::Level::Info,
                    "ReadConfig: Display swap interval is set to %d.\n",
                    interval
                );
            }

            int rate = 0;
            if (subElement->QueryIntAttribute("refreshRate", &rate) == XML_NO_ERROR) {
                instance()->setRefreshRateHint(rate);
                MessageHandler::instance()->print(
                    MessageHandler::Level::Info,
                    "ReadConfig: Display refresh rate hint is set to %d Hz.\n",
                    rate
                );
            }

            const char* maintainAspect = subElement->Attribute("tryMaintainAspectRatio");
            if (maintainAspect != nullptr) {
                instance()->setTryMaintainAspectRatio(
                    strcmp(maintainAspect, "true") == 0 ? true : false
                );
            }

            const char* exportMeshes = subElement->Attribute("exportWarpingMeshes");
            if (exportMeshes != nullptr) {
                instance()->setExportWarpingMeshes(
                    strcmp(exportMeshes, "true") == 0 ? true : false
                );
            }
        }
        else if (strcmp("OSDText", val) == 0) {
            float x = 0.f;
            float y = 0.f;

            if (subElement->Attribute("name") != nullptr) {
                instance()->setOSDTextFontName(subElement->Attribute("name"));
                MessageHandler::instance()->print(
                    MessageHandler::Level::Debug,
                    "ReadConfig: Setting font name to %s\n",
                    subElement->Attribute("name")
                );
            }

            if (subElement->Attribute("path") != nullptr) {
                instance()->setOSDTextFontPath(subElement->Attribute("path"));
                MessageHandler::instance()->print(
                    MessageHandler::Level::Debug,
                    "ReadConfig: Setting font path to %s\n",
                    subElement->Attribute("path")
                );
            }

            if (subElement->Attribute("size") != nullptr) {
                unsigned int tmpi;
                XMLError err = subElement->QueryUnsignedAttribute("size", &tmpi);
                if (err == tinyxml2::XML_NO_ERROR && tmpi > 0) {
                    instance()->setOSDTextFontSize(tmpi);
                    MessageHandler::instance()->print(
                        MessageHandler::Level::Debug,
                        "ReadConfig: Setting font size to %u\n",
                        tmpi
                    );
                }
                else {
                    MessageHandler::instance()->print(
                        MessageHandler::Level::Warning,
                        "ReadConfig: Font size not specified. Setting to default size=10!\n"
                    );
                }
            }

            if (subElement->QueryFloatAttribute("xOffset", &x) == XML_NO_ERROR) {
                instance()->setOSDTextXOffset(x);
                MessageHandler::instance()->print(
                    MessageHandler::Level::Debug,
                    "ReadConfig: Setting font x offset to %f\n", x
                );
            }

            if (subElement->QueryFloatAttribute("yOffset", &y) == XML_NO_ERROR) {
                instance()->setOSDTextYOffset(y);
                MessageHandler::instance()->print(
                    MessageHandler::Level::Debug,
                    "ReadConfig: Setting font y offset to %f\n", y
                );
            }
        }
        else if (strcmp("FXAA", val) == 0) {
            float offset = 0.f;
            if (subElement->QueryFloatAttribute("offset", &offset) == XML_NO_ERROR) {
                instance()->setFXAASubPixOffset(offset);
                MessageHandler::instance()->print(
                    MessageHandler::Level::Debug,
                    "ReadConfig: Setting FXAA sub-pixel offset to %f\n", offset
                );
            }

            float trim = 0.f;
            if (subElement->QueryFloatAttribute("trim", &trim) == XML_NO_ERROR) {
                if (trim > 0.f) {
                    instance()->setFXAASubPixTrim(1.f / trim);
                    MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Debug,
                        "ReadConfig: Setting FXAA sub-pixel trim to %f\n", 1.0f / trim
                    );
                }
                else {
                    instance()->setFXAASubPixTrim(0.f);
                    MessageHandler::instance()->print(
                        MessageHandler::Level::Debug,
                        "ReadConfig: Setting FXAA sub-pixel trim to %f\n", 0.f
                    );
                }
            }
        }

        subElement = subElement->NextSiblingElement();
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
    updateDrawBufferFlag();
}

void SGCTSettings::setUsePositionTexture(bool state) {
    mUsePositionTexture = state;
    updateDrawBufferFlag();
}

void SGCTSettings::setBufferFloatPrecision(BufferFloatPrecision bfp) {
    mCurrentBufferFloatPrecision = bfp;
}

void SGCTSettings::updateDrawBufferFlag() {
    if (mUseNormalTexture && mUsePositionTexture) {
        mCurrentDrawBuffer = Diffuse_Normal_Position;
    }
    else if (!mUseNormalTexture && !mUsePositionTexture) {
        mCurrentDrawBuffer = Diffuse;
    }
    else if (mUseNormalTexture && !mUsePositionTexture) {
        mCurrentDrawBuffer = Diffuse_Normal;
    }
    else {
        mCurrentDrawBuffer = Diffuse_Position;
    }
}

void SGCTSettings::setUseFBO(bool state) {
    mUseFBO = state;
}

void SGCTSettings::setNumberOfCaptureThreads(int count) {
    mNumberOfCaptureThreads = count;
}

void SGCTSettings::setPNGCompressionLevel(int level) {
    mMutex.lock();
    mPNGCompressionLevel = level;
    mMutex.unlock();
}

void SGCTSettings::setJPEGQuality(int quality) {
    mMutex.lock();
    mJPEGQuality = quality;
    mMutex.unlock();
}

int SGCTSettings::getPNGCompressionLevel() { 
    mMutex.lock();
    int tmpI = mPNGCompressionLevel;
    mMutex.unlock();
    return tmpI;
}

int SGCTSettings::getJPEGQuality() {
    mMutex.lock();
    int tmpI = mJPEGQuality;
    mMutex.unlock();
    return tmpI;
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

float SGCTSettings::getOSDTextXOffset() const {
    return mOSDTextOffset[0];
}

float SGCTSettings::getOSDTextYOffset() const {
    return mOSDTextOffset[1];
}

float SGCTSettings::getFXAASubPixTrim() const {
    return mFXAASubPixTrim;
}

float SGCTSettings::getFXAASubPixOffset() const {
    return mFXAASubPixOffset;
}

SGCTSettings::DrawBufferType SGCTSettings::getCurrentDrawBufferType() const {
    return mCurrentDrawBuffer;
}

void SGCTSettings::setCapturePath(std::string path, CapturePathIndex cpi) {
    if (path.empty()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "SGCTSettings: Empty screen capture path!\n"
        );
        return;
    }

    mCapturePath[cpi] = std::move(path);
}

void SGCTSettings::appendCapturePath(std::string str, CapturePathIndex cpi) {
    mCapturePath[cpi].append(std::move(str));
}

void SGCTSettings::setCaptureFormat(const char* format) {
    mMutex.lock();
    
    if (strcmp("png", format) == 0 || strcmp("PNG", format) == 0) {
        mCaptureFormat = sgct_core::ScreenCapture::CaptureFormat::PNG;
    }
    else if (strcmp("tga", format) == 0 || strcmp("TGA", format) == 0) {
        mCaptureFormat = sgct_core::ScreenCapture::CaptureFormat::TGA;
    }
    else if (strcmp("jpg", format) == 0 || strcmp("JPG", format) == 0) {
        mCaptureFormat = sgct_core::ScreenCapture::CaptureFormat::JPEG;
    }

    mMutex.unlock();
}

const char* SGCTSettings::getCapturePath(CapturePathIndex cpi) const {
    return mCapturePath[cpi].c_str();
}

sgct_core::ScreenCapture::CaptureFormat SGCTSettings::getCaptureFormat() {
    mMutex.lock();
    sgct_core::ScreenCapture::CaptureFormat tmpI = mCaptureFormat;
    mMutex.unlock();
    return tmpI;
}

void SGCTSettings::setFXAASubPixTrim(float val) {
    mFXAASubPixTrim = val;
}

void SGCTSettings::setFXAASubPixOffset(float val) {
    mFXAASubPixOffset = val;
}


void SGCTSettings::setOSDTextOffset(glm::vec2 val) {
    mOSDTextOffset[0] = val.x;
    mOSDTextOffset[1] = val.y;
}

/*!
    
*/
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
        //if power of two
        mDefaultNumberOfAASamples = samples;
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "SGCTSettings: Number of default MSAA samples must be a power of two (not %d)!\n",
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
    mMutex.lock();
    mUseRLE = state;
    mMutex.unlock();
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
    mMutex.lock();
    bool tmpB = mUseRLE;
    mMutex.unlock();
    return tmpB;
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
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "SGCTSettings: Set try maintain aspect ratio to: %s.\n", 
        mTryMaintainAspectRatio ? "true" : "false"
    );
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
