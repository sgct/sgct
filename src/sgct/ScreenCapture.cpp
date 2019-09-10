/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/ScreenCapture.h>

#include <sgct/MessageHandler.h>
#include <sgct/SGCTSettings.h>
#include <sgct/SGCTWindow.h>
#include <sgct/Engine.h>
#include <sgct/Image.h>
#include <string>

namespace {
    void screenCaptureHandler(void* arg) {
        using SCTI = sgct_core::ScreenCapture::ScreenCaptureThreadInfo;
        SCTI* ptr = reinterpret_cast<SCTI*>(arg);

        const bool saveSuccess = ptr->mFrameBufferImage->save();
        if (!saveSuccess) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Error: Failed to save '%s'\n",
                ptr->mFrameBufferImage->getFilename().c_str()
            );
        }
        ptr->mRunning = false;
    }

    GLenum sourceForCaptureSource(sgct_core::ScreenCapture::CaptureSource source) {
        using Source = sgct_core::ScreenCapture::CaptureSource;
        switch (source) {
            default:
            case Source::BackBuffer: return GL_BACK;
            case Source::LeftBackBuffer: return GL_BACK_LEFT;
            case Source::RightBackBuffer: return GL_BACK_RIGHT;
        }
    }
} // namespace

namespace sgct_core {

ScreenCapture::ScreenCapture()
    : mNumberOfThreads(sgct::SGCTSettings::instance()->getNumberOfCaptureThreads())
{}

ScreenCapture::~ScreenCapture() {
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "Clearing screen capture buffers...\n"
    );

    for (ScreenCaptureThreadInfo& info : mSCTIs) {
        // kill threads that are still running
        if (info.mFrameCaptureThread) {
            info.mFrameCaptureThread->join();
            info.mFrameCaptureThread = nullptr;
        }

        std::unique_lock lock(mMutex);
        info.mFrameBufferImage = nullptr;
        info.mRunning = false;
    }

    glDeleteBuffers(1, &mPBO);
    mPBO = 0;
}

void ScreenCapture::initOrResize(glm::ivec2 resolution, int channels, int bytesPerColor) {
    glDeleteBuffers(1, &mPBO);
    mPBO = 0;

    mResolution = resolution;
    mBytesPerColor = bytesPerColor;
    
    mChannels = channels;
    mDataSize = mResolution.x * mResolution.y * mChannels * mBytesPerColor;

    updateDownloadFormat();

    std::unique_lock lock(mMutex);
    for (ScreenCaptureThreadInfo& info : mSCTIs) {
        if (info.mFrameBufferImage) {
            // kill threads that are still running
            if (info.mFrameCaptureThread) {
                info.mFrameCaptureThread->join();
                info.mFrameCaptureThread = nullptr;
            }
            info.mFrameBufferImage = nullptr;
        }
        info.mRunning = false;
    }

    if (mUsePBO) {
        glGenBuffers(1, &mPBO);
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "ScreenCapture: Generating %dx%dx%d PBO: %u\n",
            mResolution.x, mResolution.y, mChannels, mPBO
        );

        glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO);
        glBufferData(GL_PIXEL_PACK_BUFFER, mDataSize, 0, GL_STATIC_READ);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }
}

void ScreenCapture::setTextureTransferProperties(unsigned int type, bool preferBGR) {
    mDownloadType = type;
    mDownloadTypeSetByUser = mDownloadType;
    mPreferBGR = preferBGR;

    updateDownloadFormat();
}

void ScreenCapture::setCaptureFormat(CaptureFormat cf) {
    mFormat = cf;
}

ScreenCapture::CaptureFormat ScreenCapture::getCaptureFormat() const {
    return mFormat;
}

