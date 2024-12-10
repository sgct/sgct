/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__SPOUT__H__
#define __SGCT__SPOUT__H__

#include <sgct/sgctexports.h>
#include <sgct/projection/nonlinearprojection.h>

#include <sgct/callbackdata.h>
#include <array>
#include <memory>

namespace sgct {

class OffScreenBuffer;

/**
 * This class manages and renders non-linear fisheye projections.
 */
class SGCT_EXPORT SpoutOutputProjection : public NonLinearProjection {
public:
    enum class Mapping { Fisheye, Equirectangular, Cubemap };

    SpoutOutputProjection(const Window* parent);
    virtual ~SpoutOutputProjection() override;

    void setSpoutChannels(bool right, bool zLeft, bool bottom, bool top, bool left,
        bool zRight);
    void setSpoutDrawMain(bool drawMain);
    void setSpoutMappingName(std::string name);
    void setSpoutMapping(Mapping type);
    void setSpoutRigOrientation(vec3 orientation);

    /**
     * Update projection when aspect ratio changes for the viewport.
     */
    void update(vec2 size) override;

    /**
     * Render the non linear projection to currently bounded FBO.
     */
    void render(const Window& window, const BaseViewport& viewport,
        Frustum::Mode frustumMode) override;

    /**
     * Render the enabled faces of the cubemap.
     */
    void renderCubemap(Window& window, Frustum::Mode frustumMode) override;

    void updateFrustums(Frustum::Mode mode, float nearClip, float farClip) override;
    void setUser(User* user) override;

private:
    static constexpr int NTextures = 7;
    static constexpr int NFaces = 6;

    void initTextures() override;
    void initVBO() override;
    void initViewports() override;
    void initShaders() override;
    void initFBO() override;

    // shader locations
    struct {
        int cubemap = -1;
        int depthCubemap = -1;
        int normalCubemap = -1;
        int positionCubemap = -1;
        int halfFov = -1;
        int swapColor = -1;
        int swapDepth = -1;
        int swapNear = -1;
        int swapFar = -1;
    } _shaderLoc;

    std::unique_ptr<OffScreenBuffer> _spoutFBO;

    struct SpoutInfo {
        bool enabled = true;
        void* handle = nullptr;
        unsigned int texture = 0;
    };
    std::array<SpoutInfo, NTextures> _spout;

    void* _mappingHandle = nullptr;
    unsigned int _mappingTexture = 0;
    Mapping _mappingType = Mapping::Cubemap;
    std::string _mappingName = "SPOUT_OS_MAPPING";
    vec3 _rigOrientation = vec3{ 0.f, 0.f, 0.f };

    BaseViewport _mainViewport;

    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    ShaderProgram _shader;
    ShaderProgram _flatShader;
    ShaderProgram _depthCorrectionShader;

    int _mappingWidth = 0;
    int _mappingHeight = 0;

    unsigned int _blitFbo = 0;
};

} // namespace sgct

#endif // __SGCT__SPOUT__H__
