/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__SCREENCAPTURE__H__
#define __SGCT__SCREENCAPTURE__H__

#include <sgct/sgctexports.h>

#include <sgct/math.h>
#include <vulkan/vulkan.h>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace sgct {

namespace vulkan { class Buffer; }
class Image;
class Window;

/**
 * Captures images of a window and saves them as PNG files. A capture is taken in two
 * steps: #record adds the copy of an image into a host-visible readback buffer to the
 * command buffer of the frame, and once that command buffer has finished executing,
 * #save converts the pixels and writes the file on one of the capture threads.
 */
class SGCT_EXPORT ScreenCapture {
public:
    enum class EyeIndex { Mono, StereoLeft, StereoRight };

    struct ScreenCaptureThreadInfo {
        std::string filename;
        std::unique_ptr<Image> frameBufferImage;
        std::unique_ptr<std::thread> captureThread;
        std::mutex* mutex = nullptr;
        bool isRunning = false; // needed for test if running without join
    };

    ScreenCapture(const Window& window, ScreenCapture::EyeIndex ei, bool addAlpha);
    ~ScreenCapture();

    /**
     * Records the copy of \p image into the readback buffer. The image is transitioned to
     * the transfer source layout for the copy and back to \p layout afterwards.
     *
     * \param cmd The command buffer to record into; it must not be inside a render pass
     * \param image The image to capture
     * \param layout The layout of the image at this point of the command buffer, either
     *        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL (an offscreen target that has been
     *        sampled by the composite pass) or VK_IMAGE_LAYOUT_PRESENT_SRC_KHR (a
     *        swapchain image whose render pass has finished)
     * \param extent The size of the image
     * \param format The format of the image: 8-bit RGBA and BGRA, which are saved with 8
     *        bits per channel, and 16-bit normalized, 16-bit float, and 32-bit float RGBA,
     *        which are saved with 16 bits per channel (the floats clamped to [0, 1])
     * \param layer The layer of the image to capture
     *
     * \return `true` if the copy was recorded and #save has to be called once \p cmd has
     *         finished executing, `false` if no screenshot is taken
     */
    bool record(VkCommandBuffer cmd, VkImage image, VkImageLayout layout,
        VkExtent2D extent, VkFormat format, uint32_t layer = 0);

    /**
     * Saves the image that was copied by the last call to #record on a capture thread.
     * The command buffer passed to #record must have finished executing.
     */
    void save();

private:
    std::string createFilename(uint64_t frameNumber);
    int availableCaptureThread();
    Image* prepareImage(int index, std::string file);

    std::mutex _mutex;
    std::vector<ScreenCaptureThreadInfo> _captureInfos;

    const unsigned int _nThreads;
    ivec2 _resolution = ivec2{ 0, 0 };
    const bool _addAlpha;

    std::unique_ptr<vulkan::Buffer> _readback;
    VkFormat _readbackFormat = VK_FORMAT_UNDEFINED;
    std::string _pendingFilename;
    bool _hasPendingCapture = false;

    const EyeIndex _eyeIndex;
    const Window& _window;
};

} // namespace sgct

#endif // __SGCT__SCREENCAPTURE__H__
