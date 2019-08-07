/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/ScreenCapture.h>
#include <sgct/MessageHandler.h>
#include <sgct/Engine.h>
#include <sstream>
#include <string>

namespace {
    void screenCaptureHandler(void* arg) {
        using SCTI = sgct_core::ScreenCaptureThreadInfo;
        SCTI* ptr = reinterpret_cast<SCTI*>(arg);

        if (!ptr->mframeBufferImagePtr->save()) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::NOTIFY_ERROR,
                "Error: Failed to save '%s'!\n",
                ptr->mframeBufferImagePtr->getFilename()
            );
        }

#ifdef __SGCT_MUTEX_DEBUG__
        fprintf(stderr, "Locking mutex for screencapture...\n");
#endif
        ptr->mMutexPtr->lock();
        ptr->mRunning = false;
        ptr->mMutexPtr->unlock();
#ifdef __SGCT_MUTEX_DEBUG__
        fprintf(stderr, "Mutex for screencapture is unlocked.\n");
#endif
    }
} // namespace

namespace sgct_core {

ScreenCapture::~ScreenCapture() {
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::NOTIFY_INFO,
        "Clearing screen capture buffers...\n"
    );

    mCaptureCallbackFn1 = nullptr;
    mCaptureCallbackFn2 = nullptr;

    if (mSCTIPtrs != nullptr) {
        for (unsigned int i = 0; i < mNumberOfThreads; i++) {
            //kill threads that are still running
            if (mSCTIPtrs[i].mFrameCaptureThreadPtr != nullptr) {
                mSCTIPtrs[i].mFrameCaptureThreadPtr->join();
                delete mSCTIPtrs[i].mFrameCaptureThreadPtr;
                mSCTIPtrs[i].mFrameCaptureThreadPtr = nullptr;
            }

            #ifdef __SGCT_MUTEX_DEBUG__
                fprintf(stderr, "Locking mutex for screencapture...\n");
            #endif
            mMutex.lock();
            if (mSCTIPtrs[i].mframeBufferImagePtr != nullptr) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::NOTIFY_INFO,
                    "\tBuffer %d...\n", i
                );
                delete mSCTIPtrs[i].mframeBufferImagePtr;
                mSCTIPtrs[i].mframeBufferImagePtr = nullptr;
            }

            mSCTIPtrs[i].mRunning = false;
            mMutex.unlock();
            #ifdef __SGCT_MUTEX_DEBUG__
                fprintf(stderr, "Mutex for screencapture is unlocked.\n");
            #endif
        }

        delete[] mSCTIPtrs;
        mSCTIPtrs = nullptr;
    }

    if (mPBO) {
        //delete if buffer exitsts
        glDeleteBuffers(1, &mPBO);
        mPBO = GL_FALSE;
    }
}

/*!
    Inits the pixel buffer object (PBO) or re-sizes it if the frame buffer size have changed.

    \param x the horizontal pixel resolution of the frame buffer
    \param y the vertical pixel resolution of the frame buffer
    \param channels the number of color channels

    If PBOs are not supported nothing will and the screenshot process will fall back on slower GPU data fetching.
*/
void ScreenCapture::initOrResize(int x, int y, int channels, int bytesPerColor) {
    if (mPBO) {
        //delete if buffer exists
        glDeleteBuffers(1, &mPBO);
        mPBO = 0;
    }

    mX = x;
    mY = y;
    mBytesPerColor = bytesPerColor;
    
    mChannels = channels;
    mDataSize = mX * mY * mChannels * mBytesPerColor;

    updateDownloadFormat();

    #ifdef __SGCT_MUTEX_DEBUG__
        fprintf(stderr, "Locking mutex for screencapture...\n");
    #endif
    mMutex.lock();
    for (unsigned int i = 0; i < mNumberOfThreads; i++) {
        if (mSCTIPtrs[i].mframeBufferImagePtr != nullptr) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::NOTIFY_INFO,
                "Clearing screen capture buffer %d...\n", i
            );

            // kill threads that are still running
            if (mSCTIPtrs[i].mFrameCaptureThreadPtr != nullptr) {
                mSCTIPtrs[i].mFrameCaptureThreadPtr->join();
                delete mSCTIPtrs[i].mFrameCaptureThreadPtr;
                mSCTIPtrs[i].mFrameCaptureThreadPtr = nullptr;
            }

            delete mSCTIPtrs[i].mframeBufferImagePtr;
            mSCTIPtrs[i].mframeBufferImagePtr = nullptr;
        }

        mSCTIPtrs[i].mRunning = false;
    }

    if (mUsePBO) {
        glGenBuffers(1, &mPBO);
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::NOTIFY_DEBUG,
            "ScreenCapture: Generating %dx%dx%d PBO: %u\n", mX, mY, mChannels, mPBO
        );

        glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO);
        glBufferData(GL_PIXEL_PACK_BUFFER, mDataSize, 0, GL_STATIC_READ);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }

    mMutex.unlock();
    #ifdef __SGCT_MUTEX_DEBUG__
        fprintf(stderr, "Mutex for screencapture is unlocked.\n");
    #endif
}

