/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/vulkan/offscreenbuffer.h>

#include "imageresource.h"
#include <sgct/error.h>
#include <format>
#include <string>
#include <vector>

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
} // namespace

struct OffscreenBuffer::Images {
    // The single-sampled images that are kept after the render pass
    ImageResource color;
    ImageResource normals;
    ImageResource positions;
    ImageResource depth;

    // The multisampled images that are resolved into the single-sampled ones
    ImageResource colorMultisampled;
    ImageResource normalsMultisampled;
    ImageResource positionsMultisampled;
    ImageResource depthMultisampled;
};

OffscreenBuffer::OffscreenBuffer(Context& context, VkExtent2D extent,
                                 const SceneFormat& format)
    : _context(context)
    , _extent(extent)
    , _format(format)
    , _images(std::make_unique<Images>())
{
    const bool isMultisampled = _format.samples != VK_SAMPLE_COUNT_1_BIT;

    auto create = [this](VkFormat f, VkImageUsageFlags usage, VkSampleCountFlagBits s) {
        ImageResource::Settings settings;
        settings.extent = _extent;
        settings.format = f;
        settings.usage = usage;
        settings.samples = s;
        return ImageResource(_context, settings);
    };

    constexpr VkImageUsageFlags Transient = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    constexpr VkImageUsageFlags Color = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    constexpr VkImageUsageFlags Depth = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    constexpr VkImageUsageFlags Sampled = VK_IMAGE_USAGE_SAMPLED_BIT;
    constexpr VkImageUsageFlags Kept =
        Sampled | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    // The color image is also copied from by the screen capture
    Images& img = *_images;
    img.color = create(_format.color, Color | Kept, VK_SAMPLE_COUNT_1_BIT);
    if (_format.hasNormals) {
        img.normals = create(SceneDataFormat, Color | Kept, VK_SAMPLE_COUNT_1_BIT);
    }
    if (_format.hasPositions) {
        img.positions = create(SceneDataFormat, Color | Kept, VK_SAMPLE_COUNT_1_BIT);
    }
    if (isMultisampled) {
        img.colorMultisampled = create(_format.color, Color | Transient, _format.samples);
        if (_format.hasNormals) {
            img.normalsMultisampled =
                create(SceneDataFormat, Color | Transient, _format.samples);
        }
        if (_format.hasPositions) {
            img.positionsMultisampled =
                create(SceneDataFormat, Color | Transient, _format.samples);
        }
        img.depthMultisampled = create(SceneDepthFormat, Depth | Transient, _format.samples);
        if (_format.hasDepthTexture) {
            img.depth = create(SceneDepthFormat, Depth | Kept, VK_SAMPLE_COUNT_1_BIT);
        }
    }
    else {
        img.depth = create(
            SceneDepthFormat,
            Depth | (_format.hasDepthTexture ? Kept : 0),
            VK_SAMPLE_COUNT_1_BIT
        );
    }

    _renderPass = createSceneRenderPass(_context, _format);

    // The views in the attachment order of createSceneRenderPass
    std::vector<VkImageView> views;
    if (isMultisampled) {
        views.push_back(img.colorMultisampled.view());
        if (_format.hasNormals) {
            views.push_back(img.normalsMultisampled.view());
        }
        if (_format.hasPositions) {
            views.push_back(img.positionsMultisampled.view());
        }
        views.push_back(img.depthMultisampled.view());
    }
    views.push_back(img.color.view());
    if (_format.hasNormals) {
        views.push_back(img.normals.view());
    }
    if (_format.hasPositions) {
        views.push_back(img.positions.view());
    }
    if (!isMultisampled || _format.hasDepthTexture) {
        views.push_back(img.depth.view());
    }

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = _renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(views.size());
    framebufferInfo.pAttachments = views.data();
    framebufferInfo.width = _extent.width;
    framebufferInfo.height = _extent.height;
    framebufferInfo.layers = 1;
    check(
        vkCreateFramebuffer(_context.device(), &framebufferInfo, nullptr, &_framebuffer),
        "vkCreateFramebuffer"
    );
}

OffscreenBuffer::~OffscreenBuffer() {
    if (_framebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(_context.device(), _framebuffer, nullptr);
    }
    if (_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(_context.device(), _renderPass, nullptr);
    }
}

void OffscreenBuffer::begin(VkCommandBuffer cmd) const {
    const std::vector<VkClearValue> clearValues = sceneClearValues(_format);
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _renderPass;
    renderPassInfo.framebuffer = _framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = _extent;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

VkImage OffscreenBuffer::colorImage() const {
    return _images->color.image();
}

VkImageView OffscreenBuffer::colorImageView() const {
    return _images->color.view();
}

VkImageView OffscreenBuffer::normalsImageView() const {
    return _images->normals.view();
}

VkImageView OffscreenBuffer::positionsImageView() const {
    return _images->positions.view();
}

VkImageView OffscreenBuffer::depthImageView() const {
    return _format.hasDepthTexture ? _images->depth.view() : VK_NULL_HANDLE;
}

} // namespace sgct::vulkan
