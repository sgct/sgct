/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/offscreenbuffer.h>

#include <sgct/messagehandler.h>
#include <sgct/settings.h>
#include <algorithm>

namespace {
    void setDrawBuffers() {
        switch (sgct::Settings::instance()->getDrawBufferType()) {
            case sgct::Settings::DrawBufferType::Diffuse:
            default:
            {
                GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
                glDrawBuffers(1, buffers);
                break;
            }
            case sgct::Settings::DrawBufferType::DiffuseNormal:
            {
                GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
                glDrawBuffers(2, buffers);
                break;
            }
            case sgct::Settings::DrawBufferType::DiffusePosition:
            {
                GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT2 };
                glDrawBuffers(2, buffers);
                break;
            }
            case sgct::Settings::DrawBufferType::DiffuseNormalPosition:
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

namespace sgct::core {

void OffScreenBuffer::createFBO(int width, int height, int samples) {
    glGenFramebuffers(1, &_frameBuffer);
    glGenRenderbuffers(1, &_depthBuffer);

    _size = glm::ivec2(width, height);
    _isMultiSampled = samples > 1;

    // create a multisampled buffer
    if (_isMultiSampled) {
        GLint MaxSamples;
        glGetIntegerv(GL_MAX_SAMPLES, &MaxSamples);
        samples = std::max(samples, MaxSamples);
        if (MaxSamples < 2) {
            samples = 0;
        }

        MessageHandler::instance()->printDebug("Max samples supported: %d", MaxSamples);

        // generate the multisample buffer
        glGenFramebuffers(1, &_multiSampledFrameBuffer);

        // generate render buffer for intermediate diffuse color storage
        glGenRenderbuffers(1, &_colorBuffer);

        // generate render buffer for intermediate normal storage
        if (Settings::instance()->useNormalTexture()) {
            glGenRenderbuffers(1, &_normalBuffer);
        }

        // generate render buffer for intermediate position storage
        if (Settings::instance()->usePositionTexture()) {
            glGenRenderbuffers(1, &_positionBuffer);
        }
        
        // Bind FBO
        // Setup Render Buffers for multisample FBO
        glBindFramebuffer(GL_FRAMEBUFFER, _multiSampledFrameBuffer);

        glBindRenderbuffer(GL_RENDERBUFFER, _colorBuffer);
        glRenderbufferStorageMultisample(
            GL_RENDERBUFFER,
            samples,
            _internalColorFormat,
            width,
            height
        );

        if (Settings::instance()->useNormalTexture()) {
            glBindRenderbuffer(GL_RENDERBUFFER, _normalBuffer);
            glRenderbufferStorageMultisample(
                GL_RENDERBUFFER,
                samples,
                Settings::instance()->getBufferFloatPrecisionAsGLint(),
                width,
                height
            );
        }

        if (Settings::instance()->usePositionTexture()) {
            glBindRenderbuffer(GL_RENDERBUFFER, _positionBuffer);
            glRenderbufferStorageMultisample(
                GL_RENDERBUFFER,
                samples,
                Settings::instance()->getBufferFloatPrecisionAsGLint(),
                width,
                height
            );
        }
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
    }

    // Setup depth render buffer
    glBindRenderbuffer(GL_RENDERBUFFER, _depthBuffer);
    if (_isMultiSampled) {
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
    if (_isMultiSampled) {
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_RENDERBUFFER,
            _colorBuffer
        );
        if (Settings::instance()->useNormalTexture()) {
            glFramebufferRenderbuffer(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT1,
                GL_RENDERBUFFER,
                _normalBuffer
            );
        }
        if (Settings::instance()->usePositionTexture()) {
            glFramebufferRenderbuffer(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT2,
                GL_RENDERBUFFER,
                _positionBuffer
            );
        }
    }

    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER,
        _depthBuffer
    );

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (_isMultiSampled) {
        MessageHandler::instance()->printDebug(
            "OffScreenBuffer: Created %dx%d buffers:\n\tFBO id=%d\n\tMultisample FBO "
            "id=%d\n\tRBO depth buffer id=%d\n\tRBO color buffer id=%d",
            width, height, _frameBuffer, _multiSampledFrameBuffer, _depthBuffer,
            _colorBuffer
        );
    }
    else {
        MessageHandler::instance()->printDebug(
            "OffScreenBuffer: Created %dx%d buffers:\n\tFBO id=%d\n"
            "\tRBO Depth buffer id=%d",
            width, height, _frameBuffer, _depthBuffer
        );
    }
}

void OffScreenBuffer::resizeFBO(int width, int height, int samples) {
    _size = glm::ivec2(width, height);
    _isMultiSampled = samples > 1;

    glDeleteFramebuffers(1, &_frameBuffer);
    glDeleteRenderbuffers(1, &_depthBuffer);
    glDeleteFramebuffers(1, &_multiSampledFrameBuffer);
    glDeleteRenderbuffers(1, &_colorBuffer);
    glDeleteRenderbuffers(1, &_normalBuffer);
    glDeleteRenderbuffers(1, &_positionBuffer);

    // init
    _frameBuffer = 0;
    _multiSampledFrameBuffer = 0;
    _colorBuffer = 0;
    _depthBuffer = 0;
    _normalBuffer = 0;
    _positionBuffer = 0;

    createFBO(width, height, samples);
}

void OffScreenBuffer::setInternalColorFormat(GLenum internalFormat) {
    _internalColorFormat = internalFormat;
}

void OffScreenBuffer::bind() {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (_isMultiSampled) {
        glBindFramebuffer(GL_FRAMEBUFFER, _multiSampledFrameBuffer);
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
    }

    setDrawBuffers();
}

void OffScreenBuffer::bind(GLsizei n, const GLenum* bufs) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (_isMultiSampled) {
        glBindFramebuffer(GL_FRAMEBUFFER, _multiSampledFrameBuffer);
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
    }

    glDrawBuffers(n, bufs);
}

void OffScreenBuffer::bind(bool isMultisampled) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (isMultisampled) {
        glBindFramebuffer(GL_FRAMEBUFFER, _multiSampledFrameBuffer);
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
    }

