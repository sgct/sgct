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

	// ----------- set functions ---------------- //
	void setSwapInterval(int val);
	void setRefreshRateHint(int freq);
	void setUseDepthTexture(bool state);
	void setUseFBO(bool state);
	void setNumberOfCaptureThreads(int count);
	void setPNGCompressionLevel(int level);
	void setCapturePath(std::string path, CapturePathIndexes cpi = Mono);
	void appendCapturePath(std::string str, CapturePathIndexes cpi = Mono);
	void setCaptureFormat(const char * format);
	void setFXAASubPixTrim(float val);
	void setFXAASubPixOffset(float val);
	void setOSDTextXOffset(float val);
	void setOSDTextYOffset(float val);
	void setOSDTextFontSize( int size );
	void setOSDTextFontName( std::string name );
	void setOSDTextFontPath( std::string path );
	
	// ----------- get functions ---------------- //
	const char *		getCapturePath(CapturePathIndexes cpi = Mono);
	int					getCaptureFormat();
	int					getSwapInterval();
	int					getRefreshRateHint();
	const int &			getOSDTextFontSize();
	const std::string &	getOSDTextFontName();
	const std::string &	getOSDTextFontPath();

	// ----------- inline functions ---------------- //
	//! Return true if depth buffer is rendered to texture
	inline bool		useDepthTexture() { return mUseDepthTexture; }
	//! Returns true if FBOs are used
	inline bool		useFBO() { return mUseFBO; }
	//! Get the number of capture threads (for screenshot recording)
	inline int		getNumberOfCaptureThreads() { return mNumberOfCaptureThreads; }
	//! Get the zlib compression level if png files used for saving screenshots
	inline int		getPNGCompressionLevel() { return mPNGCompressionLevel; }
	//! The relative On-Screen-Display text x-offset in range [0, 1]
	inline float	getOSDTextXOffset() { return mOSDTextOffset[0]; }
	//! The relative On-Screen-Display text y-offset in range [0, 1]
	inline float	getOSDTextYOffset() { return mOSDTextOffset[1]; }
	/*! \returns the FXAA removal of sub-pixel aliasing */
	inline float	getFXAASubPixTrim() { return mFXAASubPixTrim; }
	/*! \returns the FXAA sub-pixel offset */
	inline float	getFXAASubPixOffset() { return mFXAASubPixOffset; }

private:
	SGCTSettings();
	~SGCTSettings();

	// Don't implement these, should give compile warning if used
	SGCTSettings( const SGCTSettings & settings );
	const SGCTSettings & operator=(const SGCTSettings & settings );

private:
	static SGCTSettings * mInstance;

	int mCaptureFormat;
	int mSwapInterval;
	int mRefreshRate;
	int mNumberOfCaptureThreads;
	int mPNGCompressionLevel;
	
	bool mUseDepthTexture;
	bool mUseFBO;

	float mOSDTextOffset[2];
	float mFXAASubPixTrim;
	float mFXAASubPixOffset;

	std::string mCapturePath[3];

	//fontdata
	std::string mFontName;
	std::string mFontPath;
	int mFontSize;
};
}

#endif
