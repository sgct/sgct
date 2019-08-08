/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/SGCTSettings.h>

#include <sgct/MessageHandler.h>
#include <sgct/ScreenCapture.h>
#include <sgct/ogl_headers.h>
#include <string.h>

namespace sgct {

SGCTSettings* SGCTSettings::mInstance = nullptr;

SGCTSettings* SGCTSettings::instance() {
    if (mInstance == nullptr) {
        mInstance = new SGCTSettings();
    }

    return mInstance;
}

/*! Destroy the SGCTSettings instance */
void SGCTSettings::destroy() {
    if (mInstance != nullptr) {
        delete mInstance;
        mInstance = nullptr;
    }
}

SGCTSettings::SGCTSettings()
    : mCaptureFormat(sgct_core::ScreenCapture::NOT_SET)
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
                    instance()->setBufferFloatPrecision(Float_16Bit);
                }
                else if (fprec == 32) {
                    instance()->setBufferFloatPrecision(Float_32Bit);
                }
                else {
                    MessageHandler::instance()->print(
                        MessageHandler::NOTIFY_WARNING,
                        "ReadConfig: Invalid float precition value (%d)! Must be 16 or 32.\n",
                        fprec
                    );
                }
            }
            else {
                MessageHandler::instance()->print(
                    MessageHandler::NOTIFY_WARNING,
                    "ReadConfig: Invalid float precition value! Must be 16 or 32.\n"
                );
            }
        }
        else if (strcmp("Display", val) == 0) {
            int interval = 0;
            if (subElement->QueryIntAttribute("swapInterval", &interval) == XML_NO_ERROR) {
                instance()->setSwapInterval(interval);
                MessageHandler::instance()->print(
                    MessageHandler::NOTIFY_INFO,
                    "ReadConfig: Display swap interval is set to %d.\n",
                    interval
                );
            }

            int rate = 0;
            if (subElement->QueryIntAttribute("refreshRate", &rate) == XML_NO_ERROR) {
                instance()->setRefreshRateHint(rate);
                MessageHandler::instance()->print(
                    MessageHandler::NOTIFY_INFO,
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
                    MessageHandler::NOTIFY_DEBUG,
                    "ReadConfig: Setting font name to %s\n",
                    subElement->Attribute("name")
                );
            }

            if (subElement->Attribute("path") != nullptr) {
                instance()->setOSDTextFontPath(subElement->Attribute("path"));
                MessageHandler::instance()->print(
                    MessageHandler::NOTIFY_DEBUG,
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
                        MessageHandler::NOTIFY_DEBUG,
                        "ReadConfig: Setting font size to %u\n",
                        tmpi
                    );
                }
                else {
                    MessageHandler::instance()->print(
                        MessageHandler::NOTIFY_WARNING,
                        "ReadConfig: Font size not specified. Setting to default size=10!\n"
                    );
                }
            }

            if (subElement->QueryFloatAttribute("xOffset", &x) == XML_NO_ERROR) {
                instance()->setOSDTextXOffset(x);
                MessageHandler::instance()->print(
                    MessageHandler::NOTIFY_DEBUG,
                    "ReadConfig: Setting font x offset to %f\n", x
                );
            }

            if (subElement->QueryFloatAttribute("yOffset", &y) == XML_NO_ERROR) {
                instance()->setOSDTextYOffset(y);
                MessageHandler::instance()->print(
                    MessageHandler::NOTIFY_DEBUG,
                    "ReadConfig: Setting font y offset to %f\n", y
                );
            }
        }
        else if (strcmp("FXAA", val) == 0) {
            float offset = 0.f;
            if (subElement->QueryFloatAttribute("offset", &offset) == XML_NO_ERROR) {
                instance()->setFXAASubPixOffset(offset);
                MessageHandler::instance()->print(
                    MessageHandler::NOTIFY_DEBUG,
                    "ReadConfig: Setting FXAA sub-pixel offset to %f\n", offset
                );
            }

            float trim = 0.f;
            if (subElement->QueryFloatAttribute("trim", &trim) == XML_NO_ERROR) {
                if (trim > 0.f) {
                    instance()->setFXAASubPixTrim(1.f / trim);
                    MessageHandler::instance()->print(
                        sgct::MessageHandler::NOTIFY_DEBUG,
                        "ReadConfig: Setting FXAA sub-pixel trim to %f\n", 1.0f / trim
                    );
                }
                else {
                    instance()->setFXAASubPixTrim(0.f);
                    MessageHandler::instance()->print(
                        MessageHandler::NOTIFY_DEBUG,
                        "ReadConfig: Setting FXAA sub-pixel trim to %f\n", 0.f
                    );
                }
            }
        }

        //iterate
        subElement = subElement->NextSiblingElement();
    }
}

