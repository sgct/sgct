/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/vulkan/framebuffer.h>

#include <sgct/error.h>
#include <format>
#include <stdexcept>

namespace sgct {
namespace vulkan {

namespace {
    void check(VkResult result, const std::string& what) {
        if (result != VK_SUCCESS) {
            throw Error(
                Error::Component::Window,
                4400,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    }

    uint32_t findMemoryType(
        VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
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
            4401,
            "Failed to find a suitable memory type for the depth image"
        );
    }
} // namespace

Framebuffer::Framebuffer(Context& context, VkFormat format,
    const std::vector<VkImageView>& attachments, VkExtent2D extent)
    : _context(context)
    , _extent(extent)
{
    createDepthImage();

    // Color attachment: the swapchain image
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Depth attachment: a shared depth image, cleared every frame
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = {};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;
    subpass.pDepthStencilAttachment = &depthReference;

    // The swapchain image is only available once the image-available semaphore, which is
    // waited on at the color attachment output stage, is signaled. Make the layout
    // transition and the attachment writes wait for that stage. The shared depth image is
    // reused every frame, so its writes have to wait for the previous frame's as well
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // The swapchain image can be copied for a screenshot after the render pass, so its
    // writes and the transition to the final layout have to finish before the transfer
    VkSubpassDependency captureDependency = {};
    captureDependency.srcSubpass = 0;
    captureDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
    captureDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    captureDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    captureDependency.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    captureDependency.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    const VkSubpassDependency dependencies[2] = { dependency, captureDependency };

    // The composite uses a classic render pass with a color and a depth attachment.
    // Dynamic rendering (VK_KHR_dynamic_rendering) would remove the render pass and the
    // per-frame attachment setup, and would make resize cheaper because the pipeline
    // would no longer depend on the render pass. That is a future optimization; the
    // classic render pass is kept for now because it is well understood and the resize
    // cost is not a bottleneck for a CAVE tool
    VkAttachmentDescription attachmentsDesc[2] = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachmentsDesc;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = dependencies;

    check(
        vkCreateRenderPass(_context.device(), &renderPassInfo, nullptr, &_renderPass),
        "vkCreateRenderPass"
    );

    _framebuffers.resize(attachments.size());
    for (size_t i = 0; i < attachments.size(); i++) {
        VkImageView views[2] = { attachments[i], _depthImageView };
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = views;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        check(
            vkCreateFramebuffer(
                _context.device(), &framebufferInfo, nullptr, &_framebuffers[i]
            ),
            "vkCreateFramebuffer"
        );
    }
}

void Framebuffer::createDepthImage() {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { _extent.width, _extent.height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_D32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    check(vkCreateImage(_context.device(), &imageInfo, nullptr, &_depthImage),
        "vkCreateImage");

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(_context.device(), _depthImage, &memReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        _context.physicalDevice(), memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    check(vkAllocateMemory(_context.device(), &allocInfo, nullptr, &_depthMemory),
        "vkAllocateMemory");
    check(vkBindImageMemory(_context.device(), _depthImage, _depthMemory, 0),
        "vkBindImageMemory");

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_D32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;

    check(vkCreateImageView(_context.device(), &viewInfo, nullptr, &_depthImageView),
        "vkCreateImageView");
}

Framebuffer::~Framebuffer() {
    for (auto framebuffer : _framebuffers) {
        vkDestroyFramebuffer(_context.device(), framebuffer, nullptr);
    }
    if (_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(_context.device(), _renderPass, nullptr);
        _renderPass = VK_NULL_HANDLE;
    }
    if (_depthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(_context.device(), _depthImageView, nullptr);
        _depthImageView = VK_NULL_HANDLE;
    }
    if (_depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(_context.device(), _depthImage, nullptr);
        _depthImage = VK_NULL_HANDLE;
    }
    if (_depthMemory != VK_NULL_HANDLE) {
        vkFreeMemory(_context.device(), _depthMemory, nullptr);
        _depthMemory = VK_NULL_HANDLE;
    }
}

} // namespace vulkan
} // namespace sgct
