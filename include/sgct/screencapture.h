/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__SCREEN_CAPTURE__H__
#define __SGCT__SCREEN_CAPTURE__H__

#include <sgct/ogl_headers.h>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <glm/glm.hpp>

namespace sgct::core {

class Image;

/**
 * This class is used internally by SGCT and is called when using the takeScreenshot
 * function from the Engine.
 * 
 * Screenshots are saved as PNG or TGA images and and can also be used for movie recording
 */
class ScreenCapture {
public:
    /// The different file formats supported
    enum class CaptureFormat { PNG, TGA, JPEG };
    enum class CaptureSource { Texture, BackBuffer, LeftBackBuffer, RightBackBuffer };
    enum class EyeIndex { Mono, StereoLeft, StereoRight };

    struct ScreenCaptureThreadInfo {
        std::string filename;
        std::unique_ptr<Image> frameBufferImage;
        std::unique_ptr<std::thread> captureThread;
        std::mutex* mutex = nullptr;
        bool isRunning = false; // needed for test if running without join
    };

    ScreenCapture();
    ~ScreenCapture();

    void init(int windowIndex, EyeIndex ei);

    /**
     * Initializes the pixel buffer object (PBO) or re-sizes it if the frame buffer size
     * have changed.
     *
     * \param resolution the  pixel resolution of the frame buffer
     * \param channels the number of color channels
     *
     * If PBOs are not supported nothing will and the screenshot process will fall back on
     * slower GPU data fetching.
     */
    void initOrResize(glm::ivec2 resolution, int channels, int bytesPerColor);

    /**
     * Set the opengl texture properties for glGetTexImage.
     * Type can be: GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_HALF_FLOAT, GL_FLOAT,
     * GL_SHORT, GL_INT, GL_UNSIGNED_SHORT or GL_UNSIGNED_INT
     */
    void setTextureTransferProperties(GLenum type);

    /// Set the image format to use
    void setCaptureFormat(CaptureFormat cf);

    /// Get the image format
    CaptureFormat getCaptureFormat() const;

    /**
     * This function saves the images to disc.
     *
     * \param textureId textureId is the texture that will be streamed from the GPU if
     *        frame buffer objects are used in the rendering.
     */
    void saveScreenCapture(unsigned int textureId,
        CaptureSource capSrc = CaptureSource::Texture);
    void setPathAndFileName(std::string path, std::string filename);
    void setUsePBO(bool state);

    /**
     * Set the screen capture callback
     *
     * Parameters are: image pointer to captured image, window index, eye index and
     * OpenGL type (GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_HALF_FLOAT, GL_FLOAT,
     * GL_SHORT, GL_INT, GL_UNSIGNED_SHORT or GL_UNSIGNED_INT)
     */
    void setCaptureCallback(
        std::function<void(Image*, size_t, EyeIndex, GLenum type)> callback);

private:
    void addFrameNumberToFilename(unsigned int frameNumber);
    int getAvailableCaptureThread();
    void updateDownloadFormat();
    void checkImageBuffer(CaptureSource CapSrc);
    Image* prepareImage(int index);

    std::mutex _mutex;
    std::vector<ScreenCaptureThreadInfo> _captureInfos;

    unsigned int _nThreads;
    unsigned int _pbo = 0;
    GLenum _downloadFormat = GL_BGRA;
    GLenum _downloadType = GL_UNSIGNED_BYTE;
    GLenum _downloadTypeSetByUser = _downloadType;
    int _dataSize = 0;
    glm::ivec2 _resolution;
    int _nChannels = 0;
    int _bytesPerColor = 1;

    std::function<void(Image*, size_t, EyeIndex, GLenum type)> _captureCallback;

    std::string _filename;
    std::string _baseName;
    std::string _path;
    EyeIndex _eyeIndex = EyeIndex::Mono;
    CaptureFormat _format = CaptureFormat::PNG;
    int _windowIndex = 0;
};

} // namespace sgct::core

#endif // __SGCT__SCREEN_CAPTURE__H__