/*!
Set swap interval for all windows
    - -1 = adaptive sync (Nvidia)
    - 0  = vertical sync off
    - 1  = wait for vertical sync
    - 2  = fix when using swapgroups in xp and running half the framerate
*/
void SGCTSettings::setSwapInterval(int val) {
    mSwapInterval = val;
}

/*!
Get swap interval for all windows
    - -1 = adaptive sync (Nvidia)
    - 0  = vertical sync off
    - 1  = wait for vertical sync
    - 2  = fix when using swapgroups in xp and running half the framerate
*/
int SGCTSettings::getSwapInterval() const {
    return mSwapInterval;
}

/*!
    Set the refreshrate hint of the window in fullscreen mode.
    If it's not listed in your monitor's video-mode list than it will not be used.

    \param freq the refresh frequency/rate
*/
void SGCTSettings::setRefreshRateHint(int freq) {
    mRefreshRate = freq;
}

/*!
    Get the refreshrate hint of the window in fullscreen mode.
*/
int SGCTSettings::getRefreshRateHint() const {
    return mRefreshRate;
}

/*!
Set to true if depth buffer textures should be allocated and used.
*/
void SGCTSettings::setUseDepthTexture(bool state) {
    mUseDepthTexture = state;
}

/*!
Set to true if normal textures should be allocated and used.
*/
void SGCTSettings::setUseNormalTexture(bool state) {
    mUseNormalTexture = state;
    updateDrawBufferFlag();
}

/*!
Set to true if position buffer textures should be allocated and used.
*/
void SGCTSettings::setUsePositionTexture(bool state) {
    mUsePositionTexture = state;
    updateDrawBufferFlag();
}

/*!
Set the float precision of the float buffers (normal and position buffer)
@param bfp is the float precition that will be used in next buffer resize or creation
*/
void SGCTSettings::setBufferFloatPrecision(BufferFloatPrecision bfp) {
    mCurrentBufferFloatPrecision = bfp;
}

/*!
Update the draw buffer flags
*/
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

/*!
Set the FBO mode. This is done internally using SGCT config file.
*/
void SGCTSettings::setUseFBO(bool state) {
    mUseFBO = state;
}

/*!
Set the number of capture threads used by SGCT (multi-threaded screenshots)
*/
void SGCTSettings::setNumberOfCaptureThreads(int count) {
    mNumberOfCaptureThreads = count;
}

/*!
Set the zlib compression level used for saving png files

Compression levels 1-9.
    -1 = Default compression\n
    0 = No compression\n
    1 = Best speed\n
    9 = Best compression\n
*/
void SGCTSettings::setPNGCompressionLevel(int level) {
    mMutex.lock();
    mPNGCompressionLevel = level;
    mMutex.unlock();
}

/*!
Set the JPEG quality in range [0-100].
*/
void SGCTSettings::setJPEGQuality(int quality) {
    mMutex.lock();
    mJPEGQuality = quality;
    mMutex.unlock();
}

/*!
Get the zlib compression level used in png export.
*/
int SGCTSettings::getPNGCompressionLevel() { 
    mMutex.lock();
    int tmpI = mPNGCompressionLevel;
    mMutex.unlock();
    return tmpI;
}

/*!
Get the JPEG quality settings (0-100)
*/
int SGCTSettings::getJPEGQuality() {
    mMutex.lock();
    int tmpI = mJPEGQuality;
    mMutex.unlock();
    return tmpI;
}

bool SGCTSettings::useDepthTexture() {
    return mUseDepthTexture;
}

bool SGCTSettings::useNormalTexture() {
    return mUseNormalTexture;
}

bool SGCTSettings::usePositionTexture() {
    return mUsePositionTexture;
}

bool SGCTSettings::useFBO() {
    return mUseFBO;
}

int SGCTSettings::getNumberOfCaptureThreads() {
    return mNumberOfCaptureThreads;
}

float SGCTSettings::getOSDTextXOffset() {
    return mOSDTextOffset[0];
}

float SGCTSettings::getOSDTextYOffset() {
    return mOSDTextOffset[1];
}

