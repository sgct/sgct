/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__SPOUTOUTPUT_PROJECTION__H__
#define __SGCT__SPOUTOUTPUT_PROJECTION__H__

#include "NonLinearProjection.h"
#include <glm/glm.hpp>

namespace sgct_core {
/*!
This class manages and renders non linear fisheye projections
*/
class SpoutOutputProjection : public NonLinearProjection {
public:
    SpoutOutputProjection() = default;
    virtual ~SpoutOutputProjection();

    enum Mapping {
        Fisheye,
        Equirectangular,
        Cubemap
    };

    void setSpoutChannels(bool channels[6]);
    void setSpoutMappingName(std::string name);
    void setSpoutMapping(Mapping type);
    void setSpoutRigOrientation(glm::vec3 orientation);
    virtual void update(float width, float height) override;
    virtual void render() override;
    virtual void renderCubemap(size_t* subViewPortIndex) override;

    static const size_t spoutTotalFaces = 6;
    static const std::string spoutCubeMapFaceName[spoutTotalFaces];

private:
    void initTextures();
    void initViewports();
    void initShaders();
    void initFBO();
    void updateGeometry(float width, float height);

    void drawCubeFace(size_t face);
    void blitCubeFace(int face);
    void attachTextures(int face);
    void renderInternal();
    void renderInternalFixedPipeline();
    void renderCubemapInternal(size_t* subViewPortIndex);
    void renderCubemapInternalFixedPipeline(size_t* subViewPortIndex);

    //shader locations
    int mCubemapLoc = -1;
    int mHalfFovLoc = -1;
    int mSwapColorLoc = -1;
    int mSwapDepthLoc = -1;
    int mSwapNearLoc = -1;
    int mSwapFarLoc = -1;

    OffScreenBuffer* mSpoutFBO_Ptr = nullptr;

    struct SpoutInfo {
        bool enabled = true;
        void* handle = nullptr;
        GLuint texture = -1;
    };
    SpoutInfo mSpout[spoutTotalFaces];

    void* spoutMappingHandle = nullptr;
    GLuint spoutMappingTexture = -1;
    Mapping spoutMappingType = Mapping::Cubemap;
    std::string spoutMappingName = "SPOUT_OS_MAPPING";
    glm::vec3 spoutRigOrientation = glm::vec3(0.f);

    int spoutMappingWidth;
    int spoutMappingHeight;
};

} // namespace sgct_core

#endif // __SGCT__SPOUTOUTPUT_PROJECTION__H__
