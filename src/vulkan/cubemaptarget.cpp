/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/vulkan/cubemaptarget.h>

#include "imageresource.h"
#include <sgct/error.h>
#include <sgct/math.h>
#include <sgct/vulkan/offscreenbuffer.h>
#include <sgct/vulkan/projectionpass.h>
#include <format>
#include <span>
#include <string>
#include <vector>

namespace sgct::vulkan {

namespace {
    void check(VkResult result, const std::string& what) {
        if (result != VK_SUCCESS) {
            throw Error(
                Error::Component::Window,
                4460,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    }

    struct CorrectionConstants {
        std::array<vec4, 4> corners;
        float nearClip = 0.f;
        float farClip = 0.f;
        float resolution = 0.f;
    };

    // Copies a face from the swap target into the cube map and converts the depth from
    // the distance to the plane of the face into the distance to the center of the cube.
    // The depth is expected to follow the OpenGL convention of the projection matrices
    // (z_window = 0.5 * z_ndc + 0.5)
    constexpr std::string_view CorrectionFrag = R"(
  layout (location = 0) in vec2 in_texCoords;

  layout (location = 0) out vec4 out_color;
#if HAS_NORMALS
  layout (location = 1) out vec4 out_normal;
#endif
#if HAS_POSITIONS
  layout (location = 2) out vec4 out_position;
#endif

  layout (set = 0, binding = 0) uniform sampler2D colorTex;
  layout (set = 0, binding = 1) uniform sampler2D depthTex;
#if HAS_NORMALS
  layout (set = 0, binding = 2) uniform sampler2D normalTex;
#endif
#if HAS_POSITIONS
  layout (set = 0, binding = 3) uniform sampler2D positionTex;
#endif

  layout (push_constant) uniform Constants {
    layout (offset = 64) float near;
    float far;
    float resolution;
  } pc;

  void main() {
    // The swap target and the cube map face have the same size and orientation
    ivec2 texel = ivec2(gl_FragCoord.xy);
    out_color = texelFetch(colorTex, texel, 0);
#if HAS_NORMALS
    out_normal = texelFetch(normalTex, texel, 0);
#endif
#if HAS_POSITIONS
    out_position = texelFetch(positionTex, texel, 0);
#endif

    float depth = texelFetch(depthTex, texel, 0).r;
    if (depth >= 1.0) {
      // Nothing was rendered
      gl_FragDepth = 1.0;
      return;
    }

    float a = pc.far / (pc.far - pc.near);
    float b = pc.far * pc.near / (pc.near - pc.far);
    float z = b / (depth - a);

    // The face covers 90 degrees, so the position on the plane of the face at the
    // distance 1 is between -1 and 1
    vec2 plane = 2.0 * gl_FragCoord.xy / pc.resolution - 1.0;
    float r = z * sqrt(dot(plane, plane) + 1.0);
    gl_FragDepth = clamp(a + b / r, 0.0, 1.0);
  }
)";
} // namespace

struct CubemapTarget::Images {
    struct Cube {
        ImageResource image;
        VkImageView array = VK_NULL_HANDLE;
        /// One 2D view per layer (not per face)
        std::array<VkImageView, 6> layers = {};
    };
    std::array<Cube, 4> cubes;

