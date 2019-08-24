/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/OffScreenBuffer.h>

#include <sgct/MessageHandler.h>
#include <sgct/SGCTSettings.h>

namespace {
    void setDrawBuffers() {
        switch (sgct::SGCTSettings::instance()->getCurrentDrawBufferType()) {
            case sgct::SGCTSettings::DrawBufferType::Diffuse:
            default:
            {
                GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
                glDrawBuffers(1, buffers);
                break;
            }
            case sgct::SGCTSettings::DrawBufferType::DiffuseNormal:
            {
                GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
                glDrawBuffers(2, buffers);
                break;
            }
            case sgct::SGCTSettings::DrawBufferType::DiffusePosition:
            {
                GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT2 };
                glDrawBuffers(2, buffers);
                break;
            }
            case sgct::SGCTSettings::DrawBufferType::DiffuseNormalPosition:
            {
                GLenum buffers[] = {
                    GL_COLOR_ATTACHMENT0,
                    GL_COLOR_ATTACHMENT1,
                    GL_COLOR_ATTACHMENT2
                };
                glDrawBuffers(3, buffers);
                break;
            }
        }
    }

} // namespace

namespace sgct_core {

void OffScreenBuffer::createFBO(int width, int height, int samples) {
    glGenFramebuffers(1, &mFrameBuffer);
    glGenRenderbuffers(1, &mDepthBuffer);

    mSize = glm::ivec2(width, height);
    mIsMultiSampled = (samples > 1 && sgct::SGCTSettings::instance()->useFBO());

    // create a multisampled buffer
    if (mIsMultiSampled) {
        GLint MaxSamples;
        glGetIntegerv(GL_MAX_SAMPLES, &MaxSamples);
        if (samples > MaxSamples) {
            samples = MaxSamples;
        }
        if (MaxSamples < 2) {
            samples = 0;
        }

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "Max samples supported: %d\n",
            MaxSamples
        );

        // generate the multisample buffer
        glGenFramebuffers(1, &mMultiSampledFrameBuffer);

        // generate render buffer for intermediate diffuse color storage
        glGenRenderbuffers(1, &mColorBuffer);

        // generate render buffer for intermediate normal storage
        if (sgct::SGCTSettings::instance()->useNormalTexture()) {
            glGenRenderbuffers(1, &mNormalBuffer);
        }

        // generate render buffer for intermediate position storage
        if (sgct::SGCTSettings::instance()->usePositionTexture()) {
            glGenRenderbuffers(1, &mPositionBuffer);
        }
        
        // Bind FBO
        // Setup Render Buffers for multisample FBO
        glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffer);

        glBindRenderbuffer(GL_RENDERBUFFER, mColorBuffer);
        glRenderbufferStorageMultisample(
            GL_RENDERBUFFER,
            samples,
            mInternalColorFormat,
            width,
            height
        );

        if (sgct::SGCTSettings::instance()->useNormalTexture()) {
            glBindRenderbuffer(GL_RENDERBUFFER, mNormalBuffer);
            glRenderbufferStorageMultisample(
                GL_RENDERBUFFER,
                samples,
                sgct::SGCTSettings::instance()->getBufferFloatPrecisionAsGLint(),
                width,
                height
            );
        }

