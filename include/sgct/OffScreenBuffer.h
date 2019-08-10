/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__OFF_SCREEN_BUFFER__H__
#define __SGCT__OFF_SCREEN_BUFFER__H__

#include "ogl_headers.h"

namespace sgct_core {

/*!
Helper class for creating frame buffer objects and render buffer objects.
*/
class OffScreenBuffer
{
public:
    void createFBO(int width, int height, int samples = 1);
    void resizeFBO(int width, int height, int samples = 1);
    void setInternalColorFormat(GLint internalFormat);
    void attachColorTexture(unsigned int texId, GLenum attachment = GL_COLOR_ATTACHMENT0);
    void attachDepthTexture(unsigned int texId);
    void attachCubeMapTexture(unsigned int texId, unsigned int face,
        GLenum attachment = GL_COLOR_ATTACHMENT0);
    void attachCubeMapDepthTexture(unsigned int texId, unsigned int face);
    void bind();
    void bind(GLsizei n, const GLenum* bufs);
    void bind(bool multisampled );
    void bind(bool multisampled, GLsizei n, const GLenum* bufs);
    void bindBlit();
    void blit();
    static void unBind();
    void destroy();
    bool isMultiSampled() const;

    unsigned int getBufferID() const;
    
    int getInternalColorFormat() const;
    bool checkForErrors();

private:
    void setDrawBuffers();

    unsigned int mFrameBuffer = 0;
    unsigned int mMultiSampledFrameBuffer = 0;
    unsigned int mColorBuffer = 0;
    unsigned int mNormalBuffer = 0;
    unsigned int mPositionBuffer = 0;
    unsigned int mDepthBuffer = 0;
    int mInternalColorFormat = GL_RGBA8;

    int mWidth = 1;
    int mHeight = 1;
    bool mMultiSampled = false;
};

} // namespace sgct_core

#endif // __SGCT__OFF_SCREEN_BUFFER__H__
