/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VIEWPORT__H__
#define __SGCT__VIEWPORT__H__

#include <sgct/sgctexports.h>
#include <sgct/baseviewport.h>

#include <sgct/correctionmesh.h>
#include <sgct/window.h>
#include <sgct/vulkan/sceneformat.h>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace sgct {

namespace config { struct Viewport; }
namespace vulkan { class Texture; }
class NonLinearProjection;

/**
 * This class holds and manages viewportdata and calculates frustums.
 */
class SGCT_EXPORT Viewport final : public BaseViewport {
public:
    Viewport(const config::Viewport& viewport, const Window& parent);
    ~Viewport() override;

    /**
     * Initializes the non-linear projection of this viewport, if it has one. Requires
     * the Vulkan context.
     *
     * \param size The size of the viewport in pixels
     * \param hasStereo Whether the window renders in stereo
     * \param format The format of the window's render target
     */
    void initialize(vec2 size, bool hasStereo, const vulkan::SceneFormat& format);

    /**
     * Loads the overlay and mask textures and the correction meshes of this viewport.
     * Requires the Vulkan context.
     */
    void loadData();

    /**
     * Loads the correction meshes of this viewport. Requires the Vulkan context.
     */
    void loadMesh();

    void calculateFrustum(FrustumMode mode, float nearClip, float farClip) override;

    void renderQuadMesh(VkCommandBuffer cmd) const;
    void renderWarpMesh(VkCommandBuffer cmd) const;
    void renderMaskMesh(VkCommandBuffer cmd) const;

    bool hasOverlayTexture() const;
    bool hasBlendMaskTexture() const;
    bool hasBlackLevelMaskTexture() const;
    bool hasSubViewports() const;
    bool isTracked() const;
    /// The textures are `nullptr` if the viewport does not have them
    const vulkan::Texture* overlayTexture() const;
    const vulkan::Texture* blendMaskTexture() const;
    const vulkan::Texture* blackLevelMaskTexture() const;
    NonLinearProjection* nonLinearProjection() const;

private:
    CorrectionMesh _mesh;
    std::filesystem::path _overlayFilename;
    std::filesystem::path _blendMaskFilename;
    std::filesystem::path _blackLevelMaskFilename;
    std::filesystem::path _meshFilename;
    bool _isTracked;
    bool _useTextureMappedProjection = false;
    std::unique_ptr<vulkan::Texture> _overlayTexture;
    std::unique_ptr<vulkan::Texture> _blendMaskTexture;
    std::unique_ptr<vulkan::Texture> _blackLevelMaskTexture;

    std::unique_ptr<NonLinearProjection> _nonLinearProjection;
};

} // namespace sgct

#endif // __SGCT__VIEWPORT__H__
