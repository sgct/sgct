/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_SETTINGS
#define _SGCT_SETTINGS

#include <stdio.h>
#include <string>

namespace sgct_core
{

/*!
	This singleton class will hold global SGCT settings.
*/
class SGCTSettings
{
public:
	enum CropSides { Left = 0, Right, Bottom, Top };
	enum FBOMode { NoFBO = 0, RegularFBO, MultiSampledFBO, CubeMapFBO };

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
	void setDomeDiameter(float size);
	void setFisheyeTilt(float angle);
	void setFisheyeFOV(float angle);
	void setFisheyeCropValues(float left, float right, float bottom, float top);
	void setFisheyeOffset(float x, float y, float z = 0.0f);
	void setFisheyeOverlay(std::string filename);
	void setFXAA(bool state);
	void setDepthMapUsage(bool state);
	void setFBOMode(FBOMode mode);
	void setNumberOfCaptureThreads(int count);
	
	int getCubeMapResolution();
	float getDomeDiameter();
	float getFisheyeTilt();
	float getFisheyeFOV();
	float getFisheyeCropValue(CropSides side);
	bool isFisheyeOffaxis();
	float getFisheyeOffset(unsigned int axis);
	const char *  getFisheyeOverlay();

	//! Set to true if FXAA should be used.
	inline bool useFXAA() { return mUseFXAA; }
	//! Set to true if SGCT should generate depth maps from FBO
	inline bool useDepthMap() { return mUseDepthMap; }
	//! Returns the FBO mode.
	inline FBOMode getFBOMode() { return mFBOMode; }
	//! Get the number of capture threads (for screenshot recording)
	inline int getNumberOfCaptureThreads() { return mNumberOfCaptureThreads; }

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
	float mFisheyeOffset[3];
	bool mFisheyeOffaxis;
	float mCropFactors[4];
	int mNumberOfCaptureThreads;
	std::string mFisheyeOverlayFilename;

	//FXAA
	bool mUseFXAA;

	//FBO settings
	bool mUseDepthMap;
	FBOMode mFBOMode;
};
}

#endif
