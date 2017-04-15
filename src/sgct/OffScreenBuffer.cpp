/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/OffScreenBuffer.h>
#include <sgct/MessageHandler.h>
#include <sgct/SGCTSettings.h>

#ifndef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
    #define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS 0x8CD9
#endif


sgct_core::OffScreenBuffer::OffScreenBuffer()
{
    mFrameBuffer = GL_FALSE;
    mMultiSampledFrameBuffer = GL_FALSE;
    mColorBuffer = GL_FALSE;
    mNormalBuffer = GL_FALSE;
    mPositionBuffer = GL_FALSE;
    mDepthBuffer = GL_FALSE;
    mInternalColorFormat = GL_RGBA8;

    mWidth = 1;
    mHeight = 1;
    mMultiSampled = false;
}

void sgct_core::OffScreenBuffer::createFBO(int width, int height, int samples)
{
    glGenFramebuffers(1,    &mFrameBuffer);
    glGenRenderbuffers(1,    &mDepthBuffer);

    mWidth = width;
    mHeight = height;

    mMultiSampled = (samples > 1 && sgct::SGCTSettings::instance()->useFBO());

    //create a multisampled buffer
    if(mMultiSampled)
    {
        GLint MaxSamples;
        glGetIntegerv(GL_MAX_SAMPLES, &MaxSamples);
        if( samples > MaxSamples )
            samples = MaxSamples;
        if( MaxSamples < 2 )
            samples = 0;

        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Max samples supported: %d\n", MaxSamples);

        //generate the multisample buffer
        glGenFramebuffers(1, &mMultiSampledFrameBuffer);

        //generate render buffer for intermediate diffuse color storage
        glGenRenderbuffers(1,    &mColorBuffer);

        //generate render buffer for intermediate normal storage
        if (sgct::SGCTSettings::instance()->useNormalTexture())
            glGenRenderbuffers(1, &mNormalBuffer);

        //generate render buffer for intermediate position storage
        if (sgct::SGCTSettings::instance()->usePositionTexture())
            glGenRenderbuffers(1, &mPositionBuffer);
        
        //Bind FBO
        //Setup Render Buffers for multisample FBO
        glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffer);

        glBindRenderbuffer(GL_RENDERBUFFER, mColorBuffer);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, mInternalColorFormat, width, height);

        if (sgct::SGCTSettings::instance()->useNormalTexture())
        {
            glBindRenderbuffer(GL_RENDERBUFFER, mNormalBuffer);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, sgct::SGCTSettings::instance()->getBufferFloatPrecisionAsGLint(), width, height);
        }

        if (sgct::SGCTSettings::instance()->usePositionTexture())
        {
            glBindRenderbuffer(GL_RENDERBUFFER, mPositionBuffer);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, sgct::SGCTSettings::instance()->getBufferFloatPrecisionAsGLint(), width, height);
        }
    }
    else
        glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);

    //Setup depth render buffer
    glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);
    mMultiSampled ?
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT32, width, height ):
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height );

    //It's time to attach the RBs to the FBO
    if (mMultiSampled)
    {
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, mColorBuffer);
        if (sgct::SGCTSettings::instance()->useNormalTexture())
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_RENDERBUFFER, mNormalBuffer);
        if (sgct::SGCTSettings::instance()->usePositionTexture())
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_RENDERBUFFER, mPositionBuffer);
    }

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer);

    //unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    mMultiSampled ?
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
            "OffScreenBuffer: Created %dx%d buffers:\n\tFBO id=%d\n\tMultisample FBO id=%d\n\tRBO depth buffer id=%d\n\tRBO color buffer id=%d\n",
            width, height, mFrameBuffer, mMultiSampledFrameBuffer, mDepthBuffer, mColorBuffer) :
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
            "OffScreenBuffer: Created %dx%d buffers:\n\tFBO id=%d\n\tRBO Depth buffer id=%d\n",
            width, height, mFrameBuffer, mDepthBuffer);

    //sgct::MessageHandler::instance()->print("FBO %d x %d (x %d) created!\n", width, height, samples);
}

