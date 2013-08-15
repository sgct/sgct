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

	std::string mCapturePath[3];
	int mCaptureFormat;
};
}

#endif
