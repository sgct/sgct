/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__NONLINEARPROJECTION__H__
#define __SGCT__NONLINEARPROJECTION__H__

#include <sgct/baseviewport.h>
#include <sgct/shaderprogram.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace sgct { class Window; }

namespace sgct::core {

class OffScreenBuffer;

/// Base class for non linear projections
class NonLinearProjection {
public:
    enum class InterpolationMode { Linear, Cubic };

    NonLinearProjection(Window* parent);

    virtual ~NonLinearProjection();

    /**
     * Init the non linear projection. The arguments should match the texture settings for
     * the parent window's FBO target.
     */
    void init(GLenum internalFormat, GLenum format, GLenum type, int samples);

    virtual void render(const Window& window, const BaseViewport& viewport,
        Frustum::Mode frustumMode) = 0;
    virtual void renderCubemap(Window& window, Frustum::Mode frustumMode) = 0;
    virtual void update(glm::vec2 size) = 0;

    void updateFrustums(const Frustum::Mode& frustumMode, float nearClip, float farClip);
    
    /**
     * Set the resolution of the cubemap faces.
     *
     * \param res the pixel resolution of each quad
     */
    void setCubemapResolution(int resolution);

    /**
     * Set the interpolation mode
     *
     * \param im the selected mode
     */
    void setInterpolationMode(InterpolationMode im);

    /// Set if depth should be re-calculated to match the non linear projection.
    void setUseDepthTransformation(bool state);

    /// Set if stereoscopic rendering will be enabled.
    void setStereo(bool state);

    /**
     * Set the clear color (background color) for the non linear projection renderer.
     *
     * \param color is the RGBA color vector
     */
    void setClearColor(glm::vec4 color);

    /**
     * Set the alpha clear color value for the non-linear projection renderer.
     *
     * \param alpha is the alpha value
     */
    void setAlpha(float alpha);

    void setUser(User* user);

    /// \return the resolution of the cubemap
    int cubemapResolution() const;

    glm::ivec4 viewportCoords();

protected:
    virtual void initTextures();
    virtual void initFBO();
    virtual void initVBO();
    virtual void initViewports() = 0;
    virtual void initShaders() = 0;

    void setupViewport(BaseViewport& vp);
    void generateMap(unsigned int& texture, GLenum internalFormat);
    void generateCubeMap(unsigned int& texture, GLenum internalFormat);

    struct {
        unsigned int cubeMapColor = 0;
        unsigned int cubeMapDepth = 0;
        unsigned int cubeMapNormals = 0;
        unsigned int cubeMapPositions = 0;
        unsigned int colorSwap = 0;
        unsigned int depthSwap = 0;
        unsigned int cubeFaceRight = 0;
        unsigned int cubeFaceLeft = 0;
        unsigned int cubeFaceBottom = 0;
        unsigned int cubeFaceTop = 0;
        unsigned int cubeFaceFront = 0;
        unsigned int cubeFaceBack = 0;
    } _textures;

    struct {
        BaseViewport right;
        BaseViewport left;
        BaseViewport bottom;
        BaseViewport top;
        BaseViewport front;
        BaseViewport back;
    } _subViewports;

    InterpolationMode _interpolationMode = InterpolationMode::Linear;
    Frustum::Mode _preferedMonoFrustumMode = Frustum::Mode::MonoEye;

    int _cubemapResolution = 512;
    glm::vec4 _clearColor = glm::vec4(0.3f, 0.3f, 0.3f, 1.f);
    glm::ivec4 _vpCoords = glm::ivec4(0, 0, 0, 0);
    bool _useDepthTransformation = false;
    bool _isStereo = false;
    GLenum _texInternalFormat;
    GLenum _texFormat;
    GLenum _texType;
    int _samples = 1;
    unsigned int _vao = 0;
    unsigned int _vbo = 0;

    ShaderProgram _shader;
    ShaderProgram _depthCorrectionShader;
    std::unique_ptr<OffScreenBuffer> _cubeMapFbo;
};

} // namespace sgct::core

#endif // __SGCT__NONLINEARPROJECTION__H__
