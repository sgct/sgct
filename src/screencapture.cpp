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
#include <sgct/format.h>
#include <sgct/image.h>
#include <sgct/log.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/window.h>
#include <cstring>
#include <string>

// @TODO (abock, 2019-12-01) This class might want a complete overhaul; right now, there
// is one instance of this class for each window, and it might be better to replace it
// with a global version that keeps threads usable by all windows of the application

namespace sgct {

ScreenCapture::ScreenCapture()
    : _nThreads(Engine::instance().numberCaptureThreads())
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

        const std::unique_lock lock(_mutex);
        info.frameBufferImage = nullptr;
        info.isRunning = false;
    }

    glDeleteBuffers(1, &_pbo);
}

void ScreenCapture::initialize(int windowIndex, ScreenCapture::EyeIndex ei,
                               ivec2 resolution, int bytesPerColor,
                               unsigned int colorDataType)
{
    _eyeIndex = ei;

    _captureInfos.resize(_nThreads);
    for (unsigned int i = 0; i < _nThreads; i++) {
        _captureInfos[i].frameBufferImage = nullptr;
        _captureInfos[i].captureThread = nullptr;
        _captureInfos[i].mutex = &_mutex;
        _captureInfos[i].isRunning = false;
    }
    _windowIndex = windowIndex;

    Log::Debug(std::format("Number of screencapture threads is set to {}", _nThreads));

    _bytesPerColor = bytesPerColor;
    _downloadType = colorDataType;

    resize(resolution);
}

void ScreenCapture::resize(ivec2 resolution) {
    glDeleteBuffers(1, &_pbo);

    _resolution = std::move(resolution);

    constexpr int nChannels = 3;
    _dataSize = _resolution.x * _resolution.y * nChannels * _bytesPerColor;

    const std::unique_lock lock(_mutex);
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
    Log::Debug(std::format(
        "Generating {}x{}x{} PBO: {}", _resolution.x, _resolution.y, nChannels, _pbo
    ));

    glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbo);
    glBufferData(GL_PIXEL_PACK_BUFFER, _dataSize, nullptr, GL_STATIC_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void ScreenCapture::saveScreenCapture(unsigned int textureId, CaptureSource capSrc) {
    ZoneScoped;

    uint64_t number = Engine::instance().screenShotNumber();
    if (Engine::instance().screenshotLimit()) {
        uint64_t begin = Engine::instance().screenshotLimit()->first;
        uint64_t end = Engine::instance().screenshotLimit()->second;

        if (number < begin || number >= end) {
            Log::Debug(std::format(
                "Skipping screenshot {} outside range [{}, {}]", number, begin, end
            ));
            return;
        }
    }

    std::string file = createFilename(number);

    const Window& win = *Engine::instance().windows()[_windowIndex];
    const ivec2 res =
        capSrc == CaptureSource::Texture ?
        win.framebufferResolution() :
        win.resolution();
    if (_resolution.x != res.x && _resolution.y != res.y) {
        resize(res);
    }

    const int threadIndex = availableCaptureThread();
    if (threadIndex == -1) {
        Log::Error("Error finding available capture thread");
        return;
    }

    Image* imPtr = prepareImage(threadIndex, std::move(file));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, _pbo);

    if (capSrc == CaptureSource::Texture) {
        glBindTexture(GL_TEXTURE_2D, textureId);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, _downloadType, nullptr);
    }
    else {
        // set the target framebuffer to read
        switch (capSrc) {
            case CaptureSource::BackBuffer:
                glReadBuffer(GL_BACK);
                break;
            case CaptureSource::LeftBackBuffer:
                glReadBuffer(GL_BACK_LEFT);
                break;
            case CaptureSource::RightBackBuffer:
                glReadBuffer(GL_BACK_RIGHT);
                break;
            default:
                throw std::logic_error("Unhandled case label");
        }
        const ivec2& s = imPtr->size();
        const GLsizei w = static_cast<GLsizei>(s.x);
        const GLsizei h = static_cast<GLsizei>(s.y);
        glReadPixels(0, 0, w, h, GL_RGB, _downloadType, nullptr);
    }

    unsigned char* ptr = reinterpret_cast<unsigned char*>(
        glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY)
    );
    if (ptr) {
        std::memcpy(imPtr->data(), ptr, _dataSize);

        // save the image
        _captureInfos[threadIndex].isRunning = true;
        _captureInfos[threadIndex].captureThread = std::make_unique<std::thread>(
            [](void* arg) {
                ScreenCaptureThreadInfo* ptr =
                    reinterpret_cast<ScreenCaptureThreadInfo*>(arg);

                try {
                    ptr->frameBufferImage->save(ptr->filename);
                }
                catch (const std::runtime_error& e) {
                    Log::Error(e.what());
                }
                ptr->isRunning = false;
            },
            &_captureInfos[threadIndex]
        );
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    }
    else {
        Log::Error("Can't map data (0) from GPU in frame capture");
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
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

    std::array<char, 6> Buffer = {};
    std::fill(Buffer.begin(), Buffer.end(), '\0');
    std::format_to_n(Buffer.data(), Buffer.size(), "{:06}", frameNumber);

    std::filesystem::path file;
    if (!Engine::instance().capturePath().empty()) {
        file = Engine::instance().capturePath() / "";
    }
    if (!Engine::instance().prefixScreenshot().empty()) {
        file += Engine::instance().prefixScreenshot();
        file += '_';
    }
    if (Engine::instance().addNodeNameToScreenshot() &&
        ClusterManager::instance().numberOfNodes() > 1)
    {
        file += std::format("node{}_", ClusterManager::instance().thisNodeId());
    }
    if (Engine::instance().addWindowNameToScreenshot()) {
        const Window& w = *Engine::instance().windows()[_windowIndex];
        if (w.name().empty()) {
            file += std::format("win{}", _windowIndex);
        }
        else {
            file += w.name();
        }
        file += '_';
    }

    if (!eyeSuffix.empty()) {
        file += eyeSuffix + '_';
    }

    return std::format("{}{}.png", file, std::string(Buffer.begin(), Buffer.end()));
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

Image* ScreenCapture::prepareImage(int index, std::string file) {
    Log::Debug(std::format("Starting thread for screenshot/capture [{}]", index));

    if (_captureInfos[index].frameBufferImage == nullptr) {
        constexpr int nChannels = 3;
        _captureInfos[index].frameBufferImage = std::make_unique<Image>();
        _captureInfos[index].frameBufferImage->setBytesPerChannel(_bytesPerColor);
        _captureInfos[index].frameBufferImage->setChannels(nChannels);
        _captureInfos[index].frameBufferImage->setSize(_resolution);
        if (_bytesPerColor * nChannels * _resolution.x * _resolution.y == 0) {
            _captureInfos[index].frameBufferImage = nullptr;
            return nullptr;
        }
        _captureInfos[index].frameBufferImage->allocateOrResizeData();
    }
    _captureInfos[index].filename = std::move(file);

    return _captureInfos[index].frameBufferImage.get();
}

} // namespace sgct