void ScreenCapture::saveScreenCapture(unsigned int textureId, CaptureSource capSrc) {
    addFrameNumberToFilename(sgct::Engine::instance()->getScreenShotNumber());

    checkImageBuffer(capSrc);

    int threadIndex = getAvailableCaptureThread();
    Image* imPtr = prepareImage(threadIndex);
    if (!imPtr) {
        return;
    }
    
    glPixelStorei(GL_PACK_ALIGNMENT, 1); // byte alignment

    if (mUsePBO) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO);
        
        if (sgct::Engine::instance()->isOGLPipelineFixed()) {
            glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
            glEnable(GL_TEXTURE_2D);
        }
            
        if (capSrc == CaptureSource::Texture) {
            glBindTexture(GL_TEXTURE_2D, textureId);
            glGetTexImage(GL_TEXTURE_2D, 0, mDownloadFormat, mDownloadType, 0);
        }
        else {
            // set the target framebuffer to read
            glReadBuffer(sourceForCaptureSource(capSrc));
            glReadPixels(
                0,
                0,
                static_cast<GLsizei>(imPtr->getWidth()),
                static_cast<GLsizei>(imPtr->getHeight()),
                mDownloadFormat,
                mDownloadType,
                0
            );
        }
            
        if (sgct::Engine::instance()->isOGLPipelineFixed()) {
            glPopAttrib();
        }
        
        unsigned char* ptr = reinterpret_cast<unsigned char*>(
            glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY)
        );
        if (ptr) {
            memcpy(imPtr->getData(), ptr, mDataSize);
                
            if (mCaptureCallbackFn) {
                mCaptureCallbackFn(imPtr, mWindowIndex, mEyeIndex, mDownloadType);
            }
            else if (mBytesPerColor <= 2) {
                // save the image
                mSCTIs[threadIndex].mRunning = true;
                mSCTIs[threadIndex].mFrameCaptureThread = std::make_unique<std::thread>(
                    screenCaptureHandler,
                    &mSCTIs[threadIndex]
                );
            }
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }
        else {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Error: Can't map data (0) from GPU in frame capture!\n"
            );
        }
        
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); //unbind pbo
    }
    else {
        // no PBO
        if (sgct::Engine::instance()->isOGLPipelineFixed()) {
            glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
            glEnable(GL_TEXTURE_2D);
        }
            
        if (capSrc == CaptureSource::Texture) {
            glBindTexture(GL_TEXTURE_2D, textureId);
            glGetTexImage(
                GL_TEXTURE_2D,
                0,
                mDownloadFormat,
                mDownloadType,
                imPtr->getData()
            );
        }
        else {
            // set the target framebuffer to read
            glReadBuffer(sourceForCaptureSource(capSrc));
            glReadPixels(
                0,
                0,
                static_cast<GLsizei>(imPtr->getWidth()),
                static_cast<GLsizei>(imPtr->getHeight()),
                mDownloadFormat,
                mDownloadType,
                imPtr->getData()
            );
        }
            
        if (sgct::Engine::instance()->isOGLPipelineFixed()) {
            glPopAttrib();
        }
        
        if (mCaptureCallbackFn) {
            mCaptureCallbackFn(imPtr, mWindowIndex, mEyeIndex, mDownloadType);
        }
        else if (mBytesPerColor <= 2) {
            // save the image
            mSCTIs[threadIndex].mRunning = true;
            mSCTIs[threadIndex].mFrameCaptureThread = std::make_unique<std::thread>(
                screenCaptureHandler,
                &mSCTIs[threadIndex]
            );
        }
    }
}

void ScreenCapture::setPathAndFileName(std::string path, std::string filename) {
    mPath = std::move(path);
    mBaseName = std::move(filename);
}

void ScreenCapture::setUsePBO(bool state) {
    mUsePBO = state;
    
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "ScreenCapture: PBO rendering %s.\n", state ? "enabled" : "disabled"
    );
}

void ScreenCapture::init(int windowIndex, ScreenCapture::EyeIndex ei) {
    mEyeIndex = ei;
    
    mSCTIs.resize(mNumberOfThreads);
    for (unsigned int i = 0; i < mNumberOfThreads; i++) {
        mSCTIs[i].mFrameBufferImage = nullptr;
        mSCTIs[i].mFrameCaptureThread = nullptr;
        mSCTIs[i].mMutex = &mMutex;
        mSCTIs[i].mRunning = false;
    }
    mWindowIndex = windowIndex;

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "Number of screen capture threads is set to %d\n", mNumberOfThreads
    );
}

