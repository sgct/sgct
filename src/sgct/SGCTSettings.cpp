/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/SGCTSettings.h>
#include <sgct/MessageHandler.h>
#include <sgct/ScreenCapture.h>
#include <sgct/ogl_headers.h>
#include "external/tinythread.h"
#include <string.h>

sgct::SGCTSettings * sgct::SGCTSettings::mInstance = NULL;

sgct::SGCTSettings::SGCTSettings()
{
    mPNGCompressionLevel = 1;
    mJPEGQuality = 100;

    mNumberOfCaptureThreads = std::thread::hardware_concurrency();

    mCaptureBackBuffer            = false;
    mUseWarping                    = true;
    mShowWarpingWireframe        = false;
    mUseDepthTexture            = false;
    mUseNormalTexture            = false;
    mUsePositionTexture            = false;
    mUseFBO                        = true;
    mForceGlTexImage2D            = false;
    mUsePBO                        = true;
    mUseRLE                        = false;
    mTryMaintainAspectRatio        = true;
    mExportWarpingMeshes        = false;

    mSwapInterval = 1;
    mRefreshRate = 0;
    mOSDTextOffset[0] = 0.05f;
    mOSDTextOffset[1] = 0.05f;

    //FXAA parameters
    mFXAASubPixTrim = 1.0f/4.0f;
    mFXAASubPixOffset = 1.0f/2.0f;
    mDefaultFXAA = false;

    mDefaultNumberOfAASamples = 1;

    for(size_t i=0; i<3; i++)
        mCapturePath[i].assign("SGCT");
    mCaptureFormat = sgct_core::ScreenCapture::NOT_SET;

    mCurrentDrawBuffer = Diffuse;
    mCurrentBufferFloatPrecision = Float_16Bit;

    //font stuff
    mFontSize = 10;
    #if __WIN32__
    mFontName = "verdanab.ttf";
    #elif __APPLE__
    mFontName = "Tahoma Bold.ttf";
    #else
    mFontName = "FreeSansBold.ttf";
    #endif
}

sgct::SGCTSettings::~SGCTSettings()
{
    ;
}

