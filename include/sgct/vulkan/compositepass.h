/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VULKAN_COMPOSITEPASS__H__
#define __SGCT__VULKAN_COMPOSITEPASS__H__

#include <sgct/sgctexports.h>
#include <sgct/vulkan/context.h>
#include <sgct/vulkan/pipeline.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>

namespace sgct {
namespace vulkan {

class Texture;

/**
 * The pass that composites the offscreen render targets of a window into its swapchain
 * image. Each viewport's warp mesh (a triangle list of correction::Buffer::Vertex) is
 * drawn with a pipeline that samples the left (and for stereo modes the right) eye image
 * and combines them according to the selected Mode.
 *
 * Afterwards, the blend and black level masks are applied with a second pipeline that
 * multiplies the composited image with the mask texture drawn on each viewport's mask
 * mesh.
 *
 * The pipelines depend on the render pass they are used in, so they are rebuilt through
 * #setRenderPass whenever the swapchain is recreated.
 */
class SGCT_EXPORT CompositePass {
public:
    /// How the left and right eye images are combined into the output
    enum class Mode : int32_t {
        Mono = 0,
        AnaglyphRedCyan,
        AnaglyphAmberBlue,
        AnaglyphRedCyanWimmer,
        Checkerboard,
        CheckerboardInverted,
        VerticalInterlaced,
        VerticalInterlacedInverted,
        Dummy,
        /// Only the right eye, for the second layer of stereoscopic presentation
        Right
    };

    /// The per-draw parameters of the composite shader
    struct PushConstants {
        int32_t flipX = 0;
        int32_t flipY = 0;
        Mode mode = Mode::Mono;
        /// The height of the render target in pixels, used to emulate the bottom-left
        /// origin of OpenGL's gl_FragCoord for the interlaced and checkerboard modes
        int32_t targetHeight = 0;
    };

    explicit CompositePass(Context& context);
    ~CompositePass();

    CompositePass(const CompositePass&) = delete;
    CompositePass& operator=(const CompositePass&) = delete;

    /**
     * (Re)creates the composite and mask pipelines for the provided render pass. The GPU
     * must not be using the previous pipelines anymore.
     */
    void setRenderPass(VkRenderPass renderPass);

    /**
     * Points the sampled inputs at the provided image views, which must be in
     * VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL when the pass is executed. The GPU must not
     * be using the descriptor set anymore. For mono rendering, pass the same view twice.
     */
    void setInputs(VkImageView left, VkImageView right);

    /**
     * Binds the pipeline and the descriptor set and pushes the constants. Afterwards the
     * warp meshes can be drawn.
     */
    void bind(VkCommandBuffer cmd, const PushConstants& constants) const;

    /**
     * Binds the mask pipeline and pushes the constants (of which only the flip flags are
     * used). Afterwards, each mask is drawn by calling #setMaskTexture followed by the
     * draw of a mask mesh.
     */
    void bindMask(VkCommandBuffer cmd, const PushConstants& constants) const;

    /**
     * Sets the texture that the following mask draws multiply the image with. The mask
     * pipeline has to be bound.
     */
    void setMaskTexture(VkCommandBuffer cmd, const Texture& texture) const;

private:
    Context& _context;
    VkSampler _sampler = VK_NULL_HANDLE;
    VkDescriptorSetLayout _setLayout = VK_NULL_HANDLE;
    VkPipelineLayout _layout = VK_NULL_HANDLE;
    VkDescriptorPool _pool = VK_NULL_HANDLE;
    VkDescriptorSet _set = VK_NULL_HANDLE;
    std::unique_ptr<Pipeline> _pipeline;

    // The mask texture is set through push descriptors, as every viewport has its own
    VkDescriptorSetLayout _maskSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout _maskLayout = VK_NULL_HANDLE;
    std::unique_ptr<Pipeline> _maskPipeline;
};

} // namespace vulkan
} // namespace sgct

#endif // __SGCT__VULKAN_COMPOSITEPASS__H__
