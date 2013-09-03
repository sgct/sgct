/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include "../include/sgct/ScreenCapture.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/Engine.h"
#include "../include/sgct/SGCTSettings.h"
#include <string>

#define FILENAME_APPEND_BUFFER_LENGTH 256

void screenCaptureHandler(void *arg);

sgct_core::ScreenCaptureThreadInfo::ScreenCaptureThreadInfo()
{
	mRunning = false;
	mframeBufferImagePtr = NULL;
	mFrameCaptureThreadPtr = NULL;
	mMutexPtr = NULL;
}

sgct_core::ScreenCapture::ScreenCapture()
{
	mNumberOfThreads = sgct::SGCTSettings::instance()->getNumberOfCaptureThreads();
	mPBO = GL_FALSE;
	mDataSize = 0;
	mWindowIndex = 0;
	mUsePBO = true;
	mFormat = PNG;

	mSCTIPtrs = NULL;
}

sgct_core::ScreenCapture::~ScreenCapture()
{
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Clearing screen capture buffers...\n");

	if( mSCTIPtrs != NULL )
	{
		for(unsigned int i=0; i<mNumberOfThreads; i++)
		{
			//kill threads that are still running
			if( mSCTIPtrs[i].mFrameCaptureThreadPtr != NULL )
			{
				mSCTIPtrs[i].mFrameCaptureThreadPtr->join();
				delete mSCTIPtrs[i].mFrameCaptureThreadPtr;
				mSCTIPtrs[i].mFrameCaptureThreadPtr = NULL;
			}

			tthread::lock_guard<tthread::mutex> lock(mMutex);
			if( mSCTIPtrs[i].mframeBufferImagePtr != NULL )
			{
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "\tBuffer %d...\n", i);
				delete mSCTIPtrs[i].mframeBufferImagePtr;
				mSCTIPtrs[i].mframeBufferImagePtr = NULL;
			}

			mSCTIPtrs[i].mRunning = false;
		}

		delete [] mSCTIPtrs;
		mSCTIPtrs = NULL;
	}

	if( mUsePBO && mPBO != GL_FALSE ) //delete if buffer exitsts
	{
		glDeleteBuffers(1, &mPBO);
		mPBO = GL_FALSE;
	}
}

/*!
	Inits the pixel buffer object (PBO) or re-sizes it if the frame buffer size have changed.

	\param x the horizontal pixel resolution of the frame buffer
	\param y the vertical pixel resolution of the frame buffer
	\param channels the number of color channels

	If PBOs are not supported nothing will and the screenshot process will fall back on slower GPU data fetching.
*/
void sgct_core::ScreenCapture::initOrResize(int x, int y, int channels)
{
	if( mUsePBO && mPBO != GL_FALSE ) //delete if buffer exitsts
	{
		glDeleteBuffers(1, &mPBO);
		mPBO = GL_FALSE;
	}

	mX = x;
	mY = y;
	mChannels = channels;
	mDataSize = mX * mY * mChannels;

	tthread::lock_guard<tthread::mutex> lock(mMutex);
	for(unsigned int i=0; i<mNumberOfThreads; i++)
	{
		if( mSCTIPtrs[i].mframeBufferImagePtr != NULL )
		{
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Clearing screen capture buffer %d...\n", i);

			//kill threads that are still running
			if( mSCTIPtrs[i].mFrameCaptureThreadPtr != NULL )
			{
				mSCTIPtrs[i].mFrameCaptureThreadPtr->join();
				delete mSCTIPtrs[i].mFrameCaptureThreadPtr;
				mSCTIPtrs[i].mFrameCaptureThreadPtr = NULL;
			}

			delete mSCTIPtrs[i].mframeBufferImagePtr;
			mSCTIPtrs[i].mframeBufferImagePtr = NULL;
		}

		mSCTIPtrs[i].mRunning = false;
	}

	if( mUsePBO )
	{
		glGenBuffers(1, &mPBO);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "ScreenCapture: Generating PBO: %d\n", mPBO);

		glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO);
		//glBufferData(GL_PIXEL_PACK_BUFFER, mDataSize, 0, GL_STREAM_READ); //work but might cause incomplete buffer images
		glBufferData(GL_PIXEL_PACK_BUFFER, mDataSize, 0, GL_STATIC_READ);

		//unbind
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	}
}

/*!
Set the image format to use
*/
void sgct_core::ScreenCapture::setFormat(CaptureFormat cf)
{
	mFormat = cf;
}

/*!
Get the image format
*/
sgct_core::ScreenCapture::CaptureFormat sgct_core::ScreenCapture::getFormat()
{
	return mFormat;
}

