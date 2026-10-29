/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/screencapture.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/format.h>
#include <sgct/image.h>
#include <sgct/log.h>
#include <sgct/profiling.h>
#include <sgct/window.h>
#include <sgct/vulkan/buffer.h>
#include <sgct/vulkan/context.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {
    // The size of a pixel of the formats that screenshots can be taken of, or 0
    uint32_t bytesPerPixel(VkFormat format) {
        switch (format) {
            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_SRGB:
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_SRGB:
                return 4;
            case VK_FORMAT_R16G16B16A16_UNORM:
            case VK_FORMAT_R16G16B16A16_SFLOAT:
                return 8;
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return 16;
            default:
                return 0;
        }
    }

    // The 8-bit formats are saved with 8 bits per channel, all others with 16 bits
    bool is8Bit(VkFormat format) {
        return bytesPerPixel(format) == 4;
    }

    float halfToFloat(uint16_t h) {
        const uint32_t sign = (h >> 15) & 0x1;
        const uint32_t exponent = (h >> 10) & 0x1f;
        const uint32_t mantissa = h & 0x3ff;
        float value = 0.f;
        if (exponent == 0) {
            value = std::ldexp(static_cast<float>(mantissa), -24);
        }
        else if (exponent == 31) {
            value = mantissa == 0 ? std::numeric_limits<float>::infinity() : 0.f;
        }
        else {
            value = std::ldexp(static_cast<float>(mantissa | 0x400), exponent - 25);
        }
        return sign ? -value : value;
    }

    uint16_t toUnorm16(float value) {
        return static_cast<uint16_t>(std::clamp(value, 0.f, 1.f) * 65535.f + 0.5f);
    }

    // The RGBA channels of a pixel: the bytes of the 8-bit formats and 16-bit values
    // otherwise, for which the floating point values are clamped to [0, 1]
    std::array<uint16_t, 4> channels(const uint8_t* p, VkFormat format) {
        switch (format) {
            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_SRGB:
                return { p[2], p[1], p[0], p[3] };
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_SRGB:
                return { p[0], p[1], p[2], p[3] };
            case VK_FORMAT_R16G16B16A16_UNORM:
            {
                std::array<uint16_t, 4> values;
                std::memcpy(values.data(), p, sizeof(values));
                return values;
            }
            case VK_FORMAT_R16G16B16A16_SFLOAT:
            {
                std::array<uint16_t, 4> halfs;
                std::memcpy(halfs.data(), p, sizeof(halfs));
                return {
                    toUnorm16(halfToFloat(halfs[0])),
                    toUnorm16(halfToFloat(halfs[1])),
                    toUnorm16(halfToFloat(halfs[2])),
                    toUnorm16(halfToFloat(halfs[3]))
                };
            }
            case VK_FORMAT_R32G32B32A32_SFLOAT:
            {
                std::array<float, 4> floats;
                std::memcpy(floats.data(), p, sizeof(floats));
                return {
                    toUnorm16(floats[0]),
                    toUnorm16(floats[1]),
                    toUnorm16(floats[2]),
                    toUnorm16(floats[3])
                };
            }
            default:
                throw std::logic_error("Unhandled case label");
        }
    }
} // namespace

