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
	mType = sgct::SGCTSettings::Mono;
	mNumberOfThreads = sgct::SGCTSettings::instance()->getNumberOfCaptureThreads();
	mCurrentPBOIndex = 0;
    
	for (int i = 0; i < NUMBER_OF_PBOS; i++)
		mPBO[i] = GL_FALSE;
		
	mSaveScreenShot[0] = false;
	mSaveScreenShot[1] = false;
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

			#ifdef __SGCT_MUTEX_DEBUG__
				fprintf(stderr, "Locking mutex for screencapture...\n");
			#endif
			mMutex.lock();
			if( mSCTIPtrs[i].mframeBufferImagePtr != NULL )
			{
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "\tBuffer %d...\n", i);
				delete mSCTIPtrs[i].mframeBufferImagePtr;
				mSCTIPtrs[i].mframeBufferImagePtr = NULL;
			}

			mSCTIPtrs[i].mRunning = false;
			mMutex.unlock();
			#ifdef __SGCT_MUTEX_DEBUG__
				fprintf(stderr, "Mutex for screencapture is unlocked.\n");
			#endif
		}

		delete [] mSCTIPtrs;
		mSCTIPtrs = NULL;
	}

	if( mPBO[0] ) //delete if buffer exitsts
	{
		glDeleteBuffers(NUMBER_OF_PBOS, &mPBO[0]);
		for (int i = 0; i < NUMBER_OF_PBOS; i++)
			mPBO[i] = GL_FALSE;
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
	if( mPBO[0] ) //delete if buffer exitsts
	{
		glDeleteBuffers(NUMBER_OF_PBOS, &mPBO[0]);
		for (int i = 0; i < NUMBER_OF_PBOS; i++)
			mPBO[i] = GL_FALSE;
	}

	mX = x;
	mY = y;
	
	mChannels = channels;
	mDataSize = mX * mY * mChannels;

	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for screencapture...\n");
	#endif
	mMutex.lock();
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
		glGenBuffers(NUMBER_OF_PBOS, &mPBO[0]);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "ScreenCapture: Generating %dx%dx%d PBOs: %d - %d\n", mX, mY, mChannels, mPBO[0], mPBO[NUMBER_OF_PBOS-1]);

		for (int i = 0; i < NUMBER_OF_PBOS; i++)
		{
			glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO[i]);
			//glBufferData(GL_PIXEL_PACK_BUFFER, mDataSize, 0, GL_STREAM_READ); //work but might cause incomplete buffer images
			glBufferData(GL_PIXEL_PACK_BUFFER, mDataSize, 0, GL_STATIC_READ);
		}

		//unbind
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	}

	mMutex.unlock();
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for screencapture is unlocked.\n");
	#endif
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
void sgct_core::ScreenCapture::SaveScreenCapture(unsigned int textureId, int frameNumber)
{
	addFrameNumberToFilename(frameNumber);
    
	glPixelStorei(GL_PACK_ALIGNMENT, 1); //byte alignment

	if (mUsePBO)
	{
		fprintf(stderr, "Taking shot %u, buffer %d\n", sgct::Engine::instance()->getCurrentFrameNumber(), mCurrentPBOIndex);
		
		glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO[mCurrentPBOIndex]);
		
		if (sgct::Engine::instance()->isOGLPipelineFixed())
		{
			glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
			glEnable(GL_TEXTURE_2D);
		}
            
        glBindTexture(GL_TEXTURE_2D, textureId);
        glGetTexImage(GL_TEXTURE_2D, 0, getColorType(), GL_UNSIGNED_BYTE, 0);
            
        if (sgct::Engine::instance()->isOGLPipelineFixed())
			glPopAttrib();
		
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); //unbind pbo

		mSaveScreenShot[0] = true;

		//ping-pong
		mCurrentPBOIndex = 1 - mCurrentPBOIndex;
	}
	else //no PBO
	{
		int threadIndex = getAvailibleCaptureThread();
		Image * imPtr = prepareImage(threadIndex);
		if (!imPtr)
			return;
		
		if (sgct::Engine::instance()->isOGLPipelineFixed())
		{
			glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
			glEnable(GL_TEXTURE_2D);
		}
			
		glBindTexture(GL_TEXTURE_2D, textureId);
		glGetTexImage(GL_TEXTURE_2D, 0, getColorType(), GL_UNSIGNED_BYTE, imPtr->getData());
            
        if (sgct::Engine::instance()->isOGLPipelineFixed())
			glPopAttrib();
        
        //save the image
        mSCTIPtrs[threadIndex].mRunning = true;
        mSCTIPtrs[threadIndex].mFrameCaptureThreadPtr = new tthread::thread(screenCaptureHandler, &mSCTIPtrs[threadIndex]);
	}
}

