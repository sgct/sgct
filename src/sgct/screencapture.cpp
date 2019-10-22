/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/screencapture.h>

#include <sgct/engine.h>
#include <sgct/image.h>
#include <sgct/messagehandler.h>
#include <sgct/settings.h>
#include <sgct/window.h>
#include <string>

namespace {
    void screenCaptureHandler(void* arg) {
        using SCTI = sgct::core::ScreenCapture::ScreenCaptureThreadInfo;
        SCTI* ptr = reinterpret_cast<SCTI*>(arg);

        const bool saveSuccess = ptr->frameBufferImage->save(ptr->filename);
        if (!saveSuccess) {
            sgct::MessageHandler::printError(
                "Error: Failed to save '%s'", ptr->filename.c_str()
            );
        }
        ptr->isRunning = false;
    }

    GLenum sourceForCaptureSource(sgct::core::ScreenCapture::CaptureSource source) {
        using Source = sgct::core::ScreenCapture::CaptureSource;
        switch (source) {
            default:
            case Source::BackBuffer: return GL_BACK;
            case Source::LeftBackBuffer: return GL_BACK_LEFT;
            case Source::RightBackBuffer: return GL_BACK_RIGHT;
        }
    }

    [[nodiscard]] GLenum getDownloadFormat(int nChannels) {
        switch (nChannels) {
            default:
                return GL_BGRA;
            case 1:
                return GL_RED;
            case 2:
                return GL_RG;
            case 3:
                return GL_BGR;
        }
    }

} // namespace

