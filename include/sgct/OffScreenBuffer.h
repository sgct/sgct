/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _OFF_SCREEN_BUFFER
#define _OFF_SCREEN_BUFFER

#include "ogl_headers.h"

namespace sgct_core
{

/*!
Helper class for creating frame buffer objects and render buffer objects.
*/
class OffScreenBuffer
{
public:
	OffScreenBuffer();
	void createFBO(int width, int height, int samples = 1);
	void resizeFBO(int width, int height, int samples = 1);
	void setInternalColorFormat(GLint internalFormat);
	void attachColorTexture(unsigned int texId, GLenum attachment = GL_COLOR_ATTACHMENT0);
	void attachDepthTexture(unsigned int texId);
	void attachCubeMapTexture(unsigned int texId, unsigned int face, GLenum attachment = GL_COLOR_ATTACHMENT0);
	void attachCubeMapDepthTexture(unsigned int texId, unsigned int face);
	void bind();
	void bind( GLsizei n, const GLenum *bufs );
	void bind( bool multisampled );
	void bind( bool multisampled, GLsizei n, const GLenum *bufs );
	void bindBlit();
	void blit();
	static void unBind();
	void destroy();
	inline bool isMultiSampled() { return mMultiSampled; }

	inline unsigned int getBufferID() { return mMultiSampled ? mMultiSampledFrameBuffer : mFrameBuffer; }
    
    const int & getInternalColorFormat() const;
    bool checkForErrors();

private:
	void setDrawBuffers();

	unsigned int mFrameBuffer;
	unsigned int mMultiSampledFrameBuffer;
	unsigned int mColorBuffer;
	unsigned int mNormalBuffer;
	unsigned int mPositionBuffer;
	unsigned int mDepthBuffer;
	int mInternalColorFormat;

	int mWidth;
	int mHeight;
	bool mMultiSampled;
};

}

#endif