    setDrawBuffers();
}

void OffScreenBuffer::bind(bool isMultisampled, GLsizei n, const GLenum* bufs) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (isMultisampled) {
        glBindFramebuffer(GL_FRAMEBUFFER, _multiSampledFrameBuffer);
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
    }

    glDrawBuffers(n, bufs);
}

void OffScreenBuffer::bindBlit() {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _multiSampledFrameBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _frameBuffer);
    setDrawBuffers();
}

void OffScreenBuffer::unBind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OffScreenBuffer::blit() {
    // use no interpolation since src and dst size is equal
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    if (Settings::instance()->useDepthTexture()) {
        glBlitFramebuffer(
            0, 0, _size.x, _size.y,
            0, 0, _size.x, _size.y,
            GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST
        );
    }
    else {
        glBlitFramebuffer(
            0, 0, _size.x, _size.y,
            0, 0, _size.x, _size.y,
            GL_COLOR_BUFFER_BIT, GL_NEAREST
        );
    }

    if (Settings::instance()->useNormalTexture()) {
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glDrawBuffer(GL_COLOR_ATTACHMENT1);

        glBlitFramebuffer(
            0, 0, _size.x, _size.y,
            0, 0, _size.x, _size.y,
            GL_COLOR_BUFFER_BIT, GL_NEAREST
        );
    }

    if (Settings::instance()->usePositionTexture()) {
        glReadBuffer(GL_COLOR_ATTACHMENT2);
        glDrawBuffer(GL_COLOR_ATTACHMENT2);

        glBlitFramebuffer(
            0, 0, _size.x, _size.y,
            0, 0, _size.x, _size.y,
            GL_COLOR_BUFFER_BIT, GL_NEAREST
        );
    }
}