/*!
 Swap pbo buffers if needed and save image.
 */
void sgct_core::ScreenCapture::update()
{
	if (mSaveScreenShot[1])
	{
		fprintf(stderr, "saving shot %u, buffer %d\n", sgct::Engine::instance()->getCurrentFrameNumber(), mCurrentPBOIndex);
		mSaveScreenShot[mCurrentPBOIndex] = false;

		int threadIndex = getAvailibleCaptureThread();
		Image * imPtr = prepareImage(threadIndex);
		if (!imPtr)
			return;

		glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO[mCurrentPBOIndex]); //bind pbo

		GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		if (ptr)
		{
			memcpy(imPtr->getData(), ptr, mDataSize);
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		}
		else
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Error: Can't map data (0) from GPU in frame capture!\n");

		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); //un-bind pbo

		//save the image
		mSCTIPtrs[threadIndex].mRunning = true;
		mSCTIPtrs[threadIndex].mFrameCaptureThreadPtr = new tthread::thread(screenCaptureHandler, &mSCTIPtrs[threadIndex]);
	}

	//swap
	mSaveScreenShot[1] = mSaveScreenShot[0];
	mSaveScreenShot[0] = false;
}

void sgct_core::ScreenCapture::setUsePBO(bool state)
{
	mUsePBO = state;
    
    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "ScreenCapture: PBO rendering %s.\n", state ? "enabled" : "disabled");
}

/*!
	Init
*/
void sgct_core::ScreenCapture::init(std::size_t windowIndex, int type)
{
	mType = type;
	
	mSCTIPtrs = new ScreenCaptureThreadInfo[mNumberOfThreads];
	for( unsigned int i=0; i<mNumberOfThreads; i++ )
		mSCTIPtrs[i].mMutexPtr = &mMutex;
	mWindowIndex = windowIndex;

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Number of screen capture threads is set to %d\n", mNumberOfThreads);
}