namespace sgct {

ScreenCapture::ScreenCapture(const Window& window, ScreenCapture::EyeIndex ei,
                             bool addAlpha)
    : _nThreads(Engine::instance().settings().capture.nCaptureThreads)
    , _addAlpha(addAlpha)
    , _eyeIndex(ei)
    , _window(window)
{
    _captureInfos.resize(_nThreads);
    for (unsigned int i = 0; i < _nThreads; i++) {
        _captureInfos[i].frameBufferImage = nullptr;
        _captureInfos[i].captureThread = nullptr;
        _captureInfos[i].mutex = &_mutex;
        _captureInfos[i].isRunning = false;
    }
    Log::Debug(std::format("Number of screencapture threads is set to {}", _nThreads));
}

ScreenCapture::~ScreenCapture() {
    for (ScreenCaptureThreadInfo& info : _captureInfos) {
        // Kill threads that are still running
        if (info.captureThread) {
            info.captureThread->join();
            info.captureThread = nullptr;
        }

        const std::unique_lock lock(_mutex);
        info.frameBufferImage = nullptr;
        info.isRunning = false;
    }

    // The readback buffer is only used by the command buffers of the window, which are
    // finished when the window is closed
    _readback = nullptr;
}

bool ScreenCapture::record(VkCommandBuffer cmd, VkImage image, VkImageLayout layout,
                           VkExtent2D extent, VkFormat format, uint32_t layer)
{
    ZoneScoped;

    uint64_t number = Engine::instance().screenShotNumber();
    if (Engine::instance().settings().capture.limits) {
        uint64_t begin = Engine::instance().settings().capture.limits->first;
        uint64_t end = Engine::instance().settings().capture.limits->second;

        if (number < begin || number >= end) {
            Log::Debug(std::format(
                "Skipping screenshot {} outside range [{}, {}]", number, begin, end
            ));
            return false;
        }
    }

    const uint32_t pixelSize = bytesPerPixel(format);
    if (pixelSize == 0) {
        Log::Error(std::format(
            "Cannot take a screenshot of an image with format {}", static_cast<int>(format)
        ));
        return false;
    }
    if (extent.width == 0 || extent.height == 0) {
        return false;
    }

    const VkDeviceSize size =
        static_cast<VkDeviceSize>(extent.width) * extent.height * pixelSize;
    if (!_readback || _readback->size() < size) {
        _readback = std::make_unique<vulkan::Buffer>(
            vulkan::Context::instance(), size, vulkan::Buffer::Type::Readback
        );
    }

    // The image was last used either by the composite pass, which sampled the offscreen
    // target in its fragment shader, or as the color attachment of the composite pass,
    // whose render pass makes its writes available to the transfer stage
    const bool isSwapchainImage = layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    const VkPipelineStageFlags previousStage = isSwapchainImage ?
        VK_PIPELINE_STAGE_TRANSFER_BIT :
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    VkImageMemoryBarrier toTransfer = {};
    toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toTransfer.srcAccessMask = 0;
    toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    toTransfer.oldLayout = layout;
    toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.image = image;
    toTransfer.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, layer, 1 };
    vkCmdPipelineBarrier(
        cmd,
        previousStage,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &toTransfer
    );

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, layer, 1 };
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { extent.width, extent.height, 1 };
    vkCmdCopyImageToBuffer(
        cmd,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        _readback->handle(),
        1,
        &region
    );

    // Return the image to its layout. The next writes to an offscreen target happen in
    // the next frame's render pass, which waits for the fragment shader stage
    VkImageMemoryBarrier fromTransfer = toTransfer;
    fromTransfer.srcAccessMask = 0;
    fromTransfer.dstAccessMask = 0;
    fromTransfer.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    fromTransfer.newLayout = layout;

    // Make the copied pixels visible to the host once the frame's fence is signaled
    VkMemoryBarrier toHost = {};
    toHost.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    toHost.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toHost.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT | (isSwapchainImage ?
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT :
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT),
        0,
        1, &toHost,
        0, nullptr,
        1, &fromTransfer
    );

    _resolution = ivec2{ static_cast<int>(extent.width), static_cast<int>(extent.height) };
    _readbackFormat = format;
    _pendingFilename = createFilename(number);
    _hasPendingCapture = true;
    return true;
}

