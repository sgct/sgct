/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__OFFSCREEN_BUFFER__H__
#define __SGCT__OFFSCREEN_BUFFER__H__

#include <sgct/ogl_headers.h>
#include <glm/glm.hpp>

namespace sgct::core {

/**
 * Helper class for creating frame buffer objects and render buffer objects.
 */
class OffScreenBuffer {
public:
    void createFBO(int width, int height, int samples = 1);
    void resizeFBO(int width, int height, int samples = 1);
    void setInternalColorFormat(GLenum internalFormat);

    /**
     * \param texId GL id of the texture to attach
     * \param attachment the gl attachment enum in the form of GL_COLOR_ATTACHMENTi
     */
    void attachColorTexture(unsigned int texId, GLenum attachment = GL_COLOR_ATTACHMENT0);
    void attachDepthTexture(unsigned int texId);

    /**
     * \param texId GL id of the texture to attach
     * \param face the target cubemap face
     * \param attachment the gl attachment enum in the form of GL_COLOR_ATTACHMENTi
     */
    void attachCubeMapTexture(unsigned int texId, unsigned int face,
        GLenum attachment = GL_COLOR_ATTACHMENT0);
    void attachCubeMapDepthTexture(unsigned int texId, unsigned int face);

    /// Bind framebuffer, auto-set multisampling and draw buffers
    void bind();

    /**
     * Bind framebuffer, auto-set multisampling.
     *
     * \param n number of color buffers
     * \param bufs array with color buffers (GL_COLOR_ATTACHMENTn)
     */
    void bind(GLsizei n, const GLenum* bufs);

    /**
     * Bind framebuffer, auto-set draw buffers.
     * \param isMultisampled is true if MSAA should be used
     */
    void bind(bool isMultisampled);

    /**
     * Bind framebuffer
     * \param isMultisampled is true if MSAA should be used
     * \param n number of color buffers
     * \param bufs array with color buffers (GL_COLOR_ATTACHMENTn)
     */
    void bind(bool isMultisampled, GLsizei n, const GLenum* bufs);
    void bindBlit();
    void blit();
    static void unBind();
    void destroy();
    bool isMultiSampled() const;

    unsigned int getBufferID() const;
    
    /// \return the opengl internal texture format of the color buffer 
    GLenum getInternalColorFormat() const;

    /// \return true if no errors
    bool checkForErrors();

private:
    unsigned int _frameBuffer = 0;
    unsigned int _multiSampledFrameBuffer = 0;
    unsigned int _colorBuffer = 0;
    unsigned int _normalBuffer = 0;
    unsigned int _positionBuffer = 0;
    unsigned int _depthBuffer = 0;
    GLenum _internalColorFormat = GL_RGBA8;

    glm::ivec2 _size = glm::ivec2(-1);
    bool _isMultiSampled = false;
};

} // namespace sgct::core

#endif // __SGCT__OFFSCREEN_BUFFER__H__
