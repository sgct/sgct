/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/vulkan/sceneformat.h>

#include <sgct/error.h>
#include <array>
#include <format>
#include <string>

namespace sgct::vulkan {

namespace {
    void check(VkResult result, const std::string& what) {
        if (result != VK_SUCCESS) {
            throw Error(
                Error::Component::Window,
                4404,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    }
} // namespace

uint32_t SceneFormat::colorAttachmentCount() const {
    return hasPositions ? 3 : (hasNormals ? 2 : 1);
}

VkRenderPass createSceneRenderPass(Context& context, const SceneFormat& format) {
    const bool isMultisampled = format.samples != VK_SAMPLE_COUNT_1_BIT;

    std::vector<VkAttachmentDescription2> attachments;
    auto add = [&attachments](VkFormat f, VkSampleCountFlagBits samples,
                              VkAttachmentLoadOp load, VkAttachmentStoreOp store,
                              VkImageLayout finalLayout)
    {
        VkAttachmentDescription2 attachment = {};
        attachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
        attachment.format = f;
        attachment.samples = samples;
        attachment.loadOp = load;
        attachment.storeOp = store;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = finalLayout;
        attachments.push_back(attachment);
        return static_cast<uint32_t>(attachments.size() - 1);
    };

    // The rendered attachments. Without multisampling, they are kept; with
    // multisampling, they are resolved into single-sampled attachments that are kept
    const VkAttachmentStoreOp renderedStore =
        isMultisampled ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
    const VkImageLayout renderedLayout = isMultisampled ?
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL :
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    const uint32_t color = add(
        format.color,
        format.samples,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        renderedStore,
        renderedLayout
    );
    const uint32_t normals = format.hasNormals ?
        add(
            SceneDataFormat,
            format.samples,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            renderedStore,
            renderedLayout
        ) :
        VK_ATTACHMENT_UNUSED;
    const uint32_t positions = format.hasPositions ?
        add(
            SceneDataFormat,
            format.samples,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            renderedStore,
            renderedLayout
        ) :
        VK_ATTACHMENT_UNUSED;
    const bool keepDepth = format.hasDepthTexture && !isMultisampled;
    const uint32_t depth = add(
        SceneDepthFormat,
        format.samples,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        keepDepth ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE,
        keepDepth ?
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );

    uint32_t colorResolve = VK_ATTACHMENT_UNUSED;
    uint32_t normalsResolve = VK_ATTACHMENT_UNUSED;
    uint32_t positionsResolve = VK_ATTACHMENT_UNUSED;
    uint32_t depthResolve = VK_ATTACHMENT_UNUSED;
    if (isMultisampled) {
        auto addResolve = [&add](VkFormat f) {
            return add(
                f,
                VK_SAMPLE_COUNT_1_BIT,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                VK_ATTACHMENT_STORE_OP_STORE,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
        };
        colorResolve = addResolve(format.color);
        if (format.hasNormals) {
            normalsResolve = addResolve(SceneDataFormat);
        }
        if (format.hasPositions) {
            positionsResolve = addResolve(SceneDataFormat);
        }
        if (format.hasDepthTexture) {
            depthResolve = addResolve(SceneDepthFormat);
        }
    }

    auto reference = [](uint32_t attachment, VkImageLayout layout,
                        VkImageAspectFlags aspect)
    {
        VkAttachmentReference2 ref = {};
        ref.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
        ref.attachment = attachment;
        ref.layout = layout;
        ref.aspectMask = aspect;
        return ref;
    };
    constexpr VkImageLayout ColorLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    constexpr VkImageLayout DepthLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    constexpr VkImageAspectFlags Color = VK_IMAGE_ASPECT_COLOR_BIT;
    constexpr VkImageAspectFlags Depth = VK_IMAGE_ASPECT_DEPTH_BIT;

    const std::vector<VkAttachmentReference2> colorRefs = [&]() {
        std::vector<VkAttachmentReference2> refs = {
            reference(color, ColorLayout, Color),
            reference(normals, ColorLayout, Color),
            reference(positions, ColorLayout, Color)
        };
        refs.resize(format.colorAttachmentCount());
        return refs;
    }();
    const std::vector<VkAttachmentReference2> resolveRefs = [&]() {
        std::vector<VkAttachmentReference2> refs = {
            reference(colorResolve, ColorLayout, Color),
            reference(normalsResolve, ColorLayout, Color),
            reference(positionsResolve, ColorLayout, Color)
        };
        refs.resize(format.colorAttachmentCount());
        return refs;
    }();
    const VkAttachmentReference2 depthRef = reference(depth, DepthLayout, Depth);
    const VkAttachmentReference2 depthResolveRef =
        reference(depthResolve, DepthLayout, Depth);

    // The multisampled depth is resolved by picking the first sample, which every
    // device supports
    VkSubpassDescriptionDepthStencilResolve depthResolveInfo = {};
    depthResolveInfo.sType =
        VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
    depthResolveInfo.depthResolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
    depthResolveInfo.stencilResolveMode = VK_RESOLVE_MODE_NONE;
    depthResolveInfo.pDepthStencilResolveAttachment = &depthResolveRef;

    VkSubpassDescription2 subpass = {};
    subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
    subpass.pNext = depthResolve != VK_ATTACHMENT_UNUSED ? &depthResolveInfo : nullptr;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
    subpass.pColorAttachments = colorRefs.data();
    subpass.pResolveAttachments = isMultisampled ? resolveRefs.data() : nullptr;
    subpass.pDepthStencilAttachment = &depthRef;

    // Resolve operations happen in the color attachment output stage, also for depth
    constexpr VkPipelineStageFlags WriteStages =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    constexpr VkAccessFlags WriteAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkSubpassDependency2, 2> dependencies = {};
    // The previous composite or projection pass sampled the images, and the previous
    // render pass wrote them; both have to finish before this pass writes them
    dependencies[0].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | WriteStages;
    dependencies[0].srcAccessMask = WriteAccess;
    dependencies[0].dstStageMask = WriteStages;
    dependencies[0].dstAccessMask = WriteAccess;
    // The composite or projection pass samples the images in its fragment shader
    dependencies[1].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = WriteStages;
    dependencies[1].srcAccessMask = WriteAccess;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkRenderPassCreateInfo2 renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VkRenderPass renderPass = VK_NULL_HANDLE;
    check(
        vkCreateRenderPass2(context.device(), &renderPassInfo, nullptr, &renderPass),
        "vkCreateRenderPass2"
    );
    return renderPass;
}

std::vector<VkClearValue> sceneClearValues(const SceneFormat& format) {
    // The values follow the attachment order of createSceneRenderPass; the resolve
    // attachments come last and are not cleared
    std::vector<VkClearValue> values;
    VkClearValue zero = {};
    zero.color = {{ 0.f, 0.f, 0.f, 0.f }};
    values.push_back(zero);
    if (format.hasNormals) {
        values.push_back(zero);
    }
    if (format.hasPositions) {
        values.push_back(zero);
    }
    VkClearValue depth = {};
    depth.depthStencil = { 1.f, 0 };
    values.push_back(depth);
    return values;
}

VkSampleCountFlagBits supportedSampleCount(const Context& context, int samples) {
    const VkSampleCountFlags supported =
        context.limits().framebufferColorSampleCounts &
        context.limits().framebufferDepthSampleCounts;
    VkSampleCountFlags s = VK_SAMPLE_COUNT_64_BIT;
    for (; s > VK_SAMPLE_COUNT_1_BIT; s >>= 1) {
        if (static_cast<int>(s) <= samples && (supported & s)) {
            return static_cast<VkSampleCountFlagBits>(s);
        }
    }
    return VK_SAMPLE_COUNT_1_BIT;
}

} // namespace sgct::vulkan