        if (sgct::SGCTSettings::instance()->usePositionTexture()) {
            glBindRenderbuffer(GL_RENDERBUFFER, mPositionBuffer);
            glRenderbufferStorageMultisample(
                GL_RENDERBUFFER,
                samples,
                sgct::SGCTSettings::instance()->getBufferFloatPrecisionAsGLint(),
                width,
                height
            );
        }
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
    }

    // Setup depth render buffer
    glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);
    if (mIsMultiSampled) {
        glRenderbufferStorageMultisample(
            GL_RENDERBUFFER,
            samples,
            GL_DEPTH_COMPONENT32,
            width,
            height
        );
    }
    else {
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);
   }

    // It's time to attach the RBs to the FBO
    if (mIsMultiSampled) {
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_RENDERBUFFER,
            mColorBuffer
        );
        if (sgct::SGCTSettings::instance()->useNormalTexture()) {
            glFramebufferRenderbuffer(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT1,
                GL_RENDERBUFFER,
                mNormalBuffer
            );
        }
        if (sgct::SGCTSettings::instance()->usePositionTexture()) {
            glFramebufferRenderbuffer(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT2,
                GL_RENDERBUFFER,
                mPositionBuffer
            );
        }
    }

    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER,
        mDepthBuffer
    );

    // unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (mIsMultiSampled) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "OffScreenBuffer: Created %dx%d buffers:\n\tFBO id=%d\n\tMultisample FBO "
            "id=%d\n\tRBO depth buffer id=%d\n\tRBO color buffer id=%d\n",
            width, height, mFrameBuffer, mMultiSampledFrameBuffer, mDepthBuffer,
            mColorBuffer
        );
    }
    else {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::Level::Debug,
            "OffScreenBuffer: Created %dx%d buffers:\n\tFBO id=%d\n"
            "\tRBO Depth buffer id=%d\n",
            width, height, mFrameBuffer, mDepthBuffer
        );
    }

    //sgct::MessageHandler::instance()->print("FBO %d x %d (x %d) created!\n", width, height, samples);
}

void OffScreenBuffer::resizeFBO(int width, int height, int samples) {
    mSize = glm::ivec2(width, height);
    mIsMultiSampled = (samples > 1 && sgct::SGCTSettings::instance()->useFBO());

    // delete all
    glDeleteFramebuffers(1, &mFrameBuffer);
    glDeleteRenderbuffers(1, &mDepthBuffer);
    glDeleteFramebuffers(1, &mMultiSampledFrameBuffer);
    glDeleteRenderbuffers(1, &mColorBuffer);
    glDeleteRenderbuffers(1, &mNormalBuffer);
    glDeleteRenderbuffers(1, &mPositionBuffer);

    // init
    mFrameBuffer = 0;
    mMultiSampledFrameBuffer = 0;
    mColorBuffer = 0;
    mDepthBuffer = 0;
    mNormalBuffer = 0;
    mPositionBuffer = 0;

    createFBO(width, height, samples);
}

void OffScreenBuffer::setInternalColorFormat(GLint internalFormat) {
    mInternalColorFormat = internalFormat;
}

void OffScreenBuffer::bind() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, GL_FALSE);

    if (mIsMultiSampled) {
        glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffer);
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
    }

    setDrawBuffers();
}

void OffScreenBuffer::bind(GLsizei n, const GLenum* bufs) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, GL_FALSE);

    if (mIsMultiSampled) {
        glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffer);
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
    }

    glDrawBuffers(n, bufs);
}

void OffScreenBuffer::bind(bool isMultisampled) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, GL_FALSE);

    if (isMultisampled) {
        glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffer);
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
    }

    setDrawBuffers();
}

void OffScreenBuffer::bind(bool isMultisampled, GLsizei n, const GLenum* bufs) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, GL_FALSE);

    if (isMultisampled) {
        glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffer);
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffer);
    }

    glDrawBuffers(n, bufs);
}

void OffScreenBuffer::bindBlit() {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mMultiSampledFrameBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFrameBuffer);
    setDrawBuffers();
}

void OffScreenBuffer::unBind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OffScreenBuffer::blit() {
    // use no interpolation since src and dst size is equal
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    if (sgct::SGCTSettings::instance()->useDepthTexture()) {
        glBlitFramebuffer(
            0, 0, mSize.x, mSize.y,
            0, 0, mSize.x, mSize.y,
            GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST
        );
    }
    else {
        glBlitFramebuffer(
            0, 0, mSize.x, mSize.y,
            0, 0, mSize.x, mSize.y,
            GL_COLOR_BUFFER_BIT, GL_NEAREST
        );
    }

    if (sgct::SGCTSettings::instance()->useNormalTexture()) {
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glDrawBuffer(GL_COLOR_ATTACHMENT1);

        glBlitFramebuffer(
            0, 0, mSize.x, mSize.y,
            0, 0, mSize.x, mSize.y,
            GL_COLOR_BUFFER_BIT, GL_NEAREST
        );
    }

    if (sgct::SGCTSettings::instance()->usePositionTexture()) {
        glReadBuffer(GL_COLOR_ATTACHMENT2);
        glDrawBuffer(GL_COLOR_ATTACHMENT2);

        glBlitFramebuffer(
            0, 0, mSize.x, mSize.y,
            0, 0, mSize.x, mSize.y,
            GL_COLOR_BUFFER_BIT, GL_NEAREST
        );
    }
}

