/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__SPOUTOUTPUT_PROJECTION__H__
#define __SGCT__SPOUTOUTPUT_PROJECTION__H__

#include <sgct/nonlinearprojection.h>

#include <sgct/offscreenbuffer.h>
#include <sgct/ogl_headers.h>
#include <glm/glm.hpp>
#include <memory>

namespace sgct_core {

/**
 * This class manages and renders non linear fisheye projections
 */
class SpoutOutputProjection : public NonLinearProjection {
public:
    SpoutOutputProjection() = default;
    virtual ~SpoutOutputProjection();

    enum class Mapping {
        Fisheye,
        Equirectangular,
        Cubemap
    };

    void setSpoutChannels(bool channels[6]);
    void setSpoutMappingName(std::string name);
    void setSpoutMapping(Mapping type);
    void setSpoutRigOrientation(glm::vec3 orientation);

    /// Update projection when aspect ratio changes for the viewport.
    virtual void update(glm::vec2 size) override;

    /// Render the non linear projection to currently bounded FBO
    virtual void render() override;

    /// Render the enabled faces of the cubemap
    virtual void renderCubemap(size_t* subViewPortIndex) override;

    static const int NFaces = 6;
    inline static const char* CubeMapFaceName[] = {
        "Right",
        "zLeft",
        "Bottom",
        "Top",
        "Left",
        "zRight"
    };

private:
    virtual void initTextures() override;
    virtual void initViewports() override;
    virtual void initShaders() override;
    virtual void initFBO() override;

    void drawCubeFace(int face);
    void blitCubeFace(int face);
    void attachTextures(int face);
    void renderInternal();
    void renderInternalFixedPipeline();
    void renderCubemapInternal(size_t* subViewPortIndex);
    void renderCubemapInternalFixedPipeline(size_t* subViewPortIndex);

    // shader locations
    int mCubemapLoc = -1;
    int mHalfFovLoc = -1;
    int mSwapColorLoc = -1;
    int mSwapDepthLoc = -1;
    int mSwapNearLoc = -1;
    int mSwapFarLoc = -1;

    std::unique_ptr<OffScreenBuffer> mSpoutFBO = nullptr;

    struct SpoutInfo {
        bool enabled = true;
        void* handle = nullptr;
        GLuint texture = 0;
    };
    SpoutInfo mSpout[NFaces];

    void* mappingHandle = nullptr;
    GLuint mappingTexture = 0;
    Mapping mappingType = Mapping::Cubemap;
    std::string mappingName = "SPOUT_OS_MAPPING";
    glm::vec3 rigOrientation = glm::vec3(0.f);

    int mappingWidth;
    int mappingHeight;
};

} // namespace sgct_core

#endif // __SGCT__SPOUTOUTPUT_PROJECTION__H__
