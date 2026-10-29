/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__VULKAN_BUFFER__H__
#define __SGCT__VULKAN_BUFFER__H__

#include <sgct/sgctexports.h>
#include <sgct/vulkan/context.h>

#include <vulkan/vulkan.h>

#include <cstddef>

namespace sgct {
namespace vulkan {

/**
 * A VkBuffer with its backing VkDeviceMemory. Vertex, index, and storage buffers are
 * device-local and are filled through a one-time staging upload. Uniform, staging,
 * dynamic vertex, and readback buffers are host-visible and can be mapped directly.
 * Dynamic vertex buffers are vertex buffers that are rewritten by the host every frame,
 * and readback buffers are the destination of copies from the GPU to the host.
 */
class SGCT_EXPORT Buffer {
public:
    enum class Type { Vertex, Index, Uniform, Storage, Staging, DynamicVertex, Readback };

    /**
     * \param context The shared Vulkan context
     * \param size The size of the buffer in bytes
     * \param type The intended use of the buffer
     */
    Buffer(Context& context, VkDeviceSize size, Type type);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    /**
     * Copies \p size bytes from \p data into the buffer. For device-local buffers this uses
     * a one-time staging buffer and a command buffer submit; for host-visible buffers it
     * maps the buffer and copies directly.
     *
     * \param data The source data
     * \param size The number of bytes to copy
     */
    void upload(const void* data, VkDeviceSize size = VK_WHOLE_SIZE);

    /**
     * Returns a pointer to the contents of a host-visible buffer (Uniform, Staging,
     * DynamicVertex, or Readback type). The buffer is mapped on the first call and stays mapped until it is
     * destroyed, so the pointer remains valid for the lifetime of the buffer.
     *
     * \return A pointer to the mapped memory
     */
    void* map() const;

    VkBuffer handle() const { return _buffer; }
    VkDeviceSize size() const { return _size; }
    bool isHostVisible() const { return _hostVisible; }

private:
    void createBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags props);
    void allocateMemory(VkMemoryPropertyFlags props);

    Context& _context;
    VkBuffer _buffer = VK_NULL_HANDLE;
    VkDeviceMemory _memory = VK_NULL_HANDLE;
    VkDeviceSize _size = 0;
    bool _hostVisible = false;
    mutable void* _mapped = nullptr;
};

} // namespace vulkan
} // namespace sgct

#endif // __SGCT__VULKAN_BUFFER__H__
