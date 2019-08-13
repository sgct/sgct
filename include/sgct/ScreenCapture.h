/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__SCREEN_CAPTURE__H__
#define __SGCT__SCREEN_CAPTURE__H__

#include <sgct/ogl_headers.h>
#include <sgct/SGCTSettings.h>
#include <mutex>
#include <string>
#include <thread>

namespace sgct_core {

class Image;

struct ScreenCaptureThreadInfo {
    Image* mframeBufferImagePtr = nullptr;
    std::thread* mFrameCaptureThreadPtr = nullptr;
    std::mutex* mMutexPtr = nullptr;
    bool mRunning = false; //needed for test if running without join
};

/*!
    This class is used internally by SGCT and is called when using the takeScreenshot function from the Engine.
    Screenshots are saved as PNG or TGA images and and can also be used for movie recording.
*/
class ScreenCapture {
public:
    //! The different file formats supported
    enum CaptureFormat {
        NOT_SET = -1,
        PNG = 0,
        TGA,
        JPEG
    };
    enum CaptureSrc {
        CAPTURE_TEXTURE = 0,
        CAPTURE_BACK_BUFFER = GL_BACK,
        CAPTURE_LEFT_BACK_BUFFER = GL_BACK_LEFT,
        CAPTURE_RIGHT_BACK_BUFFER = GL_BACK_RIGHT
    };
    enum EyeIndex {
        MONO = 0,
        STEREO_LEFT,
        STEREO_RIGHT
    };

    ~ScreenCapture();

    void init(size_t windowIndex, EyeIndex ei);
    void initOrResize(int x, int y, int channels, int bytesPerColor);
    void setTextureTransferProperties(unsigned int type, bool preferBGR);
    void setCaptureFormat(CaptureFormat cf);
    CaptureFormat getCaptureFormat();
    void saveScreenCapture(unsigned int textureId, CaptureSrc CapSrc = CAPTURE_TEXTURE);
    void setPathAndFileName(std::string path, std::string filename);
    void setUsePBO(bool state);

    void setCaptureCallback(
        std::function<void(Image*, size_t, EyeIndex, unsigned int type)> callback);
    void setCaptureCallback(
        std::function<void(unsigned char*, size_t, EyeIndex, unsigned int type)> callback);

    // move to private
    std::function<void(Image*, size_t, EyeIndex, unsigned int type)> mCaptureCallbackFn1;
    std::function<void(unsigned char*, size_t, EyeIndex, unsigned int type)> mCaptureCallbackFn2;

private:
    void addFrameNumberToFilename(unsigned int frameNumber);
    int getAvailableCaptureThread();
    void updateDownloadFormat();
    void checkImageBuffer(const CaptureSrc& CapSrc);
    Image* prepareImage(int index);

    std::mutex mMutex;
    ScreenCaptureThreadInfo* mSCTIPtrs = nullptr;

    unsigned int mNumberOfThreads =
        sgct::SGCTSettings::instance()->getNumberOfCaptureThreads();
    unsigned int mPBO = 0;
    unsigned int mDownloadFormat = GL_BGRA;
    unsigned int mDownloadType = GL_UNSIGNED_BYTE;
    unsigned int mDownloadTypeSetByUser = mDownloadType;
    int mDataSize = 0;
    int mX;
    int mY;
    int mChannels;
    int mBytesPerColor = 1;

    std::string mFilename;
    std::string mBaseName;
    std::string mPath;
    bool mUsePBO = true;
    bool mPreferBGR = true;
    EyeIndex mEyeIndex = MONO;
    CaptureFormat mFormat = PNG;
    size_t mWindowIndex = 0;
};

} // namespace sgct_core

#endif // __SGCT__SCREEN_CAPTURE__H__