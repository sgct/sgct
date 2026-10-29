/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VULKAN_CUBEMAPTARGET__H__
#define __SGCT__VULKAN_CUBEMAPTARGET__H__

#include <sgct/sgctexports.h>
#include <sgct/vulkan/context.h>
#include <sgct/vulkan/sceneformat.h>

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <memory>

namespace sgct {
namespace vulkan {

class OffscreenBuffer;
class ProjectionPass;

/**
 * The render target of the cube map that the non-linear projections render the scene
 * into. It has a cube-compatible image with six layers for each single-sampled
 * attachment of its SceneFormat (the color, the normals and positions, and the depth if
 * it is a texture), a scene render pass (see createSceneRenderPass), and one framebuffer
 * per face. The faces are numbered like the OpenGL cube map faces (+X, -X, +Y, -Y, +Z,
 * -Z).
 *
 * The faces are rendered with the same y-flipped viewport as the rest of the scene, so
 * their rows are stored upside down compared to the OpenGL cube map. To compensate, the
 * images of the +Y and -Y faces are stored in each other's layer, which turns the cube
 * map into the mirror image of the OpenGL one along the y axis:
 *   - Sampling a cube view with the direction (x, -y, z) returns what sampling the
 *     OpenGL cube map with (x, y, z) returned
 *   - Sampling a face view at (s, 1 - t) returns what sampling the OpenGL face texture at
 *     (s, t) returned
 *
 * If the format has a depth texture, each face is rendered into a separate target first
 * and then copied into the cube map, which converts the depth from the distance to the
 * plane of the face into the distance to the center of the cube. That way, the depth is
 * continuous across the faces.
 *
 * All layers are cleared and in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL after the
 * construction, so faces that are never rendered can still be sampled.
 */
class SGCT_EXPORT CubemapTarget {
public:
    /// The single-sampled images of the cube map
    enum class Attachment { Color = 0, Normals, Positions, Depth };

    /**
     * \param context The shared Vulkan context
     * \param resolution The width and height of each face in pixels
     * \param format The attachments of the cube map; it has to be the format of the
     *        window's render target so that the draw callback's pipelines can be used
     *
     * \throw sgct::Error If a Vulkan call fails
     */
    CubemapTarget(Context& context, uint32_t resolution, const SceneFormat& format);
    ~CubemapTarget();

    CubemapTarget(const CubemapTarget&) = delete;
    CubemapTarget& operator=(const CubemapTarget&) = delete;

    /// The layer of the images that the provided face is stored in
    static uint32_t layer(int face);

    /**
     * Records the beginning of the scene render pass of \p face, which clears the face.
     */
    void beginFace(VkCommandBuffer cmd, int face) const;

    /**
     * Records the end of the render pass that #beginFace began. If the format has a depth
     * texture, the face is then copied into the cube map, using the clipping planes to
     * convert the depth.
     */
    void endFace(VkCommandBuffer cmd, int face, float nearClip, float farClip) const;

    /**
     * Records the copy of the color attachment of \p face into \p buffer, which has to be
     * at least resolution() * resolution() * 4 bytes large. The face has to be in the
     * layout that #endFace left it in (VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL), and is
     * returned to that layout. The pixels are made visible to the host, so the buffer can
     * be read once the command buffer that contains this copy has completed.
     */
    void copyFace(VkCommandBuffer cmd, int face, VkBuffer buffer) const;

    /// The scene render pass that the faces are rendered with
    VkRenderPass renderPass() const { return _renderPass; }

    /// Whether the cube map has the provided attachment
    bool has(Attachment attachment) const;

    /// A cube view of an attachment (to be sampled with a mirrored y direction)
    VkImageView cubeView(Attachment attachment = Attachment::Color) const;

    /// A 2D array view of all six layers of an attachment (indexed by #layer)
    VkImageView arrayView(Attachment attachment = Attachment::Color) const;

    /// A 2D view of a single face (to be sampled with a mirrored t coordinate)
    VkImageView faceView(int face, Attachment attachment = Attachment::Color) const;

    /// A linear, clamp-to-edge sampler for the views
    VkSampler sampler() const { return _sampler; }

    uint32_t resolution() const { return _resolution; }
    VkExtent2D extent() const { return { _resolution, _resolution }; }
    const SceneFormat& format() const { return _format; }

private:
    void createDirectFramebuffers();
    void createCorrection();

    Context& _context;
    const uint32_t _resolution;
    const SceneFormat _format;

    struct Images;
    std::unique_ptr<Images> _images;

    VkSampler _sampler = VK_NULL_HANDLE;
    VkRenderPass _renderPass = VK_NULL_HANDLE;
    /// The framebuffers of the faces if they are rendered into the cube map directly
    std::array<VkFramebuffer, 6> _framebuffers = {};

    // The depth correction: the faces are rendered into the swap target and then copied
    // into the cube map with the correction pass
    std::unique_ptr<OffscreenBuffer> _swap;
    VkRenderPass _correctionRenderPass = VK_NULL_HANDLE;
    std::array<VkFramebuffer, 6> _correctionFramebuffers = {};
    std::unique_ptr<ProjectionPass> _correctionPass;
};

} // namespace vulkan
} // namespace sgct

#endif // __SGCT__VULKAN_CUBEMAPTARGET__H__