float SGCTSettings::getFXAASubPixTrim() {
    return mFXAASubPixTrim;
}

float SGCTSettings::getFXAASubPixOffset() {
    return mFXAASubPixOffset;
}

SGCTSettings::DrawBufferType SGCTSettings::getCurrentDrawBufferType() {
    return mCurrentDrawBuffer;
}

/*!
Set capture/screenshot path used by SGCT

\param path the path including filename without suffix
\param cpi index to which path to set (Mono = default, Left or Right)
*/
void SGCTSettings::setCapturePath(std::string path, CapturePathIndex cpi) {
    if (path.empty()) {
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_ERROR,
            "SGCTSettings: Empty screen capture path!\n"
        );
        return;
    }

    mCapturePath[cpi] = std::move(path);
}

/*!
Append capture/screenshot path used by SGCT

\param str the string to append including filename without suffix
\param cpi index to which path to set (Mono = default, Left or Right)
*/
void SGCTSettings::appendCapturePath(std::string str, CapturePathIndex cpi) {
    mCapturePath[cpi].append(std::move(str));
}

/*!
Set the capture format which can be one of the following:
-PNG
-TGA
*/
void SGCTSettings::setCaptureFormat(const char* format) {
    mMutex.lock();
    
    if (strcmp("png", format) == 0 || strcmp("PNG", format) == 0) {
        mCaptureFormat = sgct_core::ScreenCapture::PNG;
    }
    else if (strcmp("tga", format) == 0 || strcmp("TGA", format) == 0) {
        mCaptureFormat = sgct_core::ScreenCapture::TGA;
    }
    else if (strcmp("jpg", format) == 0 || strcmp("JPG", format) == 0) {
        mCaptureFormat = sgct_core::ScreenCapture::JPEG;
    }

    mMutex.unlock();
}

/*!
    Get the capture/screenshot path

    \param cpi index to which path to get (Mono = default, Left or Right)
*/
const char* SGCTSettings::getCapturePath(CapturePathIndex cpi) const {
    return mCapturePath[cpi].c_str();
}

/*!
    Get the capture/screenshot path

    \return the captureformat if set, otherwise -1 is returned
*/
int SGCTSettings::getCaptureFormat() {
    mMutex.lock();
    int tmpI = mCaptureFormat;
    mMutex.unlock();
    return tmpI;
}

/*!
    Controls removal of sub-pixel aliasing.
    - 1/2 - low removal
    - 1/3 - medium removal
    - 1/4 - default removal
    - 1/8 - high removal
    - 0 - complete removal
*/
void SGCTSettings::setFXAASubPixTrim(float val) {
    mFXAASubPixTrim = val;
}

/*!
    Set the pixel offset for contrast/edge detection. Values should be in the range [1.0f/8.0f, 1.0f]. Default is 0.5f.
*/
void SGCTSettings::setFXAASubPixOffset(float val) {
    mFXAASubPixOffset = val;
}


/*!
    Set the horizontal OSD text Offset between 0.0 and 1.0
*/
void SGCTSettings::setOSDTextXOffset(float val) {
    mOSDTextOffset[0] = val;
}

/*!
    Set the vertical OSD text Offset between 0.0 and 1.0
*/
void SGCTSettings::setOSDTextYOffset(float val) {
    mOSDTextOffset[1] = val;
}

/*!
    Set the OSD text font size
*/
void SGCTSettings::setOSDTextFontSize(unsigned int size) {
    mFontSize = size;
}

/*!
    Set the OSD text font name
*/
void SGCTSettings::setOSDTextFontName(std::string name) {
    mFontName = std::move(name);
}

/*!
    Set the OSD text font path
*/
void SGCTSettings::setOSDTextFontPath(std::string path) {
    mFontPath = std::move(path);
}

/*!
Set the default number of AA samples (MSAA) for all windows
*/
void SGCTSettings::setDefaultNumberOfAASamples(int samples) {
    if ((samples != 0) && ((samples & (samples - 1)) == 0)) {
        //if power of two
        mDefaultNumberOfAASamples = samples;
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_WARNING,
            "SGCTSettings: Number of default MSAA samples must be a power of two (not %d)!\n",
            samples
        );
    }
}

/*!
Set the default FXAA state for all windows (enabled or disabled)
*/
void SGCTSettings::setDefaultFXAAState(bool state) {
    mDefaultFXAA = state;
}

