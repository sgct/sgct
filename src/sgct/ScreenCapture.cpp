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

#define FILENAME_APPEND_BUFFER_LENGTH 128

void GLFWCALL screenCaptureHandler(void *arg);

sgct_core::ScreenCapture::ScreenCapture()
{
	mNumberOfThreads = SGCTSettings::Instance()->getNumberOfCaptureThreads();
	mPBO = 0;
	mDataSize = 0;
	mUsePBO = true;
	mFormat = PNG;
	
	mframeBufferImagePtrs = NULL;
	mFrameCaptureThreads = NULL;
}

sgct_core::ScreenCapture::~ScreenCapture()
{
	if( mframeBufferImagePtrs != NULL )
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

		delete [] mframeBufferImagePtrs;
		mframeBufferImagePtrs = NULL;
	}

	if( mFrameCaptureThreads != NULL )
	{
		delete [] mFrameCaptureThreads;
		mFrameCaptureThreads = NULL;
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
	if( mUsePBO && mPBO != 0 ) //delete if buffer exitsts
	{
		glDeleteBuffersARB(1, &mPBO);
		mPBO = 0;
	}
	
	mX = x;
	mY = y;
	mChannels = channels;
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

	if( mUsePBO )
	{
		glGenBuffers(1, &mPBO);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO);
		glBufferData(GL_PIXEL_PACK_BUFFER, mDataSize, 0, GL_STREAM_READ);
		
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
		sgct::MessageHandler::Instance()->print("Error in finding availible thread for screenshot/capture!\n");
		return;
	}
	
	Image ** imPtr = &mframeBufferImagePtrs[ threadIndex ];
	if( (*imPtr) == NULL )
	{
		(*imPtr) = new sgct_core::Image();
		(*imPtr)->setChannels( mChannels );
		(*imPtr)->setSize( mX, mY );
		
		if( !(*imPtr)->allocateOrResizeData() ) //if allocation fails
		{
			//wait and try again
			sgct::MessageHandler::Instance()->print("Warning: Failed to allocate image memory! Trying again...\n");
			glfwSleep(0.1);
			if( !(*imPtr)->allocateOrResizeData() ) 
			{
				sgct::MessageHandler::Instance()->print("Error: Failed to allocate image memory for image '%s'!\n", mScreenShotFilename.c_str() );
				return;
			}
		}
	}
	(*imPtr)->setFilename( mScreenShotFilename.c_str() );
	
	glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
	glPixelStorei(GL_PACK_ALIGNMENT, 1); //byte alignment

	GLenum colorType;
	switch(mChannels)
	{
	default:
		colorType = (mFormat == TGA ? GL_BGRA : GL_RGBA);
		break;

	case 1:
		colorType = GL_LUMINANCE;
		break;

	case 2:
		colorType = GL_LUMINANCE_ALPHA;
		break;

	case 3:
		colorType = (mFormat == TGA ? GL_BGR : GL_RGB);
		break;
	}

	if(mUsePBO)
		glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO); //bind pbo

	if(cm == FBO_Texture || cm == FBO_Left_Texture || cm == FBO_Right_Texture)
	{
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

void sgct_core::ScreenCapture::setUsePBO(bool state)
{
	mUsePBO = state;
}

/*!
	Init
*/
void sgct_core::ScreenCapture::init()
{
	mframeBufferImagePtrs = new Image*[mNumberOfThreads];
	mFrameCaptureThreads = new int[mNumberOfThreads];
	
	for(unsigned int i=0; i<mNumberOfThreads; i++)
	{
		mframeBufferImagePtrs[i] = NULL;
		mFrameCaptureThreads[i] = -1;
	}

	sgct::MessageHandler::Instance()->print("Number of screen capture threads is set to %d\n", mNumberOfThreads);
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
		mScreenShotFilename.assign( SGCTSettings::Instance()->getCapturePath( SGCTSettings::Mono ) );
		break;

	case FBO_Left_Texture:
	case Left_Front_Buffer:
		eye.assign("_L");
		mScreenShotFilename.assign( SGCTSettings::Instance()->getCapturePath( SGCTSettings::LeftStereo ) );
		break;

	case FBO_Right_Texture:
	case Right_Front_Buffer:
		eye.assign("_R");
		mScreenShotFilename.assign( SGCTSettings::Instance()->getCapturePath( SGCTSettings::RightStereo ) );
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
		sprintf_s( tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "%s_00000%d.%s", eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 100 )
		sprintf_s( tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "%s_0000%d.%s", eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 1000 )
		sprintf_s( tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "%s_000%d.%s", eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 10000 )
		sprintf_s( tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "%s_00%d.%s", eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 100000 )
		sprintf_s( tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "%s_0%d.%s", eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 1000000 )
		sprintf_s( tmpStr, FILENAME_APPEND_BUFFER_LENGTH, "%s_%d.%s", eye.c_str(), frameNumber, suffix.c_str());
#else
    if( frameNumber < 10 )
		sprintf( tmpStr, "%s_00000%d.%s", eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 100 )
		sprintf( tmpStr, "%s_0000%d.%s", eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 1000 )
		sprintf( tmpStr, "%s_000%d.%s", eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 10000 )
		sprintf( tmpStr, "%s_00%d.%s", eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 100000 )
		sprintf( tmpStr, "%s_0%d.%s", eye.c_str(), frameNumber, suffix.c_str());
	else if( frameNumber < 1000000 )
		sprintf( tmpStr, "%s_%d.%s", eye.c_str(), frameNumber, suffix.c_str());
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
	
	if( !imPtr->save() )
	{
		sgct::MessageHandler::Instance()->print("Error: Failed to save '%s'!\n", imPtr->getFilename());
	}
}