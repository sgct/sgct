/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VULKAN_FRAMEBUFFER__H__
#define __SGCT__VULKAN_FRAMEBUFFER__H__

#include <sgct/sgctexports.h>
#include <sgct/vulkan/context.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <vector>

namespace sgct {
namespace vulkan {

/**
 * Owns a render pass and a set of framebuffers. It creates a render pass with a color
 * attachment (the swapchain images) and a shared depth attachment, and one framebuffer
 * per provided color attachment image view.
 */
class SGCT_EXPORT Framebuffer {
public:
    /**
     * Creates a color + depth render pass and one framebuffer per color attachment.
     *
     * \param context The shared Vulkan context
     * \param format The format of the color attachment (must match the image views)
     * \param attachments The color attachment image views (one per framebuffer)
     * \param extent The size of the framebuffers
     */
    Framebuffer(Context& context, VkFormat format,
        const std::vector<VkImageView>& attachments, VkExtent2D extent);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    VkRenderPass renderPass() const { return _renderPass; }
    VkFramebuffer framebuffer(uint32_t index) const { return _framebuffers.at(index); }
    uint32_t count() const { return static_cast<uint32_t>(_framebuffers.size()); }

private:
    void createDepthImage();

    Context& _context;
    VkRenderPass _renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> _framebuffers;

    // A single depth image shared by all framebuffers. It is cleared at the start of
    // every frame (loadOp = CLEAR) and all rendering is submitted to one queue in order,
    // so one depth buffer serves every in-flight frame
    VkImage _depthImage = VK_NULL_HANDLE;
    VkDeviceMemory _depthMemory = VK_NULL_HANDLE;
    VkImageView _depthImageView = VK_NULL_HANDLE;
    VkExtent2D _extent = {};
};

} // namespace vulkan
} // namespace sgct

#endif // __SGCT__VULKAN_FRAMEBUFFER__H__
