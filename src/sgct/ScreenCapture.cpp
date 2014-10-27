/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include "../include/sgct/ScreenCapture.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/Engine.h"
#include "../include/sgct/SGCTSettings.h"
#include <sstream>
#include <string>

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
	mCaptureCallbackFn = SGCT_NULL_PTR;
	
	mEyeIndex = MONO;
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

	mCaptureCallbackFn = SGCT_NULL_PTR;
	
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

	if( mPBO ) //delete if buffer exitsts
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
	if( mPBO ) //delete if buffer exitsts
	{
		glDeleteBuffers(1, &mPBO);
		mPBO = GL_FALSE;
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
		glGenBuffers(1, &mPBO);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "ScreenCapture: Generating %dx%dx%d PBO: %u\n", mX, mY, mChannels, mPBO);

		glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO);
        //glBufferData(GL_PIXEL_PACK_BUFFER, mDataSize, 0, GL_STREAM_READ); //work but might cause incomplete buffer images
        glBufferData(GL_PIXEL_PACK_BUFFER, mDataSize, 0, GL_STATIC_READ);

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

@param textureId textureId is the texture that will be streamed from the GPU if frame buffer objects are used in the rendering.
*/
void sgct_core::ScreenCapture::saveScreenCapture(unsigned int textureId)
{
	addFrameNumberToFilename(sgct::Engine::instance()->getScreenShotNumber());
    
	glPixelStorei(GL_PACK_ALIGNMENT, 1); //byte alignment

	if (mUsePBO)
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO);
		
		if (sgct::Engine::instance()->isOGLPipelineFixed())
		{
			glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
			glEnable(GL_TEXTURE_2D);
		}
            
        glBindTexture(GL_TEXTURE_2D, textureId);
        glGetTexImage(GL_TEXTURE_2D, 0, getColorType(), GL_UNSIGNED_BYTE, 0);
            
        if (sgct::Engine::instance()->isOGLPipelineFixed())
			glPopAttrib();
        
        int threadIndex = getAvailibleCaptureThread();
        Image * imPtr = prepareImage(threadIndex);
        if (!imPtr)
            return;
        
		GLubyte * ptr = reinterpret_cast<GLubyte*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));
        if (ptr)
        {
            //memcpy(imPtr->getData(), ptr, mDataSize);
			int stride = imPtr->getWidth() * imPtr->getChannels();
			for (int r = 0; r < imPtr->getHeight(); r++)
				memcpy(imPtr->getData()+stride*r, ptr+stride*r, stride);
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }
        else
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Error: Can't map data (0) from GPU in frame capture!\n");
        
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); //unbind pbo

		if (mCaptureCallbackFn != SGCT_NULL_PTR)
			mCaptureCallbackFn(imPtr, mWindowIndex, mEyeIndex);
		else
		{
			//save the image
			mSCTIPtrs[threadIndex].mRunning = true;
			mSCTIPtrs[threadIndex].mFrameCaptureThreadPtr = new tthread::thread(screenCaptureHandler, &mSCTIPtrs[threadIndex]);
		}
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
        
		if (mCaptureCallbackFn != SGCT_NULL_PTR)
			mCaptureCallbackFn(imPtr, mWindowIndex, mEyeIndex);
		else
		{
			//save the image
			mSCTIPtrs[threadIndex].mRunning = true;
			mSCTIPtrs[threadIndex].mFrameCaptureThreadPtr = new tthread::thread(screenCaptureHandler, &mSCTIPtrs[threadIndex]);
		}
	}
}

void sgct_core::ScreenCapture::setPathAndFileName(std::string path, std::string filename)
{
	mPath.assign(path);
	mBaseName.assign(filename);
}

void sgct_core::ScreenCapture::setUsePBO(bool state)
{
	mUsePBO = state;
    
    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "ScreenCapture: PBO rendering %s.\n", state ? "enabled" : "disabled");
}

/*!
	Init
*/
void sgct_core::ScreenCapture::init(std::size_t windowIndex, sgct_core::ScreenCapture::EyeIndex ei)
{
	mEyeIndex = ei;
	
	mSCTIPtrs = new ScreenCaptureThreadInfo[mNumberOfThreads];
	for( unsigned int i=0; i<mNumberOfThreads; i++ )
		mSCTIPtrs[i].mMutexPtr = &mMutex;
	mWindowIndex = windowIndex;

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Number of screen capture threads is set to %d\n", mNumberOfThreads);
}

void sgct_core::ScreenCapture::addFrameNumberToFilename( unsigned int frameNumber )
{
	std::string eye;
	
	//use default settings if path is empty
	bool useDefaultSettings = true;
	if (!mPath.empty())
		useDefaultSettings = false;

	std::string tmpPath;
	switch (mEyeIndex)
	{
	case MONO:
	default:
		eye.assign("");
		if (useDefaultSettings)
			tmpPath.assign(sgct::SGCTSettings::instance()->getCapturePath(sgct::SGCTSettings::Mono));
		break;

	case STEREO_LEFT:
		eye.assign("_L");
		if (useDefaultSettings)
			tmpPath.assign(sgct::SGCTSettings::instance()->getCapturePath(sgct::SGCTSettings::LeftStereo));
		break;

	case STEREO_RIGHT:
		eye.assign("_R");
		if (useDefaultSettings)
			tmpPath.assign(sgct::SGCTSettings::instance()->getCapturePath(sgct::SGCTSettings::RightStereo));
		break;
	}

	std::string suffix;
	if(mFormat == PNG)
		suffix.assign("png");
	else if(mFormat == TGA)
		suffix.assign("tga");
	else
		suffix.assign("jpg");

	std::stringstream ss;
	if (useDefaultSettings)
	{
		ss << tmpPath;
		sgct::SGCTWindow * win = sgct::Engine::instance()->getWindowPtr(mWindowIndex);
		
		if (win->getName().empty() )
			ss << "_win" << mWindowIndex;
		else
			ss << "_" << win->getName();
	}
	else
		ss << mPath << "/" << mBaseName;

	ss << eye;

	//add frame numbers
	if (frameNumber < 10)
		ss << "_00000" << frameNumber;
	else if( frameNumber < 100 )
		ss << "_0000" << frameNumber;
	else if( frameNumber < 1000 )
		ss << "_000" << frameNumber;
	else if( frameNumber < 10000 )
		ss << "_00" << frameNumber;
	else if( frameNumber < 100000 )
		ss << "_0" << frameNumber;
	else if( frameNumber < 1000000 )
		ss << "_" << frameNumber;

	//add suffix
	ss << "." << suffix;
	mFilename = ss.str();
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
		if (!(*imPtr)->allocateOrResizeData())
		{
			delete (*imPtr);
			return NULL;
		}
	}
	(*imPtr)->setFilename(mFilename);

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

/*!
Set the screen capture callback\n
Parameters are: image pointer to captured image, window index and eye index
*/
void sgct_core::ScreenCapture::setCaptureCallback(sgct_cppxeleven::function<void(sgct_core::Image*, std::size_t, sgct_core::ScreenCapture::EyeIndex)> callback)
{
	mCaptureCallbackFn = callback;
}
