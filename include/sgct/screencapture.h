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

#include <sgct/glm.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace sgct {

class Image;
class Window;

/**
 * This class is used internally by SGCT and is called when taking screenshots.
 */
class SGCT_EXPORT ScreenCapture {
public:
    /**
     * The different file formats supported.
     */
    enum class CaptureSource { Texture, BackBuffer, LeftBackBuffer, RightBackBuffer };
    enum class EyeIndex { Mono, StereoLeft, StereoRight };

    struct ScreenCaptureThreadInfo {
        std::string filename;
        std::unique_ptr<Image> frameBufferImage;
        std::unique_ptr<std::thread> captureThread;
        std::mutex* mutex = nullptr;
        // Needed to test if the thread is running without joining it. It is set by the
        // capture thread and read by the main thread
        std::atomic_bool isRunning = false;
    };

    ScreenCapture(const Window& window, ScreenCapture::EyeIndex ei, int bytesPerColor,
        unsigned int colorDataType, bool addAlpha);
    ~ScreenCapture();

    /**
     * Initializes the PBO or re-sizes it if the frame buffer size have changed.
     *
     * \param resolution The pixel resolution of the frame buffer
     */
    void resize(glm::ivec2 resolution);

    /**
     * This function starts to copy the image from the GPU. The image is saved to disk by
     * a later call to #update, by the next screenshot, or when this object is destroyed,
     * whichever comes first, so that the rendering does not have to wait for the GPU.
     *
     * \param textureId The texture that will be streamed from the GPU if frame buffer
     *        objects are used in the rendering
     * \param capSrc The object that should be captured
     */
    void saveScreenCapture(unsigned int textureId,
        CaptureSource capSrc = CaptureSource::Texture);

    /**
     * Saves the image of the last screenshot to disk if the GPU has finished copying it.
     * This function does not wait for the GPU and has to be called once per frame.
     */
    void update();

private:
    std::string createFilename(uint64_t frameNumber);
    int availableCaptureThread();
    Image* prepareImage(int index, std::string file);

    /**
     * Reads the pixels of the pending screenshot from the buffer and starts a thread that
     * saves them to disk.
     *
     * \param wait If this is `true`, the function waits until the GPU has finished
     *        copying the pixels. Otherwise it returns if the pixels are not available yet
     */
    void finishCapture(bool wait);

    std::mutex _mutex;
    std::vector<ScreenCaptureThreadInfo> _captureInfos;

    const unsigned int _nThreads;
    unsigned int _pbo = 0;
    /// The OpenGL sync object of the screenshot that has not been read from the GPU yet
    void* _pendingSync = nullptr;
    std::string _pendingFilename;
    const unsigned int _downloadType;
    int _dataSize = 0;
    glm::ivec2 _resolution = glm::ivec2(0, 0);
    const int _bytesPerColor;
    const bool _addAlpha;

    const EyeIndex _eyeIndex;
    const Window& _window;
};

} // namespace sgct

#endif // __SGCT__SCREENCAPTURE__H__
