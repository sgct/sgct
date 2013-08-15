/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/OffScreenBuffer.h"
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/SGCTSettings.h"

sgct_core::OffScreenBuffer::OffScreenBuffer()
{
	mFrameBuffer = GL_FALSE;
	mMultiSampledFrameBuffer = GL_FALSE;
	mColorBuffer = GL_FALSE;
	mDepthBuffer = GL_FALSE;

	mWidth = 1;
	mHeight = 1;
	mMultiSampled = false;
}

void sgct_core::OffScreenBuffer::createFBO(int width, int height, int samples)
{
	glGenFramebuffers(1,	&mFrameBuffer);
	glGenRenderbuffers(1,	&mDepthBuffer);

	mWidth = width;
	mHeight = height;

	mMultiSampled = (samples > 1 && sgct::SGCTSettings::Instance()->useFBO());

	//create a multisampled buffer
	if(mMultiSampled)
	{
		GLint MaxSamples;
		glGetIntegerv(GL_MAX_SAMPLES, &MaxSamples);
		if( samples > MaxSamples )
			samples = MaxSamples;
		if( MaxSamples < 2 )
			samples = 0;

		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Max samples supported: %d\n", MaxSamples);

		//generate the multisample buffer
		glGenFramebuffers(1, &mMultiSampledFrameBuffer);

		//generate render buffer for intermediate storage
		glGenRenderbuffers(1,	&mColorBuffer);
	}

	//Bind FBO
	//Setup Render Buffers for multisample FBO
	if( mMultiSampled )
	{
		glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffer);
		
		glBindRenderbuffer(GL_RENDERBUFFER, mColorBuffer);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, width, height);
	}
	else
		glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);

	//Setup depth render buffer
	glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);
	mMultiSampled ?
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT32, width, height ):
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height );

	//It's time to attach the RBs to the FBO
	if( mMultiSampled )
	{
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, mColorBuffer);
	}
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer);

	//Does the GPU support current FBO configuration?
	if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || glGetError() != GL_NO_ERROR )
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: Something went wrong creating FBO!\n");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	mMultiSampled ?
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
			"OffScreenBuffer: Created buffers:\n\tFBO id=%d\n\tMultisample FBO id=%d\n\tRBO depth buffer id=%d\n\tRBO color buffer id=%d\n",
			mFrameBuffer, mMultiSampledFrameBuffer, mDepthBuffer, mColorBuffer) :
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
			"OffScreenBuffer: Created buffers:\n\tFBO id=%d\n\tRBO Depth buffer id=%d\n",
			mFrameBuffer, mDepthBuffer);

	//sgct::MessageHandler::Instance()->print("FBO %d x %d (x %d) created!\n", width, height, samples);
}

void sgct_core::OffScreenBuffer::resizeFBO(int width, int height, int samples)
{
	mWidth = width;
	mHeight = height;

	mMultiSampled = ( samples > 1 && sgct::SGCTSettings::Instance()->useFBO() );
	
	//delete all
	glDeleteFramebuffers(1,		&mFrameBuffer);
	glDeleteRenderbuffers(1,	&mDepthBuffer);
	if(mMultiSampled)
	{
		glDeleteFramebuffers(1,		&mMultiSampledFrameBuffer);
		glDeleteRenderbuffers(1,	&mColorBuffer);
	}
		
	//init
	mFrameBuffer = GL_FALSE;
	mMultiSampledFrameBuffer = GL_FALSE;
	mColorBuffer = GL_FALSE;
	mDepthBuffer = GL_FALSE;

	createFBO(width, height, samples);
}

void sgct_core::OffScreenBuffer::bind()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, GL_FALSE);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, GL_FALSE);
	
	mMultiSampled ? 
		glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffer) :
		glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer );
}

void sgct_core::OffScreenBuffer::bind( bool multisampled )
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, GL_FALSE);
	//glBindTexture(GL_TEXTURE_CUBE_MAP, GL_FALSE);
	
	multisampled ? 
		glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffer) :
		glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer );
}

void sgct_core::OffScreenBuffer::bindBlit()
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, mMultiSampledFrameBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFrameBuffer);
}

void sgct_core::OffScreenBuffer::unBind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0 );
}

void sgct_core::OffScreenBuffer::blit()
{
	/*
		use linear interpolation since src and dst size is equal
	*/
	glReadBuffer( GL_COLOR_ATTACHMENT0 );
	glDrawBuffer( GL_COLOR_ATTACHMENT0 );
	sgct::SGCTSettings::Instance()->useDepthTexture() ?
		glBlitFramebuffer(
			0, 0, mWidth, mHeight,
			0, 0, mWidth, mHeight,
			GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST) :
		glBlitFramebuffer(
			0, 0, mWidth, mHeight,
			0, 0, mWidth, mHeight,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void sgct_core::OffScreenBuffer::destroy()
{
	//delete all
	glDeleteFramebuffers(1,		&mFrameBuffer);
	glDeleteRenderbuffers(1,	&mDepthBuffer);
	if(mMultiSampled)
	{
		glDeleteFramebuffers(1,		&mMultiSampledFrameBuffer);
		glDeleteRenderbuffers(1,	&mColorBuffer);
	}
}

void sgct_core::OffScreenBuffer::attachColorTexture(unsigned int texId)
{
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
}

void sgct_core::OffScreenBuffer::attachDepthTexture(unsigned int texId)
{
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texId, 0);
}

void sgct_core::OffScreenBuffer::attachCubeMapTexture(unsigned int texId, unsigned int face)
{
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texId, 0);
}

void sgct_core::OffScreenBuffer::attachCubeMapDepthTexture(unsigned int texId, unsigned int face)
{
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texId, 0);
}