void sgct_core::ScreenCapture::addFrameNumberToFilename( int frameNumber )
{
	std::string eye;
	switch (mType)
	{
	case sgct::SGCTSettings::Mono:
	default:
		eye.assign("");
		mScreenShotFilename.assign( sgct::SGCTSettings::instance()->getCapturePath( sgct::SGCTSettings::Mono ) );
		break;

	case sgct::SGCTSettings::LeftStereo:
		eye.assign("_L");
		mScreenShotFilename.assign( sgct::SGCTSettings::instance()->getCapturePath( sgct::SGCTSettings::LeftStereo ) );
		break;

	case sgct::SGCTSettings::RightStereo:
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
	char window_node_info_str[32];
	window_node_info_str[0] = '\0';

//get window information
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if (sgct::Engine::instance()->getNumberOfWindows() > 1)
		sprintf_s( window_node_info_str, 32, "_win%Iu", mWindowIndex );
#else
	#ifdef __WIN32__
    if (sgct::Engine::instance()->getNumberOfWindows() > 1)
		sprintf( window_node_info_str, "_win%u", mWindowIndex );
	#else //linux & mac
    if (sgct::Engine::instance()->getNumberOfWindows() > 1)
		sprintf( window_node_info_str, "_win%zu", mWindowIndex );
	#endif
#endif

#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if( frameNumber < 10 )
		sprintf_s(tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "%s%s_00000%d.%s", window_node_info_str, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 100 )
		sprintf_s(tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "%s%s_0000%d.%s", window_node_info_str, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 1000 )
		sprintf_s(tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "%s%s_000%d.%s", window_node_info_str, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 10000 )
		sprintf_s(tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "%s%s_00%d.%s", window_node_info_str, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 100000 )
		sprintf_s(tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "%s%s_0%d.%s", window_node_info_str, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 1000000 )
		sprintf_s(tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "%s%s_%d.%s", window_node_info_str, eye.c_str(), frameNumber, suffix.c_str());
#else
   if( frameNumber < 10 )
		sprintf( tmpStr, "%s%s_00000%d.%s", window_node_info_str, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 100 )
		sprintf( tmpStr, "%s%s_0000%d.%s", window_node_info_str, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 1000 )
		sprintf( tmpStr, "%s%s_000%d.%s", window_node_info_str, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 10000 )
		sprintf( tmpStr, "%s%s_00%d.%s", window_node_info_str, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 100000 )
		sprintf( tmpStr, "%s%s_0%d.%s", window_node_info_str, eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 1000000 )
		sprintf( tmpStr, "%s%s_%d.%s", window_node_info_str, eye.c_str(), frameNumber, suffix.c_str());
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
				#ifdef __SGCT_MUTEX_DEBUG__
					fprintf(stderr, "Locking mutex for screencapture...\n");
				#endif
				mMutex.lock();
					running = mSCTIPtrs[i].mRunning;
				mMutex.unlock();
				#ifdef __SGCT_MUTEX_DEBUG__
					fprintf(stderr, "Mutex for screencapture is unlocked.\n");
				#endif

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

unsigned int sgct_core::ScreenCapture::getColorType()
{
	unsigned int colorType;

	switch (mChannels)
	{
	default:
		colorType = GL_BGRA;
		break;

	case 1:
		colorType = (sgct::Engine::instance()->isOGLPipelineFixed() ? GL_LUMINANCE : GL_RED);
		break;

	case 2:
		colorType = (sgct::Engine::instance()->isOGLPipelineFixed() ? GL_LUMINANCE_ALPHA : GL_RG);
		break;

	case 3:
		colorType = GL_BGR;
		break;
	}

	return colorType;
}

sgct_core::Image * sgct_core::ScreenCapture::prepareImage(int index)
{
	if (index == -1)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Error in finding availible thread for screenshot/capture!\n");
		return NULL;
	}
	else
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Starting thread for screenshot/capture [%d]\n", index);

	Image ** imPtr = &mSCTIPtrs[index].mframeBufferImagePtr;
	if ((*imPtr) == NULL)
	{
		(*imPtr) = new sgct_core::Image();
		(*imPtr)->setChannels(mChannels);
		(*imPtr)->setSize(mX, mY);

		if (!(*imPtr)->allocateOrResizeData()) //if allocation fails
		{
			//wait and try again
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Warning: Failed to allocate image memory! Trying again...\n");
			tthread::this_thread::sleep_for(tthread::chrono::milliseconds(100));
			if (!(*imPtr)->allocateOrResizeData())
			{
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Error: Failed to allocate image memory for image '%s'!\n", mScreenShotFilename.c_str());
				return NULL;
			}
		}
	}
	(*imPtr)->setFilename(mScreenShotFilename.c_str());

	return (*imPtr);
}

//multi-threaded screenshot saver
void screenCaptureHandler(void *arg)
{
	sgct_core::ScreenCaptureThreadInfo * ptr = reinterpret_cast<sgct_core::ScreenCaptureThreadInfo *>(arg);

	if( !ptr->mframeBufferImagePtr->save() )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Error: Failed to save '%s'!\n", ptr->mframeBufferImagePtr->getFilename());
	}

	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for screencapture...\n");
	#endif
	ptr->mMutexPtr->lock();
	ptr->mRunning = false;
	ptr->mMutexPtr->unlock();
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for screencapture is unlocked.\n");
	#endif
}
