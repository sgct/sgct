/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VULKAN_PROJECTIONPASS__H__
#define __SGCT__VULKAN_PROJECTIONPASS__H__

#include <sgct/sgctexports.h>
#include <sgct/vulkan/context.h>
#include <sgct/vulkan/pipeline.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace sgct {
namespace vulkan {

/**
 * The pipeline with which a non-linear projection draws its cube map into a viewport of
 * the window's offscreen target. The textures are set through push descriptors
 * (combined image samplers at the bindings 0 to `samplerCount - 1` of set 0) and the
 * parameters through push constants that are visible to the vertex and fragment stage.
 *
 * By default, the pipeline has no vertex input and draws a triangle strip of four
 * vertices, whose positions and texture coordinates the vertex shader of #QuadVert reads
 * from the first 64 bytes of the push constants. Alternatively, the pipeline draws the
 * triangle lists of correction meshes (see correction::Buffer::Vertex).
 */
class SGCT_EXPORT ProjectionPass {
public:
    /**
     * A vertex shader that reads the four corners of a triangle strip from the push
     * constants as `vec4 corners[4]` (`xy` is the position in normalized device
     * coordinates, `zw` the texture coordinates) and passes the texture coordinates to
     * location 0.
     */
    static const char* QuadVert;

    struct Settings {
        std::string vertexShader = QuadVert;
        std::string fragmentShader;
        /// The size of the push constant range in bytes
        uint32_t pushConstantSize = 0;
        /// The number of combined image samplers
        uint32_t samplerCount = 1;
        /// Whether the pipeline draws correction meshes instead of the quad
        bool useMeshVertices = false;
        /// The number of color attachments and the sample count of the render pass
        uint32_t colorAttachmentCount = 1;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        /// Whether the fragment shader writes all color attachments or only the first
        bool writesAllAttachments = false;
        /// Whether the fragment shader writes gl_FragDepth, which is then always stored
        bool writesDepth = false;
    };

    /**
     * \param context The shared Vulkan context
     * \param renderPass A render pass that is compatible with the scene render passes
     * \param settings The shaders and resources of the pipeline
     *
     * \throw sgct::Error If a Vulkan call fails or a shader fails to compile
     */
    ProjectionPass(Context& context, VkRenderPass renderPass, const Settings& settings);
    ~ProjectionPass();

    ProjectionPass(const ProjectionPass&) = delete;
    ProjectionPass& operator=(const ProjectionPass&) = delete;

    /**
     * Binds the pipeline and pushes \p constants, which must have the size of
     * Settings::pushConstantSize.
     */
    void bind(VkCommandBuffer cmd, const void* constants) const;

    /**
     * Sets the textures of the bindings 0 to `images.size() - 1`; the bindings whose
     * image view is VK_NULL_HANDLE are skipped. The pipeline has to be bound.
     */
    void setTextures(VkCommandBuffer cmd,
        std::span<const VkDescriptorImageInfo> images) const;

    /**
     * Draws the quad whose corners were pushed with #bind.
     */
    void drawQuad(VkCommandBuffer cmd) const;

private:
    Context& _context;
    const uint32_t _pushConstantSize;
    VkDescriptorSetLayout _setLayout = VK_NULL_HANDLE;
    VkPipelineLayout _layout = VK_NULL_HANDLE;
    std::unique_ptr<Pipeline> _pipeline;
};

} // namespace vulkan
} // namespace sgct

#endif // __SGCT__VULKAN_PROJECTIONPASS__H__