/*!
Set the opengl texture properties for glGetTexImage.
Type can be: GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_HALF_FLOAT, GL_FLOAT, GL_SHORT, GL_INT, GL_UNSIGNED_SHORT or GL_UNSIGNED_INT
*/
void ScreenCapture::setTextureTransferProperties(unsigned int type, bool preferBGR) {
    mDownloadType = type;
    mDownloadTypeSetByUser = mDownloadType;
    mPreferBGR = preferBGR;

    updateDownloadFormat();
}

/*!
Set the image format to use
*/
void ScreenCapture::setCaptureFormat(CaptureFormat cf) {
    mFormat = cf;
}

/*!
Get the image format
*/
ScreenCapture::CaptureFormat ScreenCapture::getCaptureFormat() {
    return mFormat;
}

/*!
This function saves the images to disc.

@param textureId textureId is the texture that will be streamed from the GPU if frame buffer objects are used in the rendering.
*/
void ScreenCapture::saveScreenCapture(unsigned int textureId, CaptureSrc CapSrc) {
    addFrameNumberToFilename(sgct::Engine::instance()->getScreenShotNumber());

    checkImageBuffer(CapSrc);

    int threadIndex = getAvailableCaptureThread();
    Image* imPtr = prepareImage(threadIndex);
    if (!imPtr) {
        return;
    }
    
    glPixelStorei(GL_PACK_ALIGNMENT, 1); //byte alignment

    if (mUsePBO) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, mPBO);
        
        if (sgct::Engine::instance()->isOGLPipelineFixed()) {
            glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
            glEnable(GL_TEXTURE_2D);
        }
            
        if (CapSrc == CAPTURE_TEXTURE) {
            glBindTexture(GL_TEXTURE_2D, textureId);
            glGetTexImage(GL_TEXTURE_2D, 0, mDownloadFormat, mDownloadType, 0);
        }
        else {
            // set the target framebuffer to read
            glReadBuffer(CapSrc);
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
            if (mCaptureCallbackFn2 != nullptr) {
                mCaptureCallbackFn2(ptr, mWindowIndex, mEyeIndex, mDownloadType);
            }
            else {
                memcpy(imPtr->getData(), ptr, mDataSize);
                
                if (mCaptureCallbackFn1 != nullptr) {
                    mCaptureCallbackFn1(imPtr, mWindowIndex, mEyeIndex, mDownloadType);
                }
                else if (mBytesPerColor <= 2) {
                    //save the image
                    mSCTIPtrs[threadIndex].mRunning = true;
                    mSCTIPtrs[threadIndex].mFrameCaptureThreadPtr = new std::thread(
                        screenCaptureHandler,
                        &mSCTIPtrs[threadIndex]
                    );
                }
            }
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }
        else {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::NOTIFY_ERROR,
                "Error: Can't map data (0) from GPU in frame capture!\n"
            );
        }
        
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0); //unbind pbo
    }
    else {
        //no PBO
        if (sgct::Engine::instance()->isOGLPipelineFixed()) {
            glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
            glEnable(GL_TEXTURE_2D);
        }
            
        if (CapSrc == CAPTURE_TEXTURE) {
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
            glReadBuffer(CapSrc);
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
        
        if (mCaptureCallbackFn1 != nullptr) {
            mCaptureCallbackFn1(imPtr, mWindowIndex, mEyeIndex, mDownloadType);
        }
        else if (mBytesPerColor <= 2) {
            //save the image
            mSCTIPtrs[threadIndex].mRunning = true;
            mSCTIPtrs[threadIndex].mFrameCaptureThreadPtr = new std::thread(
                screenCaptureHandler,
                &mSCTIPtrs[threadIndex]
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
        sgct::MessageHandler::NOTIFY_INFO,
        "ScreenCapture: PBO rendering %s.\n", state ? "enabled" : "disabled"
    );
}

/*!
    Init
*/
void ScreenCapture::init(size_t windowIndex, ScreenCapture::EyeIndex ei) {
    mEyeIndex = ei;
    
    mSCTIPtrs = new ScreenCaptureThreadInfo[mNumberOfThreads];
    for (unsigned int i = 0; i < mNumberOfThreads; i++) {
        mSCTIPtrs[i].mMutexPtr = &mMutex;
    }
    mWindowIndex = windowIndex;

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::NOTIFY_DEBUG,
        "Number of screen capture threads is set to %d\n", mNumberOfThreads
    );
}

void ScreenCapture::addFrameNumberToFilename(unsigned int frameNumber) {
    std::string eye;
    
    //use default settings if path is empty
    bool useDefaultSettings = mPath.empty() && mBaseName.empty();

    std::string tmpPath;
    using Settings = sgct::SGCTSettings;
    switch (mEyeIndex) {
        case MONO:
        default:
            if (useDefaultSettings) {
                tmpPath = Settings::instance()->getCapturePath(Settings::Mono);
            }
            break;

        case STEREO_LEFT:
            eye = "_L";
            if (useDefaultSettings) {
                tmpPath = Settings::instance()->getCapturePath(Settings::LeftStereo);
            }
            break;

        case STEREO_RIGHT:
            eye = "_R";
            if (useDefaultSettings)
                tmpPath = Settings::instance()->getCapturePath(Settings::RightStereo);
            break;
    }

    std::string suffix;
    if (mFormat == PNG) {
        suffix = "png";
    }
    else if (mFormat == TGA) {
        suffix = "tga";
    }
    else {
        suffix = "jpg";
    }

    std::stringstream ss;
    if (useDefaultSettings) {
        ss << tmpPath;
        sgct::SGCTWindow* win = sgct::Engine::instance()->getWindowPtr(mWindowIndex);
        
        if (win->getName().empty()) {
            ss << "_win" << mWindowIndex;
        }
        else {
            ss << "_" << win->getName();
        }
    }
    else {
        if (mPath.empty()) {
            ss << mBaseName;
        }
        else {
            ss << mPath << "/" << mBaseName;
        }
    }

    ss << eye;

    //add frame numbers
    if (frameNumber < 10) {
        ss << "_00000" << frameNumber;
    }
    else if (frameNumber < 100) {
        ss << "_0000" << frameNumber;
    }
    else if (frameNumber < 1000) {
        ss << "_000" << frameNumber;
    }
    else if (frameNumber < 10000) {
        ss << "_00" << frameNumber;
    }
    else if (frameNumber < 100000) {
        ss << "_0" << frameNumber;
    }
    else if (frameNumber < 1000000) {
        ss << "_" << frameNumber;
    }

    //add suffix
    ss << "." << suffix;
    mFilename = ss.str();
}

int ScreenCapture::getAvailableCaptureThread() {
    while (true) {
        for (unsigned int i = 0; i < mNumberOfThreads; i++) {
            //check if thread is dead
            if (mSCTIPtrs[i].mFrameCaptureThreadPtr == nullptr) {
                return i;
            }
            #ifdef __SGCT_MUTEX_DEBUG__
                fprintf(stderr, "Locking mutex for screencapture...\n");
            #endif
            mMutex.lock();
            bool running = mSCTIPtrs[i].mRunning;
            mMutex.unlock();
            #ifdef __SGCT_MUTEX_DEBUG__
                fprintf(stderr, "Mutex for screencapture is unlocked.\n");
            #endif

            if (!running) {
                mSCTIPtrs[i].mFrameCaptureThreadPtr->join();
                delete mSCTIPtrs[i].mFrameCaptureThreadPtr;
                mSCTIPtrs[i].mFrameCaptureThreadPtr = nullptr;

                return i;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return -1;
}

void ScreenCapture::updateDownloadFormat() {
    bool fixedPipeline = sgct::Engine::instance()->isOGLPipelineFixed();
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

void ScreenCapture::checkImageBuffer(const CaptureSrc& CapSrc) {
    sgct::SGCTWindow* win = sgct::Engine::instance()->getWindowPtr(mWindowIndex);
    
    if (CapSrc == CAPTURE_TEXTURE) {
        if (mX != win->getXFramebufferResolution() ||
            mY != win->getYFramebufferResolution())
        {
            mDownloadType = mDownloadTypeSetByUser;
            int bytesPerColor = win->getFramebufferBPCC();
            initOrResize(
                win->getXFramebufferResolution(),
                win->getYFramebufferResolution(),
                mChannels,
                bytesPerColor
            );
        }
    }
    else { //capture directly from back buffer (no HDR support)
        if (mX != win->getXResolution() || mY != win->getYResolution()) {
            mDownloadType = GL_UNSIGNED_BYTE;
            initOrResize(win->getXResolution(), win->getYResolution(), mChannels, 1);
        }
    }
}

Image* ScreenCapture::prepareImage(int index) {
    if (index == -1) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::NOTIFY_ERROR,
            "Error in finding availible thread for screenshot/capture!\n"
        );
        return nullptr;
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::NOTIFY_DEBUG,
            "Starting thread for screenshot/capture [%d]\n", index
        );
    }

    Image** imPtr = &mSCTIPtrs[index].mframeBufferImagePtr;
    if ((*imPtr) == nullptr) {
        (*imPtr) = new sgct_core::Image();
        (*imPtr)->setBytesPerChannel(mBytesPerColor);
        (*imPtr)->setPreferBGRExport(mPreferBGR);
        (*imPtr)->setChannels(mChannels);
        (*imPtr)->setSize(mX, mY);
        if (!(*imPtr)->allocateOrResizeData()) {
            delete (*imPtr);
            return nullptr;
        }
    }
    (*imPtr)->setFilename(mFilename);

    return (*imPtr);
}

//multi-threaded screenshot saver


/*!
Set the screen capture callback\n
Parameters are: image pointer to captured image, window index, eye index and OpenGL type (GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_HALF_FLOAT, GL_FLOAT, GL_SHORT, GL_INT, GL_UNSIGNED_SHORT or GL_UNSIGNED_INT)
*/
void ScreenCapture::setCaptureCallback(
                std::function<void(Image*, size_t, EyeIndex, unsigned int type)> callback)
{
    mCaptureCallbackFn1 = callback;
    mCaptureCallbackFn2 = nullptr; //only allow one callback
}

/*!
Set the screen capture callback\n
Parameters are: raw buffer, window index, eye index and OpenGL type (GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_HALF_FLOAT, GL_FLOAT, GL_SHORT, GL_INT, GL_UNSIGNED_SHORT or GL_UNSIGNED_INT)
*/
void ScreenCapture::setCaptureCallback(
        std::function<void(unsigned char*, size_t, EyeIndex, unsigned int type)> callback)
{
    mCaptureCallbackFn2 = callback;
    mCaptureCallbackFn1 = nullptr; //only allow one callback
}

} // namespace sgct_core
