/*************************************************************************
Copyright (c) 2012 Miroslav Andel, Linköping University.
All rights reserved.

Original Authors:
Miroslav Andel, Alexander Fridlund

For any questions or information about the SGCT project please contact: miroslav.andel@liu.se

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to
Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
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

	mFisheyeOverlayFilename = NULL;
	mUseFXAA = false;
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
Set the cubemap size used in the fisheye renderer (used for the viewplane distance calculations)

@param size size of the cubemap sides in meters
*/
void sgct_core::SGCTSettings::setCubeMapSize(float size)
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
Set the fisheye overlay image.
*/
void sgct_core::SGCTSettings::setFisheyeOverlay(const char * filename)
{
	mFisheyeOverlayFilename = filename;
}

/*!
Set if FXAA should be used.
*/
void sgct_core::SGCTSettings::setFXAA(bool state)
{
	mUseFXAA = state;
	sgct::MessageHandler::Instance()->print("FXAA status: %s\n", state ? "enabled" : "disabled");
}

//! Get the cubemap size in pixels used in the fisheye renderer
int sgct_core::SGCTSettings::getCubeMapResolution()
{
	return mCubeMapResolution;
}

//! Get the cubemap size in meters used in the fisheye renderer
float sgct_core::SGCTSettings::getCubeMapSize()
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

//! Get the fisheye overlay image filename/path.
const char * sgct_core::SGCTSettings::getFisheyeOverlay()
{
	return mFisheyeOverlayFilename;
}