void ScreenCapture::save() {
    ZoneScoped;

    if (!_hasPendingCapture) {
        return;
    }
    _hasPendingCapture = false;

    const int threadIndex = availableCaptureThread();
    if (threadIndex == -1) {
        Log::Error("Error finding available capture thread");
        return;
    }

    Image* imPtr = prepareImage(threadIndex, std::move(_pendingFilename));
    if (!imPtr) {
        return;
    }

    // The Image stores the pixels in BGR(A) order with the bottom row first (as OpenGL
    // read them back), whereas the copy of the Vulkan image starts with the top row
    const int nChannels = _addAlpha ? 4 : 3;
    const size_t w = static_cast<size_t>(_resolution.x);
    const size_t h = static_cast<size_t>(_resolution.y);
    const size_t pixelSize = bytesPerPixel(_readbackFormat);
    const uint8_t* src = static_cast<const uint8_t*>(_readback->map());
    unsigned char* dst = imPtr->data();
    const int bytesPerChannel = imPtr->bytesPerChannel();
    for (size_t y = 0; y < h; y++) {
        const uint8_t* srcRow = src + (h - 1 - y) * w * pixelSize;
        unsigned char* dstRow = dst + y * w * nChannels * bytesPerChannel;
        for (size_t x = 0; x < w; x++) {
            // RGBA in [0, 1] (or the raw bytes for the 8-bit formats)
            const std::array<uint16_t, 4> rgba =
                channels(srcRow + x * pixelSize, _readbackFormat);
            unsigned char* d = dstRow + x * nChannels * bytesPerChannel;
            const std::array<uint16_t, 4> bgra = { rgba[2], rgba[1], rgba[0], rgba[3] };
            for (int c = 0; c < nChannels; c++) {
                if (bytesPerChannel == 1) {
                    d[c] = static_cast<unsigned char>(bgra[c]);
                }
                else {
                    std::memcpy(d + 2 * c, &bgra[c], sizeof(uint16_t));
                }
            }
        }
    }

    // Save the image
    _captureInfos[threadIndex].isRunning = true;
    _captureInfos[threadIndex].captureThread = std::make_unique<std::thread>(
        [](void* arg) {
            ScreenCaptureThreadInfo* ptr = reinterpret_cast<ScreenCaptureThreadInfo*>(arg);

            try {
                ptr->frameBufferImage->save(ptr->filename);
            }
            catch (const std::runtime_error& e) {
                Log::Error(e.what());
            }
            ptr->isRunning = false;
        },
        &_captureInfos[threadIndex]
    );
}

std::string ScreenCapture::createFilename(uint64_t frameNumber) {
    const std::string eyeSuffix = [](EyeIndex eyeIndex) {
        switch (eyeIndex) {
            case EyeIndex::Mono:        return "";
            case EyeIndex::StereoLeft:  return "L";
            case EyeIndex::StereoRight: return "R";
            default:                    throw std::logic_error("Unhandled case label");
        }
    }(_eyeIndex);

    std::array<char, 6> Buffer = {};
    std::fill(Buffer.begin(), Buffer.end(), '\0');
    std::format_to_n(Buffer.data(), Buffer.size(), "{:06}", frameNumber);

    std::filesystem::path file;
    if (!Engine::instance().settings().capture.capturePath.empty()) {
        file = Engine::instance().settings().capture.capturePath / "";
    }
    if (!Engine::instance().settings().capture.prefix.empty()) {
        file += Engine::instance().settings().capture.prefix;
        file += '_';
    }
    if (Engine::instance().settings().capture.addNodeName &&
        ClusterManager::instance().numberOfNodes() > 1)
    {
        file += std::format("node{}_", ClusterManager::instance().thisNodeId());
    }
    if (Engine::instance().settings().capture.addWindowName) {
        file +=
            _window.name().empty() ? std::format("win{}", _window.id()) : _window.name();
        file += '_';
    }

    if (!eyeSuffix.empty()) {
        file += eyeSuffix + '_';
    }
    std::string bufferString = std::string(Buffer.begin(), Buffer.end());
    return std::format("{}{}.png", file, bufferString);
}

int ScreenCapture::availableCaptureThread() {
    while (true) {
        for (unsigned int i = 0; i < _captureInfos.size(); i++) {
            // Check if thread is dead
            if (_captureInfos[i].captureThread == nullptr) {
                return i;
            }

            if (!_captureInfos[i].isRunning) {
                _captureInfos[i].captureThread->join();
                _captureInfos[i].captureThread = nullptr;

                return i;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

Image* ScreenCapture::prepareImage(int index, std::string file) {
    Log::Debug(std::format("Starting thread for screenshot/capture [{}]", index));

    // The images are reused between captures unless the resolution has changed
    const int nChannels = _addAlpha ? 4 : 3;
    std::unique_ptr<Image>& img = _captureInfos[index].frameBufferImage;
    const int bytesPerChannel = is8Bit(_readbackFormat) ? 1 : 2;
    if (img && (img->size() != _resolution || img->bytesPerChannel() != bytesPerChannel)) {
        img = nullptr;
    }
    if (img == nullptr) {
        if (nChannels * _resolution.x * _resolution.y == 0) {
            return nullptr;
        }
        img = std::make_unique<Image>();
        img->setBytesPerChannel(is8Bit(_readbackFormat) ? 1 : 2);
        img->setChannels(nChannels);
        img->setSize(_resolution);
        img->allocateOrResizeData();
    }
    _captureInfos[index].filename = std::move(file);

    return _captureInfos[index].frameBufferImage.get();
}

} // namespace sgct
