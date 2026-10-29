/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VULKAN_OFFSCREENBUFFER__H__
#define __SGCT__VULKAN_OFFSCREENBUFFER__H__

#include <sgct/sgctexports.h>
#include <sgct/vulkan/context.h>
#include <sgct/vulkan/sceneformat.h>

#include <vulkan/vulkan.h>

#include <memory>

namespace sgct {
namespace vulkan {

class ImageResource;

/**
 * An offscreen render target with the attachments of a SceneFormat, a scene render pass
 * (see createSceneRenderPass), and a single framebuffer. The scene is rendered into the
 * render pass; afterwards the single-sampled images are in
 * VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL so that they can be sampled, for example by
 * the composite pass that draws the warp meshes into the swapchain image.
 *
 * It is recreated whenever the window is resized, so there is no resize method.
 */
class SGCT_EXPORT OffscreenBuffer {
public:
    /**
     * \param context The shared Vulkan context
     * \param extent The size of the render target
     * \param format The attachments of the render target
     */
    OffscreenBuffer(Context& context, VkExtent2D extent, const SceneFormat& format);
    ~OffscreenBuffer();

    OffscreenBuffer(const OffscreenBuffer&) = delete;
    OffscreenBuffer& operator=(const OffscreenBuffer&) = delete;

    /**
     * Records the beginning of the render pass on the whole target, which clears all
     * attachments (see sceneClearValues).
     */
    void begin(VkCommandBuffer cmd) const;

    VkRenderPass renderPass() const { return _renderPass; }
    VkFramebuffer framebuffer() const { return _framebuffer; }
    VkExtent2D extent() const { return _extent; }
    const SceneFormat& format() const { return _format; }
    VkFormat colorFormat() const { return _format.color; }

    /// The single-sampled color image
    VkImage colorImage() const;
    VkImageView colorImageView() const;

    /// The normals, or VK_NULL_HANDLE if the format has no normals
    VkImageView normalsImageView() const;

    /// The positions, or VK_NULL_HANDLE if the format has no positions
    VkImageView positionsImageView() const;

    /// The single-sampled depth, or VK_NULL_HANDLE if the format has no depth texture
    VkImageView depthImageView() const;

private:
    Context& _context;
    const VkExtent2D _extent;
    const SceneFormat _format;

    struct Images;
    std::unique_ptr<Images> _images;

    VkRenderPass _renderPass = VK_NULL_HANDLE;
    VkFramebuffer _framebuffer = VK_NULL_HANDLE;
};

} // namespace vulkan
} // namespace sgct

#endif // __SGCT__VULKAN_OFFSCREENBUFFER__H__
