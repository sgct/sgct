/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__OFFSCREENBUFFER__H__
#define __SGCT__OFFSCREENBUFFER__H__

#include <sgct/math.h>

namespace sgct {

/// Helper class for creating frame buffer objects and render buffer objects.
class OffScreenBuffer {
public:
    static void unbind();

    ~OffScreenBuffer();

    void createFBO(int width, int height, int samples = 1);
    void resizeFBO(int width, int height, int samples = 1);
    void setInternalColorFormat(unsigned int internalFormat);

    /**
     * \param texId GL id of the texture to attach
     * \param attachment the gl attachment enum in the form of GL_COLOR_ATTACHMENTi
     */
    void attachColorTexture(unsigned int texId, unsigned int attachment);
    void attachDepthTexture(unsigned int texId);

    /**
     * \param texId GL id of the texture to attach
     * \param face the target cubemap face
     * \param attachment the gl attachment enum in the form of GL_COLOR_ATTACHMENTi
     */
    void attachCubeMapTexture(unsigned int texId, unsigned int face,
        unsigned int attachment);
    void attachCubeMapDepthTexture(unsigned int texId, unsigned int face);

    /// Bind framebuffer, auto-set multisampling and draw buffers
    void bind();

    /**
     * Bind framebuffer.
     *
     * \param isMultisampled is true if MSAA should be used
     * \param n number of color buffers
     * \param bufs array with color buffers (GL_COLOR_ATTACHMENTn)
     */
    void bind(bool isMultisampled, int n, const unsigned int* bufs);
    void bindBlit();
    void blit();
    bool isMultiSampled() const;

private:
    unsigned int _frameBuffer = 0;
    unsigned int _multiSampledFrameBuffer = 0;
    unsigned int _colorBuffer = 0;
    unsigned int _normalBuffer = 0;
    unsigned int _positionBuffer = 0;
    unsigned int _depthBuffer = 0;
    unsigned int _internalColorFormat = 0x8058; // GL_RGBA8;

    ivec2 _size = ivec2{ -1, -1 };
    bool _isMultiSampled = false;
};

} // namespace sgct

#endif // __SGCT__OFFSCREENBUFFER__H__
