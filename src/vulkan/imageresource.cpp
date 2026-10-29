/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include "imageresource.h"

#include <sgct/error.h>
#include <sgct/vulkan/sceneformat.h>
#include <format>
#include <string>
#include <utility>

namespace sgct::vulkan {

namespace {
    void check(VkResult result, const std::string& what) {
        if (result != VK_SUCCESS) {
            throw Error(
                Error::Component::Window,
                4402,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    }

    uint32_t findMemoryType(VkPhysicalDevice device, uint32_t typeFilter,
                            VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(device, &memProps);
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProps.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        throw Error(
            Error::Component::Window,
            4403,
            "Failed to find a suitable memory type for a render target"
        );
    }

    bool isDepthFormat(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D16_UNORM ||
            format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT_S8_UINT;
    }
} // namespace

ImageResource::ImageResource(Context& context, const Settings& settings)
    : _context(&context)
    , _format(settings.format)
    , _aspect(isDepthFormat(settings.format) ?
        VK_IMAGE_ASPECT_DEPTH_BIT :
        VK_IMAGE_ASPECT_COLOR_BIT
    )
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = settings.isCube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { settings.extent.width, settings.extent.height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = settings.isCube ? 6 : settings.layers;
    imageInfo.format = settings.format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = settings.usage;
    imageInfo.samples = settings.samples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    check(
        vkCreateImage(_context->device(), &imageInfo, nullptr, &_image),
        "vkCreateImage"
    );

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(_context->device(), _image, &memReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        _context->physicalDevice(),
        memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    check(
        vkAllocateMemory(_context->device(), &allocInfo, nullptr, &_memory),
        "vkAllocateMemory"
    );
    check(
        vkBindImageMemory(_context->device(), _image, _memory, 0),
        "vkBindImageMemory"
    );

    const VkImageViewType type = settings.isCube ?
        VK_IMAGE_VIEW_TYPE_CUBE :
        (settings.layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D);
    _view = createView(type, 0, imageInfo.arrayLayers);
}

ImageResource::~ImageResource() {
    destroy();
}

ImageResource::ImageResource(ImageResource&& rhs) noexcept
    : _context(rhs._context)
    , _image(std::exchange(rhs._image, VK_NULL_HANDLE))
    , _memory(std::exchange(rhs._memory, VK_NULL_HANDLE))
    , _view(std::exchange(rhs._view, VK_NULL_HANDLE))
    , _format(rhs._format)
    , _aspect(rhs._aspect)
{}

ImageResource& ImageResource::operator=(ImageResource&& rhs) noexcept {
    if (this != &rhs) {
        destroy();
        _context = rhs._context;
        _image = std::exchange(rhs._image, VK_NULL_HANDLE);
        _memory = std::exchange(rhs._memory, VK_NULL_HANDLE);
        _view = std::exchange(rhs._view, VK_NULL_HANDLE);
        _format = rhs._format;
        _aspect = rhs._aspect;
    }
    return *this;
}

void ImageResource::destroy() {
    if (!_context) {
        return;
    }
    if (_view != VK_NULL_HANDLE) {
        vkDestroyImageView(_context->device(), _view, nullptr);
        _view = VK_NULL_HANDLE;
    }
    if (_image != VK_NULL_HANDLE) {
        vkDestroyImage(_context->device(), _image, nullptr);
        _image = VK_NULL_HANDLE;
    }
    if (_memory != VK_NULL_HANDLE) {
        vkFreeMemory(_context->device(), _memory, nullptr);
        _memory = VK_NULL_HANDLE;
    }
}

VkImageView ImageResource::createView(VkImageViewType type, uint32_t baseLayer,
                                      uint32_t layerCount) const
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _image;
    viewInfo.viewType = type;
    viewInfo.format = _format;
    viewInfo.subresourceRange.aspectMask = _aspect;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = baseLayer;
    viewInfo.subresourceRange.layerCount = layerCount;

    VkImageView view = VK_NULL_HANDLE;
    check(
        vkCreateImageView(_context->device(), &viewInfo, nullptr, &view),
        "vkCreateImageView"
    );
    return view;
}

void recordClear(VkCommandBuffer cmd, const ImageResource& image, uint32_t layers,
                 const VkClearValue& clear)
{
    VkImageSubresourceRange range = {};
    range.aspectMask = image.aspect();
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = layers;

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image.image();
    barrier.subresourceRange = range;
    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier
    );

    if (image.aspect() & VK_IMAGE_ASPECT_DEPTH_BIT) {
        vkCmdClearDepthStencilImage(
            cmd,
            image.image(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            &clear.depthStencil,
            1,
            &range
        );
    }
    else {
        vkCmdClearColorImage(
            cmd,
            image.image(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            &clear.color,
            1,
            &range
        );
    }

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier
    );
}

} // namespace sgct::vulkan
