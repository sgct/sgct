/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/vulkan/texture.h>

#include <sgct/error.h>
#include <sgct/image.h>
#include <sgct/vulkan/buffer.h>
#include <algorithm>
#include <bit>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>

namespace sgct {
namespace vulkan {

namespace {
    void check(VkResult result, const std::string& what) {
        if (result != VK_SUCCESS) {
            throw Error(
                Error::Component::Window,
                4500,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    }

    uint32_t findMemoryType(
        const Context& context, uint32_t typeFilter, VkMemoryPropertyFlags props)
    {
        VkPhysicalDeviceMemoryProperties memoryProperties = {};
        vkGetPhysicalDeviceMemoryProperties(context.physicalDevice(), &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memoryProperties.memoryTypes[i].propertyFlags & props) == props)
            {
                return i;
            }
        }
        throw Error(
            Error::Component::Window, 4501, "Failed to find a suitable Vulkan memory type"
        );
    }

    struct LayoutState {
        VkImageLayout layout;
        VkPipelineStageFlags stage;
        VkAccessFlags access;
    };
    constexpr LayoutState Undefined = {
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        0
    };
    constexpr LayoutState TransferDst = {
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT
    };
    constexpr LayoutState TransferSrc = {
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_TRANSFER_READ_BIT
    };
    constexpr LayoutState ShaderRead = {
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_ACCESS_SHADER_READ_BIT
    };