void sgct::SGCTSettings::configure(tinyxml2::XMLElement * element)
{
    const char * val;
    tinyxml2::XMLElement * subElement = element->FirstChildElement();

    while (subElement != NULL)
    {
        val = subElement->Value();

        if (strcmp("DepthBufferTexture", val) == 0)
        {
            if (subElement->Attribute("value") != NULL)
                sgct::SGCTSettings::instance()->setUseDepthTexture(strcmp(subElement->Attribute("value"), "true") == 0 ? true : false);
        }
        else if (strcmp("NormalTexture", val) == 0)
        {
            if (subElement->Attribute("value") != NULL)
                sgct::SGCTSettings::instance()->setUseNormalTexture(strcmp(subElement->Attribute("value"), "true") == 0 ? true : false);
        }
        else if (strcmp("PositionTexture", val) == 0)
        {
            if (subElement->Attribute("value") != NULL)
                sgct::SGCTSettings::instance()->setUsePositionTexture(strcmp(subElement->Attribute("value"), "true") == 0 ? true : false);
        }
        else if (strcmp("PBO", val) == 0)
        {
            if (subElement->Attribute("value") != NULL)
                sgct::SGCTSettings::instance()->setUsePBO(strcmp(subElement->Attribute("value"), "true") == 0 ? true : false);
        }
        else if (strcmp("Precision", val) == 0)
        {
            int fprec = 0;
            if (subElement->QueryIntAttribute("float", &fprec) == tinyxml2::XML_NO_ERROR)
            {
                if (fprec == 16)
                    sgct::SGCTSettings::instance()->setBufferFloatPrecision(sgct::SGCTSettings::Float_16Bit);
                else if (fprec == 32)
                    sgct::SGCTSettings::instance()->setBufferFloatPrecision(sgct::SGCTSettings::Float_32Bit);
                else
                    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "ReadConfig: Invalid float precition value (%d)! Must be 16 or 32.\n", fprec);
            }
            else
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "ReadConfig: Invalid float precition value! Must be 16 or 32.\n");
        }
        else if (strcmp("Display", val) == 0)
        {
            int tmpInterval = 0;
            if (subElement->QueryIntAttribute("swapInterval", &tmpInterval) == tinyxml2::XML_NO_ERROR)
            {
                sgct::SGCTSettings::instance()->setSwapInterval(tmpInterval);
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "ReadConfig: Display swap interval is set to %d.\n", tmpInterval);
            }

            int rate = 0;
            if (subElement->QueryIntAttribute("refreshRate", &rate) == tinyxml2::XML_NO_ERROR)
            {
                sgct::SGCTSettings::instance()->setRefreshRateHint(rate);
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "ReadConfig: Display refresh rate hint is set to %d Hz.\n", rate);
            }

            if (subElement->Attribute("tryMaintainAspectRatio") != NULL)
                sgct::SGCTSettings::instance()->setTryMaintainAspectRatio(strcmp(subElement->Attribute("tryMaintainAspectRatio"), "true") == 0 ? true : false);

            if (subElement->Attribute("exportWarpingMeshes") != NULL)
                sgct::SGCTSettings::instance()->setExportWarpingMeshes(strcmp(subElement->Attribute("exportWarpingMeshes"), "true") == 0 ? true : false);
        }
        else if (strcmp("OSDText", val) == 0)
        {
            float x = 0.0f;
            float y = 0.0f;

            if (subElement->Attribute("name") != NULL)
            {
                sgct::SGCTSettings::instance()->setOSDTextFontName(subElement->Attribute("name"));
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
                    "ReadConfig: Setting font name to %s\n", subElement->Attribute("name"));
            }

            if (subElement->Attribute("path") != NULL)
            {
                sgct::SGCTSettings::instance()->setOSDTextFontPath(subElement->Attribute("path"));
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
                    "ReadConfig: Setting font path to %s\n", subElement->Attribute("path"));
            }

            if (subElement->Attribute("size") != NULL)
            {
                unsigned int tmpi;
                if (subElement->QueryUnsignedAttribute("size", &tmpi) == tinyxml2::XML_NO_ERROR && tmpi > 0)
                {
                    sgct::SGCTSettings::instance()->setOSDTextFontSize(tmpi);
                    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
                        "ReadConfig: Setting font size to %u\n", tmpi);
                }
                else
                    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "ReadConfig: Font size not specified. Setting to default size=10!\n");
            }

            if (subElement->QueryFloatAttribute("xOffset", &x) == tinyxml2::XML_NO_ERROR)
            {
                sgct::SGCTSettings::instance()->setOSDTextXOffset(x);
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
                    "ReadConfig: Setting font x offset to %f\n", x);
            }

            if (subElement->QueryFloatAttribute("yOffset", &y) == tinyxml2::XML_NO_ERROR)
            {
                sgct::SGCTSettings::instance()->setOSDTextYOffset(y);
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
                    "ReadConfig: Setting font y offset to %f\n", y);
            }
        }
        else if (strcmp("FXAA", val) == 0)
        {
            float offset = 0.0f;
            if (subElement->QueryFloatAttribute("offset", &offset) == tinyxml2::XML_NO_ERROR)
            {
                sgct::SGCTSettings::instance()->setFXAASubPixOffset(offset);
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
                    "ReadConfig: Setting FXAA sub-pixel offset to %f\n", offset);
            }

            float trim = 0.0f;
            if (subElement->QueryFloatAttribute("trim", &trim) == tinyxml2::XML_NO_ERROR)
            {
                if (trim > 0.0f)
                {
                    sgct::SGCTSettings::instance()->setFXAASubPixTrim(1.0f / trim);
                    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
                        "ReadConfig: Setting FXAA sub-pixel trim to %f\n", 1.0f / trim);
                }
                else
                {
                    sgct::SGCTSettings::instance()->setFXAASubPixTrim(0.0f);
                    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
                        "ReadConfig: Setting FXAA sub-pixel trim to %f\n", 0.0f);
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
void sgct::SGCTSettings::setSwapInterval(int val)
{
    mSwapInterval = val;
}

