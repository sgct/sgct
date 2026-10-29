/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CORRECTION_MESH__H__
#define __SGCT__CORRECTION_MESH__H__

#include <sgct/sgctexports.h>

#include <vulkan/vulkan.h>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>

namespace sgct {

namespace correction { struct Buffer; }
namespace vulkan { class Buffer; }
class BaseViewport;

/**
 * Helper class for reading and rendering a correction mesh. A correction mesh is used for
 * warping and edge-blending.
 */
class SGCT_EXPORT CorrectionMesh {
public:
    /**
     * This function finds a suitable parser for warping meshes and loads them.
     *
     * \param path The path to the mesh data
     * \param parent The pointer to parent viewport
     * \param needsMaskGeometry If `true`, a separate geometry to applying blend masks is
     *        loaded
     * \param textureRenderMode Only used if the loaded mesh is a `pfm` mesh. If `true`
     *        a single mesh for one eye is loaded from the file. If `false`, two meshes,
     *        one for the left and one for the right eye are loaded
     *
     * \throw std::runtime_error if mesh was not loaded successfully
     */
    void loadMesh(const std::filesystem::path& path, BaseViewport& parent,
        bool needsMaskGeometry = false, bool textureRenderMode = false);

    /**
     * Records the draw of the unwarped quad covering the parent viewport into \p cmd. A
     * pipeline consuming correction::Buffer::Vertex triangle lists must be bound.
     */
    void renderQuadMesh(VkCommandBuffer cmd) const;

    /**
     * Records the draw of the warp mesh, which maps the offscreen image to the screen,
     * into \p cmd. A pipeline consuming correction::Buffer::Vertex triangle lists must be
     * bound.
     */
    void renderWarpMesh(VkCommandBuffer cmd) const;

    /**
     * Records the draw of the unwarped mesh used for applying the blend masks into
     * \p cmd. A pipeline consuming correction::Buffer::Vertex triangle lists must be
     * bound.
     */
    void renderMaskMesh(VkCommandBuffer cmd) const;

private:
    /// A mesh uploaded as an indexed triangle list
    struct CorrectionMeshGeometry {
        explicit CorrectionMeshGeometry(const correction::Buffer& buffer);
        CorrectionMeshGeometry(CorrectionMeshGeometry&&) noexcept;
        ~CorrectionMeshGeometry();

        CorrectionMeshGeometry& operator=(CorrectionMeshGeometry&&) noexcept;

        void render(VkCommandBuffer cmd) const;

        std::unique_ptr<vulkan::Buffer> vertexBuffer;
        std::unique_ptr<vulkan::Buffer> indexBuffer;
        uint32_t nIndices = 0;
    };

    std::optional<CorrectionMeshGeometry> _quadGeometry;
    std::optional<CorrectionMeshGeometry> _warpGeometry;
    std::optional<CorrectionMeshGeometry> _maskGeometry;
};

} // namespace sgct

#endif // __SGCT__CORRECTION_MESH__H__
