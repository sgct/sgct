/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/screencapture.h>

#include <sgct/engine.h>
#include <sgct/image.h>
#include <sgct/log.h>
#include <sgct/profiling.h>
#include <sgct/settings.h>
#include <sgct/window.h>
#include <cstring>
#include <string>

// @TODO (abock, 2019-12-01) This class might want a complete overhaul; right now, there
// is one instance of this class for each window, and it might be better to replace it
// with a global version that keeps threads usable by all windows of the application

namespace {
    void screenCaptureHandler(void* arg) {
        using SCTI = sgct::ScreenCapture::ScreenCaptureThreadInfo;
        SCTI* ptr = reinterpret_cast<SCTI*>(arg);

        try {
            ptr->frameBufferImage->save(ptr->filename);
        }
        catch (const std::runtime_error& e) {
            sgct::Log::Error("%s", e.what());
        }
        ptr->isRunning = false;
    }

    GLenum sourceForCaptureSource(sgct::ScreenCapture::CaptureSource source) {
        using Source = sgct::ScreenCapture::CaptureSource;
        switch (source) {
            case Source::BackBuffer: return GL_BACK;
            case Source::LeftBackBuffer: return GL_BACK_LEFT;
            case Source::RightBackBuffer: return GL_BACK_RIGHT;
            default: throw std::logic_error("Unhandled case label");
        }
    }

    GLenum getDownloadFormat(int nChannels) {
        switch (nChannels) {
            case 1: return GL_RED;
            case 2: return GL_RG;
            case 3: return GL_BGR;
            case 4: return GL_BGRA;
            default: throw std::logic_error("Unhandled case label");
        }
    }
} // namespace

namespace sgct {

ScreenCapture::ScreenCapture()
    : _nThreads(Settings::instance().numberCaptureThreads())
{
    ZoneScoped
}

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

    _downloadFormat = getDownloadFormat(_nChannels);

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
    Log::Debug(
        "Generating %dx%dx%d PBO: %u", _resolution.x, _resolution.y, _nChannels, _pbo
    );

    glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbo);
    glBufferData(GL_PIXEL_PACK_BUFFER, _dataSize, nullptr, GL_STATIC_READ);
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

void ScreenCapture::saveScreenCapture(unsigned int textureId, CaptureSource capSrc) {
    ZoneScoped
        
    std::string file = addFrameNumberToFilename(Engine::instance().screenShotNumber());
    checkImageBuffer(capSrc);

    int threadIndex = availableCaptureThread();
    if (threadIndex == -1) {
        Log::Error("Error finding available capture thread");
        return;
    }

    Image* imPtr = prepareImage(threadIndex, std::move(file));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbo);
        
    if (capSrc == CaptureSource::Texture) {
        glBindTexture(GL_TEXTURE_2D, textureId);
        glGetTexImage(GL_TEXTURE_2D, 0, _downloadFormat, _downloadType, nullptr);
    }
    else {
        // set the target framebuffer to read
        glReadBuffer(sourceForCaptureSource(capSrc));
        const glm::ivec2& s = imPtr->size();
        const GLsizei w = static_cast<GLsizei>(s.x);
        const GLsizei h = static_cast<GLsizei>(s.y);
        glReadPixels(0, 0, w, h, _downloadFormat, _downloadType, nullptr);
    }

    unsigned char* ptr = reinterpret_cast<unsigned char*>(
        glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY)
    );
    if (ptr) {
        std::memcpy(imPtr->data(), ptr, _dataSize);

        // save the image
        _captureInfos[threadIndex].isRunning = true;
        _captureInfos[threadIndex].captureThread = std::make_unique<std::thread>(
            screenCaptureHandler,
            &_captureInfos[threadIndex]
        );
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    }
    else {
        Log::Error("Can't map data (0) from GPU in frame capture");
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void ScreenCapture::setPathAndFileName(std::string path, std::string filename) {
    _path = std::move(path);
    _baseName = std::move(filename);
}

void ScreenCapture::initialize(int windowIndex, ScreenCapture::EyeIndex ei) {
    _eyeIndex = ei;
    
    _captureInfos.resize(_nThreads);
    for (unsigned int i = 0; i < _nThreads; i++) {
        _captureInfos[i].frameBufferImage = nullptr;
        _captureInfos[i].captureThread = nullptr;
        _captureInfos[i].mutex = &_mutex;
        _captureInfos[i].isRunning = false;
    }
    _windowIndex = windowIndex;

    Log::Debug("Number of screencapture threads is set to %d", _nThreads);
}

std::string ScreenCapture::addFrameNumberToFilename(unsigned int frameNumber) {
    const std::string suffix = [](CaptureFormat format) {
        switch (format) {
            case CaptureFormat::PNG: return "png";
            case CaptureFormat::TGA: return "tga";
            case CaptureFormat::JPEG: return "jpg";
            default: throw std::logic_error("Unhandled case label");
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
                filename = Settings::instance().capturePath(CapturePath::Mono);
                break;
            case EyeIndex::StereoLeft:
                eye = "_L";
                filename = Settings::instance().capturePath(CapturePath::LeftStereo);
                break;
            case EyeIndex::StereoRight:
                eye = "_R";
                filename = Settings::instance().capturePath(CapturePath::RightStereo);
                break;
            default:
                throw std::logic_error("Unhandled case label");
        }
        const Window& win = *Engine::instance().windows()[_windowIndex];
        
        if (win.name().empty()) {
            filename += "_win" + std::to_string(_windowIndex);
        }
        else {
            filename += '_' + win.name();
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

    return filename + '.' + suffix;
}

int ScreenCapture::availableCaptureThread() {
    while (true) {
        for (unsigned int i = 0; i < _captureInfos.size(); i++) {
            // check if thread is dead
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

void ScreenCapture::checkImageBuffer(CaptureSource captureSource) {
    const Window& win = *Engine::instance().windows()[_windowIndex];

    if (captureSource == CaptureSource::Texture) {
        if (_resolution != win.framebufferResolution()) {
            _downloadType = _downloadTypeSetByUser;
            const int bytesPerColor = win.framebufferBPCC();
            initOrResize(win.framebufferResolution(), _nChannels, bytesPerColor);
        }
    }
    else {
        // capture directly from back buffer (no HDR support)
        if (_resolution != win.resolution()) {
            _downloadType = GL_UNSIGNED_BYTE;
            initOrResize(win.resolution(), _nChannels, 1);
        }
    }
}

Image* ScreenCapture::prepareImage(int index, std::string file) {
    Log::Debug("Starting thread for screenshot/capture [%d]", index);

    if (_captureInfos[index].frameBufferImage == nullptr) {
        _captureInfos[index].frameBufferImage = std::make_unique<Image>();
        _captureInfos[index].frameBufferImage->setBytesPerChannel(_bytesPerColor);
        _captureInfos[index].frameBufferImage->setChannels(_nChannels);
        _captureInfos[index].frameBufferImage->setSize(_resolution);
        if (_bytesPerColor * _nChannels * _resolution.x * _resolution.y == 0) {
            _captureInfos[index].frameBufferImage = nullptr;
            return nullptr;
        }
        _captureInfos[index].frameBufferImage->allocateOrResizeData();
    }
    _captureInfos[index].filename = std::move(file);

    return _captureInfos[index].frameBufferImage.get();
}

} // namespace sgct
