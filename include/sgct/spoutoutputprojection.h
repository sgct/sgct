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

namespace sgct::core {

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

    void setSpoutChannels(const bool channels[6]);
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
    int _cubemapLoc = -1;
    int _halfFovLoc = -1;
    int _swapColorLoc = -1;
    int _swapDepthLoc = -1;
    int _swapNearLoc = -1;
    int _swapFarLoc = -1;

    std::unique_ptr<OffScreenBuffer> _spoutFBO = nullptr;

    struct SpoutInfo {
        bool enabled = true;
        void* handle = nullptr;
        GLuint texture = 0;
    };
    SpoutInfo _spout[NFaces];

    void* _mappingHandle = nullptr;
    GLuint _mappingTexture = 0;
    Mapping _mappingType = Mapping::Cubemap;
    std::string _mappingName = "SPOUT_OS_MAPPING";
    glm::vec3 _rigOrientation = glm::vec3(0.f);

    int _mappingWidth;
    int _mappingHeight;
};

} // namespace sgct::core

#endif // __SGCT__SPOUTOUTPUT_PROJECTION__H__
