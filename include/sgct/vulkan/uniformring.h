/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VULKAN_UNIFORMRING__H__
#define __SGCT__VULKAN_UNIFORMRING__H__

#include <sgct/sgctexports.h>
#include <sgct/vulkan/buffer.h>
#include <sgct/vulkan/context.h>
#include <sgct/vulkan/swapchain.h>

#include <vulkan/vulkan.h>

#include <cstdint>

namespace sgct {
namespace vulkan {

/**
 * A host-visible uniform buffer from which per-draw uniform blocks are sub-allocated.
 * The draw callback is invoked once per viewport and eye into the same command buffer,
 * so writing to a single uniform buffer would let later calls overwrite the values of
 * earlier ones before the GPU executes them. Instead, every draw pushes its uniform block
 * into this ring and binds it through a `VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC`
 * descriptor, passing the returned offset as the dynamic offset.
 *
 * The buffer is split into `Swapchain::MaxFramesInFlight + 1` segments and each engine
 * frame writes into the segment `frameNumber % segmentCount`. When frame N starts
 * writing, every window has already waited on its submission of frame N - 3 (while
 * acquiring its image in frame N - 1), so the segment is no longer read by the GPU.
 */
class SGCT_EXPORT UniformRing {
public:
    /**
     * \param context The shared Vulkan context
     * \param bytesPerFrame The capacity of each frame's segment; it has to hold all of the
     *        uniform blocks pushed in one frame across all windows, viewports, and eyes,
     *        each rounded up to the device's `minUniformBufferOffsetAlignment`
     */
    UniformRing(Context& context, VkDeviceSize bytesPerFrame);

    UniformRing(const UniformRing&) = delete;
    UniformRing& operator=(const UniformRing&) = delete;

    /**
     * Copies \p size bytes from \p data into the segment of the frame \p frameNumber. The
     * segment is reset the first time it is used in a new frame.
     *
     * \param frameNumber The engine frame that is being recorded (`RenderData::frameNumber`)
     * \param data The uniform block to copy
     * \param size The size of the uniform block in bytes
     *
     * \return The offset of the copy in the buffer, to be used as the dynamic offset
     * \throw Error If the segment does not have enough space left for the block
     */
    uint32_t push(uint64_t frameNumber, const void* data, VkDeviceSize size);

    template <typename T>
    uint32_t push(uint64_t frameNumber, const T& value) {
        return push(frameNumber, &value, sizeof(T));
    }

    VkBuffer handle() const { return _buffer.handle(); }

private:
    static constexpr uint32_t SegmentCount = Swapchain::MaxFramesInFlight + 1;

    Buffer _buffer;
    VkDeviceSize _segmentSize = 0;
    VkDeviceSize _alignment = 1;

    uint64_t _frameNumber = 0;
    bool _hasFrame = false;
    VkDeviceSize _head = 0;
};

} // namespace vulkan
} // namespace sgct

#endif // __SGCT__VULKAN_UNIFORMRING__H__
