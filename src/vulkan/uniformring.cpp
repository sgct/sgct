/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/vulkan/uniformring.h>

#include <sgct/error.h>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <format>
#include <limits>

namespace sgct {
namespace vulkan {

namespace {
    VkDeviceSize alignUp(VkDeviceSize value, VkDeviceSize alignment) {
        return (value + alignment - 1) / alignment * alignment;
    }
} // namespace

UniformRing::UniformRing(Context& context, VkDeviceSize bytesPerFrame)
    : _buffer(
        context,
        alignUp(
            bytesPerFrame,
            std::max<VkDeviceSize>(context.limits().minUniformBufferOffsetAlignment, 1)
        ) * SegmentCount,
        Buffer::Type::Uniform
    )
    , _segmentSize(_buffer.size() / SegmentCount)
    , _alignment(std::max<VkDeviceSize>(context.limits().minUniformBufferOffsetAlignment, 1))
{
    if (_buffer.size() > std::numeric_limits<uint32_t>::max()) {
        // Dynamic offsets are 32-bit values
        throw Error(
            Error::Component::Window,
            4420,
            std::format("Uniform ring of {} bytes is too large", _buffer.size())
        );
    }
}

uint32_t UniformRing::push(uint64_t frameNumber, const void* data, VkDeviceSize size) {
    if (!_hasFrame || frameNumber != _frameNumber) {
        _frameNumber = frameNumber;
        _hasFrame = true;
        _head = 0;
    }

    if (_head + size > _segmentSize) {
        throw Error(
            Error::Component::Window,
            4421,
            std::format(
                "Uniform ring is out of space: {} bytes requested, {} of {} bytes used",
                size, _head, _segmentSize
            )
        );
    }

    const VkDeviceSize offset = (frameNumber % SegmentCount) * _segmentSize + _head;
    std::memcpy(static_cast<std::byte*>(_buffer.map()) + offset, data, size);
    _head = alignUp(_head + size, _alignment);
    return static_cast<uint32_t>(offset);
}

} // namespace vulkan
} // namespace sgct
