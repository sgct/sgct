/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/vulkan/pipeline.h>

#include <sgct/error.h>
#include <algorithm>
#include <format>
#include <stdexcept>

namespace sgct {
namespace vulkan {

namespace {
    void check(VkResult result, const std::string& what) {
        if (result != VK_SUCCESS) {
            throw Error(
                Error::Component::Window,
                4300,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    }
} // namespace

Pipeline::Pipeline(Context& context, const char* vertexSrc, const char* fragmentSrc,
    const Settings& settings)
    : _context(context)
{
    _vertexModule = _context.createShaderModule(
        _context.compileShader(vertexSrc, Stage::Vertex)
    );
    _fragmentModule = _context.createShaderModule(
        _context.compileShader(fragmentSrc, Stage::Fragment)
    );

    if (settings.layout == VK_NULL_HANDLE) {
        VkPipelineLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 0;
        check(
            vkCreatePipelineLayout(_context.device(), &layoutInfo, nullptr, &_layout),
            "vkCreatePipelineLayout"
        );
        _ownsLayout = true;
    }
    else {
        _layout = settings.layout;
    }

    VkPipelineShaderStageCreateInfo vertexStage = {};
    vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStage.module = _vertexModule;
    vertexStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentStage = {};
    fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStage.module = _fragmentModule;
    fragmentStage.pName = "main";

    std::array<VkPipelineShaderStageCreateInfo, 2> stages = { vertexStage, fragmentStage };

    VkPipelineVertexInputStateCreateInfo vertexInput = {};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount =
        static_cast<uint32_t>(settings.vertexBindings.size());
    vertexInput.pVertexBindingDescriptions =
        settings.vertexBindings.empty() ? nullptr : settings.vertexBindings.data();
    vertexInput.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(settings.vertexAttributes.size());
    vertexInput.pVertexAttributeDescriptions =
        settings.vertexAttributes.empty() ? nullptr : settings.vertexAttributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = settings.topology;
    // Primitive restart on list topologies requires the primitiveTopologyListRestart
    // feature, so only enable it for the strip and fan topologies where it is core
    const bool isStripOrFan =
        settings.topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP ||
        settings.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP ||
        settings.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    inputAssembly.primitiveRestartEnable = isStripOrFan ? VK_TRUE : VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization = {};
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.depthClampEnable = VK_FALSE;
    rasterization.rasterizerDiscardEnable = VK_FALSE;
    rasterization.polygonMode = settings.polygonMode;
    rasterization.cullMode = settings.cullMode;
    rasterization.frontFace = settings.frontFace;
    rasterization.depthBiasEnable = VK_FALSE;
    rasterization.lineWidth = 1.f;

    VkPipelineMultisampleStateCreateInfo multisample = {};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = settings.samples;
    multisample.sampleShadingEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    if (settings.blend) {
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = settings.srcColorBlendFactor;
        colorBlendAttachment.dstColorBlendFactor = settings.dstColorBlendFactor;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = settings.srcAlphaBlendFactor;
        colorBlendAttachment.dstAlphaBlendFactor = settings.dstAlphaBlendFactor;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    else {
        colorBlendAttachment.blendEnable = VK_FALSE;
    }

    // The additional attachments (normals and positions) are never blended
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(
        std::max(settings.colorAttachmentCount, 1u)
    );
    colorBlendAttachments[0] = colorBlendAttachment;
    for (size_t i = 1; i < colorBlendAttachments.size(); i++) {
        colorBlendAttachments[i].blendEnable = VK_FALSE;
        colorBlendAttachments[i].colorWriteMask =
            settings.writesAllAttachments ? colorBlendAttachment.colorWriteMask : 0;
    }

    VkPipelineColorBlendStateCreateInfo colorBlend = {};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlend.pAttachments = colorBlendAttachments.data();

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = settings.depthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = settings.depthWrite ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = settings.depthCompare;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.stageCount = static_cast<uint32_t>(stages.size());
    createInfo.pStages = stages.data();
    createInfo.pVertexInputState = &vertexInput;
    createInfo.pInputAssemblyState = &inputAssembly;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterization;
    createInfo.pMultisampleState = &multisample;
    createInfo.pColorBlendState = &colorBlend;
    createInfo.pDepthStencilState = &depthStencil;
    createInfo.pDynamicState = &dynamicState;
    createInfo.layout = _layout;
    createInfo.renderPass = settings.renderPass;
    createInfo.subpass = 0;

    check(vkCreateGraphicsPipelines(
        _context.device(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &_pipeline
    ), "vkCreateGraphicsPipelines");
}

void Pipeline::Settings::setSceneFormat(const SceneFormat& format) {
    colorAttachmentCount = format.colorAttachmentCount();
    samples = format.samples;
}

Pipeline::~Pipeline() {
    if (_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(_context.device(), _pipeline, nullptr);
        _pipeline = VK_NULL_HANDLE;
    }
    if (_vertexModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(_context.device(), _vertexModule, nullptr);
        _vertexModule = VK_NULL_HANDLE;
    }
    if (_fragmentModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(_context.device(), _fragmentModule, nullptr);
        _fragmentModule = VK_NULL_HANDLE;
    }
    if (_ownsLayout && _layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(_context.device(), _layout, nullptr);
        _layout = VK_NULL_HANDLE;
    }
}

} // namespace vulkan
} // namespace sgct
