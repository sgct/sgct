/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
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
	enum CropSides { CropLeft = 0, CropRight, CropBottom, CropTop };
	enum FBOMode { NoFBO = 0, RegularFBO, MultiSampledFBO };
	enum CapturePathIndexes { Mono = 0, LeftStereo, RightStereo };

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
	void setFisheyeAlpha(bool state);
	void setFXAA(bool state);
	void setFBOMode(FBOMode mode);
	void setNumberOfCaptureThreads(int count);
	void setPNGCompressionLevel(int level);
	void setCapturePath(std::string path, CapturePathIndexes cpi = Mono);
	void appendCapturePath(std::string str, CapturePathIndexes cpi = Mono);
	void setCaptureFormat(const char * format);
	
	int getCubeMapResolution();
	float getDomeDiameter();
	float getFisheyeTilt();
	float getFisheyeFOV();
	float getFisheyeCropValue(CropSides side);
	bool isFisheyeOffaxis();
	float getFisheyeOffset(unsigned int axis);
	const char * getFisheyeOverlay();
	const char * getCapturePath(CapturePathIndexes cpi = Mono);
	int getCaptureFormat();

	//! Set to true if alpha should be used in fisheye rendering
	inline bool useFisheyeAlpha() { return mFisheyeAlpha; }
	//! Set to true if FXAA should be used.
	inline bool useFXAA() { return mUseFXAA; }
	//! Set to true if PostFX pass should be used
	inline bool usePostFX() { return mUsePostFX; }
	//! Returns the FBO mode.
	inline FBOMode getFBOMode() { return mFBOMode; }
	//! Get the number of capture threads (for screenshot recording)
	inline int getNumberOfCaptureThreads() { return mNumberOfCaptureThreads; }
	//! Get the zlib compression level if png files used for saving screenshots
	inline int getPNGCompressionLevel() { return mPNGCompressionLevel; }

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
	bool mFisheyeAlpha;
	float mCropFactors[4];
	int mNumberOfCaptureThreads;
	int mPNGCompressionLevel;
	std::string mFisheyeOverlayFilename;

	//FXAA
	bool mUseFXAA;
	bool mUsePostFX;

	//FBO settings
	FBOMode mFBOMode;

	std::string mCapturePath[3];
	int mCaptureFormat;
};
}

#endif
