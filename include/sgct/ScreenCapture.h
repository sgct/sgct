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
	//! The different capture enums used by the SaveScreenCapture function
	enum CaptureMode { FBO_Texture = 0, FBO_Left_Texture, FBO_Right_Texture, Front_Buffer, Left_Front_Buffer, Right_Front_Buffer };
	//! The different file formats supported
	enum CaptureFormat { NOT_SET = -1, PNG = 0, TGA };

	ScreenCapture();
	~ScreenCapture();

	void init(std::size_t windowIndex);
	void initOrResize(int x, int y, int channels=4);
	void setFormat(CaptureFormat cf);
	CaptureFormat getFormat();
	void SaveScreenCapture(unsigned int textureId, int frameNumber, CaptureMode cm = FBO_Texture);
	void setUsePBO(bool state);

private: 
	void addFrameNumberToFilename( int frameNumber, CaptureMode cm = FBO_Texture );
	int getAvailibleCaptureThread(); 

	tthread::mutex mMutex;
	ScreenCaptureThreadInfo * mSCTIPtrs;

	unsigned int mNumberOfThreads;
	unsigned int mPBO;
	int mDataSize;
	int mX;
	int mY;
	int mChannels;

	std::string mScreenShotFilename;
	bool mUsePBO;
	CaptureFormat mFormat;
	std::size_t mWindowIndex;
};

}

#endif