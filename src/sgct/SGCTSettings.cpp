/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include "../include/sgct/SGCTSettings.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/ScreenCapture.h"
#include "../include/sgct/ogl_headers.h"

#define DEFAULT_NUMBER_OF_CAPTURE_THREADS 8

sgct::SGCTSettings * sgct::SGCTSettings::mInstance = NULL;

sgct::SGCTSettings::SGCTSettings()
{
	mPNGCompressionLevel = 1;
	mNumberOfCaptureThreads = DEFAULT_NUMBER_OF_CAPTURE_THREADS;

	mUseWarping = true;
	mUseDepthTexture = false;
	mUseNormalTexture = false;
	mUsePositionTexture = false;
	mUseFBO = true;
	mForceGlTexImage2D = false;
	mUsePBO = true;
	mUseRLE = false;

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
    mFisheyeMethod = FourFaceCube;

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
int sgct::SGCTSettings::getSwapInterval()
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
int sgct::SGCTSettings::getRefreshRateHint()
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
Get the zlib compression level used in png export.
*/
int sgct::SGCTSettings::getPNGCompressionLevel()
{ 
	int tmpI;
	mMutex.lock();
	tmpI = mPNGCompressionLevel;
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

	mMutex.unlock();
}

/*!
	Get the capture/screenshot path

	\param cpi index to which path to get (Mono = default, Left or Right)
*/
const char * sgct::SGCTSettings::getCapturePath(sgct::SGCTSettings::CapturePathIndex cpi)
{
	return mCapturePath[cpi].c_str();
}

/*!
	Get the capture/screenshot path

	\return the captureformat if set, otherwise -1 is returned
*/
int sgct::SGCTSettings::getCaptureFormat()
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
void sgct::SGCTSettings::setOSDTextFontSize( int size )
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
bool sgct::SGCTSettings::getForceGlTexImage2D()
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
bool sgct::SGCTSettings::getUsePBO()
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
Get if run length encoding (RLE) is used in PNG and TGA export.
*/
bool sgct::SGCTSettings::getUseRLE()
{
	bool tmpB;
	mMutex.lock();
	tmpB = mUseRLE;
	mMutex.unlock();
	return tmpB;
}

/*!
Get if screen warping is used
*/
bool sgct::SGCTSettings::getUseWarping()
{
	return mUseWarping;
}

/*!
Set which method should be used to generate a fisheye image.\n
Four cubemap faces render less iterations but uses more fillrate\n
Five cubemap faces is more fillrate efficient (33%) but calls the draw callback one additional time.
 */
void sgct::SGCTSettings::setFisheyeMethod(FisheyeMethod fm)
{
    mFisheyeMethod = fm;
}

/*!
Get the method used to generate fisheye images.
 */
sgct::SGCTSettings::FisheyeMethod sgct::SGCTSettings::getFisheyeMethod()
{
    return mFisheyeMethod;
}

/*!
	Get the OSD text font size
*/
const int &	sgct::SGCTSettings::getOSDTextFontSize()
{
	return mFontSize;
}

/*!
	Get the OSD text font name
*/
const std::string & sgct::SGCTSettings::getOSDTextFontName()
{
	return mFontName;
}

/*!
	Get the OSD text font path
*/
const std::string & sgct::SGCTSettings::getOSDTextFontPath()
{
	return mFontPath;
}

/*!
	Get the precision of the float buffers as an GLint (GL_RGB16F or GL_RGB32F)
*/
int	sgct::SGCTSettings::getBufferFloatPrecisionAsGLint()
{
	return mCurrentBufferFloatPrecision == Float_16Bit ? GL_RGB16F : GL_RGB32F;
}

/*!
Get the default MSAA setting
*/
int sgct::SGCTSettings::getDefaultNumberOfAASamples()
{
	return mDefaultNumberOfAASamples;
}

/*!
Get the FXAA default state
*/
bool sgct::SGCTSettings::getDefaultFXAAState()
{
	return mDefaultFXAA;
}
