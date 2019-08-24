/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__NON_LINEAR_PROJECTION__H__
#define __SGCT__NON_LINEAR_PROJECTION__H__

#include <sgct/BaseViewport.h>
#include <sgct/ShaderProgram.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace sgct_core {

/**
 * Returns the pixel size for the specified resolution of the cubemap faces.
 *
 * \param quality the quality hint
 *   - low (256x256)
 *   - medium (512x512)
 *   - high (1024x1024)
 *   - 1k (1024x1024)
 *   - 2k (2048x2048)
 *   - 4k (4096x4096)
 *   - 8k (8192x8192)
 *   - 16k (16384x16384)
 */
int cubeMapResolutionForQuality(const std::string& quality);

class OffScreenBuffer;

/**
 * Base class for non linear projections
 */
class NonLinearProjection {
public:
    enum class InterpolationMode { Linear, Cubic };

    virtual ~NonLinearProjection();

    /**
     * Init the non linear projection. The arguments should match the texture settings for
     * the parent window's FBO target.
     */
    void init(int internalTextureFormat, unsigned int textureFormat,
        unsigned int textureType, int samples = 1);

    virtual void render() = 0;
    virtual void renderCubemap(size_t* subViewPortIndex) = 0;
    virtual void update(float width, float height) = 0;

    void updateFrustums(const Frustum::FrustumMode& frustumMode, float nearClipPlane,
        float farClipPlane);
    
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

    /**
     * Set which projection frustum to use for mono projections (can be used for custom
     * passive stereo).
     *
     * \param fm the prefered mono frustum mode
     */
    void setPreferedMonoFrustumMode(Frustum::FrustumMode fm);

    void setUser(SGCTUser* user);

    /// \return the resolution of the cubemap
    int getCubemapResolution() const;

    /// \return the interpolation mode used for the non linear rendering
    InterpolationMode getInterpolationMode() const;

    OffScreenBuffer* getOffScreenBuffer();

    glm::ivec4 getViewportCoords();

protected:
    virtual void initTextures();
    virtual void initFBO();
    virtual void initVBO();
    virtual void initViewports() = 0;
    virtual void initShaders() = 0;

    void setupViewport(BaseViewport& vp);
    void generateMap(unsigned int& texture, int internalFormat, unsigned int format,
        unsigned int type);
    void generateCubeMap(unsigned int& texture, int internalFormat, unsigned int format,
        unsigned int type);

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
    } mTextures;

    BaseViewport mSubViewports[6];

    std::vector<float> mVerts;

    InterpolationMode mInterpolationMode = InterpolationMode::Linear;
    Frustum::FrustumMode mPreferedMonoFrustumMode = Frustum::MonoEye;

    int mCubemapResolution = 512;
    glm::vec4 mClearColor = glm::vec4(0.3f, 0.3f, 0.3f, 1.f);
    glm::ivec4 mVpCoords = glm::ivec4(0);
    bool mUseDepthTransformation = false;
    bool mStereo = false;
    int mTexInternalFormat;
    unsigned int mTexFormat;
    unsigned int mTexType;
    int mSamples = 1;
    unsigned int mVBO = 0;
    unsigned int mVAO = 0;

    sgct::ShaderProgram mShader;
    sgct::ShaderProgram mDepthCorrectionShader;
    std::unique_ptr<OffScreenBuffer> mCubeMapFbo;
};

} // namespace sgct_core

#endif // __SGCT__NON_LINEAR_PROJECTION__H__
