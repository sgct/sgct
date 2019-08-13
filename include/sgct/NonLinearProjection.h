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
#include <string>

namespace sgct_core {

class OffScreenBuffer;

/*!
Base class for non linear projections
*/
class NonLinearProjection {
public:
    enum class InterpolationMode { Linear, Cubic };

    NonLinearProjection();
    virtual ~NonLinearProjection();

    void init(int internalTextureFormat, unsigned int textureFormat,
        unsigned int textureType, int samples = 1);

    virtual void render() = 0;
    virtual void renderCubemap(size_t* subViewPortIndex) = 0;
    virtual void update(float width, float height) = 0;

    void updateFrustums(const Frustum::FrustumMode& frustumMode, float near_clipping_plane,
        float far_clipping_plane);
    void setCubemapResolution(int res);
    void setCubemapResolution(const std::string& quality);
    void setInterpolationMode(InterpolationMode im);
    void setUseDepthTransformation(bool state);
    void setStereo(bool state);
    void setClearColor(float red, float green, float blue, float alpha = 1.f);
    void setClearColor(glm::vec4 color);
    void setAlpha(float alpha);
    void setPreferedMonoFrustumMode(Frustum::FrustumMode fm);

    int getCubemapResolution() const;
    InterpolationMode getInterpolationMode() const;

    void bindShaderProgram() const;
    void bindDepthCorrectionShaderProgram() const;

    BaseViewport& getSubViewportPtr(size_t index);
    OffScreenBuffer* getOffScreenBuffer();
    glm::ivec4 getViewportCoords();

protected:
    enum TextureIndex { CubeMapColor, CubeMapDepth, CubeMapNormals, CubeMapPositions,
        ColorSwap, DepthSwap, CubeFaceRight, CubeFaceLeft, CubeFaceBottom, CubeFaceTop,
        CubeFaceFront, CubeFaceBack, LastIndex };

    virtual void initTextures();
    virtual void initFBO();
    virtual void initVBO();
    virtual void initViewports() = 0;
    virtual void initShaders() = 0;

    void setupViewport(size_t face);
    void generateMap(TextureIndex ti, int internalFormat, unsigned int format, unsigned int type);
    void generateCubeMap(TextureIndex ti, int internalFormat, unsigned int format, unsigned int type);

    int getCubemapRes(const std::string& quality);
    int mCubemapResolution = 512;

    bool mUseDepthTransformation = false;
    bool mStereo = false;

    BaseViewport mSubViewports[6]; //cubemap
    InterpolationMode mInterpolationMode = InterpolationMode::Linear;
    Frustum::FrustumMode mPreferedMonoFrustumMode = Frustum::MonoEye;

    //opengl data
    unsigned int mTextures[LastIndex];
    int mTextureInternalFormat;
    unsigned int mTextureFormat;
    unsigned int mTextureType;
    int mSamples = 1;
    unsigned int mVBO = 0;
    unsigned int mVAO = 0;
    float* mVerts = nullptr;
    glm::ivec4 mVpCoords = glm::ivec4(0);

    sgct::ShaderProgram mShader;
    sgct::ShaderProgram mDepthCorrectionShader;
    OffScreenBuffer* mCubeMapFBO_Ptr = nullptr;
    glm::vec4 mClearColor = glm::vec4(0.3f, 0.3f, 0.3f, 1.f);
};

} // namespace sgct_core

#endif // __SGCT__NON_LINEAR_PROJECTION__H__