    void barrier(VkCommandBuffer cmd, VkImage image, uint32_t baseLevel,
                 uint32_t levelCount, const LayoutState& from, const LayoutState& to)
    {
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = from.access;
        barrier.dstAccessMask = to.access;
        barrier.oldLayout = from.layout;
        barrier.newLayout = to.layout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = {
            VK_IMAGE_ASPECT_COLOR_BIT, baseLevel, levelCount, 0, 1
        };
        vkCmdPipelineBarrier(
            cmd,
            from.stage, to.stage, 0,
            0, nullptr, 0, nullptr, 1, &barrier
        );
    }
} // namespace

uint32_t Texture::bytesPerPixel(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:
            return 4;
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_UNORM:
        case VK_FORMAT_B8G8R8_SRGB:
            return 3;
        case VK_FORMAT_R8G8_UNORM:
            return 2;
        case VK_FORMAT_R8_UNORM:
            return 1;
        default:
            throw Error(
                Error::Component::Window,
                4502,
                std::format("Unsupported texture format: {}", static_cast<int>(format))
            );
    }
}

std::unique_ptr<Texture> Texture::fromImage(Context& context, const Image& image) {
    const int channels = image.channels();
    if (image.bytesPerChannel() != 1 || channels < 1 || channels > 4 ||
        image.size().x <= 0 || image.size().y <= 0 || !image.data())
    {
        throw Error(
            Error::Component::Window,
            4503,
            std::format(
                "Unsupported image for a texture ({}x{}, {} channels, {} bytes/channel)",
                image.size().x, image.size().y, channels, image.bytesPerChannel()
            )
        );
    }

    // Image stores 3- and 4-channel pixels in BGR(A) order, so the expanded pixels are
    // BGRA as well
    const size_t nPixels = static_cast<size_t>(image.size().x) * image.size().y;
    const unsigned char* src = image.data();
    std::vector<uint8_t> pixels(nPixels * 4);
    for (size_t i = 0; i < nPixels; i++) {
        const unsigned char* p = src + i * channels;
        uint8_t* d = pixels.data() + i * 4;
        switch (channels) {
            case 1:
                d[0] = p[0]; d[1] = p[0]; d[2] = p[0]; d[3] = 255;
                break;
            case 2:
                d[0] = p[0]; d[1] = p[0]; d[2] = p[0]; d[3] = p[1];
                break;
            case 3:
                d[0] = p[0]; d[1] = p[1]; d[2] = p[2]; d[3] = 255;
                break;
            case 4:
                d[0] = p[0]; d[1] = p[1]; d[2] = p[2]; d[3] = p[3];
                break;
        }
    }

    return std::make_unique<Texture>(
        context,
        static_cast<uint32_t>(image.size().x),
        static_cast<uint32_t>(image.size().y),
        VK_FORMAT_B8G8R8A8_UNORM,
        pixels.data()
    );
}

Texture::Texture(Context& context, uint32_t width, uint32_t height, VkFormat format,
                 const uint8_t* data, VkFilter filter, uint32_t mipLevels)
    : _context(context)
    , _width(width)
    , _height(height)
    , _format(format)
{
    const VkDeviceSize imageSize =
        static_cast<VkDeviceSize>(width) * height * bytesPerPixel(format);

    // The levels are generated by blitting each level into the next smaller one
    const uint32_t maxLevels = std::bit_width(std::max(std::max(width, height), 1u));
    _mipLevels = std::clamp(mipLevels, 1u, maxLevels);
    if (_mipLevels > 1) {
        VkFormatProperties properties = {};
        vkGetPhysicalDeviceFormatProperties(
            _context.physicalDevice(),
            format,
            &properties
        );
        constexpr VkFormatFeatureFlags Required = VK_FORMAT_FEATURE_BLIT_SRC_BIT |
            VK_FORMAT_FEATURE_BLIT_DST_BIT |
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
        if ((properties.optimalTilingFeatures & Required) != Required) {
            _mipLevels = 1;
        }
    }

    // Staging buffer holding the host pixels
    Buffer staging(_context, imageSize, Buffer::Type::Staging);
    staging.upload(data, imageSize);

    // Device-local image
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { width, height, 1 };
    imageInfo.mipLevels = _mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        (_mipLevels > 1 ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0);
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    check(vkCreateImage(_context.device(), &imageInfo, nullptr, &_image),
        "vkCreateImage");

    VkMemoryRequirements requirements = {};
    vkGetImageMemoryRequirements(_context.device(), _image, &requirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = requirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        _context, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    check(vkAllocateMemory(_context.device(), &allocInfo, nullptr, &_memory),
        "vkAllocateMemory");
    check(vkBindImageMemory(_context.device(), _image, _memory, 0),
        "vkBindImageMemory");

    // View
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange = {
        VK_IMAGE_ASPECT_COLOR_BIT, 0, _mipLevels, 0, 1
    };

    check(vkCreateImageView(_context.device(), &viewInfo, nullptr, &_imageView),
        "vkCreateImageView");

    // Sampler: clamp to edge
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = filter;
    samplerInfo.minFilter = filter;
    samplerInfo.mipmapMode = _mipLevels > 1 ?
        VK_SAMPLER_MIPMAP_MODE_LINEAR :
        VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(_mipLevels - 1);
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    check(vkCreateSampler(_context.device(), &samplerInfo, nullptr, &_sampler),
        "vkCreateSampler");

    // One-time copy: UNDEFINED -> TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL. Each
    // mipmap level is blitted from the previous one, which is TRANSFER_SRC_OPTIMAL then
    VkCommandBufferAllocateInfo cmdAlloc = {};
    cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAlloc.commandPool = _context.commandPool();
    cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAlloc.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    check(
        vkAllocateCommandBuffers(_context.device(), &cmdAlloc, &commandBuffer),
        "vkAllocateCommandBuffers"
    );

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(vkBeginCommandBuffer(commandBuffer, &beginInfo), "vkBeginCommandBuffer");

    barrier(commandBuffer, _image, 0, _mipLevels, Undefined, TransferDst);

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource = {
        VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1
    };
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };
    vkCmdCopyBufferToImage(
        commandBuffer, staging.handle(), _image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region
    );

    for (uint32_t level = 1; level < _mipLevels; level++) {
        barrier(commandBuffer, _image, level - 1, 1, TransferDst, TransferSrc);

        VkImageBlit blit = {};
        blit.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, 1 };
        blit.srcOffsets[1] = {
            static_cast<int32_t>(std::max(width >> (level - 1), 1u)),
            static_cast<int32_t>(std::max(height >> (level - 1), 1u)),
            1
        };
        blit.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1 };
        blit.dstOffsets[1] = {
            static_cast<int32_t>(std::max(width >> level, 1u)),
            static_cast<int32_t>(std::max(height >> level, 1u)),
            1
        };
        vkCmdBlitImage(
            commandBuffer,
            _image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR
        );
    }
    if (_mipLevels > 1) {
        barrier(commandBuffer, _image, 0, _mipLevels - 1, TransferSrc, ShaderRead);
    }
    barrier(commandBuffer, _image, _mipLevels - 1, 1, TransferDst, ShaderRead);

    check(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer");

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    check(vkQueueSubmit(_context.queue(), 1, &submitInfo, VK_NULL_HANDLE),
        "vkQueueSubmit");
    check(vkQueueWaitIdle(_context.queue()), "vkQueueWaitIdle");

    vkFreeCommandBuffers(
        _context.device(), _context.commandPool(), 1, &commandBuffer
    );
}

Texture::~Texture() {
    if (_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(_context.device(), _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    }
    if (_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(_context.device(), _imageView, nullptr);
        _imageView = VK_NULL_HANDLE;
    }
    if (_image != VK_NULL_HANDLE) {
        vkDestroyImage(_context.device(), _image, nullptr);
        _image = VK_NULL_HANDLE;
    }
    if (_memory != VK_NULL_HANDLE) {
        vkFreeMemory(_context.device(), _memory, nullptr);
        _memory = VK_NULL_HANDLE;
    }
}

} // namespace vulkan
} // namespace sgct
