/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__SPOUTOUTPUT_PROJECTION__H__
#define __SGCT__SPOUTOUTPUT_PROJECTION__H__

#include <sgct/nonlinearprojection.h>

#include <sgct/offscreenbuffer.h>
#include <sgct/ogl_headers.h>
#include <glm/glm.hpp>
#include <array>
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

    void setSpoutChannels(bool right, bool zLeft, bool bottom, bool top, bool left,
        bool zRight);
    void setSpoutMappingName(std::string name);
    void setSpoutMapping(Mapping type);
    void setSpoutRigOrientation(glm::vec3 orientation);

    /// Update projection when aspect ratio changes for the viewport.
    void update(glm::vec2 size) override;

    /// Render the non linear projection to currently bounded FBO
    void render() override;

    /// Render the enabled faces of the cubemap
    void renderCubemap() override;

    static const int NFaces = 6;
    inline static const std::array<const char*, 6> CubeMapFaceName = {
        "Right",
        "zLeft",
        "Bottom",
        "Top",
        "Left",
        "zRight"
    };

private:
    void initTextures() override;
    void initViewports() override;
    void initShaders() override;
    void initFBO() override;

    void drawCubeFace(int face);
    void blitCubeFace(int face);
    void attachTextures(int face);

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
    std::array<SpoutInfo, NFaces> _spout;

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
