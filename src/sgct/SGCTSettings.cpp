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
	mCubeMapResolution = 256; //low
	mCubeMapSize = 14.8f; //dome diamter
	mFisheyeTilt = 0.0f;
	mFieldOfView = 180.0f;

	mCropFactors[0] = 0.0f;
	mCropFactors[1] = 0.0f;
	mCropFactors[2] = 0.0f;
	mCropFactors[3] = 0.0f;

	mFisheyeOffset[0] = 0.0f;
	mFisheyeOffset[1] = 0.0f;
	mFisheyeOffset[2] = 0.0f;
	mFisheyeOffaxis = false;
	mFisheyeAlpha = false;

	mNumberOfCaptureThreads = DEFAULT_NUMBER_OF_CAPTURE_THREADS;

	mUseFXAA = false;
	mUsePostFX = false;
	mFBOMode = MultiSampledFBO;

	for(size_t i=0; i<3; i++)
		mCapturePath[i].assign("SGCT");
	mCaptureFormat = ScreenCapture::NOT_SET;
}

sgct_core::SGCTSettings::~SGCTSettings()
{
	;
}

/*!
Set the cubemap resolution used in the fisheye renderer

@param res resolution of the cubemap sides (should be a power of two for best performance)
*/
void sgct_core::SGCTSettings::setCubeMapResolution(int res)
{
	mCubeMapResolution = res;
}

/*!
Set the dome diameter used in the fisheye renderer (used for the viewplane distance calculations)

@param size size of the dome diameter (cube side) in meters
*/
void sgct_core::SGCTSettings::setDomeDiameter(float size)
{
	mCubeMapSize = size;
}

/*!
Set the fisheye/dome tilt angle used in the fisheye renderer.
The tilt angle is from the horizontal.

@param angle the tilt angle in degrees
*/
void sgct_core::SGCTSettings::setFisheyeTilt(float angle)
{
	mFisheyeTilt = angle;
}

/*!
Set the fisheye/dome field-of-view angle used in the fisheye renderer.

@param angle the FOV angle in degrees
*/
void sgct_core::SGCTSettings::setFisheyeFOV(float angle)
{
	mFieldOfView = angle;
}

/*!
Set the fisheye crop values. Theese values are used when rendering content for a single projector dome.
The elumenati geodome has usually a 4:3 SXGA+ (1400x1050) projector and the fisheye is cropped 25% (350 pixels) at the top.
*/
void sgct_core::SGCTSettings::setFisheyeCropValues(float left, float right, float bottom, float top)
{
	mCropFactors[ CropLeft ] = (left < 1.0f && left > 0.0f) ? left : 0.0f;
	mCropFactors[ CropRight ] = (right < 1.0f && right > 0.0f) ? right : 0.0f;
	mCropFactors[ CropBottom ] = (bottom < 1.0f && bottom > 0.0f) ? bottom : 0.0f;
	mCropFactors[ CropTop ] = (top < 1.0f && top > 0.0f) ? top : 0.0f;
}

/*!
	Set fisheye offset to render offaxis. Length of vector must be smaller then 1.
	Base of fisheye is the XY-plane.
*/
void sgct_core::SGCTSettings::setFisheyeOffset(float x, float y, float z)
{
	mFisheyeOffset[0] = x;
	mFisheyeOffset[1] = y;
	mFisheyeOffset[2] = z;

	if( x == 0.0f && y == 0.0f && z == 0.0f )
		mFisheyeOffaxis = false;
	else
		mFisheyeOffaxis = true;
}

/*!
Set the fisheye overlay image.
*/
void sgct_core::SGCTSettings::setFisheyeOverlay(std::string filename)
{
	mFisheyeOverlayFilename.assign(filename);
}

/*!
Set if FXAA should be used.
*/
void sgct_core::SGCTSettings::setFXAA(bool state)
{
	mUseFXAA = state;
	mUsePostFX = state;
	//sgct::MessageHandler::Instance()->print("FXAA status: %s\n", state ? "enabled" : "disabled");
}

/*!
Set if fisheye alpha state. Should only be set using XML config of before calling Engine::init.
*/
void sgct_core::SGCTSettings::setFisheyeAlpha(bool state)
{
	mFisheyeAlpha = state;
}

/*!
Set the FBO mode. This is done internally using SGCT config file.
*/
void sgct_core::SGCTSettings::setFBOMode(sgct_core::SGCTSettings::FBOMode mode)
{
	mFBOMode = mode;
}

/*!
Set the number of capture threads used by SGCT (multi-threaded screenshots)
*/
void sgct_core::SGCTSettings::setNumberOfCaptureThreads(int count)
{
	mNumberOfCaptureThreads = count;
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
		sgct::MessageHandler::Instance()->print("SGCTSettings: Empty screen capture path!\n");
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

//! Get the cubemap size in pixels used in the fisheye renderer
int sgct_core::SGCTSettings::getCubeMapResolution()
{
	return mCubeMapResolution;
}

//! Get the dome diameter in meters used in the fisheye renderer
float sgct_core::SGCTSettings::getDomeDiameter()
{
	return mCubeMapSize;
}

//! Get the fisheye/dome tilt angle in degrees
float sgct_core::SGCTSettings::getFisheyeTilt()
{
	return mFisheyeTilt;
}

//! Get the fisheye/dome field-of-view angle in degrees
float sgct_core::SGCTSettings::getFisheyeFOV()
{
	return mFieldOfView;
}

/*! Get the fisheye crop value for a side:
	- Left
	- Right
	- Bottom
	- Top
*/
float sgct_core::SGCTSettings::getFisheyeCropValue(CropSides side)
{
	return mCropFactors[side];
}

//! Get if fisheye is offaxis (not rendered from centre)
bool sgct_core::SGCTSettings::isFisheyeOffaxis()
{
	return mFisheyeOffaxis;
}

//! Get the offset (3-float vec) if fisheye is offaxis.
float sgct_core::SGCTSettings::getFisheyeOffset(unsigned int axis)
{
	return mFisheyeOffset[axis];
}

//! Get the fisheye overlay image filename/path.
const char * sgct_core::SGCTSettings::getFisheyeOverlay()
{
	return mFisheyeOverlayFilename.empty() ? NULL : mFisheyeOverlayFilename.c_str();
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
