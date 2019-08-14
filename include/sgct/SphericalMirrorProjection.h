/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__SPHERICAL_MIRROR_PROJECTION__H__
#define __SGCT__SPHERICAL_MIRROR_PROJECTION__H__

#include <sgct/NonLinearProjection.h>

#include <sgct/CorrectionMesh.h>
#include <glm/glm.hpp>

namespace sgct_core {
/*!
This class manages and renders non linear fisheye projections
*/
class SphericalMirrorProjection : public NonLinearProjection {
public:
    SphericalMirrorProjection() = default;
    virtual ~SphericalMirrorProjection() = default;

    virtual void update(float width, float height) override;
    virtual void render() override;
    virtual void renderCubemap(size_t* subViewPortIndex) override;

    void setTilt(float angle);
    void setMeshPaths(std::string bottom, std::string left, std::string right,
        std::string top);

private:
    virtual void initTextures() override;
    virtual void initVBO() override;
    virtual void initViewports() override;
    virtual void initShaders() override;
        
    void drawCubeFace(size_t face);
    void blitCubeFace(TextureIndex ti);
    void attachTextures(TextureIndex ti);
    void renderInternal();
    void renderInternalFixedPipeline();
    void renderCubemapInternal(size_t* subViewPortIndex);
    void renderCubemapInternalFixedPipeline(size_t* subViewPortIndex);

    float mTilt = 0.f;
    float mDiameter = 2.4f;
        
    //mesh data
    struct {
        CorrectionMesh bottom;
        CorrectionMesh left;
        CorrectionMesh right;
        CorrectionMesh top;
    } mMeshes;
    struct {
        std::string bottom;
        std::string left;
        std::string right;
        std::string top;
    } mMeshPaths;

    //shader locations
    int mTexLoc = -1;
    int mMatrixLoc = -1;
};

} // namespace sgct_core

#endif // __SGCT__SPHERICAL_MIRROR_PROJECTION__H__
