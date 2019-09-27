/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

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
    enum class CaptureFormat {
        PNG = 0,
        TGA,
        JPEG
    };

    enum class CaptureSource {
        Texture = 0,
        BackBuffer,
        LeftBackBuffer,
        RightBackBuffer
    };

    enum class EyeIndex {
        Mono = 0,
        StereoLeft,
        StereoRight
    };

    struct ScreenCaptureThreadInfo {
        std::unique_ptr<Image> mFrameBufferImage;
        std::unique_ptr<std::thread> mFrameCaptureThread;
        std::mutex* mMutex = nullptr;
        bool mRunning = false; // needed for test if running without join
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
    void setTextureTransferProperties(unsigned int type, bool preferBGR);

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
        std::function<void(Image*, size_t, EyeIndex, unsigned int type)> callback);

private:
    void addFrameNumberToFilename(unsigned int frameNumber);
    int getAvailableCaptureThread();
    void updateDownloadFormat();
    void checkImageBuffer(CaptureSource CapSrc);
    Image* prepareImage(int index);

    std::mutex mMutex;
    std::vector<ScreenCaptureThreadInfo> mSCTIs;

    unsigned int mNumberOfThreads;
    unsigned int mPBO = 0;
    unsigned int mDownloadFormat = GL_BGRA;
    unsigned int mDownloadType = GL_UNSIGNED_BYTE;
    unsigned int mDownloadTypeSetByUser = mDownloadType;
    int mDataSize = 0;
    glm::ivec2 mResolution;
    int mChannels;
    int mBytesPerColor = 1;

    std::function<void(Image*, size_t, EyeIndex, unsigned int type)> mCaptureCallbackFn;

    std::string mFilename;
    std::string mBaseName;
    std::string mPath;
    bool mUsePBO = true;
    bool mPreferBGR = true;
    EyeIndex mEyeIndex = EyeIndex::Mono;
    CaptureFormat mFormat = CaptureFormat::PNG;
    int mWindowIndex = 0;
};

} // namespace sgct::core

#endif // __SGCT__SCREEN_CAPTURE__H__
