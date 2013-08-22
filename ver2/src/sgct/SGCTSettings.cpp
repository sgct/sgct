/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
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

	mUseDepthTexture = false;
	mUseFBO = true;

	mOSDTextOffset[0] = 0.05f;
	mOSDTextOffset[1] = 0.05f;

	//FXAA parameters
	mFXAASubPixTrim = 1.0f/8.0f;
	mFXAASubPixOffset = 1.0f/2.0f;

	for(size_t i=0; i<3; i++)
		mCapturePath[i].assign("SGCT");
	mCaptureFormat = sgct_core::ScreenCapture::NOT_SET;
}

sgct::SGCTSettings::~SGCTSettings()
{
	;
}

/*!
Set to true if depth buffer textures should be allocated and used.
*/
void sgct::SGCTSettings::setUseDepthTexture(bool state)
{
	mUseDepthTexture = state;
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
	-1 = Default compression
	0 = No compression
	1 = Best speed
	9 = Best compression
*/
void sgct::SGCTSettings::setPNGCompressionLevel(int level)
{
	mPNGCompressionLevel = level;
}

/*!
Set capture/screenshot path used by SGCT

\param path the path including filename without suffix
\param cpi index to which path to set (Mono = default, Left or Right)
*/
void sgct::SGCTSettings::setCapturePath(std::string path, sgct::SGCTSettings::CapturePathIndexes cpi)
{
	if( path.empty() ) //invalid filename
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "SGCTSettings: Empty screen capture path!\n");
		return;
	}
		
	mCapturePath[cpi].assign(path);
}

/*!
Append capture/screenshot path used by SGCT

\param str the string to append including filename without suffix
\param cpi index to which path to set (Mono = default, Left or Right)
*/
void sgct::SGCTSettings::appendCapturePath(std::string str, sgct::SGCTSettings::CapturePathIndexes cpi)
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
	if( strcmp("png", format) == 0 || strcmp("PNG", format) == 0 )
	{
		mCaptureFormat = sgct_core::ScreenCapture::PNG;
	}
	else if( strcmp("tga", format) == 0 || strcmp("TGA", format) == 0 )
	{
		mCaptureFormat = sgct_core::ScreenCapture::TGA;
	}
}

/*!
	Get the capture/screenshot path

	\param cpi index to which path to get (Mono = default, Left or Right)
*/
const char * sgct::SGCTSettings::getCapturePath(sgct::SGCTSettings::CapturePathIndexes cpi)
{
	return mCapturePath[cpi].c_str();
}

/*!
	Get the capture/screenshot path

	\return the captureformat if set, otherwise -1 is returned
*/
int sgct::SGCTSettings::getCaptureFormat()
{
	return mCaptureFormat;
}

/*!
	Controls removal of sub-pixel aliasing.
	- 1/2 – low removal
	- 1/3 – medium removal
	- 1/4 – default removal
	- 1/8 – high removal
	- 0 – complete removal
	Default is 1.0f/8.0f.
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