/*!
This function saves the images to disc.

@param textureId textureId is the texture that will be streamed from the GPU if frame buffer objects are used in the rendering. If normal front buffer is used then this parameter has no effect.
@param frameNumber frameNumber is the index that will be added to the filename
@param cm cm is the capture mode used and can be one of the following:
	1. FBO_Texture
	2. FBO_Left_Texture
	3. FBO_Right_Texture
	4. Front_Buffer
	5. Left_Front_Buffer
	6. Right_Front_Buffer
*/
void sgct_core::ScreenCapture::SaveScreenCapture(unsigned int textureId, int frameNumber, sgct_core::ScreenCapture::CaptureMode cm)
{
	bool error = false;

	addFrameNumberToFilename(frameNumber, cm);

	int threadIndex = getAvailibleCaptureThread();
	if( threadIndex == -1 )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Error in finding availible thread for screenshot/capture!\n");
		return;
	}
	else
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Starting thread for screenshot/capture [%d]\n", threadIndex);

	Image ** imPtr = &mSCTIPtrs[ threadIndex ].mframeBufferImagePtr;
	if( (*imPtr) == NULL )
	{
		(*imPtr) = new sgct_core::Image();
		(*imPtr)->setChannels( mChannels );
		(*imPtr)->setSize( mX, mY );

		if( !(*imPtr)->allocateOrResizeData() ) //if allocation fails
		{
			//wait and try again
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Warning: Failed to allocate image memory! Trying again...\n");
			tthread::this_thread::sleep_for(tthread::chrono::milliseconds(100));
			if( !(*imPtr)->allocateOrResizeData() )
			{
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Error: Failed to allocate image memory for image '%s'!\n", mScreenShotFilename.c_str() );
				return;
			}
		}
	}
	(*imPtr)->setFilename( mScreenShotFilename.c_str() );

	if( sgct::Engine::instance()->isOGLPipelineFixed() )
		glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
	glPixelStorei(GL_PACK_ALIGNMENT, 1); //byte alignment

	GLenum colorType;
	switch(mChannels)
	{
	default:
		colorType = (mFormat == TGA ? GL_BGRA : GL_RGBA);
		break;

	case 1:
		colorType = (sgct::Engine::instance()->isOGLPipelineFixed() ? GL_LUMINANCE : GL_RED);
		break;

	case 2:
		colorType = (sgct::Engine::instance()->isOGLPipelineFixed() ? GL_LUMINANCE_ALPHA : GL_RG);
		break;

	case 3:
		colorType = (mFormat == TGA ? GL_BGR : GL_RGB);
		break;
	}

	if(mUsePBO)
		glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO); //bind pbo

	if(cm == FBO_Texture || cm == FBO_Left_Texture || cm == FBO_Right_Texture)
	{
		if( sgct::Engine::instance()->isOGLPipelineFixed() )
			glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textureId);

		mUsePBO ?
			glGetTexImage(GL_TEXTURE_2D, 0, colorType, GL_UNSIGNED_BYTE, 0) :
			glGetTexImage(GL_TEXTURE_2D, 0, colorType, GL_UNSIGNED_BYTE, (*imPtr)->getData());
	}
	else if(cm == Front_Buffer)
	{
		glReadBuffer(GL_FRONT);
		mUsePBO ?
			glReadPixels(0, 0, mX, mY, colorType, GL_UNSIGNED_BYTE, 0) :
			glReadPixels(0, 0, mX, mY, colorType, GL_UNSIGNED_BYTE, (*imPtr)->getData());
	}
	else if(cm == Left_Front_Buffer)
	{
		glReadBuffer(GL_FRONT_LEFT);
		mUsePBO ?
			glReadPixels(0, 0, mX, mY, colorType, GL_UNSIGNED_BYTE, 0) :
			glReadPixels(0, 0, mX, mY, colorType, GL_UNSIGNED_BYTE, (*imPtr)->getData());
	}
	else if(cm == Right_Front_Buffer)
	{
		glReadBuffer(GL_FRONT_RIGHT);
		mUsePBO ?
			glReadPixels(0, 0, mX, mY, colorType, GL_UNSIGNED_BYTE, 0) :
			glReadPixels(0, 0, mX, mY, colorType, GL_UNSIGNED_BYTE, (*imPtr)->getData());
	}

	if(mUsePBO)
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO); //map pbo
		GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		if(ptr)
		{
			memcpy( (*imPtr)->getData(), ptr, mDataSize );
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		}
		else
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Error: Can't map data (0) from GPU in frame capture!\n");

		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); //un-bind pbo
	}

	if(sgct::Engine::checkForOGLErrors())
	{
		mSCTIPtrs[ threadIndex ].mRunning = true;
		mSCTIPtrs[ threadIndex ].mFrameCaptureThreadPtr = new tthread::thread( screenCaptureHandler, &mSCTIPtrs[ threadIndex ] );
	}
	else
		error = true;

	if( sgct::Engine::instance()->isOGLPipelineFixed() )
		glPopAttrib();

	if(error)
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Error in taking screenshot/capture!\n");
}

