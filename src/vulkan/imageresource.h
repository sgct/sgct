/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VULKAN_IMAGERESOURCE__H__
#define __SGCT__VULKAN_IMAGERESOURCE__H__

#include <sgct/vulkan/context.h>

#include <vulkan/vulkan.h>

#include <cstdint>

// Internal helpers of the render targets; not part of the public API

namespace sgct::vulkan {

/**
 * A device-local image with its memory and a view of all its layers. The image is owned
 * and destroyed with the object.
 */
class ImageResource {
public:
    struct Settings {
        VkExtent2D extent = {};
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkImageUsageFlags usage = 0;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        uint32_t layers = 1;
        /// Whether the image has six layers and the view is a cube view
        bool isCube = false;
    };

    ImageResource() = default;
    ImageResource(Context& context, const Settings& settings);
    ~ImageResource();

    ImageResource(ImageResource&& rhs) noexcept;
    ImageResource& operator=(ImageResource&& rhs) noexcept;
    ImageResource(const ImageResource&) = delete;
    ImageResource& operator=(const ImageResource&) = delete;

    /**
     * Creates an additional view of the image, which the caller owns.
     */
    VkImageView createView(VkImageViewType type, uint32_t baseLayer,
        uint32_t layerCount) const;

    VkImage image() const { return _image; }
    VkImageView view() const { return _view; }
    VkFormat format() const { return _format; }
    VkImageAspectFlags aspect() const { return _aspect; }
    explicit operator bool() const { return _image != VK_NULL_HANDLE; }

private:
    void destroy();

    Context* _context = nullptr;
    VkImage _image = VK_NULL_HANDLE;
    VkDeviceMemory _memory = VK_NULL_HANDLE;
    VkImageView _view = VK_NULL_HANDLE;
    VkFormat _format = VK_FORMAT_UNDEFINED;
    VkImageAspectFlags _aspect = 0;
};

/**
 * Records a barrier that moves all layers of \p image from VK_IMAGE_LAYOUT_UNDEFINED
 * into VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL after clearing them to \p clear, so that
 * images that are not rendered to can be sampled.
 */
void recordClear(VkCommandBuffer cmd, const ImageResource& image, uint32_t layers,
    const VkClearValue& clear);

/**
 * Records the commands of \p record into a one-time command buffer, submits it, and
 * waits for it to finish.
 */
template <typename F>
void submitImmediately(Context& context, F record);

} // namespace sgct::vulkan

#include "imageresource.inl"

#endif // __SGCT__VULKAN_IMAGERESOURCE__H__
