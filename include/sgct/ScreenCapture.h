/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SCREEN_CAPTURE_H_
#define _SCREEN_CAPTURE_H_

#include "Image.h"
#include "helpers/SGCTCPPEleven.h"
#include <string>

#ifndef SGCT_DONT_USE_EXTERNAL
#include "external/tinythread.h"
#else
#include <tinythread.h>
#endif

namespace sgct_core
{

class ScreenCaptureThreadInfo
{
public:
	ScreenCaptureThreadInfo();
	sgct_core::Image * mframeBufferImagePtr;
	tthread::thread * mFrameCaptureThreadPtr;
	tthread::mutex * mMutexPtr;
	bool mRunning; //needed for test if running without join
};

/*!
	This class is used internally by SGCT and is called when using the takeScreenshot function from the Engine.
	Screenshots are saved as PNG or TGA images and and can also be used for movie recording.
*/
class ScreenCapture
{
public:
	//! The different file formats supported
	enum CaptureFormat { NOT_SET = -1, PNG = 0, TGA, JPEG };
    enum EyeIndex { MONO = 0, STEREO_LEFT, STEREO_RIGHT};

	ScreenCapture();
	~ScreenCapture();

	void init(std::size_t windowIndex, EyeIndex ei);
	void initOrResize(int x, int y, int channels, int bytesPerColor);
	void setTextureTransferProperties(unsigned int type, bool preferBGR);
	void setCaptureFormat(CaptureFormat cf);
	CaptureFormat getCaptureFormat();
	void saveScreenCapture(unsigned int textureId);
	void setPathAndFileName(std::string path, std::string filename);
	void setUsePBO(bool state);

#ifdef __LOAD_CPP11_FUN__
	void setCaptureCallback(sgct_cppxeleven::function<void(Image*, std::size_t, EyeIndex, unsigned int type)> callback);
	sgct_cppxeleven::function< void(Image *, std::size_t, EyeIndex, unsigned int type) > mCaptureCallbackFn;
#endif

private:
	void addFrameNumberToFilename( unsigned int frameNumber);
	int getAvailibleCaptureThread();
	void updateDownloadFormat();
	Image * prepareImage(int index);

	tthread::mutex mMutex;
	ScreenCaptureThreadInfo * mSCTIPtrs;

	unsigned int mNumberOfThreads;
	unsigned int mPBO;
	unsigned int mDownloadFormat;
	unsigned int mDownloadType;
	int mDataSize;
	int mX;
	int mY;
	int mChannels;
	int mBytesPerColor;

	std::string mFilename;
	std::string mBaseName;
	std::string mPath;
	bool mUsePBO;
	bool mPreferBGR;
	EyeIndex mEyeIndex;
	CaptureFormat mFormat;
	std::size_t mWindowIndex;
};

}

#endif