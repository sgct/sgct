/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__CALLBACKDATA__H__
#define __SGCT__CALLBACKDATA__H__

#include <sgct/sgctexports.h>

#include <sgct/definitions.h>
#include <sgct/math.h>
#include <vulkan/vulkan.h>

#include <cstdint>

namespace sgct {

class BaseViewport;
class Window;

struct SGCT_EXPORT RenderData {
    const Window& window;
    const BaseViewport& viewport;
    const FrustumMode frustumMode;

    mat4 modelMatrix;
    mat4 viewMatrix;
    mat4 projectionMatrix;
    // @TODO (abock, 2019-12-03) Performance measurements needed to see whether this
    // caching is necessary
    mat4 modelViewProjectionMatrix;

    ivec2 bufferSize;

    // Vulkan frame state, valid when rendering through the Vulkan pipeline. The draw
    // callback records its commands into `commandBuffer` for the frame in flight
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    uint32_t frameIndex = 0;
    uint32_t imageIndex = 0;
    /// The engine's frame counter; per-draw uniforms are pushed into a
    /// `vulkan::UniformRing` with it so they stay valid while the frame is in flight
    uint64_t frameNumber = 0;
    /// The size of the render target the commands are recorded for
    VkExtent2D extent = {};

    /// The area of the render target covered by the viewport that is rendered. The
    /// viewport has a negative height so that the OpenGL-style projection matrices (with
    /// y pointing up in normalized device coordinates) produce an upright image. Both are
    /// already set as the dynamic viewport and scissor state of `commandBuffer`
    VkViewport vkViewport = {};
    VkRect2D vkScissor = {};
};

} // namespace sgct

#endif // __SGCT__CALLBACKDATA__H__
