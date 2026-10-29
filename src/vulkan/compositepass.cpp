/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/vulkan/compositepass.h>

#include <sgct/error.h>
#include <sgct/correction/buffer.h>
#include <sgct/vulkan/texture.h>
#include <array>
#include <cstddef>
#include <format>

namespace {
    using namespace sgct;

    void check(VkResult result, const std::string& what) {
        if (result != VK_SUCCESS) {
            throw Error(
                Error::Component::Window,
                4410,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    }

    constexpr const char* CompositeVert = R"(
  #version 460

  layout (location = 0) in vec2 in_position;
  layout (location = 1) in vec2 in_texCoords;
  layout (location = 2) in vec4 in_color;

  layout (location = 0) out vec2 out_texCoords;
  layout (location = 1) out vec4 out_color;

  void main() {
    gl_Position = vec4(in_position, 0.0, 1.0);
    out_texCoords = in_texCoords;
    out_color = in_color;
  }
)";

    // The warp meshes carry OpenGL texture coordinates whose origin is at the bottom of
    // the image, whereas the offscreen images have their first row at the top (the scene
    // is rendered with a y-flipped viewport), so the t coordinate is inverted unless the
    // image is supposed to be mirrored vertically. The modes mirror the stereo shaders
    // of the former OpenGL renderer
    constexpr const char* CompositeFrag = R"(
  #version 460

  layout (location = 0) in vec2 in_texCoords;
  layout (location = 1) in vec4 in_color;

  layout (location = 0) out vec4 out_color;

  layout (set = 0, binding = 0) uniform sampler2D leftTex;
  layout (set = 0, binding = 1) uniform sampler2D rightTex;

  layout (push_constant) uniform PushConstants {
    int flipX;
    int flipY;
    int mode;
    int targetHeight;
  } pc;

  const int Mono = 0;
  const int AnaglyphRedCyan = 1;
  const int AnaglyphAmberBlue = 2;
  const int AnaglyphRedCyanWimmer = 3;
  const int Checkerboard = 4;
  const int CheckerboardInverted = 5;
  const int VerticalInterlaced = 6;
  const int VerticalInterlacedInverted = 7;
  const int Dummy = 8;
  const int Right = 9;

  void main() {
    vec2 uv = in_texCoords;
    if (pc.flipX != 0) {
      uv.x = 1.0 - uv.x;
    }
    if (pc.flipY == 0) {
      uv.y = 1.0 - uv.y;
    }

    // gl_FragCoord with OpenGL's bottom-left origin
    vec2 fragCoord = vec2(gl_FragCoord.x, float(pc.targetHeight) - gl_FragCoord.y);

    if (pc.mode == Mono) {
      out_color = in_color * texture(leftTex, uv);
      return;
    }
    if (pc.mode == Right) {
      out_color = in_color * texture(rightTex, uv);
      return;
    }

    vec4 left = texture(leftTex, uv);
    vec4 right = texture(rightTex, uv);
    if (pc.mode == AnaglyphRedCyan) {
      float leftLum = 0.3 * left.r + 0.59 * left.g + 0.11 * left.b;
      float rightLum = 0.3 * right.r + 0.59 * right.g + 0.11 * right.b;
      out_color = in_color * vec4(leftLum, rightLum, rightLum, max(left.a, right.a));
    }
    else if (pc.mode == AnaglyphAmberBlue) {
      const vec3 coef = vec3(0.15, 0.15, 0.70);
      float rightMix = dot(right.rbg, coef);
      out_color = in_color * vec4(left.r, left.g, rightMix, max(left.a, right.a));
    }
    else if (pc.mode == AnaglyphRedCyanWimmer) {
      vec4 fact = vec4(0.7 * left.g + 0.3 * left.b, right.r, right.b, max(left.a, right.a));
      out_color = in_color * fact;
    }
    else if (pc.mode == Checkerboard || pc.mode == CheckerboardInverted) {
      float v = mix(fragCoord.x, fragCoord.y, 0.5);
      bool useRight = (v - floor(v)) == 0.0;
      if (pc.mode == CheckerboardInverted) {
        useRight = !useRight;
      }
      out_color = in_color * (useRight ? right : left);
    }
    else if (pc.mode == VerticalInterlaced || pc.mode == VerticalInterlacedInverted) {
      float v = fragCoord.y * 0.5;
      bool useRight = (v - floor(v)) > 0.5;
      if (pc.mode == VerticalInterlacedInverted) {
        useRight = !useRight;
      }
      out_color = in_color * (useRight ? right : left);
    }
    else {
      // Dummy
      out_color = in_color * mix(left, right, 0.5);
    }
  }
)";

    // The mask textures are loaded through Image, which flips them vertically, so their
    // first row is at the bottom just like the origin of the mask mesh' texture
    // coordinates; only the mirroring of the window flips them. The masks are multiplied
    // with the composited image (see the blend factors in setRenderPass) and mirror the
    // BaseFrag shader of the former OpenGL renderer
    constexpr const char* MaskFrag = R"(
  #version 460

  layout (location = 0) in vec2 in_texCoords;
  layout (location = 1) in vec4 in_color;

  layout (location = 0) out vec4 out_color;

  layout (set = 0, binding = 0) uniform sampler2D tex;

  layout (push_constant) uniform PushConstants {
    int flipX;
    int flipY;
    int mode;
    int targetHeight;
  } pc;

