/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__NONLINEARPROJECTION__H__
#define __SGCT__NONLINEARPROJECTION__H__

#include <sgct/sgctexports.h>

#include <sgct/baseviewport.h>
#include <sgct/definitions.h>
#include <cstdint>
#include <memory>

namespace sgct {

class OffScreenBuffer;
class Window;

/**
 * Base class for non-linear projections.
 */
class SGCT_EXPORT NonLinearProjection {
public:
    enum class InterpolationMode : uint8_t { Linear, Cubic };

    explicit NonLinearProjection(const Window& parent);

    virtual ~NonLinearProjection();

    /**
     * Initialize the non-linear projection. The arguments should match the texture
     * settings for the parent window's FBO target.
     */
    virtual void initialize(unsigned int internalFormat, int nSamples);

    virtual void render(const BaseViewport& viewport, FrustumMode frustumMode) const = 0;
    virtual void renderCubemap(FrustumMode frustumMode) const = 0;
    virtual void update(const vec2& size) const = 0;

    virtual void updateFrustums(FrustumMode mode, float nearClip, float farClip);

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

    virtual void setUser(User& user);

    /**
     * \return the resolution of the cubemap
     */
    ivec2 cubemapResolution() const;

protected:
    virtual void initTextures(unsigned int internalFormat);
    virtual void initFBO(unsigned int internalFormat, int nSamples);
    virtual void initVBO() = 0;
    virtual void initViewports() = 0;
    virtual void initShaders() = 0;

    void setupViewport(const BaseViewport& vp) const;
    void generateMap(unsigned int& texture, unsigned int internalFormat);
    void generateCubeMap(unsigned int& texture, unsigned int internalFormat);

    void attachTextures(int face) const;
    void blitCubeFace(int face) const;
    void renderCubeFace(const BaseViewport& vp, int idx, FrustumMode mode) const;

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
    FrustumMode _preferedMonoFrustumMode = FrustumMode::Mono;

    bool _useDepthTransformation = false;
    bool _isStereo = false;

    ivec2 _cubemapResolution = ivec2(512, 512);
    vec4 _clearColor = vec4(0.3f, 0.3f, 0.3f, 1.f);

    std::unique_ptr<OffScreenBuffer> _cubeMapFbo;
};

} // namespace sgct

#endif // __SGCT__NONLINEARPROJECTION__H__