void ScreenCapture::addFrameNumberToFilename(unsigned int frameNumber) {
    const std::string suffix = [](CaptureFormat format) {
        switch (format) {
            default:
            case CaptureFormat::PNG:
                return "png";
            case CaptureFormat::TGA:
                return "tga";
            case CaptureFormat::JPEG:
                return "jpg";
        }
    }(mFormat);

    // use default settings if path is empty
    std::string filename;
    std::string eye;
    bool useDefaultSettings = mPath.empty() && mBaseName.empty();
    if (useDefaultSettings) {
        std::string tmpPath;
        using Settings = sgct::SGCTSettings;
        using CapturePath = sgct::SGCTSettings::CapturePath;
        switch (mEyeIndex) {
            case EyeIndex::Mono:
            default:
                tmpPath = Settings::instance()->getCapturePath(CapturePath::Mono);
                break;
            case EyeIndex::StereoLeft:
                eye = "_L";
                tmpPath = Settings::instance()->getCapturePath(CapturePath::LeftStereo);
                break;
            case EyeIndex::StereoRight:
                eye = "_R";
                tmpPath = Settings::instance()->getCapturePath(CapturePath::RightStereo);
                break;
        }
        filename = tmpPath;
        sgct::SGCTWindow& win = sgct::Engine::instance()->getWindow(mWindowIndex);
        
        if (win.getName().empty()) {
            filename += "_win" + std::to_string(mWindowIndex);
        }
        else {
            filename += '_' + win.getName();
        }
    }
    else {
        filename = mPath.empty() ? mBaseName : mPath + '/' + mBaseName;
    }

    filename += eye;

    // add frame numbers
    if (frameNumber < 10) {
        filename += "_00000" + std::to_string(frameNumber);
    }
    else if (frameNumber < 100) {
        filename += "_0000" + std::to_string(frameNumber);
    }
    else if (frameNumber < 1000) {
        filename += "_000" + std::to_string(frameNumber);
    }
    else if (frameNumber < 10000) {
        filename += "_00" + std::to_string(frameNumber);
    }
    else if (frameNumber < 100000) {
        filename += "_0" + std::to_string(frameNumber);
    }
    else if (frameNumber < 1000000) {
        filename += '_' + std::to_string(frameNumber);
    }

    filename += '.' + suffix;
    mFilename = std::move(filename);
}

int ScreenCapture::getAvailableCaptureThread() {
    while (true) {
        for (unsigned int i = 0; i < mSCTIs.size(); i++) {
            // check if thread is dead
            if (mSCTIs[i].mFrameCaptureThread == nullptr) {
                return i;
            }

            bool running = mSCTIs[i].mRunning;
            if (!running) {
                mSCTIs[i].mFrameCaptureThread->join();
                mSCTIs[i].mFrameCaptureThread = nullptr;

                return i;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return -1;
}

void ScreenCapture::updateDownloadFormat() {
    const bool fixedPipeline = sgct::Engine::instance()->isOGLPipelineFixed();
    switch (mChannels) {
        default:
            mDownloadFormat = mPreferBGR ? GL_BGRA : GL_RGBA;
            break;
        case 1:
            mDownloadFormat = (fixedPipeline ? GL_LUMINANCE : GL_RED);
            break;
        case 2:
            mDownloadFormat = (fixedPipeline ? GL_LUMINANCE_ALPHA : GL_RG);
            break;
        case 3:
            mDownloadFormat = mPreferBGR ? GL_BGR : GL_RGB;
            break;
    }
}

void ScreenCapture::checkImageBuffer(CaptureSource CapSrc) {
    sgct::SGCTWindow& win = sgct::Engine::instance()->getWindow(mWindowIndex);
    
    if (CapSrc == CaptureSource::Texture) {
        if (mResolution != win.getFramebufferResolution()) {
            mDownloadType = mDownloadTypeSetByUser;
            const int bytesPerColor = win.getFramebufferBPCC();
            initOrResize(win.getFramebufferResolution(), mChannels, bytesPerColor);
        }
    }
    else {
        // capture directly from back buffer (no HDR support)
        if (mResolution != win.getResolution()) {
            mDownloadType = GL_UNSIGNED_BYTE;
            initOrResize(win.getResolution(), mChannels, 1);
        }
    }
}

Image* ScreenCapture::prepareImage(int index) {
    if (index == -1) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "Error in finding availible thread for screenshot/capture\n"
        );
        return nullptr;
    }

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "Starting thread for screenshot/capture [%d]\n", index
    );

    if (mSCTIs[index].mFrameBufferImage == nullptr) {
        mSCTIs[index].mFrameBufferImage = std::make_unique<sgct_core::Image>();
        mSCTIs[index].mFrameBufferImage->setBytesPerChannel(mBytesPerColor);
        mSCTIs[index].mFrameBufferImage->setPreferBGRExport(mPreferBGR);
        mSCTIs[index].mFrameBufferImage->setChannels(mChannels);
        mSCTIs[index].mFrameBufferImage->setSize(mResolution.x, mResolution.y);
        const bool success = mSCTIs[index].mFrameBufferImage->allocateOrResizeData();
        if (!success) {
            mSCTIs[index].mFrameBufferImage = nullptr;
            return nullptr;
        }
    }
    mSCTIs[index].mFrameBufferImage->setFilename(mFilename);

    return mSCTIs[index].mFrameBufferImage.get();
}

void ScreenCapture::setCaptureCallback(
                std::function<void(Image*, size_t, EyeIndex, unsigned int type)> callback)
{
    mCaptureCallbackFn = std::move(callback);
}

} // namespace sgct_core
