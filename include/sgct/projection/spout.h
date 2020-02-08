/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__SPOUT__H__
#define __SGCT__SPOUT__H__

#include <sgct/projection/nonlinearprojection.h>

#include <sgct/callbackdata.h>
#include <array>
#include <memory>

namespace sgct {

class OffScreenBuffer;

/// This class manages and renders non linear fisheye projections
class SpoutOutputProjection : public NonLinearProjection {
public:
    enum class Mapping { Fisheye, Equirectangular, Cubemap };

    SpoutOutputProjection(const Window* parent);
    virtual ~SpoutOutputProjection();

    void setSpoutChannels(bool right, bool zLeft, bool bottom, bool top, bool left,
        bool zRight);
    void setSpoutMappingName(std::string name);
    void setSpoutMapping(Mapping type);
    void setSpoutRigOrientation(vec3 orientation);

    /// Update projection when aspect ratio changes for the viewport.
    void update(vec2 size) override;

    /// Render the non linear projection to currently bounded FBO
    void render(const Window& window, const BaseViewport& viewport,
        Frustum::Mode frustumMode) override;

    /// Render the enabled faces of the cubemap
    void renderCubemap(Window& window, Frustum::Mode frustumMode) override;

private:
    static const int NFaces = 6;

    void initTextures() override;
    void initVBO() override;
    void initViewports() override;
    void initShaders() override;
    void initFBO() override;

    void drawCubeFace(BaseViewport& viewport, RenderData renderData);
    void blitCubeFace(int face);
    void attachTextures(int face);

    // shader locations
    int _cubemapLoc = -1;
    int _halfFovLoc = -1;
    int _swapColorLoc = -1;
    int _swapDepthLoc = -1;
    int _swapNearLoc = -1;
    int _swapFarLoc = -1;

    std::unique_ptr<OffScreenBuffer> _spoutFBO;

    struct SpoutInfo {
        bool enabled = true;
        void* handle = nullptr;
        unsigned int texture = 0;
    };
    std::array<SpoutInfo, NFaces> _spout;

    void* _mappingHandle = nullptr;
    unsigned int _mappingTexture = 0;
    Mapping _mappingType = Mapping::Cubemap;
    std::string _mappingName = "SPOUT_OS_MAPPING";
    vec3 _rigOrientation = vec3{ 0.f, 0.f, 0.f };

    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    ShaderProgram _shader;
    ShaderProgram _depthCorrectionShader;

    int _mappingWidth;
    int _mappingHeight;
};

} // namespace sgct

#endif // __SGCT__SPOUT__H__