/*!
Get swap interval for all windows
    - -1 = adaptive sync (Nvidia)
    - 0  = vertical sync off
    - 1  = wait for vertical sync
    - 2  = fix when using swapgroups in xp and running half the framerate
*/
const int sgct::SGCTSettings::getSwapInterval() const
{
    return mSwapInterval;
}

/*!
    Set the refreshrate hint of the window in fullscreen mode.
    If it's not listed in your monitor's video-mode list than it will not be used.

    \param freq the refresh frequency/rate
*/
void sgct::SGCTSettings::setRefreshRateHint(int freq)
{
    mRefreshRate = freq;
}

/*!
    Get the refreshrate hint of the window in fullscreen mode.
*/
const int sgct::SGCTSettings::getRefreshRateHint() const
{
    return mRefreshRate;
}


/*!
Set to true if depth buffer textures should be allocated and used.
*/
void sgct::SGCTSettings::setUseDepthTexture(bool state)
{
    mUseDepthTexture = state;
}

/*!
Set to true if normal textures should be allocated and used.
*/
void sgct::SGCTSettings::setUseNormalTexture(bool state)
{
    mUseNormalTexture = state;
    updateDrawBufferFlag();
}

/*!
Set to true if position buffer textures should be allocated and used.
*/
void sgct::SGCTSettings::setUsePositionTexture(bool state)
{
    mUsePositionTexture = state;
    updateDrawBufferFlag();
}

/*!
Set the float precision of the float buffers (normal and position buffer)
@param bfp is the float precition that will be used in next buffer resize or creation
*/
void sgct::SGCTSettings::setBufferFloatPrecision(BufferFloatPrecision bfp)
{
    mCurrentBufferFloatPrecision = bfp;
}

/*!
Update the draw buffer flags
*/
void sgct::SGCTSettings::updateDrawBufferFlag()
{
    if (mUseNormalTexture && mUsePositionTexture)
        mCurrentDrawBuffer = Diffuse_Normal_Position;
    else if (!mUseNormalTexture && !mUsePositionTexture)
        mCurrentDrawBuffer = Diffuse;
    else if (mUseNormalTexture && !mUsePositionTexture)
        mCurrentDrawBuffer = Diffuse_Normal;
    else
        mCurrentDrawBuffer = Diffuse_Position;
}

/*!
Set the FBO mode. This is done internally using SGCT config file.
*/
void sgct::SGCTSettings::setUseFBO(bool state)
{
    mUseFBO = state;
}

/*!
Set the number of capture threads used by SGCT (multi-threaded screenshots)
*/
void sgct::SGCTSettings::setNumberOfCaptureThreads(int count)
{
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
void sgct::SGCTSettings::setPNGCompressionLevel(int level)
{
    mMutex.lock();
    mPNGCompressionLevel = level;
    mMutex.unlock();
}

/*!
Set the JPEG quality in range [0-100].
*/
void sgct::SGCTSettings::setJPEGQuality(int quality)
{
    mMutex.lock();
    mJPEGQuality = quality;
    mMutex.unlock();
}

/*!
Get the zlib compression level used in png export.
*/
const int sgct::SGCTSettings::getPNGCompressionLevel()
{ 
    int tmpI;
    mMutex.lock();
    tmpI = mPNGCompressionLevel;
    mMutex.unlock();
    return tmpI;
}

/*!
Get the JPEG quality settings (0-100)
*/
const int sgct::SGCTSettings::getJPEGQuality()
{
    int tmpI;
    mMutex.lock();
    tmpI = mJPEGQuality;
    mMutex.unlock();
    return tmpI;
}

/*!
Set capture/screenshot path used by SGCT

\param path the path including filename without suffix
\param cpi index to which path to set (Mono = default, Left or Right)
*/
void sgct::SGCTSettings::setCapturePath(std::string path, sgct::SGCTSettings::CapturePathIndex cpi)
{
    if( path.empty() ) //invalid filename
    {
        MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "SGCTSettings: Empty screen capture path!\n");
        return;
    }

    mCapturePath[cpi].assign(path);
}

