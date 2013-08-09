/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/SGCTSettings.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/ScreenCapture.h"

#define DEFAULT_NUMBER_OF_CAPTURE_THREADS 8

sgct_core::SGCTSettings * sgct_core::SGCTSettings::mInstance = NULL;

sgct_core::SGCTSettings::SGCTSettings()
{
	mPNGCompressionLevel = 1;

	mNumberOfCaptureThreads = DEFAULT_NUMBER_OF_CAPTURE_THREADS;

	mUseFBO = true;

	for(size_t i=0; i<3; i++)
		mCapturePath[i].assign("SGCT");
	mCaptureFormat = ScreenCapture::NOT_SET;
}

sgct_core::SGCTSettings::~SGCTSettings()
{
	;
}

/*!
Set the FBO mode. This is done internally using SGCT config file.
*/
void sgct_core::SGCTSettings::setUseFBO(bool state)
{
	mUseFBO = state;
}

/*!
Set the number of capture threads used by SGCT (multi-threaded screenshots)
*/
void sgct_core::SGCTSettings::setNumberOfCaptureThreads(int count)
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
void sgct_core::SGCTSettings::setPNGCompressionLevel(int level)
{
	mPNGCompressionLevel = level;
}

/*!
Set capture/screenshot path used by SGCT

\param path the path including filename without suffix
\param cpi index to which path to set (Mono = default, Left or Right)
*/
void sgct_core::SGCTSettings::setCapturePath(std::string path, sgct_core::SGCTSettings::CapturePathIndexes cpi)
{
	if( path.empty() ) //invalid filename
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "SGCTSettings: Empty screen capture path!\n");
		return;
	}
		
	mCapturePath[cpi].assign(path);
}

/*!
Append capture/screenshot path used by SGCT

\param str the string to append including filename without suffix
\param cpi index to which path to set (Mono = default, Left or Right)
*/
void sgct_core::SGCTSettings::appendCapturePath(std::string str, sgct_core::SGCTSettings::CapturePathIndexes cpi)
{
	mCapturePath[cpi].append( str );
}

/*!
Set the capture format which can be one of the following:
-PNG
-TGA
*/
void sgct_core::SGCTSettings::setCaptureFormat(const char * format)
{
	if( strcmp("png", format) == 0 || strcmp("PNG", format) == 0 )
	{
		mCaptureFormat = ScreenCapture::PNG;
	}
	else if( strcmp("tga", format) == 0 || strcmp("TGA", format) == 0 )
	{
		mCaptureFormat = ScreenCapture::TGA;
	}
}

/*!
	Get the capture/screenshot path

	\param cpi index to which path to get (Mono = default, Left or Right)
*/
const char * sgct_core::SGCTSettings::getCapturePath(sgct_core::SGCTSettings::CapturePathIndexes cpi)
{
	return mCapturePath[cpi].c_str();
}

/*!
	Get the capture/screenshot path

	\return the captureformat if set, otherwise -1 is returned
*/
int sgct_core::SGCTSettings::getCaptureFormat()
{
	return mCaptureFormat;
}