void OffScreenBuffer::destroy() {
    glDeleteFramebuffers(1, &mFrameBuffer);
    glDeleteRenderbuffers(1, &mDepthBuffer);
    
    if(mIsMultiSampled) {
        glDeleteFramebuffers(1, &mMultiSampledFrameBuffer);
        glDeleteRenderbuffers(1, &mColorBuffer);

        if (sgct::SGCTSettings::instance()->useNormalTexture()) {
            glDeleteRenderbuffers(1, &mNormalBuffer);
        }
        if (sgct::SGCTSettings::instance()->usePositionTexture()) {
            glDeleteRenderbuffers(1, &mPositionBuffer);
        }
    }

    mFrameBuffer = 0;
    mMultiSampledFrameBuffer = 0;
    mColorBuffer = 0;
    mDepthBuffer = 0;
    mNormalBuffer = 0;
    mPositionBuffer = 0;
}

bool OffScreenBuffer::isMultiSampled() const {
    return mIsMultiSampled;
}

unsigned int OffScreenBuffer::getBufferID() const {
    return mIsMultiSampled ? mMultiSampledFrameBuffer : mFrameBuffer;
}

void OffScreenBuffer::attachColorTexture(unsigned int texId, GLenum attachment) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texId, 0);
}

void OffScreenBuffer::attachDepthTexture(unsigned int texId) {
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texId, 0);
}

void OffScreenBuffer::attachCubeMapTexture(unsigned int texId, unsigned int face,
                                           GLenum attachment)
{
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        attachment,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
        texId,
        0
    );
}

void OffScreenBuffer::attachCubeMapDepthTexture(unsigned int texId, unsigned int face) {
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
        texId,
        0
    );
}

int OffScreenBuffer::getInternalColorFormat() const {
    return mInternalColorFormat;
}

bool OffScreenBuffer::checkForErrors() {
    // Does the GPU support current FBO configuration?
    const GLenum FBOStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    const GLenum GLStatus = glGetError();
    if (FBOStatus == GL_FRAMEBUFFER_COMPLETE && GLStatus == GL_NO_ERROR) {
        return true;
    }
    switch (FBOStatus) {
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: FBO has incomplete attachments\n"
            );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: FBO has no attachments\n"
            );
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: Unsupported FBO format\n"
            );
            break;
        case GL_FRAMEBUFFER_UNDEFINED:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: Undefined FBO\n"
            );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: FBO has incomplete draw buffer\n"
            );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: FBO has incomplete read buffer\n"
            );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: FBO has missmatching multisample values\n"
            );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: FBO has incomplete layer targets\n"
            );
            break;
        case GL_FRAMEBUFFER_COMPLETE: //no error
            break;
        default: // Unknown error
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: Unknown FBO error: 0x%X\n", FBOStatus
            );
            break;
    }

    switch (GLStatus) {
        case GL_INVALID_ENUM:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: Creating FBO triggered an GL_INVALID_ENUM error\n"
            );
            break;
        case GL_INVALID_VALUE:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: Creating FBO triggered an GL_INVALID_VALUE error\n"
            );
            break;
        case GL_INVALID_OPERATION:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: Creating FBO triggered an GL_INVALID_OPERATION error\n"
            );
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: Creating FBO triggered an "
                "GL_INVALID_FRAMEBUFFER_OPERATION error!\n"
            );
            break;
        case GL_OUT_OF_MEMORY:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: Creating FBO triggered an GL_OUT_OF_MEMORY error\n"
            );
            break;
        case GL_STACK_UNDERFLOW:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: Creating FBO triggered an GL_STACK_UNDERFLOW error\n"
            );
            break;
        case GL_STACK_OVERFLOW:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: Creating FBO triggered an GL_STACK_OVERFLOW error\n"
            );
            break;
        case GL_NO_ERROR:
            break;
        default:
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "OffScreenBuffer: Creating FBO triggered an unknown GL error 0x%X\n",
                GLStatus
            );
            break;
    }
    return false;
}

} // namespace sgct_core
