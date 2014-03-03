/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SCREEN_CAPTURE_H_
#define _SCREEN_CAPTURE_H_

#include "Image.h"
#include <string>
#include "external/tinythread.h"

#define NUMBER_OF_PBOS 2

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
	Screenshots are saved as PNG-files and and can also be used for movie recording.
*/
class ScreenCapture
{
public:
	//! The different file formats supported
	enum CaptureFormat { NOT_SET = -1, PNG = 0, TGA };

	ScreenCapture();
	~ScreenCapture();

	void init(std::size_t windowIndex, int type);
	void initOrResize(int x, int y, int channels=4);
	void setFormat(CaptureFormat cf);
	CaptureFormat getFormat();
	void SaveScreenCapture(unsigned int textureId, int frameNumber);
	void setUsePBO(bool state);
    void update();

private:
	void addFrameNumberToFilename( int frameNumber);
	int getAvailibleCaptureThread();
	unsigned int getColorType();
	Image * prepareImage(int index);

	tthread::mutex mMutex;
	ScreenCaptureThreadInfo * mSCTIPtrs;

	unsigned int mNumberOfThreads;
	unsigned int mPBO[NUMBER_OF_PBOS];
	int mDataSize;
	int mX;
	int mY;
	int mChannels;

	std::string mScreenShotFilename;
	bool mUsePBO;
	bool mSaveScreenShot[2];
    unsigned int mCurrentPBOIndex;
	int mType;
	CaptureFormat mFormat;
	std::size_t mWindowIndex;
};

}

#endif