void OffScreenBuffer::destroy() {
    glDeleteFramebuffers(1, &_frameBuffer);
    glDeleteRenderbuffers(1, &_depthBuffer);
    
    if (_isMultiSampled) {
        glDeleteFramebuffers(1, &_multiSampledFrameBuffer);
        glDeleteRenderbuffers(1, &_colorBuffer);

        if (Settings::instance()->useNormalTexture()) {
            glDeleteRenderbuffers(1, &_normalBuffer);
        }
        if (Settings::instance()->usePositionTexture()) {
            glDeleteRenderbuffers(1, &_positionBuffer);
        }
    }

    _frameBuffer = 0;
    _multiSampledFrameBuffer = 0;
    _colorBuffer = 0;
    _depthBuffer = 0;
    _normalBuffer = 0;
    _positionBuffer = 0;
}

bool OffScreenBuffer::isMultiSampled() const {
    return _isMultiSampled;
}

unsigned int OffScreenBuffer::getBufferID() const {
    return _isMultiSampled ? _multiSampledFrameBuffer : _frameBuffer;
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

GLenum OffScreenBuffer::getInternalColorFormat() const {
    return _internalColorFormat;
}

bool OffScreenBuffer::checkForErrors() {
    // Does the GPU support current FBO configuration?
    const GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    const GLenum glStatus = glGetError();
    if (fboStatus == GL_FRAMEBUFFER_COMPLETE && glStatus == GL_NO_ERROR) {
        return true;
    }
    switch (fboStatus) {
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            MessageHandler::instance()->printError(
                "OffScreenBuffer: FBO has incomplete attachments"
            );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            MessageHandler::instance()->printError(
                "OffScreenBuffer: FBO has no attachments"
            );
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            MessageHandler::instance()->printError(
                "OffScreenBuffer: Unsupported FBO format"
            );
            break;
        case GL_FRAMEBUFFER_UNDEFINED:
            MessageHandler::instance()->printError("OffScreenBuffer: Undefined FBO");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            MessageHandler::instance()->printError(
                "OffScreenBuffer: FBO has incomplete draw buffer"
            );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            MessageHandler::instance()->printError(
                "OffScreenBuffer: FBO has incomplete read buffer"
            );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            MessageHandler::instance()->printError(
                "OffScreenBuffer: FBO has missmatching multisample values"
            );
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            MessageHandler::instance()->printError(
                "OffScreenBuffer: FBO has incomplete layer targets"
            );
            break;
        case GL_FRAMEBUFFER_COMPLETE: //no error
            break;
        default: // Unknown error
            MessageHandler::instance()->printError(
                "OffScreenBuffer: Unknown FBO error: 0x%X", fboStatus
            );
            break;
    }

    switch (glStatus) {
        case GL_INVALID_ENUM:
            MessageHandler::instance()->printError(
                "OffScreenBuffer: Creating FBO triggered an GL_INVALID_ENUM error"
            );
            break;
        case GL_INVALID_VALUE:
            MessageHandler::instance()->printError(
                "OffScreenBuffer: Creating FBO triggered an GL_INVALID_VALUE error"
            );
            break;
        case GL_INVALID_OPERATION:
            MessageHandler::instance()->printError(
                "OffScreenBuffer: Creating FBO triggered an GL_INVALID_OPERATION error"
            );
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            MessageHandler::instance()->printError(
                "OffScreenBuffer: Creating FBO triggered an "
                "GL_INVALID_FRAMEBUFFER_OPERATION error"
            );
            break;
        case GL_OUT_OF_MEMORY:
            MessageHandler::instance()->printError(
                "OffScreenBuffer: Creating FBO triggered an GL_OUT_OF_MEMORY error"
            );
            break;
        case GL_STACK_UNDERFLOW:
            MessageHandler::instance()->printError(
                "OffScreenBuffer: Creating FBO triggered an GL_STACK_UNDERFLOW error"
            );
            break;
        case GL_STACK_OVERFLOW:
            MessageHandler::instance()->printError(
                "OffScreenBuffer: Creating FBO triggered an GL_STACK_OVERFLOW error"
            );
            break;
        case GL_NO_ERROR:
            break;
        default:
            MessageHandler::instance()->printError(
                "OffScreenBuffer: Creating FBO triggered an unknown GL error 0x%X",
                glStatus
            );
            break;
    }
    return false;
}

} // namespace sgct::core
