/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

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
    unsigned int mFrameBuffer = 0;
    unsigned int mMultiSampledFrameBuffer = 0;
    unsigned int mColorBuffer = 0;
    unsigned int mNormalBuffer = 0;
    unsigned int mPositionBuffer = 0;
    unsigned int mDepthBuffer = 0;
    GLenum mInternalColorFormat = GL_RGBA8;

    glm::ivec2 mSize = glm::ivec2(-1);
    bool mIsMultiSampled = false;
};

} // namespace sgct::core

#endif // __SGCT__OFFSCREEN_BUFFER__H__