void sgct_core::OffScreenBuffer::resizeFBO(int width, int height, int samples)
{
    mWidth = width;
    mHeight = height;

    mMultiSampled = ( samples > 1 && sgct::SGCTSettings::instance()->useFBO() );

    //delete all
    glDeleteFramebuffers(1,        &mFrameBuffer);
    glDeleteRenderbuffers(1,    &mDepthBuffer);
    if(mMultiSampled)
    {
        glDeleteFramebuffers(1,        &mMultiSampledFrameBuffer);
        glDeleteRenderbuffers(1,    &mColorBuffer);

        if (sgct::SGCTSettings::instance()->useNormalTexture())
            glDeleteRenderbuffers(1, &mNormalBuffer);
        if (sgct::SGCTSettings::instance()->usePositionTexture())
            glDeleteRenderbuffers(1, &mPositionBuffer);
    }

    //init
    mFrameBuffer = GL_FALSE;
    mMultiSampledFrameBuffer = GL_FALSE;
    mColorBuffer = GL_FALSE;
    mDepthBuffer = GL_FALSE;
    mNormalBuffer = GL_FALSE;
    mPositionBuffer = GL_FALSE;

    createFBO(width, height, samples);
}

void sgct_core::OffScreenBuffer::setInternalColorFormat(GLint internalFormat)
{
    mInternalColorFormat = internalFormat;
}

void sgct_core::OffScreenBuffer::setDrawBuffers()
{
    switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType())
    {
    case sgct::SGCTSettings::Diffuse:
    default:
        {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, buffers);
        }
        break;

    case sgct::SGCTSettings::Diffuse_Normal:
        {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(2, buffers);
        }
        break;

    case sgct::SGCTSettings::Diffuse_Position:
        {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT2 };
            glDrawBuffers(2, buffers);
        }
        break;

    case sgct::SGCTSettings::Diffuse_Normal_Position:
        {
            GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
            glDrawBuffers(3, buffers);
        }
        break;
    }
}

/*!
Bind framebuffer, auto-set multisampling and draw buffers
*/
void sgct_core::OffScreenBuffer::bind()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, GL_FALSE);
    //glBindTexture(GL_TEXTURE_CUBE_MAP, GL_FALSE);

    mMultiSampled ?
        glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffer) :
        glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer );

    setDrawBuffers();
}

/*!
Bind framebuffer, auto-set multisampling
@param n number of color buffers
@param bufs array with color buffers (GL_COLOR_ATTACHMENTn)
*/
void sgct_core::OffScreenBuffer::bind(GLsizei n, const GLenum *bufs)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, GL_FALSE);
    //glBindTexture(GL_TEXTURE_CUBE_MAP, GL_FALSE);

    mMultiSampled ?
        glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffer) :
        glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);

    glDrawBuffers(n, bufs);
}

/*!
Bind framebuffer, auto-set draw buffers
@param multisampled is true if MSAA should be used
*/
void sgct_core::OffScreenBuffer::bind( bool multisampled )
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, GL_FALSE);
    //glBindTexture(GL_TEXTURE_CUBE_MAP, GL_FALSE);

    multisampled ?
        glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffer) :
        glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer );

    setDrawBuffers();
}

/*!
Bind framebuffer
@param multisampled is true if MSAA should be used
@param n number of color buffers
@param bufs array with color buffers (GL_COLOR_ATTACHMENTn)
*/
void sgct_core::OffScreenBuffer::bind(bool multisampled, GLsizei n, const GLenum *bufs)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, GL_FALSE);
    //glBindTexture(GL_TEXTURE_CUBE_MAP, GL_FALSE);

    multisampled ?
        glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffer) :
        glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);

    glDrawBuffers(n, bufs);
}

void sgct_core::OffScreenBuffer::bindBlit()
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mMultiSampledFrameBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFrameBuffer);

    setDrawBuffers();
}

void sgct_core::OffScreenBuffer::unBind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0 );
}

