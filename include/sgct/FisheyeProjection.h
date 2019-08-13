/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__FISHEYE_PROJECTION__H__
#define __SGCT__FISHEYE_PROJECTION__H__

#include <sgct/NonLinearProjection.h>

#include <glm/glm.hpp>

namespace sgct_core {
/*!
This class manages and renders non linear fisheye projections
*/
class FisheyeProjection : public NonLinearProjection {
public:
    enum FisheyeMethod { FourFaceCube = 0, FiveFaceCube, SixFaceCube };
    enum FisheyeCropSide { CropLeft = 0, CropRight, CropBottom, CropTop };

    void update(float width, float height);
    void render();
    void renderCubemap(size_t* subViewPortIndex);

    void setDomeDiameter(float size);
    void setTilt(float angle);
    void setFOV(float angle);
    void setRenderingMethod(FisheyeMethod method);
    void setCropFactors(float left, float right, float bottom, float top);
    void setOffset(glm::vec3 offset);
    void setOffset(float x, float y, float z = 0.f);
    void setBaseOffset(glm::vec3 offset);
    void setIgnoreAspectRatio(bool state);

    glm::vec3 getOffset() const;

private:
    void initViewports();
    void initShaders();
    void updateGeometry(float width, float height);

    void drawCubeFace(size_t face);
    void blitCubeFace(int face);
    void attachTextures(int face);
    void renderInternal();
    void renderInternalFixedPipeline();
    void renderCubemapInternal(size_t* subViewPortIndex);
    void renderCubemapInternalFixedPipeline(size_t* subViewPortIndex);

    float mFOV = 180.f;
    float mTilt = 0.f;
    float mDiameter = 14.8f;
    float mCropFactors[4] = { 0.f, 0.f, 0.f, 0.f };

    bool mOffAxis = false;
    bool mIgnoreAspectRatio = false;
        
    glm::vec3 mOffset = glm::vec3(0.f);
    glm::vec3 mBaseOffset = glm::vec3(0.f);
    glm::vec3 mTotalOffset = mBaseOffset + mOffset;

    FisheyeMethod mMethod = FourFaceCube;

    //shader locations
    struct {
        int cubemapLoc = -1;
        int depthCubemapLoc = -1;
        int normalCubemapLoc = -1;
        int positionCubemapLoc = -1;
        int halfFovLoc = -1;
        int offsetLoc = -1;
        int swapColorLoc = -1;
        int swapDepthLoc = -1;
        int swapNearLoc = -1;
        int swapFarLoc = -1;
    } mShaderLoc;
};

} // namespace sgct_core

#endif // __SGCT__FISHEYE_PROJECTION__H__