/*!
Set the glTexImage2D (legacy) should be used instead of glTexStorage2D (modern). For example gDebugger can't display textures created using glTexStorage2D.
*/
void SGCTSettings::setForceGlTexImage2D(bool state) {
    mForceGlTexImage2D = state;
}

/*!
Get if glTexImage2D(legacy) should be used instead of glTexStorage2D(modern). For example gDebugger can't display textures created using glTexStorage2D.
*/
bool SGCTSettings::getForceGlTexImage2D() const {
    return mForceGlTexImage2D;
}

/*!
Set if pixel buffer object transferes should be used
*/
void SGCTSettings::setUsePBO(bool state) {
    mUsePBO = state;
}

/*!
Get if pixel buffer object transferes should be used
*/
bool SGCTSettings::getUsePBO() const {
    return mUsePBO;
}

/*!
Set if run length encoding (RLE) should be used in PNG and TGA export.
*/
void SGCTSettings::setUseRLE(bool state) {
    mMutex.lock();
    mUseRLE = state;
    mMutex.unlock();
}

/*!
Set if screen warping should be used or not
*/
void SGCTSettings::setUseWarping(bool state) {
    mUseWarping = state;
}

/*!
Set if warping mesh wireframe should be rendered
*/
void SGCTSettings::setShowWarpingWireframe(bool state) {
    mShowWarpingWireframe = state;
}

/*!
Set if capture should capture warped from backbuffer instead of texture. Backbuffer data includes masks and warping.
*/
void SGCTSettings::setCaptureFromBackBuffer(bool state) {
    mCaptureBackBuffer = state;
}

/*!
Set to true if warping meshes should be exported as OBJ files.
*/
void SGCTSettings::setExportWarpingMeshes(bool state) {
    mExportWarpingMeshes = state;
}

/*!
Get if run length encoding (RLE) is used in PNG and TGA export.
*/
bool SGCTSettings::getUseRLE() {
    mMutex.lock();
    bool tmpB = mUseRLE;
    mMutex.unlock();
    return tmpB;
}

/*!
Get if aspect ratio is taken into acount when generation some display geometries.
*/
bool SGCTSettings::getTryMaintainAspectRatio() const {
    return mTryMaintainAspectRatio;
}

/*!
Get if warping meshes should be exported as obj-files.
*/
bool SGCTSettings::getExportWarpingMeshes() const {
    return mExportWarpingMeshes;
}

/*!
Get if screen warping is used
*/
bool SGCTSettings::getUseWarping() const {
    return mUseWarping;
}

/*!
Get if warping wireframe mesh should be rendered
*/
bool SGCTSettings::getShowWarpingWireframe() const {
    return mShowWarpingWireframe;
}

/*!
Get if capture should use backbuffer data or texture. Backbuffer data includes masks and warping.
*/
bool SGCTSettings::getCaptureFromBackBuffer() const {
    return mCaptureBackBuffer;
}

/*!
Set if geometry should try to adapt after framebuffer dimensions. This is valid for multi-viewport renderings like fisheye projections.
*/
void SGCTSettings::setTryMaintainAspectRatio(bool state) {
    mTryMaintainAspectRatio = state;
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::NOTIFY_DEBUG,
        "SGCTSettings: Set try maintain aspect ratio to: %s.\n", 
        mTryMaintainAspectRatio ? "true" : "false"
    );
}

/*!
    Get the OSD text font size
*/
unsigned int SGCTSettings::getOSDTextFontSize() const {
    return mFontSize;
}

/*!
    Get the OSD text font name
*/
const std::string& SGCTSettings::getOSDTextFontName() const {
    return mFontName;
}

/*!
    Get the OSD text font path
*/
const std::string& SGCTSettings::getOSDTextFontPath() const {
    return mFontPath;
}

/*!
    Get the precision of the float buffers as an GLint (GL_RGB16F or GL_RGB32F)
*/
int SGCTSettings::getBufferFloatPrecisionAsGLint() const {
    return mCurrentBufferFloatPrecision == Float_16Bit ? GL_RGB16F : GL_RGB32F;
}

/*!
Get the default MSAA setting
*/
int SGCTSettings::getDefaultNumberOfAASamples() const {
    return mDefaultNumberOfAASamples;
}

/*!
Get the FXAA default state
*/
bool SGCTSettings::getDefaultFXAAState() const {
    return mDefaultFXAA;
}

} // namespace sgct