void sgct_core::OffScreenBuffer::blit()
{
    /*
        use no interpolation since src and dst size is equal
    */
    glReadBuffer( GL_COLOR_ATTACHMENT0 );
    glDrawBuffer( GL_COLOR_ATTACHMENT0 );
    sgct::SGCTSettings::instance()->useDepthTexture() ?
        glBlitFramebuffer(
            0, 0, mWidth, mHeight,
            0, 0, mWidth, mHeight,
            GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST) :
        glBlitFramebuffer(
            0, 0, mWidth, mHeight,
            0, 0, mWidth, mHeight,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);

    if (sgct::SGCTSettings::instance()->useNormalTexture())
    {
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glDrawBuffer(GL_COLOR_ATTACHMENT1);

        glBlitFramebuffer(
            0, 0, mWidth, mHeight,
            0, 0, mWidth, mHeight,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    if (sgct::SGCTSettings::instance()->usePositionTexture())
    {
        glReadBuffer(GL_COLOR_ATTACHMENT2);
        glDrawBuffer(GL_COLOR_ATTACHMENT2);

        glBlitFramebuffer(
            0, 0, mWidth, mHeight,
            0, 0, mWidth, mHeight,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
}

void sgct_core::OffScreenBuffer::destroy()
{
    //delete all
    if( mFrameBuffer )
        glDeleteFramebuffers(1,        &mFrameBuffer);
    
    if( mDepthBuffer )
        glDeleteRenderbuffers(1,    &mDepthBuffer);
    
    if(mMultiSampled)
    {
        if( mMultiSampledFrameBuffer )
            glDeleteFramebuffers(1,        &mMultiSampledFrameBuffer);
        
        if( mColorBuffer )
            glDeleteRenderbuffers(1,    &mColorBuffer);

        if (sgct::SGCTSettings::instance()->useNormalTexture() && mNormalBuffer)
            glDeleteRenderbuffers(1, &mNormalBuffer);
        if (sgct::SGCTSettings::instance()->usePositionTexture() && mPositionBuffer)
            glDeleteRenderbuffers(1, &mPositionBuffer);
    }

    mFrameBuffer = GL_FALSE;
    mMultiSampledFrameBuffer = GL_FALSE;
    mColorBuffer = GL_FALSE;
    mDepthBuffer = GL_FALSE;
    mNormalBuffer = GL_FALSE;
    mPositionBuffer = GL_FALSE;
}

/*!
@param texId GL id of the texture to attach
@param attachment the gl attachment enum in the form of GL_COLOR_ATTACHMENTi
*/
void sgct_core::OffScreenBuffer::attachColorTexture(unsigned int texId, GLenum attachment)
{
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texId, 0);
}

void sgct_core::OffScreenBuffer::attachDepthTexture(unsigned int texId)
{
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texId, 0);
}

/*!
@param texId GL id of the texture to attach
@param face the target cubemap face
@param attachment the gl attachment enum in the form of GL_COLOR_ATTACHMENTi
*/
void sgct_core::OffScreenBuffer::attachCubeMapTexture(unsigned int texId, unsigned int face, GLenum attachment)
{
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texId, 0);
}

void sgct_core::OffScreenBuffer::attachCubeMapDepthTexture(unsigned int texId, unsigned int face)
{
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texId, 0);
}

/*!
@returns the opengl internal texture format of the color buffer 
*/
const int & sgct_core::OffScreenBuffer::getInternalColorFormat() const
{
    return mInternalColorFormat;
}

/*!
@returns true if no errors
*/
bool sgct_core::OffScreenBuffer::checkForErrors()
{
    //Does the GPU support current FBO configuration?
    GLenum FBOStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    GLenum GLStatus = glGetError();
    if( FBOStatus != GL_FRAMEBUFFER_COMPLETE || GLStatus != GL_NO_ERROR )
    {
        switch( FBOStatus )
        {
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: FBO has incomplete attachments!\n");
            break;

        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: FBO has missmatching dimensions!\n");
            break;

        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: FBO has no attachments!\n");
            break;

        case GL_FRAMEBUFFER_UNSUPPORTED:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: Unsupported FBO format!\n");
            break;

        case GL_FRAMEBUFFER_UNDEFINED:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: Undefined FBO!\n");
            break;

        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: FBO has incomplete draw buffer!\n");
            break;

        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: FBO has incomplete read buffer!\n");
            break;

        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: FBO has missmatching multisample values!\n");
            break;

        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: FBO has incomplete layer targets!\n");
            break;

        case GL_FRAMEBUFFER_COMPLETE: //no error
            break;

        default: //No error
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: Unknown FBO error: 0x%X!\n", FBOStatus);
            break;

        }

        switch( GLStatus )
        {
        case GL_INVALID_ENUM:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: Creating FBO triggered an GL_INVALID_ENUM error!\n");
            break;

        case GL_INVALID_VALUE:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: Creating FBO triggered an GL_INVALID_VALUE error!\n");
            break;

        case GL_INVALID_OPERATION:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: Creating FBO triggered an GL_INVALID_OPERATION error!\n");
            break;

        case GL_INVALID_FRAMEBUFFER_OPERATION:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: Creating FBO triggered an GL_INVALID_FRAMEBUFFER_OPERATION error!\n");
            break;

        case GL_OUT_OF_MEMORY:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: Creating FBO triggered an GL_OUT_OF_MEMORY error!\n");
            break;

        case GL_STACK_UNDERFLOW:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: Creating FBO triggered an GL_STACK_UNDERFLOW error!\n");
            break;

        case GL_STACK_OVERFLOW:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: Creating FBO triggered an GL_STACK_OVERFLOW error!\n");
            break;

        case GL_NO_ERROR:
            break;

        default:
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "OffScreenBuffer: Creating FBO triggered an unknown GL error 0x%X!\n", GLStatus);
            break;

        }

        return false;
    }

    return true;
}
