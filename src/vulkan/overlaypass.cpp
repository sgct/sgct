/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/vulkan/overlaypass.h>

#include <sgct/error.h>
#include <sgct/vulkan/texture.h>
#include <cstdint>
#include <format>
#include <string>

namespace {
    using namespace sgct;

    void check(VkResult result, const std::string& what) {
        if (result != VK_SUCCESS) {
            throw Error(
                Error::Component::Window,
                4430,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    }

    // Two triangles covering the viewport. The texture coordinates have their origin at
    // the bottom-left, which matches the textures loaded through Image (whose first row
    // is the bottom of the image) when drawn with a flipped viewport
    constexpr const char* OverlayVert = R"(
  #version 460

  layout (location = 0) out vec2 out_texCoords;

  layout (push_constant) uniform Constants {
    // Whether the image has its first row at the top instead of the bottom
    int flipY;
  } pc;

  const vec2 Positions[6] = vec2[](
    vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
    vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0)
  );

  void main() {
    vec2 p = Positions[gl_VertexIndex];
    gl_Position = vec4(p, 0.0, 1.0);
    out_texCoords = p * 0.5 + 0.5;
    if (pc.flipY != 0) {
      out_texCoords.y = 1.0 - out_texCoords.y;
    }
  }
)";

    // Mirrors the OverlayFrag shader of the former OpenGL renderer
    constexpr const char* OverlayFrag = R"(
  #version 460

  layout (location = 0) in vec2 in_texCoords;
  layout (location = 0) out vec4 out_color;

  layout (set = 0, binding = 0) uniform sampler2D tex;

  void main() {
    out_color = texture(tex, in_texCoords);
  }
)";
} // namespace

namespace sgct::vulkan {

OverlayPass::OverlayPass(Context& context)
    : _context(context)
{
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
    setLayoutInfo.bindingCount = 1;
    setLayoutInfo.pBindings = &binding;
    check(
        vkCreateDescriptorSetLayout(
            _context.device(), &setLayoutInfo, nullptr, &_setLayout
        ),
        "vkCreateDescriptorSetLayout"
    );

    VkPushConstantRange pushConstants = {};
    pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstants.offset = 0;
    pushConstants.size = sizeof(int32_t);
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
}

OverlayPass::~OverlayPass() {
    _pipeline = nullptr;
    if (_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_context.device(), _layout, nullptr);
    }
    if (_setLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_context.device(), _setLayout, nullptr);
    }
}

void OverlayPass::setRenderPass(VkRenderPass renderPass, const SceneFormat& format) {
    Pipeline::Settings settings;
    settings.renderPass = renderPass;
    settings.setSceneFormat(format);
    settings.layout = _layout;
    settings.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    settings.cullMode = VK_CULL_MODE_NONE;
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) of the OpenGL path, which applies
    // to the alpha channel as well
    settings.blend = true;
    settings.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    settings.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    _pipeline = nullptr;
    _pipeline = std::make_unique<Pipeline>(_context, OverlayVert, OverlayFrag, settings);
}

void OverlayPass::draw(VkCommandBuffer cmd, const Texture& texture) const {
    record(cmd, texture.imageView(), texture.sampler(), false);
}

void OverlayPass::draw(VkCommandBuffer cmd, VkImageView view, VkSampler sampler) const {
    record(cmd, view, sampler, true);
}

void OverlayPass::record(VkCommandBuffer cmd, VkImageView view, VkSampler sampler,
                         bool flipY) const
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->handle());

    const int32_t flip = flipY ? 1 : 0;
    vkCmdPushConstants(cmd, _layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(flip), &flip);

    VkDescriptorImageInfo image = {};
    image.sampler = sampler;
    image.imageView = view;
    image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image;
    vkCmdPushDescriptorSet(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _layout, 0, 1, &write);

    vkCmdDraw(cmd, 6, 1, 0, 0);
}

} // namespace sgct::vulkan
