/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SCREEN_CAPTURE_H_
#define _SCREEN_CAPTURE_H_

#include "Image.h"

#define FILENAME_BUFFER_LENGTH 256

namespace sgct_core
{

/*!
	This class is used internally by SGCT and is called when using the takeScreenshot function from the Engine.
	Screenshots are saved as PNG-files and and can also be used for movie recording.
*/
class ScreenCapture
{
public:
	//! The different capture enums used by the SaveScreenCapture function
	enum CaptureMode { FBO_Texture = 0, FBO_Left_Texture, FBO_Right_Texture, Front_Buffer, Left_Front_Buffer, Right_Front_Buffer };

	ScreenCapture();
	ScreenCapture( unsigned int numberOfThreads );
	~ScreenCapture();

	void initOrResizePBO(int x, int y);
	void SaveScreenCapture(unsigned int textureId, int frameNumber, CaptureMode cm = FBO_Texture);
	void setFilename( const char * filename );

private: 
	void init();
	void addFrameNumberToFilename( int frameNumber, CaptureMode cm = FBO_Texture );
	int getAvailibleCaptureThread(); 

	sgct_core::Image ** mframeBufferImagePtrs;
	int * mFrameCaptureThreads;
	unsigned int mNumberOfThreads;
	unsigned int mPBO;
	int mDataSize;
	int mX;
	int mY;
	int mChannels;

	char mScreenShotFilename[FILENAME_BUFFER_LENGTH];
	char * mFilename;
};

}

#endif