  void main() {
    vec2 uv = in_texCoords;
    if (pc.flipX != 0) {
      uv.x = 1.0 - uv.x;
    }
    if (pc.flipY != 0) {
      uv.y = 1.0 - uv.y;
    }
    out_color = in_color * texture(tex, uv);
  }
)";
} // namespace

namespace sgct::vulkan {

CompositePass::CompositePass(Context& context)
    : _context(context)
{
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

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};
    for (uint32_t i = 0; i < bindings.size(); i++) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    setLayoutInfo.pBindings = bindings.data();
    check(
        vkCreateDescriptorSetLayout(
            _context.device(), &setLayoutInfo, nullptr, &_setLayout
        ),
        "vkCreateDescriptorSetLayout"
    );

    VkPushConstantRange pushConstants = {};
    pushConstants.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstants.offset = 0;
    pushConstants.size = sizeof(PushConstants);
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &_setLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushConstants;
    check(
        vkCreatePipelineLayout(_context.device(), &layoutInfo, nullptr, &_layout),
        "vkCreatePipelineLayout"
    );

    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = static_cast<uint32_t>(bindings.size());
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    check(
        vkCreateDescriptorPool(_context.device(), &poolInfo, nullptr, &_pool),
        "vkCreateDescriptorPool"
    );

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &_setLayout;
    check(
        vkAllocateDescriptorSets(_context.device(), &allocInfo, &_set),
        "vkAllocateDescriptorSets"
    );

    VkDescriptorSetLayoutBinding maskBinding = {};
    maskBinding.binding = 0;
    maskBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    maskBinding.descriptorCount = 1;
    maskBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo maskSetLayoutInfo = {};
    maskSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    maskSetLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
    maskSetLayoutInfo.bindingCount = 1;
    maskSetLayoutInfo.pBindings = &maskBinding;
    check(
        vkCreateDescriptorSetLayout(
            _context.device(), &maskSetLayoutInfo, nullptr, &_maskSetLayout
        ),
        "vkCreateDescriptorSetLayout"
    );

    VkPipelineLayoutCreateInfo maskLayoutInfo = layoutInfo;
    maskLayoutInfo.pSetLayouts = &_maskSetLayout;
    check(
        vkCreatePipelineLayout(_context.device(), &maskLayoutInfo, nullptr, &_maskLayout),
        "vkCreatePipelineLayout"
    );
}

CompositePass::~CompositePass() {
    _pipeline = nullptr;
    _maskPipeline = nullptr;
    if (_maskLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_context.device(), _maskLayout, nullptr);
    }
    if (_maskSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_context.device(), _maskSetLayout, nullptr);
    }
    // Destroying the pool frees the descriptor set allocated from it
    if (_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(_context.device(), _pool, nullptr);
    }
    if (_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_context.device(), _layout, nullptr);
    }
    if (_setLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_context.device(), _setLayout, nullptr);
    }
    if (_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(_context.device(), _sampler, nullptr);
    }
}

void CompositePass::setRenderPass(VkRenderPass renderPass) {
    using Vertex = correction::Buffer::Vertex;

    Pipeline::Settings settings;
    settings.renderPass = renderPass;
    settings.layout = _layout;
    settings.vertexBindings = {
        { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX }
    };
    settings.vertexAttributes = {
        { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, x) },
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, s) },
        { 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, r) }
    };
    settings.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    settings.cullMode = VK_CULL_MODE_NONE;

    _pipeline = nullptr;
    _pipeline = std::make_unique<Pipeline>(
        _context, CompositeVert, CompositeFrag, settings
    );

    // result = destination * mask, like glBlendFunc(GL_ZERO, GL_SRC_COLOR) of the OpenGL
    // path, which applies to the alpha channel as well
    settings.layout = _maskLayout;
    settings.blend = true;
    settings.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    settings.dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
    settings.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    settings.dstAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    _maskPipeline = nullptr;
    _maskPipeline = std::make_unique<Pipeline>(
        _context, CompositeVert, MaskFrag, settings
    );
}

void CompositePass::setInputs(VkImageView left, VkImageView right) {
    std::array<VkDescriptorImageInfo, 2> images = {};
    images[0].sampler = _sampler;
    images[0].imageView = left;
    images[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    images[1].sampler = _sampler;
    images[1].imageView = right;
    images[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    std::array<VkWriteDescriptorSet, 2> writes = {};
    for (uint32_t i = 0; i < writes.size(); i++) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = _set;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].pImageInfo = &images[i];
    }
    vkUpdateDescriptorSets(
        _context.device(),
        static_cast<uint32_t>(writes.size()),
        writes.data(),
        0,
        nullptr
    );
}

void CompositePass::bind(VkCommandBuffer cmd, const PushConstants& constants) const {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->handle());
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _layout, 0, 1, &_set, 0, nullptr
    );
    vkCmdPushConstants(
        cmd,
        _layout,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PushConstants),
        &constants
    );
}

void CompositePass::bindMask(VkCommandBuffer cmd, const PushConstants& constants) const {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _maskPipeline->handle());
    vkCmdPushConstants(
        cmd,
        _maskLayout,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(PushConstants),
        &constants
    );
}

void CompositePass::setMaskTexture(VkCommandBuffer cmd, const Texture& texture) const {
    VkDescriptorImageInfo image = {};
    image.sampler = texture.sampler();
    image.imageView = texture.imageView();
    image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image;
    vkCmdPushDescriptorSet(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _maskLayout, 0, 1, &write
    );
}

} // namespace sgct::vulkan
