/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VULKAN_OVERLAYPASS__H__
#define __SGCT__VULKAN_OVERLAYPASS__H__

#include <sgct/sgctexports.h>
#include <sgct/vulkan/context.h>
#include <sgct/vulkan/pipeline.h>
#include <sgct/vulkan/sceneformat.h>

#include <vulkan/vulkan.h>

#include <memory>

namespace sgct {
namespace vulkan {

class Texture;

/**
 * Draws a texture over the whole area of the current viewport with alpha blending. It is
 * used for the viewports' overlay textures, which are drawn into the offscreen render
 * target after the scene. The quad is generated in the vertex shader, so no vertex buffer
 * is needed, and the texture is set through a push descriptor.
 *
 * The pipeline depends on the render pass it is used in, so it is rebuilt through
 * #setRenderPass whenever the offscreen render target is recreated.
 */
class SGCT_EXPORT OverlayPass {
public:
    explicit OverlayPass(Context& context);
    ~OverlayPass();

    OverlayPass(const OverlayPass&) = delete;
    OverlayPass& operator=(const OverlayPass&) = delete;

    /**
     * (Re)creates the pipeline for the provided scene render pass of \p format. The GPU
     * must not be using the previous pipeline anymore.
     */
    void setRenderPass(VkRenderPass renderPass, const SceneFormat& format);

    /**
     * Records the draw of \p texture into the current viewport and scissor of \p cmd.
     * The viewport has to be flipped (negative height) like the one passed to the draw
     * callback, so that the bottom row of the texture ends up at the bottom.
     */
    void draw(VkCommandBuffer cmd, const Texture& texture) const;

    /**
     * Records the draw of an image in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL. The image
     * has to be stored with its first row at the top, unlike a Texture, so it is not
     * flipped.
     */
    void draw(VkCommandBuffer cmd, VkImageView view, VkSampler sampler) const;

private:
    void record(VkCommandBuffer cmd, VkImageView view, VkSampler sampler,
        bool flipY) const;

    Context& _context;
    VkDescriptorSetLayout _setLayout = VK_NULL_HANDLE;
    VkPipelineLayout _layout = VK_NULL_HANDLE;
    std::unique_ptr<Pipeline> _pipeline;
};

} // namespace vulkan
} // namespace sgct

#endif // __SGCT__VULKAN_OVERLAYPASS__H__
