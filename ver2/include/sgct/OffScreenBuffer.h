/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _OFF_SCREEN_BUFFER
#define _OFF_SCREEN_BUFFER

namespace sgct_core
{

class OffScreenBuffer
{
public:
	OffScreenBuffer();
	void createFBO(int width, int height, int samples = 1);
	void resizeFBO(int width, int height, int samples = 1);
	void attachColorTexture(unsigned int texId);
	void attachDepthTexture(unsigned int texId);
	void attachCubeMapTexture(unsigned int texId, unsigned int face);
	void bind();
	void bindBlit();
	void blit();
	static void unBind();
	void destroy();
	inline bool isMultiSampled() { return mMultiSampled; }

	inline unsigned int getBufferID() { return mMultiSampled ? mMultiSampledFrameBuffer : mFrameBuffer; }

private:
	unsigned int mFrameBuffer;
	unsigned int mMultiSampledFrameBuffer;
	unsigned int mRenderBuffer;
	unsigned int mDepthBuffer;

	int mWidth;
	int mHeight;
	bool mMultiSampled;
};

}

#endif