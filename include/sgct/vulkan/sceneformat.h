/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VULKAN_SCENEFORMAT__H__
#define __SGCT__VULKAN_SCENEFORMAT__H__

#include <sgct/sgctexports.h>
#include <sgct/vulkan/context.h>

#include <vulkan/vulkan.h>

#include <compare>
#include <cstdint>
#include <vector>

namespace sgct {
namespace vulkan {

/// The format of the depth attachment of the scene render passes
constexpr VkFormat SceneDepthFormat = VK_FORMAT_D32_SFLOAT;

/// The format of the normal and position attachments of the scene render passes
constexpr VkFormat SceneDataFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

/**
 * The attachments of the render passes that the scene is rendered with. The pipelines of
 * the draw callback have to be created for this format (see Pipeline::Settings::scene),
 * which is the same for the offscreen targets of a window and the cube maps of its
 * non-linear projections.
 *
 * The fragment shaders write the color to location 0. If the format has normals or
 * positions, they are written to location 1 and 2, respectively.
 */
struct SGCT_EXPORT SceneFormat {
    VkFormat color = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    /// Whether there is a SceneDataFormat attachment for the normals at location 1
    bool hasNormals = false;
    /// Whether there is a SceneDataFormat attachment for the positions at location 2
    bool hasPositions = false;
    /// Whether the depth is kept after the render pass so that it can be sampled
    bool hasDepthTexture = false;

    /// The number of color attachments of the subpass, which is the number of color
    /// blend attachments that the pipelines need. Locations without an attachment (the
    /// normals if only the positions are used) are unused
    uint32_t colorAttachmentCount() const;

    auto operator<=>(const SceneFormat&) const = default;
};

/**
 * Creates a render pass for \p format. All attachments are cleared, and the
 * single-sampled images that are kept (the color, the normals and positions, and the
 * depth if it is a texture) end up in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL to be
 * sampled afterwards. All scene render passes are created by this function, so that the
 * render passes of the same format are compatible. The caller owns the render pass.
 *
 * The attachments are, in this order:
 *   - Without multisampling: color, [normals], [positions], depth
 *   - With multisampling: multisampled color, [normals], [positions], and depth, followed
 *     by the single-sampled color, [normals], [positions], and [depth] that they are
 *     resolved into
 *
 * \throw sgct::Error If the render pass cannot be created
 */
SGCT_EXPORT VkRenderPass createSceneRenderPass(Context& context,
    const SceneFormat& format);

/**
 * \return The clear values for the attachments of a scene render pass of \p format: the
 *         colors, normals, and positions are cleared to 0 and the depth to 1
 */
SGCT_EXPORT std::vector<VkClearValue> sceneClearValues(const SceneFormat& format);

/**
 * \return The highest sample count of at most \p samples that the device supports for
 *         color and depth attachments
 */
SGCT_EXPORT VkSampleCountFlagBits supportedSampleCount(const Context& context,
    int samples);

} // namespace vulkan
} // namespace sgct

#endif // __SGCT__VULKAN_SCENEFORMAT__H__
