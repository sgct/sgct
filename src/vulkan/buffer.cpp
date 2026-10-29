/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/vulkan/buffer.h>

#include <sgct/error.h>
#include <algorithm>
#include <format>
#include <stdexcept>
#include <string>

namespace sgct {
namespace vulkan {

namespace {
    void check(VkResult result, const std::string& what) {
        if (result != VK_SUCCESS) {
            throw Error(
                Error::Component::Window,
                4200,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    }

    /// Finds a memory type index that satisfies the required properties
    uint32_t findMemoryType(
        const Context& context, uint32_t typeFilter, VkMemoryPropertyFlags props)
    {
        VkPhysicalDeviceMemoryProperties memoryProperties = {};
        vkGetPhysicalDeviceMemoryProperties(context.physicalDevice(), &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memoryProperties.memoryTypes[i].propertyFlags & props) == props)
            {
                return i;
            }
        }
        throw Error(
            Error::Component::Window, 4201, "Failed to find a suitable Vulkan memory type"
        );
    }
} // namespace

Buffer::Buffer(Context& context, VkDeviceSize size, Type type)
    : _context(context)
    , _size(size)
{
    // Device-local buffers are filled by a staging copy in `upload`, which requires them
    // to be valid transfer destinations
    switch (type) {
        case Type::Vertex:
            createBuffer(
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
            break;
        case Type::Index:
            createBuffer(
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
            break;
        case Type::Uniform:
            createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            _hostVisible = true;
            break;
        case Type::Storage:
            createBuffer(
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
            break;
        case Type::Staging:
            createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            _hostVisible = true;
            break;
        case Type::DynamicVertex:
            createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            _hostVisible = true;
            break;
        case Type::Readback:
            createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            _hostVisible = true;
            break;
    }
}

Buffer::~Buffer() {
    if (_mapped) {
        vkUnmapMemory(_context.device(), _memory);
        _mapped = nullptr;
    }
    if (_memory != VK_NULL_HANDLE) {
        vkFreeMemory(_context.device(), _memory, nullptr);
        _memory = VK_NULL_HANDLE;
    }
    if (_buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(_context.device(), _buffer, nullptr);
        _buffer = VK_NULL_HANDLE;
    }
}

void Buffer::createBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags props) {
    VkBufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = _size;
    createInfo.usage = usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    check(vkCreateBuffer(_context.device(), &createInfo, nullptr, &_buffer),
        "vkCreateBuffer");
    allocateMemory(props);
}

void Buffer::allocateMemory(VkMemoryPropertyFlags props) {
    VkMemoryRequirements requirements = {};
    vkGetBufferMemoryRequirements(_context.device(), _buffer, &requirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = requirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        _context, requirements.memoryTypeBits, props
    );

    check(vkAllocateMemory(_context.device(), &allocInfo, nullptr, &_memory),
        "vkAllocateMemory");
    check(vkBindBufferMemory(_context.device(), _buffer, _memory, 0),
        "vkBindBufferMemory");
}

void Buffer::upload(const void* data, VkDeviceSize size) {
    if (_hostVisible) {
        void* mapped = map();
        std::copy_n(
            static_cast<const uint8_t*>(data),
            size == VK_WHOLE_SIZE ? _size : size,
            static_cast<uint8_t*>(mapped)
        );
        return;
    }

    // Device-local: copy through a one-time staging buffer
    Buffer staging(_context, size == VK_WHOLE_SIZE ? _size : size, Type::Staging);
    staging.upload(data, size == VK_WHOLE_SIZE ? _size : size);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _context.commandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    check(
        vkAllocateCommandBuffers(_context.device(), &allocInfo, &commandBuffer),
        "vkAllocateCommandBuffers"
    );

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(vkBeginCommandBuffer(commandBuffer, &beginInfo), "vkBeginCommandBuffer");

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size == VK_WHOLE_SIZE ? _size : size;
    vkCmdCopyBuffer(commandBuffer, staging.handle(), _buffer, 1, &copyRegion);

    check(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer");

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    check(vkQueueSubmit(_context.queue(), 1, &submitInfo, VK_NULL_HANDLE),
        "vkQueueSubmit");
    check(vkQueueWaitIdle(_context.queue()), "vkQueueWaitIdle");

    vkFreeCommandBuffers(
        _context.device(), _context.commandPool(), 1, &commandBuffer
    );
}

void* Buffer::map() const {
    // Host-visible buffers stay persistently mapped from the first call until the buffer
    // is destroyed; mapping memory that is already mapped is not allowed
    if (!_mapped) {
        check(
            vkMapMemory(_context.device(), _memory, 0, _size, 0, &_mapped),
            "vkMapMemory"
        );
    }
    return _mapped;
}

} // namespace vulkan
} // namespace sgct
