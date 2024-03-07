/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/screencapture.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/fmt.h>
#include <sgct/image.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
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
            sgct::Log::Error(e.what());
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
    ZoneScoped;
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

void ScreenCapture::initOrResize(ivec2 resolution, int channels, int bytesPerColor) {
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
    Log::Debug(fmt::format(
        "Generating {}x{}x{} PBO: {}", _resolution.x, _resolution.y, _nChannels, _pbo
    ));

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
    ZoneScoped;

    uint64_t number = Engine::instance().screenShotNumber();
    if (Settings::instance().hasScreenshotLimit()) {
        uint64_t begin = Settings::instance().screenshotLimitBegin();
        uint64_t end = Settings::instance().screenshotLimitEnd();

        if (number < begin || number >= end) {
            Log::Debug(fmt::format(
                "Skipping screenshot {} outside range [{}, {}]", number, begin, end
            ));
            return;
        }
    }

    std::string file = createFilename(number);
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
        const ivec2& s = imPtr->size();
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

    Log::Debug(fmt::format("Number of screencapture threads is set to {}", _nThreads));
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

    std::array<char, 6> Buffer;
    std::fill(Buffer.begin(), Buffer.end(), '\0');
    fmt::format_to_n(Buffer.data(), Buffer.size(), "{:06}", frameNumber);

    const std::string suffix = [](CaptureFormat format) {
        switch (format) {
            case CaptureFormat::PNG: return "png";
            case CaptureFormat::TGA: return "tga";
            case CaptureFormat::JPEG: return "jpg";
            default: throw std::logic_error("Unhandled case label");
        }
    }(_format);

    std::string file;
    if (!Settings::instance().capturePath().empty()) {
        file = Settings::instance().capturePath().string() + '/';
    }
    if (!Settings::instance().prefixScreenshot().empty()) {
        file += Settings::instance().prefixScreenshot();
        file += '_';
    }
    if (Settings::instance().addNodeNameToScreenshot() &&
        ClusterManager::instance().numberOfNodes() > 1)
    {
        file += "node" + std::to_string(ClusterManager::instance().thisNodeId());
        file += '_';
    }
    if (Settings::instance().addWindowNameToScreenshot()) {
        const Window& w = *Engine::instance().windows()[_windowIndex];
        file += w.name().empty() ? "win" + std::to_string(_windowIndex) : w.name();
        file += '_';
    }

    if (!eyeSuffix.empty()) {
        file += eyeSuffix + '_';
    }

    return file + std::string(Buffer.begin(), Buffer.end()) + '.' + suffix;
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
        if (_resolution.x != win.framebufferResolution().x &&
            _resolution.y != win.framebufferResolution().y)
        {
            _downloadType = _downloadTypeSetByUser;
            const int bytesPerColor = win.framebufferBPCC();
            initOrResize(win.framebufferResolution(), _nChannels, bytesPerColor);
        }
    }
    else {
        // capture directly from back buffer (no HDR support)
        if (_resolution.x != win.resolution().x && _resolution.y != win.resolution().y) {
            _downloadType = GL_UNSIGNED_BYTE;
            initOrResize(win.resolution(), _nChannels, 1);
        }
    }
}

Image* ScreenCapture::prepareImage(int index, std::string file) {
    Log::Debug(fmt::format("Starting thread for screenshot/capture [{}]", index));

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