    // The images that the faces are rendered into before they are resolved into the
    // cube map, which are shared by all faces
    ImageResource colorMultisampled;
    ImageResource normalsMultisampled;
    ImageResource positionsMultisampled;
    ImageResource depth;
};

CubemapTarget::CubemapTarget(Context& context, uint32_t resolution,
                             const SceneFormat& format)
    : _context(context)
    , _resolution(resolution)
    , _format(format)
    , _images(std::make_unique<Images>())
{
    constexpr VkImageUsageFlags Sampled =
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    const std::array<VkFormat, 4> formats = {
        _format.color, SceneDataFormat, SceneDataFormat, SceneDepthFormat
    };
    for (int a = 0; a < 4; a++) {
        if (!has(static_cast<Attachment>(a))) {
            continue;
        }
        ImageResource::Settings settings;
        settings.extent = extent();
        settings.format = formats[a];
        settings.usage = Sampled | (a == static_cast<int>(Attachment::Depth) ?
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        settings.isCube = true;

        Images::Cube& cube = _images->cubes[a];
        cube.image = ImageResource(_context, settings);
        cube.array = cube.image.createView(VK_IMAGE_VIEW_TYPE_2D_ARRAY, 0, 6);
        for (uint32_t i = 0; i < 6; i++) {
            cube.layers[i] = cube.image.createView(VK_IMAGE_VIEW_TYPE_2D, i, 1);
        }
    }

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod = 0.f;
    check(
        vkCreateSampler(_context.device(), &samplerInfo, nullptr, &_sampler),
        "vkCreateSampler"
    );

    if (_format.hasDepthTexture) {
        // The faces are rendered into the swap target and copied into the cube map
        _swap = std::make_unique<OffscreenBuffer>(_context, extent(), _format);
        _renderPass = createSceneRenderPass(_context, _format);
        createCorrection();
    }
    else {
        _renderPass = createSceneRenderPass(_context, _format);
        createDirectFramebuffers();
    }

    // Clears all layers and moves them into the layout in which they are sampled, so
    // that the faces that are not rendered by a projection can be sampled, too
    submitImmediately(_context, [this](VkCommandBuffer cmd) {
        for (int a = 0; a < 4; a++) {
            if (!has(static_cast<Attachment>(a))) {
                continue;
            }
            VkClearValue clear = {};
            if (a == static_cast<int>(Attachment::Depth)) {
                clear.depthStencil = { 1.f, 0 };
            }
            else {
                clear.color = {{ 0.f, 0.f, 0.f, 0.f }};
            }
            recordClear(cmd, _images->cubes[a].image, 6, clear);
        }
    });
}

CubemapTarget::~CubemapTarget() {
    VkDevice device = _context.device();
    _correctionPass = nullptr;
    for (VkFramebuffer framebuffer : _correctionFramebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    if (_correctionRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, _correctionRenderPass, nullptr);
    }
    _swap = nullptr;
    for (VkFramebuffer framebuffer : _framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    if (_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, _renderPass, nullptr);
    }
    if (_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, _sampler, nullptr);
    }
    for (Images::Cube& cube : _images->cubes) {
        for (VkImageView view : cube.layers) {
            if (view != VK_NULL_HANDLE) {
                vkDestroyImageView(device, view, nullptr);
            }
        }
        if (cube.array != VK_NULL_HANDLE) {
            vkDestroyImageView(device, cube.array, nullptr);
        }
    }
}

uint32_t CubemapTarget::layer(int face) {
    // The +Y (2) and -Y (3) faces swap their layers, see the class documentation
    constexpr std::array<uint32_t, 6> Layers = { 0, 1, 3, 2, 4, 5 };
    return Layers[face];
}

bool CubemapTarget::has(Attachment attachment) const {
    switch (attachment) {
        case Attachment::Color:     return true;
        case Attachment::Normals:   return _format.hasNormals;
        case Attachment::Positions: return _format.hasPositions;
        case Attachment::Depth:     return _format.hasDepthTexture;
        default:                    return false;
    }
}

VkImageView CubemapTarget::cubeView(Attachment attachment) const {
    return _images->cubes[static_cast<int>(attachment)].image.view();
}

VkImageView CubemapTarget::arrayView(Attachment attachment) const {
    return _images->cubes[static_cast<int>(attachment)].array;
}

VkImageView CubemapTarget::faceView(int face, Attachment attachment) const {
    return _images->cubes[static_cast<int>(attachment)].layers[layer(face)];
}

void CubemapTarget::createDirectFramebuffers() {
    const bool isMultisampled = _format.samples != VK_SAMPLE_COUNT_1_BIT;

    auto create = [this](VkFormat f, VkImageUsageFlags usage) {
        ImageResource::Settings settings;
        settings.extent = extent();
        settings.format = f;
        settings.usage = usage | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        settings.samples = _format.samples;
        return ImageResource(_context, settings);
    };
    constexpr VkImageUsageFlags Color = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (isMultisampled) {
        _images->colorMultisampled = create(_format.color, Color);
        if (_format.hasNormals) {
            _images->normalsMultisampled = create(SceneDataFormat, Color);
        }
        if (_format.hasPositions) {
            _images->positionsMultisampled = create(SceneDataFormat, Color);
        }
    }
    _images->depth = create(SceneDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    for (int face = 0; face < 6; face++) {
        // The views in the attachment order of createSceneRenderPass
        std::vector<VkImageView> views;
        if (isMultisampled) {
            views.push_back(_images->colorMultisampled.view());
            if (_format.hasNormals) {
                views.push_back(_images->normalsMultisampled.view());
            }
            if (_format.hasPositions) {
                views.push_back(_images->positionsMultisampled.view());
            }
            views.push_back(_images->depth.view());
        }
        views.push_back(faceView(face, Attachment::Color));
        if (_format.hasNormals) {
            views.push_back(faceView(face, Attachment::Normals));
        }
        if (_format.hasPositions) {
            views.push_back(faceView(face, Attachment::Positions));
        }
        if (!isMultisampled) {
            views.push_back(_images->depth.view());
        }

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(views.size());
        framebufferInfo.pAttachments = views.data();
        framebufferInfo.width = _resolution;
        framebufferInfo.height = _resolution;
        framebufferInfo.layers = 1;
        check(
            vkCreateFramebuffer(
                _context.device(),
                &framebufferInfo,
                nullptr,
                &_framebuffers[face]
            ),
            "vkCreateFramebuffer"
        );
    }
}

void CubemapTarget::createCorrection() {
    // The correction pass overwrites the whole face: color, [normals], [positions], depth
    std::vector<VkAttachmentDescription> attachments;
    auto add = [&attachments](VkFormat f) {
        VkAttachmentDescription attachment = {};
        attachment.format = f;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        attachments.push_back(attachment);
        return static_cast<uint32_t>(attachments.size() - 1);
    };
    const uint32_t color = add(_format.color);
    const uint32_t normals = _format.hasNormals ? add(SceneDataFormat) : VK_ATTACHMENT_UNUSED;
    const uint32_t positions =
        _format.hasPositions ? add(SceneDataFormat) : VK_ATTACHMENT_UNUSED;
    const uint32_t depth = add(SceneDepthFormat);

    constexpr VkImageLayout ColorLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    std::vector<VkAttachmentReference> colorRefs = {
        { color, ColorLayout },
        { normals, ColorLayout },
        { positions, ColorLayout }
    };
    colorRefs.resize(_format.colorAttachmentCount());
    const VkAttachmentReference depthRef = {
        depth,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
    subpass.pColorAttachments = colorRefs.data();
    subpass.pDepthStencilAttachment = &depthRef;

    constexpr VkPipelineStageFlags WriteStages =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    constexpr VkAccessFlags WriteAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    std::array<VkSubpassDependency, 2> dependencies = {};
    // The projection pass sampled the cube map in the previous frame
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | WriteStages;
    dependencies[0].srcAccessMask = WriteAccess;
    dependencies[0].dstStageMask = WriteStages;
    dependencies[0].dstAccessMask = WriteAccess;
    // The projection pass samples the cube map
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = WriteStages;
    dependencies[1].srcAccessMask = WriteAccess;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();
    check(
        vkCreateRenderPass(
            _context.device(),
            &renderPassInfo,
            nullptr,
            &_correctionRenderPass
        ),
        "vkCreateRenderPass"
    );

    for (int face = 0; face < 6; face++) {
        std::vector<VkImageView> views = { faceView(face, Attachment::Color) };
        if (_format.hasNormals) {
            views.push_back(faceView(face, Attachment::Normals));
        }
        if (_format.hasPositions) {
            views.push_back(faceView(face, Attachment::Positions));
        }
        views.push_back(faceView(face, Attachment::Depth));

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _correctionRenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(views.size());
        framebufferInfo.pAttachments = views.data();
        framebufferInfo.width = _resolution;
        framebufferInfo.height = _resolution;
        framebufferInfo.layers = 1;
        check(
            vkCreateFramebuffer(
                _context.device(),
                &framebufferInfo,
                nullptr,
                &_correctionFramebuffers[face]
            ),
            "vkCreateFramebuffer"
        );
    }

    ProjectionPass::Settings settings;
    settings.fragmentShader = std::format(
        "#version 460\n#define HAS_NORMALS {}\n#define HAS_POSITIONS {}\n{}",
        _format.hasNormals ? 1 : 0,
        _format.hasPositions ? 1 : 0,
        CorrectionFrag
    );
    settings.pushConstantSize = sizeof(CorrectionConstants);
    settings.samplerCount = 4;
    settings.colorAttachmentCount = _format.colorAttachmentCount();
    settings.writesAllAttachments = true;
    settings.writesDepth = true;
    _correctionPass = std::make_unique<ProjectionPass>(
        _context,
        _correctionRenderPass,
        settings
    );
}

void CubemapTarget::beginFace(VkCommandBuffer cmd, int face) const {
    if (_swap) {
        _swap->begin(cmd);
        return;
    }

    const std::vector<VkClearValue> clearValues = sceneClearValues(_format);
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _renderPass;
    renderPassInfo.framebuffer = _framebuffers[face];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = extent();
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void CubemapTarget::endFace(VkCommandBuffer cmd, int face, float nearClip,
                            float farClip) const
{
    vkCmdEndRenderPass(cmd);
    if (!_swap) {
        return;
    }

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _correctionRenderPass;
    renderPassInfo.framebuffer = _correctionFramebuffers[face];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = extent();
    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.width = static_cast<float>(_resolution);
    viewport.height = static_cast<float>(_resolution);
    viewport.maxDepth = 1.f;
    VkRect2D scissor = {};
    scissor.extent = extent();
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    CorrectionConstants constants;
    constants.corners = {
        vec4{ -1.f, -1.f, 0.f, 0.f },
        vec4{ -1.f,  1.f, 0.f, 1.f },
        vec4{  1.f, -1.f, 1.f, 0.f },
        vec4{  1.f,  1.f, 1.f, 1.f }
    };
    constants.nearClip = nearClip;
    constants.farClip = farClip;
    constants.resolution = static_cast<float>(_resolution);

    auto image = [this](VkImageView view) {
        VkDescriptorImageInfo info = {};
        info.sampler = _sampler;
        info.imageView = view;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        return info;
    };
    const std::array<VkDescriptorImageInfo, 4> images = {
        image(_swap->colorImageView()),
        image(_swap->depthImageView()),
        image(_swap->normalsImageView()),
        image(_swap->positionsImageView())
    };

    _correctionPass->bind(cmd, &constants);
    _correctionPass->setTextures(cmd, images);
    _correctionPass->drawQuad(cmd);

    vkCmdEndRenderPass(cmd);
}

void CubemapTarget::copyFace(VkCommandBuffer cmd, int face, VkBuffer buffer) const {
    const uint32_t l = layer(face);
    const VkImage image =
        _images->cubes[static_cast<int>(Attachment::Color)].image.image();

    // The face was last written by the scene render pass, which makes its writes
    // available to the transfer stage
    VkImageMemoryBarrier toTransfer = {};
    toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toTransfer.srcAccessMask = 0;
    toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    toTransfer.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.image = image;
    toTransfer.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, l, 1 };
    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &toTransfer
    );

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, l, 1 };
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { _resolution, _resolution, 1 };
    vkCmdCopyImageToBuffer(
        cmd,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        buffer,
        1,
        &region
    );

    // Return the face to its layout and make the copied pixels visible to the host
    VkImageMemoryBarrier fromTransfer = toTransfer;
    fromTransfer.srcAccessMask = 0;
    fromTransfer.dstAccessMask = 0;
    fromTransfer.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    fromTransfer.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkMemoryBarrier toHost = {};
    toHost.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    toHost.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toHost.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        1, &toHost,
        0, nullptr,
        1, &fromTransfer
    );
}

} // namespace sgct::vulkan
