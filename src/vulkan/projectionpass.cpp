/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/vulkan/projectionpass.h>

#include <sgct/error.h>
#include <sgct/correction/buffer.h>
#include <cstddef>
#include <format>
#include <vector>

namespace sgct::vulkan {

namespace {
    void check(VkResult result, const std::string& what) {
        if (result != VK_SUCCESS) {
            throw Error(
                Error::Component::Window,
                4470,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    }

    constexpr VkShaderStageFlags PushConstantStages =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
} // namespace

const char* ProjectionPass::QuadVert = R"(
  #version 460

  layout (location = 0) out vec2 out_texCoords;

  layout (push_constant) uniform Quad {
    vec4 corners[4];
  } quad;

  void main() {
    vec4 corner = quad.corners[gl_VertexIndex];
    gl_Position = vec4(corner.xy, 0.0, 1.0);
    out_texCoords = corner.zw;
  }
)";

ProjectionPass::ProjectionPass(Context& context, VkRenderPass renderPass,
                               const Settings& settings)
    : _context(context)
    , _pushConstantSize(settings.pushConstantSize)
{
    std::vector<VkDescriptorSetLayoutBinding> bindings(settings.samplerCount);
    for (uint32_t i = 0; i < settings.samplerCount; i++) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
    setLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    setLayoutInfo.pBindings = bindings.data();
    check(
        vkCreateDescriptorSetLayout(
            _context.device(), &setLayoutInfo, nullptr, &_setLayout
        ),
        "vkCreateDescriptorSetLayout"
    );

    VkPushConstantRange pushConstants = {};
    pushConstants.stageFlags = PushConstantStages;
    pushConstants.offset = 0;
    pushConstants.size = _pushConstantSize;
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &_setLayout;
    layoutInfo.pushConstantRangeCount = _pushConstantSize > 0 ? 1 : 0;
    layoutInfo.pPushConstantRanges = &pushConstants;
    check(
        vkCreatePipelineLayout(_context.device(), &layoutInfo, nullptr, &_layout),
        "vkCreatePipelineLayout"
    );

    Pipeline::Settings pipelineSettings;
    pipelineSettings.renderPass = renderPass;
    pipelineSettings.layout = _layout;
    pipelineSettings.cullMode = VK_CULL_MODE_NONE;
    pipelineSettings.colorAttachmentCount = settings.colorAttachmentCount;
    pipelineSettings.samples = settings.samples;
    pipelineSettings.writesAllAttachments = settings.writesAllAttachments;
    if (settings.writesDepth) {
        pipelineSettings.depthTest = true;
        pipelineSettings.depthWrite = true;
        pipelineSettings.depthCompare = VK_COMPARE_OP_ALWAYS;
    }
    if (settings.useMeshVertices) {
        using Vertex = correction::Buffer::Vertex;
        pipelineSettings.vertexBindings = {
            { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX }
        };
        pipelineSettings.vertexAttributes = {
            { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, x) },
            { 1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, s) },
            { 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, r) }
        };
        pipelineSettings.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
    else {
        pipelineSettings.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    }
    _pipeline = std::make_unique<Pipeline>(
        _context,
        settings.vertexShader.c_str(),
        settings.fragmentShader.c_str(),
        pipelineSettings
    );
}

ProjectionPass::~ProjectionPass() {
    _pipeline = nullptr;
    if (_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_context.device(), _layout, nullptr);
    }
    if (_setLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(_context.device(), _setLayout, nullptr);
    }
}

void ProjectionPass::bind(VkCommandBuffer cmd, const void* constants) const {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->handle());
    if (_pushConstantSize > 0) {
        vkCmdPushConstants(
            cmd,
            _layout,
            PushConstantStages,
            0,
            _pushConstantSize,
            constants
        );
    }
}

void ProjectionPass::setTextures(VkCommandBuffer cmd,
                                 std::span<const VkDescriptorImageInfo> images) const
{
    std::vector<VkWriteDescriptorSet> writes;
    writes.reserve(images.size());
    for (size_t i = 0; i < images.size(); i++) {
        if (images[i].imageView == VK_NULL_HANDLE) {
            continue;
        }
        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstBinding = static_cast<uint32_t>(i);
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &images[i];
        writes.push_back(write);
    }
    vkCmdPushDescriptorSet(
        cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _layout,
        0,
        static_cast<uint32_t>(writes.size()),
        writes.data()
    );
}

void ProjectionPass::drawQuad(VkCommandBuffer cmd) const {
    vkCmdDraw(cmd, 4, 1, 0, 0);
}

} // namespace sgct::vulkan
