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
	mFrameBuffer = 0;
	mMultiSampledFrameBuffer = 0;
	mRenderBuffer = 0;
	mDepthBuffer = 0;

	mWidth = 1;
	mHeight = 1;
	mMultiSampled = false;
}

void sgct_core::OffScreenBuffer::createFBO(int width, int height, int samples)
{
	glGenFramebuffers(1,		&mFrameBuffer);
	glGenRenderbuffers(1,		&mDepthBuffer);
	glGenRenderbuffers(1,		&mRenderBuffer);

	mWidth = width;
	mHeight = height;

	mMultiSampled = (samples > 1 && SGCTSettings::Instance()->useFBO());

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
	}

	//setup render buffer
	mMultiSampled ?
		glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffer) :
		glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, mRenderBuffer);

	//Set render buffer storage depending on if buffer is multisampled or not
	mMultiSampled ?
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA, width, height) :
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, mRenderBuffer);

	//setup depth buffer
	//needed for depth testing
	mMultiSampled ?
		glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffer) :
		glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);

	mMultiSampled ?
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT32, width, height ):
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height );
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer);

	//Does the GPU support current FBO configuration?
	if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || glGetError() != GL_NO_ERROR )
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: Something went wrong creating FBO!\n");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//sgct::MessageHandler::Instance()->print("FBO %d x %d (x %d) created!\n", width, height, samples);
}

void sgct_core::OffScreenBuffer::resizeFBO(int width, int height, int samples)
{
	mWidth = width;
	mHeight = height;

	mMultiSampled = ( samples > 1 && SGCTSettings::Instance()->useFBO() );
	
	//delete all
	glDeleteFramebuffers(1,	&mFrameBuffer);
	if(mMultiSampled)
		glDeleteFramebuffers(1,	&mMultiSampledFrameBuffer);
	glDeleteRenderbuffers(1,	&mRenderBuffer);
	glDeleteRenderbuffers(1,	&mDepthBuffer);
		
	//init
	mFrameBuffer = 0;
	mMultiSampledFrameBuffer = 0;
	mRenderBuffer = 0;
	mDepthBuffer = 0;

	createFBO(width, height, samples);
}

void sgct_core::OffScreenBuffer::bind()
{
	mMultiSampled ? 
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
	//blit color
	glBlitFramebuffer(
		0, 0, mWidth, mHeight,
		0, 0, mWidth, mHeight,
		GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void sgct_core::OffScreenBuffer::destroy()
{
	//delete all
	glDeleteFramebuffers(1,	&mFrameBuffer);
	if(mMultiSampled)
		glDeleteFramebuffers(1,	&mMultiSampledFrameBuffer);
	glDeleteRenderbuffers(1,	&mRenderBuffer);
	glDeleteRenderbuffers(1,	&mDepthBuffer);
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