/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_SETTINGS
#define _SGCT_SETTINGS

#include <stdio.h>

namespace sgct_core
{

/*!
	This singleton class will hold global SGCT settings.
*/
class SGCTSettings
{
public:
	enum CropSides { Left = 0, Right, Bottom, Top };

	/*! Get the SGCTSettings instance */
	static SGCTSettings * Instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new SGCTSettings();
		}

		return mInstance;
	}

	/*! Destroy the SGCTSettings instance */
	static void Destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

	void setCubeMapResolution(int res);
	void setCubeMapSize(float size);
	void setFisheyeTilt(float angle);
	void setFisheyeFOV(float angle);
	void setFisheyeCropValues(float left, float right, float bottom, float top);
	void setFisheyeOverlay(const char * filename);
	void setFXAA(bool state);
	
	int getCubeMapResolution();
	float getCubeMapSize();
	float getFisheyeTilt();
	float getFisheyeFOV();
	float getFisheyeCropValue(CropSides side);
	const char * getFisheyeOverlay();

	//! Set to true if FXAA should be used.
	inline bool useFXAA() { return mUseFXAA; }

private:
	SGCTSettings();
	~SGCTSettings();

	// Don't implement these, should give compile warning if used
	SGCTSettings( const SGCTSettings & settings );
	const SGCTSettings & operator=(const SGCTSettings & settings );

private:
	static SGCTSettings * mInstance;
	
	//Cubemap settings
	int mCubeMapResolution;
	float mCubeMapSize;

	//fisheye settings
	float mFisheyeTilt;
	float mFieldOfView;
	float mCropFactors[4];
	const char * mFisheyeOverlayFilename;

	//FXAA
	bool mUseFXAA;
};
}

#endif