namespace sgct::core {

ScreenCapture::ScreenCapture()
    : _nThreads(Settings::instance()->getNumberOfCaptureThreads())
{}

ScreenCapture::~ScreenCapture() {
    for (ScreenCaptureThreadInfo& info : _captureInfos) {
        // kill threads that are still running
        if (info.captureThread) {
            info.captureThread->join();
            info.captureThread = nullptr;
        }

        std::unique_lock lock(_mutex);
        info.frameBufferImage = nullptr;
        info.isRunning = false;
    }

    glDeleteBuffers(1, &_pbo);
}

void ScreenCapture::initOrResize(glm::ivec2 resolution, int channels, int bytesPerColor) {
    glDeleteBuffers(1, &_pbo);

    _resolution = std::move(resolution);
    _bytesPerColor = bytesPerColor;
    
    _nChannels = channels;
    _dataSize = _resolution.x * _resolution.y * _nChannels * _bytesPerColor;

    _downloadType = getDownloadFormat(_nChannels);

    std::unique_lock lock(_mutex);
    for (ScreenCaptureThreadInfo& info : _captureInfos) {
        if (info.frameBufferImage) {
            // kill threads that are still running
            if (info.captureThread) {
                info.captureThread->join();
                info.captureThread = nullptr;
            }
            info.frameBufferImage = nullptr;
        }
        info.isRunning = false;
    }

    glGenBuffers(1, &_pbo);
    MessageHandler::printDebug(
        "Generating %dx%dx%d PBO: %u", _resolution.x, _resolution.y, _nChannels, _pbo
    );

    glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbo);
    glBufferData(GL_PIXEL_PACK_BUFFER, _dataSize, 0, GL_STATIC_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void ScreenCapture::setTextureTransferProperties(GLenum type) {
    _downloadType = type;
    _downloadTypeSetByUser = _downloadType;
    _downloadFormat = getDownloadFormat(_nChannels);
}

void ScreenCapture::setCaptureFormat(CaptureFormat cf) {
    _format = cf;
}

ScreenCapture::CaptureFormat ScreenCapture::getCaptureFormat() const {
    return _format;
}

void ScreenCapture::saveScreenCapture(unsigned int textureId, CaptureSource capSrc) {
    addFrameNumberToFilename(Engine::instance()->getScreenShotNumber());
    checkImageBuffer(capSrc);

    int threadIndex = getAvailableCaptureThread();
    Image* imPtr = prepareImage(threadIndex);
    if (!imPtr) {
        return;
    }
    
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbo);
        
    if (capSrc == CaptureSource::Texture) {
        glBindTexture(GL_TEXTURE_2D, textureId);
        glGetTexImage(GL_TEXTURE_2D, 0, _downloadFormat, _downloadType, 0);
    }
    else {
        // set the target framebuffer to read
        glReadBuffer(sourceForCaptureSource(capSrc));
        const glm::ivec2& s = imPtr->getSize();
        const GLsizei w = static_cast<GLsizei>(s.x);
        const GLsizei h = static_cast<GLsizei>(s.y);
        glReadPixels(0, 0, w, h, _downloadFormat, _downloadType, 0);
    }

    unsigned char* ptr = reinterpret_cast<unsigned char*>(
        glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY)
    );
    if (ptr) {
        memcpy(imPtr->getData(), ptr, _dataSize);

        if (_captureCallback) {
            _captureCallback(imPtr, _windowIndex, _eyeIndex, _downloadType);
        }
        else if (_bytesPerColor <= 2) {
            // save the image
            _captureInfos[threadIndex].isRunning = true;
            _captureInfos[threadIndex].captureThread = std::make_unique<std::thread>(
                screenCaptureHandler,
                &_captureInfos[threadIndex]
            );
        }
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    }
    else {
        MessageHandler::printError("Can't map data (0) from GPU in frame capture");
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void ScreenCapture::setPathAndFileName(std::string path, std::string filename) {
    _path = std::move(path);
    _baseName = std::move(filename);
}

void ScreenCapture::init(int windowIndex, ScreenCapture::EyeIndex ei) {
    _eyeIndex = ei;
    
    _captureInfos.resize(_nThreads);
    for (unsigned int i = 0; i < _nThreads; i++) {
        _captureInfos[i].frameBufferImage = nullptr;
        _captureInfos[i].captureThread = nullptr;
        _captureInfos[i].mutex = &_mutex;
        _captureInfos[i].isRunning = false;
    }
    _windowIndex = windowIndex;

    MessageHandler::printDebug("Number of screencapture threads is set to %d", _nThreads);
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
    }(_format);

    // use default settings if path is empty
    std::string filename;
    std::string eye;
    const bool useDefaultSettings = _path.empty() && _baseName.empty();
    if (useDefaultSettings) {
        using CapturePath = Settings::CapturePath;
        switch (_eyeIndex) {
            case EyeIndex::Mono:
            default:
                filename = Settings::instance()->getCapturePath(CapturePath::Mono);
                break;
            case EyeIndex::StereoLeft:
                eye = "_L";
                filename = Settings::instance()->getCapturePath(CapturePath::LeftStereo);
                break;
            case EyeIndex::StereoRight:
                eye = "_R";
                filename = Settings::instance()->getCapturePath(CapturePath::RightStereo);
                break;
        }
        Window& win = Engine::instance()->getWindow(_windowIndex);
        
        if (win.getName().empty()) {
            filename += "_win" + std::to_string(_windowIndex);
        }
        else {
            filename += '_' + win.getName();
        }
    }
    else {
        filename = _path.empty() ? _baseName : _path + '/' + _baseName;
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
    _filename = std::move(filename);
}

int ScreenCapture::getAvailableCaptureThread() {
    while (true) {
        for (unsigned int i = 0; i < _captureInfos.size(); i++) {
            // check if thread is dead
            if (_captureInfos[i].captureThread == nullptr) {
                return i;
            }

            bool running = _captureInfos[i].isRunning;
            if (!running) {
                _captureInfos[i].captureThread->join();
                _captureInfos[i].captureThread = nullptr;

                return i;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void ScreenCapture::checkImageBuffer(CaptureSource captureSource) {
    Window& win = Engine::instance()->getWindow(_windowIndex);

    if (captureSource == CaptureSource::Texture) {
        if (_resolution != win.getFramebufferResolution()) {
            _downloadType = _downloadTypeSetByUser;
            const int bytesPerColor = win.getFramebufferBPCC();
            initOrResize(win.getFramebufferResolution(), _nChannels, bytesPerColor);
        }
    }
    else {
        // capture directly from back buffer (no HDR support)
        if (_resolution != win.getResolution()) {
            _downloadType = GL_UNSIGNED_BYTE;
            initOrResize(win.getResolution(), _nChannels, 1);
        }
    }
}

Image* ScreenCapture::prepareImage(int index) {
    if (index == -1) {
        MessageHandler::printError(
            "Error in finding availible thread for screenshot/capture"
        );
        return nullptr;
    }

    MessageHandler::printDebug("Starting thread for screenshot/capture [%d]", index);

    if (_captureInfos[index].frameBufferImage == nullptr) {
        _captureInfos[index].frameBufferImage = std::make_unique<core::Image>();
        _captureInfos[index].frameBufferImage->setBytesPerChannel(_bytesPerColor);
        _captureInfos[index].frameBufferImage->setChannels(_nChannels);
        _captureInfos[index].frameBufferImage->setSize(_resolution);
        const bool success = _captureInfos[index].frameBufferImage->allocateOrResizeData();
        if (!success) {
            _captureInfos[index].frameBufferImage = nullptr;
            return nullptr;
        }
    }
    _captureInfos[index].filename = _filename;

    return _captureInfos[index].frameBufferImage.get();
}

void ScreenCapture::setCaptureCallback(
                      std::function<void(Image*, size_t, EyeIndex, GLenum type)> callback)
{
    _captureCallback = std::move(callback);
}

} // namespace sgct::core
