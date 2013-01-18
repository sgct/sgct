/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/SGCTSettings.h"
#include "../include/sgct/MessageHandler.h"

sgct_core::SGCTSettings * sgct_core::SGCTSettings::mInstance = NULL;

sgct_core::SGCTSettings::SGCTSettings()
{
	mCubeMapResolution = 256; //low
	mCubeMapSize = 10.0f;
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

	mUseFXAA = false;
	mUseDepthMap = false;
	mFBOMode = MultiSampledFBO;
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
	mCropFactors[ Left ] = (left < 1.0f && left > 0.0f) ? left : 0.0f;
	mCropFactors[ Right ] = (right < 1.0f && right > 0.0f) ? right : 0.0f;
	mCropFactors[ Bottom ] = (bottom < 1.0f && bottom > 0.0f) ? bottom : 0.0f;
	mCropFactors[ Top ] = (top < 1.0f && top > 0.0f) ? top : 0.0f;
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
	//sgct::MessageHandler::Instance()->print("FXAA status: %s\n", state ? "enabled" : "disabled");
}

/*!
Set if depth texture maps should be generated from FBO.
*/
void sgct_core::SGCTSettings::setDepthMapUsage(bool state)
{
	mUseDepthMap = state;
}

/*!
Set the FBO mode. This is done internally using SGCT config file.
*/
void sgct_core::SGCTSettings::setFBOMode(sgct_core::SGCTSettings::FBOMode mode)
{
	mFBOMode = mode;
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
const char *   sgct_core::SGCTSettings::getFisheyeOverlay()
{
	return mFisheyeOverlayFilename.empty() ? NULL : mFisheyeOverlayFilename.c_str();
}
