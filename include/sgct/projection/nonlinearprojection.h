/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__NONLINEARPROJECTION__H__
#define __SGCT__NONLINEARPROJECTION__H__

#include <sgct/sgctexports.h>
#include <sgct/baseviewport.h>
#include <sgct/shaderprogram.h>
#include <memory>
#include <string>

namespace sgct {

class OffScreenBuffer;
class Window;

/**
 * Base class for non-linear projections.
 */
class SGCT_EXPORT NonLinearProjection {
public:
    enum class InterpolationMode { Linear, Cubic };

    NonLinearProjection(const Window* parent);

    virtual ~NonLinearProjection();

    /**
     * Initialize the non-linear projection. The arguments should match the texture
     * settings for the parent window's FBO target.
     */
    void initialize(unsigned int internalFormat, unsigned int format, unsigned int type,
        int samples);

    virtual void render(const Window& window, const BaseViewport& viewport,
        Frustum::Mode frustumMode) = 0;
    virtual void renderCubemap(Window& window, Frustum::Mode frustumMode) = 0;
    virtual void update(vec2 size) = 0;

    virtual void updateFrustums(Frustum::Mode mode, float nearClip, float farClip);

    /**
     * Set the resolution of the cubemap faces.
     *
     * \param resolution The pixel resolution of each quad
     */
    void setCubemapResolution(int resolution);

    /**
     * Set the interpolation mode.
     *
     * \param im The selected mode
     */
    void setInterpolationMode(InterpolationMode im);

    /**
     * Set if depth should be re-calculated to match the non-linear projection.
     */
    void setUseDepthTransformation(bool state);

    /**
     * Set if stereoscopic rendering will be enabled.
     */
    void setStereo(bool state);

    /**
     * Set the clear color (background color) for the non linear projection renderer.
     *
     * \param color The RGBA color vector
     */
    void setClearColor(vec4 color);

    virtual void setUser(User* user);

    /**
     * \return the resolution of the cubemap
     */
    ivec2 cubemapResolution() const;

    ivec4 viewportCoords();

protected:
    virtual void initTextures();
    virtual void initFBO();
    virtual void initVBO() = 0;
    virtual void initViewports() = 0;
    virtual void initShaders() = 0;

    void setupViewport(BaseViewport& vp);
    void generateMap(unsigned int& texture, unsigned int internalFormat,
        unsigned int format, unsigned int type);
    void generateCubeMap(unsigned int& texture, unsigned int internalFormat,
        unsigned int format, unsigned int type);

    void attachTextures(int face);
    void blitCubeFace(int face);
    void renderCubeFace(const Window& win, BaseViewport& vp, int idx, Frustum::Mode mode);
    void renderCubeFaces(Window& window, Frustum::Mode frustumMode);

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

    ivec2 _cubemapResolution = ivec2(512, 512);
    vec4 _clearColor = vec4(0.3f, 0.3f, 0.3f, 1.f);
    ivec4 _vpCoords = ivec4(0, 0, 0, 0);
    bool _useDepthTransformation = false;
    bool _isStereo = false;
    unsigned int _texInternalFormat = 0;
    unsigned int _texFormat = 0;
    unsigned int _texType = 0;
    int _samples = 1;

    std::unique_ptr<OffScreenBuffer> _cubeMapFbo;
};

} // namespace sgct

#endif // __SGCT__NONLINEARPROJECTION__H__