/*!
Append capture/screenshot path used by SGCT

\param str the string to append including filename without suffix
\param cpi index to which path to set (Mono = default, Left or Right)
*/
void sgct::SGCTSettings::appendCapturePath(std::string str, sgct::SGCTSettings::CapturePathIndex cpi)
{
    mCapturePath[cpi].append( str );
}

/*!
Set the capture format which can be one of the following:
-PNG
-TGA
*/
void sgct::SGCTSettings::setCaptureFormat(const char * format)
{
    mMutex.lock();
    
    if( strcmp("png", format) == 0 || strcmp("PNG", format) == 0 )
    {
        mCaptureFormat = sgct_core::ScreenCapture::PNG;
    }
    else if( strcmp("tga", format) == 0 || strcmp("TGA", format) == 0 )
    {
        mCaptureFormat = sgct_core::ScreenCapture::TGA;
    }
    else if (strcmp("jpg", format) == 0 || strcmp("JPG", format) == 0)
    {
        mCaptureFormat = sgct_core::ScreenCapture::JPEG;
    }

    mMutex.unlock();
}

/*!
    Get the capture/screenshot path

    \param cpi index to which path to get (Mono = default, Left or Right)
*/
const char * sgct::SGCTSettings::getCapturePath(sgct::SGCTSettings::CapturePathIndex cpi) const
{
    return mCapturePath[cpi].c_str();
}

/*!
    Get the capture/screenshot path

    \return the captureformat if set, otherwise -1 is returned
*/
const int sgct::SGCTSettings::getCaptureFormat()
{
    int tmpI;
    mMutex.lock();
    tmpI = mCaptureFormat;
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
void sgct::SGCTSettings::setFXAASubPixTrim(float val)
{
    mFXAASubPixTrim = val;
}

/*!
    Set the pixel offset for contrast/edge detection. Values should be in the range [1.0f/8.0f, 1.0f]. Default is 0.5f.
*/
void sgct::SGCTSettings::setFXAASubPixOffset(float val)
{
    mFXAASubPixOffset = val;
}


/*!
    Set the horizontal OSD text Offset between 0.0 and 1.0
*/
void sgct::SGCTSettings::setOSDTextXOffset(float val)
{
    mOSDTextOffset[ 0 ] = val;
}

/*!
    Set the vertical OSD text Offset between 0.0 and 1.0
*/
void sgct::SGCTSettings::setOSDTextYOffset(float val)
{
    mOSDTextOffset[ 1 ] = val;
}

/*!
    Set the OSD text font size
*/
void sgct::SGCTSettings::setOSDTextFontSize( unsigned int size )
{
    mFontSize = size;
}

/*!
    Set the OSD text font name
*/
void sgct::SGCTSettings::setOSDTextFontName( std::string name )
{
    mFontName.assign( name );
}

/*!
    Set the OSD text font path
*/
void sgct::SGCTSettings::setOSDTextFontPath( std::string path )
{
    mFontPath.assign( path );
}

/*!
Set the default number of AA samples (MSAA) for all windows
*/
void sgct::SGCTSettings::setDefaultNumberOfAASamples(int samples)
{
    if ((samples != 0) && ((samples & (samples - 1)) == 0)) //if power of two
        mDefaultNumberOfAASamples = samples;
    else
        MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "SGCTSettings: Number of default MSAA samples must be a power of two (not %d)!\n", samples);
}

/*!
Set the default FXAA state for all windows (enabled or disabled)
*/
void sgct::SGCTSettings::setDefaultFXAAState(bool state)
{
    mDefaultFXAA = state;
}

/*!
Set the glTexImage2D (legacy) should be used instead of glTexStorage2D (modern). For example gDebugger can't display textures created using glTexStorage2D.
*/
void sgct::SGCTSettings::setForceGlTexImage2D(bool state)
{
    mForceGlTexImage2D = state;
}

/*!
Get if glTexImage2D(legacy) should be used instead of glTexStorage2D(modern). For example gDebugger can't display textures created using glTexStorage2D.
*/
const bool sgct::SGCTSettings::getForceGlTexImage2D() const
{
    return mForceGlTexImage2D;
}

