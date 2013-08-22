/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_SETTINGS
#define _SGCT_SETTINGS

#include <stdio.h>
#include <string>

namespace sgct
{

/*!
	This singleton class will hold global SGCT settings.
*/
class SGCTSettings
{
public:
	enum CapturePathIndexes { Mono = 0, LeftStereo, RightStereo };

	/*! Get the SGCTSettings instance */
	static SGCTSettings * instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new SGCTSettings();
		}

		return mInstance;
	}

	/*! Destroy the SGCTSettings instance */
	static void destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

	void setUseDepthTexture(bool state);
	void setUseFBO(bool state);
	void setNumberOfCaptureThreads(int count);
	void setPNGCompressionLevel(int level);
	void setCapturePath(std::string path, CapturePathIndexes cpi = Mono);
	void appendCapturePath(std::string str, CapturePathIndexes cpi = Mono);
	void setCaptureFormat(const char * format);
	
	const char * getCapturePath(CapturePathIndexes cpi = Mono);
	int getCaptureFormat();
	
	//! Return true if depth buffer is rendered to texture
	inline bool useDepthTexture() { return mUseDepthTexture; }
	//! Returns true if FBOs are used
	inline bool useFBO() { return mUseFBO; }
	//! Get the number of capture threads (for screenshot recording)
	inline int getNumberOfCaptureThreads() { return mNumberOfCaptureThreads; }
	//! Get the zlib compression level if png files used for saving screenshots
	inline int getPNGCompressionLevel() { return mPNGCompressionLevel; }
	//! The relative On-Screen-Display text x-offset in range [0, 1]
	inline float getOSDTextXOffset() { return mOSDTextOffset[0]; }
	//! The relative On-Screen-Display text y-offset in range [0, 1]
	inline float getOSDTextYOffset() { return mOSDTextOffset[1]; }

	//--------------------------------------------------------------------------//
	//FXAA functions
		
	/*! \returns the FXAA removal of sub-pixel aliasing */
	inline float getFXAASubPixTrim() { return mFXAASubPixTrim; }
	/*! \returns the FXAA sub-pixel offset */
	inline float getFXAASubPixOffset() { return mFXAASubPixOffset; }

	void setFXAASubPixTrim(float val);
	void setFXAASubPixOffset(float val);

private:
	SGCTSettings();
	~SGCTSettings();

	// Don't implement these, should give compile warning if used
	SGCTSettings( const SGCTSettings & settings );
	const SGCTSettings & operator=(const SGCTSettings & settings );

private:
	static SGCTSettings * mInstance;

	int mNumberOfCaptureThreads;
	int mPNGCompressionLevel;
	bool mUseDepthTexture;
	bool mUseFBO;

	float mOSDTextOffset[2];

	//FXAA parameters
	float mFXAASubPixTrim;
	float mFXAASubPixOffset;

	std::string mCapturePath[3];
	int mCaptureFormat;
};
}

#endif