void sgct_core::ScreenCapture::setUsePBO(bool state)
{
	mUsePBO = state;
}

/*!
	Init
*/
void sgct_core::ScreenCapture::init(std::size_t windowIndex)
{
	mSCTIPtrs = new ScreenCaptureThreadInfo[mNumberOfThreads];
	for( unsigned int i=0; i<mNumberOfThreads; i++ )
		mSCTIPtrs[i].mMutexPtr = &mMutex;
	mWindowIndex = windowIndex;

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Number of screen capture threads is set to %d\n", mNumberOfThreads);
}

void sgct_core::ScreenCapture::addFrameNumberToFilename( int frameNumber, sgct_core::ScreenCapture::CaptureMode cm)
{
	std::string eye;
	switch( cm )
	{
	case FBO_Texture:
	case Front_Buffer:
	default:
		eye.assign("");
		mScreenShotFilename.assign( sgct::SGCTSettings::instance()->getCapturePath( sgct::SGCTSettings::Mono ) );
		break;

	case FBO_Left_Texture:
	case Left_Front_Buffer:
		eye.assign("_L");
		mScreenShotFilename.assign( sgct::SGCTSettings::instance()->getCapturePath( sgct::SGCTSettings::LeftStereo ) );
		break;

	case FBO_Right_Texture:
	case Right_Front_Buffer:
		eye.assign("_R");
		mScreenShotFilename.assign( sgct::SGCTSettings::instance()->getCapturePath( sgct::SGCTSettings::RightStereo ) );
		break;
	}

	std::string suffix;
	if(mFormat == PNG)
		suffix.assign("png");
	else/* if(mFormat == TGA)*/
		suffix.assign("tga");

	char tmpStr[FILENAME_APPEND_BUFFER_LENGTH];

#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if( frameNumber < 10 )
		sprintf_s( tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "_win%u%s_00000%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 100 )
		sprintf_s( tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "_win%u%s_0000%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 1000 )
		sprintf_s( tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "_win%u%s_000%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 10000 )
		sprintf_s( tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "_win%u%s_00%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 100000 )
		sprintf_s( tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "_win%u%s_0%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 1000000 )
		sprintf_s( tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "_win%u%s_%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
#else
    #ifdef __WIN32__
    if( frameNumber < 10 )
		sprintf( tmpStr, "_win%u%s_00000%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 100 )
		sprintf( tmpStr, "_win%u%s_0000%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 1000 )
		sprintf( tmpStr, "_win%u%s_000%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 10000 )
		sprintf( tmpStr, "_win%u%s_00%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 100000 )
		sprintf( tmpStr, "_win%u%s_0%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 1000000 )
		sprintf( tmpStr, "_win%u%s_%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
    #else //linux & mac
    if( frameNumber < 10 )
		sprintf( tmpStr, "_win%zu%s_00000%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 100 )
		sprintf( tmpStr, "_win%zu%s_0000%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 1000 )
		sprintf( tmpStr, "_win%zu%s_000%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 10000 )
		sprintf( tmpStr, "_win%zu%s_00%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 100000 )
		sprintf( tmpStr, "_win%zu%s_0%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 1000000 )
		sprintf( tmpStr, "_win%zu%s_%d.%s", mWindowIndex, eye.c_str(), frameNumber, suffix.c_str());
    #endif
#endif

	mScreenShotFilename.append( std::string(tmpStr) );
}

int sgct_core::ScreenCapture::getAvailibleCaptureThread()
{
	while( true )
	{
		for(unsigned int i=0; i<mNumberOfThreads; i++)
		{
			//check if thread is dead
			if( mSCTIPtrs[i].mFrameCaptureThreadPtr == NULL )
			{
				return i;
			}
			else
			{
				bool running;
				mMutex.lock();
					running = mSCTIPtrs[i].mRunning;
				mMutex.unlock();

				if( !running )
				{
					mSCTIPtrs[i].mFrameCaptureThreadPtr->join();
					delete mSCTIPtrs[i].mFrameCaptureThreadPtr;
					mSCTIPtrs[i].mFrameCaptureThreadPtr = NULL;

					return i;
				}
			}
		}

		tthread::this_thread::sleep_for(tthread::chrono::milliseconds(10));
	}

	return -1;
}

//multi-threaded screenshot saver
void screenCaptureHandler(void *arg)
{
	sgct_core::ScreenCaptureThreadInfo * ptr = reinterpret_cast<sgct_core::ScreenCaptureThreadInfo *>(arg);

	if( !ptr->mframeBufferImagePtr->save() )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Error: Failed to save '%s'!\n", ptr->mframeBufferImagePtr->getFilename());
	}

	tthread::lock_guard<tthread::mutex> lock( *(ptr->mMutexPtr) );
	ptr->mRunning = false;
}
