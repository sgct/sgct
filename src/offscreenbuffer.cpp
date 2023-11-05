/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/offscreenbuffer.h>

#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <sgct/settings.h>
#include <algorithm>

// @TODO (abock, 2020-01-07) It would probably be better to only create a single offscreen
// buffer of the maximum window size and reuse that between all windows.  That way we
// wouldn't waste as much memory if a lot of windows are created.  Also, we don't even
// need an offscreen buffer if we don't have any mesh or mask to apply

namespace {
    void setDrawBuffers() {
        GLenum a[] = { GL_COLOR_ATTACHMENT0 };
        GLenum b[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        GLenum c[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT2 };
        GLenum d[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        switch (sgct::Settings::instance().drawBufferType()) {
            case sgct::Settings::DrawBufferType::Diffuse:
                glDrawBuffers(1, a);
                break;
            case sgct::Settings::DrawBufferType::DiffuseNormal:
                glDrawBuffers(2, b);
                break;
            case sgct::Settings::DrawBufferType::DiffusePosition:
                glDrawBuffers(2, c);
                break;
            case sgct::Settings::DrawBufferType::DiffuseNormalPosition:
                glDrawBuffers(3, d);
                break;
        }
    }
} // namespace

namespace sgct {

OffScreenBuffer::~OffScreenBuffer() {
    glDeleteFramebuffers(1, &_frameBuffer);
    glDeleteRenderbuffers(1, &_depthBuffer);
    glDeleteFramebuffers(1, &_multiSampledFrameBuffer);
    glDeleteRenderbuffers(1, &_colorBuffer);
    glDeleteRenderbuffers(1, &_normalBuffer);
    glDeleteRenderbuffers(1, &_positionBuffer);
}

void OffScreenBuffer::createFBO(int width, int height, int samples, bool mirrored) {
    // @TODO (abock, 2019-11-15)  When calling this function initially with checking
    // FBO mode enabled, the bind functions further down will trigger missing attachment
    // warnings due to the fact that SGCT handles the creation of the FBO and attachments
    // separately.  This should be fixed properly, but that is going to be a problem for
    // later as it doesn't impact the rendering.  All FBOs will be complete before we
    // render anything either way

    glGenFramebuffers(1, &_frameBuffer);
    glGenRenderbuffers(1, &_depthBuffer);

    _size = ivec2{ width, height };
    _isMultiSampled = samples > 1;
    _mirror = mirrored;

    // create a multisampled buffer
    if (_isMultiSampled) {
        GLint maxSamples;
        glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
        samples = std::max(samples, maxSamples);
        if (maxSamples < 2) {
            samples = 0;
        }

        Log::Debug(fmt::format("Max samples supported: {}", maxSamples));

        // generate the multisample buffer
        glGenFramebuffers(1, &_multiSampledFrameBuffer);

        // generate render buffer for intermediate diffuse color storage
        glGenRenderbuffers(1, &_colorBuffer);

        // generate render buffer for intermediate normal storage
        if (Settings::instance().useNormalTexture()) {
            glGenRenderbuffers(1, &_normalBuffer);
        }

        // generate render buffer for intermediate position storage
        if (Settings::instance().usePositionTexture()) {
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

        if (Settings::instance().useNormalTexture()) {
            glBindRenderbuffer(GL_RENDERBUFFER, _normalBuffer);
            glRenderbufferStorageMultisample(
                GL_RENDERBUFFER,
                samples,
                Settings::instance().bufferFloatPrecision(),
                width,
                height
            );
        }

        if (Settings::instance().usePositionTexture()) {
            glBindRenderbuffer(GL_RENDERBUFFER, _positionBuffer);
            glRenderbufferStorageMultisample(
                GL_RENDERBUFFER,
                samples,
                Settings::instance().bufferFloatPrecision(),
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
        if (Settings::instance().useNormalTexture()) {
            glFramebufferRenderbuffer(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT1,
                GL_RENDERBUFFER,
                _normalBuffer
            );
        }
        if (Settings::instance().usePositionTexture()) {
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

    if (_isMultiSampled) {
        Log::Debug(fmt::format(
            "Created {}x{} buffers: FBO id={}  Multisample FBO id={}"
            "RBO depth buffer id={}  RBO color buffer id={}", width, height,
            _frameBuffer, _multiSampledFrameBuffer, _depthBuffer, _colorBuffer
        ));
    }
    else {
        Log::Debug(fmt::format(
            "Created {}x{} buffers: FBO id={}  RBO Depth buffer id={}",
            width, height, _frameBuffer, _depthBuffer
        ));
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OffScreenBuffer::resizeFBO(int width, int height, int samples) {
    _size = ivec2{ width, height };
    _isMultiSampled = samples > 1;

    glDeleteFramebuffers(1, &_frameBuffer);
    glDeleteRenderbuffers(1, &_depthBuffer);
    glDeleteFramebuffers(1, &_multiSampledFrameBuffer);
    glDeleteRenderbuffers(1, &_colorBuffer);
    glDeleteRenderbuffers(1, &_normalBuffer);
    glDeleteRenderbuffers(1, &_positionBuffer);
    createFBO(width, height, samples);
}

void OffScreenBuffer::setInternalColorFormat(unsigned int internalFormat) {
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

void OffScreenBuffer::bind(bool isMultisampled, int n, const unsigned int* bufs) {
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

void OffScreenBuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OffScreenBuffer::blit() {
    ivec2 src0 = ivec2{ 0, 0 };
    ivec2 src1 = ivec2{ _size.x, _size.y };
    ivec2 dst0 = ivec2{ 0, 0 };
    ivec2 dst1 = ivec2{ _size.x, _size.y };

    if (_mirror) {
        dst0.x = _size.x;
        dst1.x = 0;
    }

    // use no interpolation since src and dst size is equal
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    if (Settings::instance().useDepthTexture()) {
        glBlitFramebuffer(
            src0.x, src0.y, src1.x, src1.y,
            dst0.x, dst0.y, dst1.x, dst1.y,
            GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST
        );
    }
    else {
        glBlitFramebuffer(
            src0.x, src0.y, src1.x, src1.y,
            dst0.x, dst0.y, dst1.x, dst1.y,
            GL_COLOR_BUFFER_BIT, GL_NEAREST
        );
    }

    if (Settings::instance().useNormalTexture()) {
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glDrawBuffer(GL_COLOR_ATTACHMENT1);

        glBlitFramebuffer(
            src0.x, src0.y, src1.x, src1.y,
            dst0.x, dst0.y, dst1.x, dst1.y,
            GL_COLOR_BUFFER_BIT, GL_NEAREST
        );
    }

    if (Settings::instance().usePositionTexture()) {
        glReadBuffer(GL_COLOR_ATTACHMENT2);
        glDrawBuffer(GL_COLOR_ATTACHMENT2);

        glBlitFramebuffer(
            src0.x, src0.y, src1.x, src1.y,
            dst0.x, dst0.y, dst1.x, dst1.y,
            GL_COLOR_BUFFER_BIT, GL_NEAREST
        );
    }
}

bool OffScreenBuffer::isMultiSampled() const {
    return _isMultiSampled;
}

void OffScreenBuffer::attachColorTexture(unsigned int texId, GLenum attachment) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texId, 0);
}

void OffScreenBuffer::attachDepthTexture(unsigned int texId) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texId, 0);
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

} // namespace sgct
