/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#define DEFAULT_NUMBER_OF_CAPTURE_THREADS 8

#include "../include/sgct/ScreenCapture.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/Engine.h"
#include <string>

void GLFWCALL screenCaptureHandler(void *arg);

sgct_core::ScreenCapture::ScreenCapture()
{
	mNumberOfThreads = DEFAULT_NUMBER_OF_CAPTURE_THREADS;
	init();
}

sgct_core::ScreenCapture::ScreenCapture( unsigned int numberOfThreads )
{
	mNumberOfThreads = (numberOfThreads > 0 ? numberOfThreads : 1);
	init();
}

sgct_core::ScreenCapture::~ScreenCapture()
{
	for(unsigned int i=0; i<mNumberOfThreads; i++)
	{
		if( mframeBufferImagePtrs[i] != NULL )
		{
			sgct::MessageHandler::Instance()->print("Clearing screen capture buffer %d...\n", i);

			//kill threads that are still running
			if( mFrameCaptureThreads[i] > 0 &&
				glfwWaitThread( mFrameCaptureThreads[i], GLFW_NOWAIT ) == GL_FALSE )
			{
				glfwDestroyThread( mFrameCaptureThreads[i] );
				mFrameCaptureThreads[i] = -1;
			}
		
			delete mframeBufferImagePtrs[i];
			mframeBufferImagePtrs[i] = NULL;
		}
	}

	if( mframeBufferImagePtrs != NULL )
	{
		delete [] mframeBufferImagePtrs;
		mframeBufferImagePtrs = NULL;
	}

	if( mFrameCaptureThreads != NULL )
	{
		delete [] mFrameCaptureThreads;
		mFrameCaptureThreads = NULL;
	}

	if( mFilename != NULL )
	{
		delete [] mFilename;
		mFilename = NULL;
	}
}

void sgct_core::ScreenCapture::initOrResizePBO(int x, int y)
{
	if(!GLEW_EXT_pixel_buffer_object)
	{
		sgct::MessageHandler::Instance()->print("Warning: pixel buffer objects are not supported by hardware!");
		return;
	}
	
	if( mPBO != 0 ) //delete if buffer exitsts
	{
		glDeleteBuffersARB(1, &mPBO);
		mPBO = 0;
	}
	
	mX = x;
	mY = y;
	mDataSize = mX * mY * mChannels;

	for(unsigned int i=0; i<mNumberOfThreads; i++)
	{
		if( mframeBufferImagePtrs[i] != NULL )
		{
			sgct::MessageHandler::Instance()->print("Clearing screen capture buffer %d...\n", i);

			//kill threads that are still running
			if( mFrameCaptureThreads[i] > 0 &&
				glfwWaitThread( mFrameCaptureThreads[i], GLFW_NOWAIT ) == GL_FALSE )
			{
				glfwDestroyThread( mFrameCaptureThreads[i] );
				mFrameCaptureThreads[i] = -1;
			}
		
			delete mframeBufferImagePtrs[i];
			mframeBufferImagePtrs[i] = NULL;
		}
	}

	glGenBuffers(1, &mPBO);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO);
	glBufferData(GL_PIXEL_PACK_BUFFER, mDataSize, 0, GL_STREAM_READ);
		
	//unbind
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void sgct_core::ScreenCapture::SaveScreenCapture(unsigned int textureId, int frameNumber, sgct_core::ScreenCapture::CaptureMode cm)
{
	bool error = false;

	addFrameNumberToFilename(frameNumber, cm);

	int threadIndex = getAvailibleCaptureThread();
	if( threadIndex == -1 )
	{
		sgct::MessageHandler::Instance()->print("Error in finding availible thread for screenshot/capture!\n");
		return;
	}
	
	Image ** imPtr = &mframeBufferImagePtrs[ threadIndex ];
	if( (*imPtr) == NULL )
	{
		(*imPtr) = new sgct_core::Image();
		(*imPtr)->setChannels( mChannels );
		(*imPtr)->setSize( mX, mY );
		(*imPtr)->allocateOrResizeData();
	}
	(*imPtr)->setFilename( mScreenShotFilename );
	
	glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
	glPixelStorei(GL_PACK_ALIGNMENT, 1); //byte alignment

	glFinish(); //wait for all rendering to finish

	if(GLEW_EXT_pixel_buffer_object)
		glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO); //bind pbo

	if(cm == FBO_Texture || cm == FBO_Left_Texture || cm == FBO_Right_Texture)
	{
		glEnable(GL_TEXTURE_2D);	
		glBindTexture(GL_TEXTURE_2D, textureId);

		GLEW_EXT_pixel_buffer_object ? 
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0) :
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (*imPtr)->getData());
	}
	else if(cm == Front_Buffer)
	{
		glReadBuffer(GL_FRONT);
		GLEW_EXT_pixel_buffer_object ?
			glReadPixels(0, 0, mX, mY, GL_RGBA, GL_UNSIGNED_BYTE, 0) :
			glReadPixels(0, 0, mX, mY, GL_RGBA, GL_UNSIGNED_BYTE, (*imPtr)->getData());
	}
	else if(cm == Left_Front_Buffer)
	{
		glReadBuffer(GL_FRONT_LEFT);
		GLEW_EXT_pixel_buffer_object ?
			glReadPixels(0, 0, mX, mY, GL_RGBA, GL_UNSIGNED_BYTE, 0) :
			glReadPixels(0, 0, mX, mY, GL_RGBA, GL_UNSIGNED_BYTE, (*imPtr)->getData());
	}
	else if(cm == Right_Front_Buffer)
	{
		glReadBuffer(GL_FRONT_RIGHT);
		GLEW_EXT_pixel_buffer_object ?
			glReadPixels(0, 0, mX, mY, GL_RGBA, GL_UNSIGNED_BYTE, 0) :
			glReadPixels(0, 0, mX, mY, GL_RGBA, GL_UNSIGNED_BYTE, (*imPtr)->getData());
	}

	if(GLEW_EXT_pixel_buffer_object)
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO); //map pbo
		GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		if(ptr)
		{
			memcpy( (*imPtr)->getData(), ptr, mDataSize );
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		}
		else
			sgct::MessageHandler::Instance()->print("Error: Can't map data (0) from GPU in frame capture!\n");
	
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); //un-bind pbo
	}
	
	if(sgct::Engine::checkForOGLErrors())
		mFrameCaptureThreads[ threadIndex ] = glfwCreateThread( screenCaptureHandler, (*imPtr) );
	else
		error = true;

	glPopAttrib();

	if(error)
		sgct::MessageHandler::Instance()->print("Error in taking screenshot/capture!\n");
}

