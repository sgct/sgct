/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VULKAN_PIPELINE__H__
#define __SGCT__VULKAN_PIPELINE__H__

#include <sgct/sgctexports.h>
#include <sgct/vulkan/context.h>
#include <sgct/vulkan/sceneformat.h>

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace sgct {
namespace vulkan {

/**
 * An immutable Vulkan graphics pipeline. It compiles the provided GLSL vertex and fragment
 * sources to SPIR-V (through the Context), creates the shader modules, and builds the
 * VkPipeline. Viewport and scissor are dynamic so a single pipeline can be used for
 * different viewport sizes.
 */
class SGCT_EXPORT Pipeline {
public:
    /// The configuration used to build the pipeline
    struct SGCT_EXPORT Settings {
        /// The render pass the pipeline is compatible with
        VkRenderPass renderPass = VK_NULL_HANDLE;

        /// The pipeline layout; if `VK_NULL_HANDLE` a layout with no descriptor sets is
        /// created and owned by the pipeline
        VkPipelineLayout layout = VK_NULL_HANDLE;

        /// Vertex input bindings and attributes
        std::vector<VkVertexInputBindingDescription> vertexBindings;
        std::vector<VkVertexInputAttributeDescription> vertexAttributes;

        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
        VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        bool depthTest = false;
        bool depthWrite = false;
        VkCompareOp depthCompare = VK_COMPARE_OP_LESS_OR_EQUAL;

        /// Whether blending is enabled; the factors default to the usual alpha blending
        bool blend = false;
        VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

        /// The number of color attachments and the sample count of the subpass, which
        /// have to match the render pass. See #setSceneFormat for the scene render passes
        uint32_t colorAttachmentCount = 1;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

        /// Whether the fragment shader writes all color attachments. Otherwise, only the
        /// first one is written (and blended) and the others are left unchanged
        bool writesAllAttachments = false;

        /**
         * Sets the number of color attachments and the sample count for a scene render
         * pass of \p format, such as the one of Window::renderPass.
         */
        void setSceneFormat(const SceneFormat& format);
    };

    /**
     * \param context The shared Vulkan context
     * \param vertexSrc The GLSL vertex shader source (must contain a `#version` directive)
     * \param fragmentSrc The GLSL fragment shader source
     * \param settings The pipeline configuration
     *
     * \throw std::runtime_error If shader compilation or pipeline creation fails
     */
    Pipeline(Context& context, const char* vertexSrc, const char* fragmentSrc,
        const Settings& settings);
    ~Pipeline();

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    VkPipeline handle() const { return _pipeline; }
    VkPipelineLayout layout() const { return _layout; }
    bool ownsLayout() const { return _ownsLayout; }

private:
    Context& _context;
    VkShaderModule _vertexModule = VK_NULL_HANDLE;
    VkShaderModule _fragmentModule = VK_NULL_HANDLE;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _layout = VK_NULL_HANDLE;
    bool _ownsLayout = false;
};

} // namespace vulkan
} // namespace sgct

#endif // __SGCT__VULKAN_PIPELINE__H__