/*!
Set if pixel buffer object transferes should be used
*/
void sgct::SGCTSettings::setUsePBO(bool state)
{
    mUsePBO = state;
}

/*!
Get if pixel buffer object transferes should be used
*/
const bool sgct::SGCTSettings::getUsePBO() const
{
    return mUsePBO;
}

/*!
Set if run length encoding (RLE) should be used in PNG and TGA export.
*/
void sgct::SGCTSettings::setUseRLE(bool state)
{
    mMutex.lock();
    mUseRLE = state;
    mMutex.unlock();
}

/*!
Set if screen warping should be used or not
*/
void sgct::SGCTSettings::setUseWarping(bool state)
{
    mUseWarping = state;
}

/*!
Set if warping mesh wireframe should be rendered
*/
void sgct::SGCTSettings::setShowWarpingWireframe(bool state)
{
    mShowWarpingWireframe = state;
}

/*!
Set if capture should capture warped from backbuffer instead of texture. Backbuffer data includes masks and warping.
*/
void sgct::SGCTSettings::setCaptureFromBackBuffer(bool state)
{
    mCaptureBackBuffer = state;
}

/*!
Set to true if warping meshes should be exported as OBJ files.
*/
void sgct::SGCTSettings::setExportWarpingMeshes(bool state)
{
    mExportWarpingMeshes = state;
}

/*!
Get if run length encoding (RLE) is used in PNG and TGA export.
*/
const bool sgct::SGCTSettings::getUseRLE()
{
    bool tmpB;
    mMutex.lock();
    tmpB = mUseRLE;
    mMutex.unlock();
    return tmpB;
}

/*!
Get if aspect ratio is taken into acount when generation some display geometries.
*/
const bool sgct::SGCTSettings::getTryMaintainAspectRatio() const
{
    return mTryMaintainAspectRatio;
}

/*!
Get if warping meshes should be exported as obj-files.
*/
const bool sgct::SGCTSettings::getExportWarpingMeshes() const
{
    return mExportWarpingMeshes;
}

/*!
Get if screen warping is used
*/
const bool sgct::SGCTSettings::getUseWarping() const
{
    return mUseWarping;
}

/*!
Get if warping wireframe mesh should be rendered
*/
const bool sgct::SGCTSettings::getShowWarpingWireframe() const
{
    return mShowWarpingWireframe;
}

/*!
Get if capture should use backbuffer data or texture. Backbuffer data includes masks and warping.
*/
const bool sgct::SGCTSettings::getCaptureFromBackBuffer() const
{
    return mCaptureBackBuffer;
}

/*!
Set if geometry should try to adapt after framebuffer dimensions. This is valid for multi-viewport renderings like fisheye projections.
*/
void sgct::SGCTSettings::setTryMaintainAspectRatio(bool state)
{
    mTryMaintainAspectRatio = state;
    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "SGCTSettings: Set try maintain aspect ratio to: %s.\n", 
        mTryMaintainAspectRatio ? "true" : "false");
}

/*!
    Get the OSD text font size
*/
const unsigned int & sgct::SGCTSettings::getOSDTextFontSize() const
{
    return mFontSize;
}

/*!
    Get the OSD text font name
*/
const std::string & sgct::SGCTSettings::getOSDTextFontName() const
{
    return mFontName;
}

/*!
    Get the OSD text font path
*/
const std::string & sgct::SGCTSettings::getOSDTextFontPath() const
{
    return mFontPath;
}

/*!
    Get the precision of the float buffers as an GLint (GL_RGB16F or GL_RGB32F)
*/
const int    sgct::SGCTSettings::getBufferFloatPrecisionAsGLint() const
{
    return mCurrentBufferFloatPrecision == Float_16Bit ? GL_RGB16F : GL_RGB32F;
}

/*!
Get the default MSAA setting
*/
const int sgct::SGCTSettings::getDefaultNumberOfAASamples() const
{
    return mDefaultNumberOfAASamples;
}

/*!
Get the FXAA default state
*/
const bool sgct::SGCTSettings::getDefaultFXAAState() const
{
    return mDefaultFXAA;
}