void sgct_core::ScreenCapture::setFilename( const char * filename )
{
	if( filename == NULL || strlen(filename) < 1 ) //invalid filename
	{
		sgct::MessageHandler::Instance()->print("ScreenCapture: Invalid filename!\n");
		return;
	}
	
	if( mFilename != NULL )
	{
		delete [] mFilename;
		mFilename = NULL;
	}

	//copy filename
	mFilename = new char[strlen(filename)+1];
	#if (_MSC_VER >= 1400) //visual studio 2005 or later
    if( strcpy_s(mFilename, strlen(filename)+1, filename ) != 0)
		return;
    #else
		strcpy(mFilename, filename );
    #endif
}

void sgct_core::ScreenCapture::init()
{
	mPBO = 0;
	mDataSize = 0;
	mChannels = 4;
	
	mframeBufferImagePtrs = NULL;
	mFrameCaptureThreads = NULL;

	mFilename = new char [32];
	#if (_MSC_VER >= 1400) //visual studio 2005 or later
		strcpy_s(mFilename, 32, "SGCT_frame");
	#else
		strcpy(mFilename, "SGCT_frame" );
    #endif
	
	mframeBufferImagePtrs = new Image*[mNumberOfThreads];
	mFrameCaptureThreads = new int[mNumberOfThreads];
	
	for(unsigned int i=0; i<mNumberOfThreads; i++)
	{
		mframeBufferImagePtrs[i] = NULL;
		mFrameCaptureThreads[i] = -1;
	}
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
		break;

	case FBO_Left_Texture:
	case Left_Front_Buffer:
		eye.assign("_L_");
		break;

	case FBO_Right_Texture:
	case Right_Front_Buffer:
		eye.assign("_R_");
		break;
	}
	
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if( frameNumber < 10 )
		sprintf_s( mScreenShotFilename, FILENAME_BUFFER_LENGTH, "%s%s_00000%d.png", mFilename, eye.c_str(), frameNumber);
	else if( frameNumber < 100 )
		sprintf_s( mScreenShotFilename, FILENAME_BUFFER_LENGTH, "%s%s_0000%d.png", mFilename, eye.c_str(), frameNumber);
	else if( frameNumber < 1000 )
		sprintf_s( mScreenShotFilename, FILENAME_BUFFER_LENGTH, "%s%s_000%d.png", mFilename, eye.c_str(), frameNumber);
	else if( frameNumber < 10000 )
		sprintf_s( mScreenShotFilename, FILENAME_BUFFER_LENGTH, "%s%s_00%d.png", mFilename, eye.c_str(), frameNumber);
	else if( frameNumber < 100000 )
		sprintf_s( mScreenShotFilename, FILENAME_BUFFER_LENGTH, "%s%s_0%d.png", mFilename, eye.c_str(), frameNumber);
	else if( frameNumber < 1000000 )
		sprintf_s( mScreenShotFilename, FILENAME_BUFFER_LENGTH, "%s%s_%d.png", mFilename, eye.c_str(), frameNumber);
#else
    if( frameNumber < 10 )
		sprintf( mScreenShotFilename, "%s%s_00000%d.png", mFilename, eye.c_str(), frameNumber);
	else if( frameNumber < 100 )
		sprintf( mScreenShotFilename, "%s%s_0000%d.png", mFilename, eye.c_str(), frameNumber);
	else if( frameNumber < 1000 )
		sprintf( mScreenShotFilename, "%s%s_000%d.png", mFilename, eye.c_str(), frameNumber);
	else if( frameNumber < 10000 )
		sprintf( mScreenShotFilename, "%s%s_00%d.png", mFilename, eye.c_str(), frameNumber);
	else if( frameNumber < 100000 )
		sprintf( mScreenShotFilename, "%s%s_0%d.png", mFilename, eye.c_str(), frameNumber);
	else if( frameNumber < 1000000 )
		sprintf( mScreenShotFilename, "%s%s_%d.png", mFilename, eye.c_str(), frameNumber);
#endif
}

int sgct_core::ScreenCapture::getAvailibleCaptureThread()
{
	while( true )
	{
		for(unsigned int i=0; i<mNumberOfThreads; i++)
		{
			//check if thread is dead
			if( glfwWaitThread( mFrameCaptureThreads[i], GLFW_NOWAIT ) == GL_TRUE )
			{
				mFrameCaptureThreads[i] = -1;
				return i;
			}
		}

		glfwSleep( 0.01 );
	}

	return -1;
}

//multi-threaded screenshot saver
void GLFWCALL screenCaptureHandler(void *arg)
{
	sgct_core::Image * imPtr = reinterpret_cast<sgct_core::Image *>(arg);
	
	if( !imPtr->savePNG(1) )
		sgct::MessageHandler::Instance()->print("Error: Failed to save '%s'!\n", imPtr->getFilename());
}