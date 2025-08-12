/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/window.h>
#include <sgct/clustermanager.h>
#include <sgct/config.h>
#include <sgct/engine.h>
#include <sgct/error.h>
#include <sgct/format.h>
#include <sgct/internalshaders.h>
#include <sgct/log.h>
#include <sgct/networkmanager.h>
#include <sgct/node.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/opengl.h>
#include <sgct/profiling.h>
#include <sgct/screencapture.h>
#include <sgct/statisticsrenderer.h>
#include <sgct/texturemanager.h>
#include <sgct/projection/nonlinearprojection.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>

#ifdef SGCT_HAS_SCALABLE
#include "EasyBlendSDK.h"
#endif // SGCT_HAS_SCALABLE

#ifdef SGCT_HAS_SPOUT
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <SpoutLibrary.h>
#endif // SGCT_HAS_SPOUT

#ifdef WIN32
#include <Windows.h>
#include <glad/glad_wgl.h>
#endif // WIN32

#ifdef WIN32
#include <glad/glad_wgl.h>
#else // ^^^^ WIN32 // !WIN32 vvvv
#include <glad/glad.h>
#endif // WIN32

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define Err(code, msg) Error(Error::Component::Window, code, msg)

namespace {
    sgct::Window::StereoMode convert(sgct::config::Window::StereoMode mode) {
        using SM = sgct::config::Window::StereoMode;
        switch (mode) {
            case SM::NoStereo:
                return sgct::Window::StereoMode::NoStereo;
            case SM::Active:
                return sgct::Window::StereoMode::Active;
            case SM::AnaglyphRedCyan:
                return sgct::Window::StereoMode::AnaglyphRedCyan;
            case SM::AnaglyphAmberBlue:
                return sgct::Window::StereoMode::AnaglyphAmberBlue;
            case SM::AnaglyphRedCyanWimmer:
                return sgct::Window::StereoMode::AnaglyphRedCyanWimmer;
            case SM::Checkerboard:
                return sgct::Window::StereoMode::Checkerboard;
            case SM::CheckerboardInverted:
                return sgct::Window::StereoMode::CheckerboardInverted;
            case SM::VerticalInterlaced:
                return sgct::Window::StereoMode::VerticalInterlaced;
            case SM::VerticalInterlacedInverted:
                return sgct::Window::StereoMode::VerticalInterlacedInverted;
            case SM::Dummy:
                return sgct::Window::StereoMode::Dummy;
            case SM::SideBySide:
                return sgct::Window::StereoMode::SideBySide;
            case SM::SideBySideInverted:
                return sgct::Window::StereoMode::SideBySideInverted;
            case SM::TopBottom:
                return sgct::Window::StereoMode::TopBottom;
            case SM::TopBottomInverted:
                return sgct::Window::StereoMode::TopBottomInverted;
            default:
                throw std::logic_error("Unhandled case label");
        }
    }

    unsigned int colorBitDepthToColorFormat(sgct::config::Window::ColorBitDepth cbd) {
        using CBD = sgct::config::Window::ColorBitDepth;
        switch (cbd) {
            case CBD::Depth8:       return GL_RGBA8;
            case CBD::Depth16:      return GL_RGBA16;
            case CBD::Depth16Float: return GL_RGBA16F;
            case CBD::Depth32Float: return GL_RGBA32F;
            case CBD::Depth16Int:   return GL_RGBA16I;
            case CBD::Depth32Int:   return GL_RGBA32I;
            case CBD::Depth16UInt:  return GL_RGBA16UI;
            case CBD::Depth32UInt:  return GL_RGBA32UI;
            default: throw std::logic_error("Unhandled case label");
        }
    }

    unsigned int colorBitDepthToDataType(sgct::config::Window::ColorBitDepth cbd) {
        using CBD = sgct::config::Window::ColorBitDepth;
        switch (cbd) {
            case CBD::Depth8:       return GL_UNSIGNED_BYTE;
            case CBD::Depth16:      return GL_UNSIGNED_SHORT;
            case CBD::Depth16Float: return GL_HALF_FLOAT;
            case CBD::Depth32Float: return GL_FLOAT;
            case CBD::Depth16Int:   return GL_SHORT;
            case CBD::Depth32Int:   return GL_INT;
            case CBD::Depth16UInt:  return GL_UNSIGNED_SHORT;
            case CBD::Depth32UInt:  return GL_UNSIGNED_INT;
            default: throw std::logic_error("Unhandled case label");
        }
    }

    unsigned int colorBitDepthToBytesPerColor(sgct::config::Window::ColorBitDepth cbd) {
        using CBD = sgct::config::Window::ColorBitDepth;
        switch (cbd) {
            case CBD::Depth8:       return 1;
            case CBD::Depth16:      return 2;
            case CBD::Depth16Float: return 2;
            case CBD::Depth32Float: return 4;
            case CBD::Depth16Int:   return 2;
            case CBD::Depth32Int:   return 4;
            case CBD::Depth16UInt:  return 2;
            case CBD::Depth32UInt:  return 4;
            default: throw std::logic_error("Unhandled case label");
        }
    }

    enum class BufferMode { BackBufferBlack, RenderToTexture };
    void setAndClearBuffer(const sgct::Window& window, BufferMode buffer,
                           sgct::FrustumMode frustum)
    {
        ZoneScoped;

        if (buffer == BufferMode::BackBufferBlack) {
            // Set buffer
            if (window.stereoMode() != sgct::Window::StereoMode::Active) {
                glDrawBuffer(GL_BACK);
                glReadBuffer(GL_BACK);
            }
            else if (frustum == sgct::FrustumMode::StereoLeft) {
                // if active left
                glDrawBuffer(GL_BACK_LEFT);
                glReadBuffer(GL_BACK_LEFT);
            }
            else if (frustum == sgct::FrustumMode::StereoRight) {
                // if active right
                glDrawBuffer(GL_BACK_RIGHT);
                glReadBuffer(GL_BACK_RIGHT);
            }

            // when rendering textures to backbuffer (using fbo)
            glClearColor(0.f, 0.f, 0.f, 0.f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        else {
            glClearColor(0.f, 0.f, 0.f, 0.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
    }
} // namespace

namespace sgct {

static GLFWwindow* _activeContext = nullptr;

bool Window::_useSwapGroups = false;
bool Window::_isBarrierActive = false;
GLFWwindow* Window::_sharedHandle = nullptr;

void Window::makeSharedContextCurrent() {
    ZoneScoped;

    if (_activeContext == _sharedHandle) {
        // glfwMakeContextCurrent is expensive even if we don't change the context
        return;
    }
    _activeContext = _sharedHandle;
    glfwMakeContextCurrent(_sharedHandle);
}

void Window::initNvidiaSwapGroups() {
    ZoneScoped;

#ifdef WIN32
    if (glfwExtensionSupported("WGL_NV_swap_group")) {
        Log::Info("Joining Nvidia swap group");

        HDC hDC = wglGetCurrentDC();
        unsigned int maxBarrier = 0;
        unsigned int maxGroup = 0;
        const BOOL res = wglQueryMaxSwapGroupsNV(hDC, &maxGroup, &maxBarrier);
        if (res == GL_FALSE) {
            throw Err(3006, "Error requesting maximum number of swap groups");
        }
        Log::Info(std::format(
            "WGL_NV_swap_group extension is supported. Max number of groups: {}. "
            "Max number of barriers: {}", maxGroup, maxBarrier
        ));

        if (maxGroup > 0) {
            _useSwapGroups = wglJoinSwapGroupNV(hDC, 1) == GL_TRUE;
            Log::Info(std::format(
                "Joining swapgroup 1 [{}]", _useSwapGroups ? "ok" : "failed"
            ));
        }
        else {
            Log::Error("No swap group found. This instance will not use swap groups");
            _useSwapGroups = false;
        }
    }
    else {
        _useSwapGroups = false;
    }
#endif // WIN32
}

void Window::resetSwapGroupFrameNumber() {
#ifdef WIN32
    if (_isBarrierActive && glfwExtensionSupported("WGL_NV_swap_group")) {
        HDC hDC = wglGetCurrentDC();
        int res = wglResetFrameCountNV(hDC);
        if (res) {
            Log::Info("Resetting frame counter");
        }
        else {
            Log::Info("Resetting frame counter failed");
        }
    }
#endif // WIN32
}

void Window::setBarrier([[maybe_unused]] bool state) {
#ifdef WIN32
    if (_useSwapGroups && state != _isBarrierActive) {
        Log::Info("Enabling Nvidia swap barrier");
        _isBarrierActive = wglBindSwapBarrierNV(1, state ? 1 : 0) == GL_TRUE;
    }
#endif // WIN32
}

bool Window::isBarrierActive() {
    return _isBarrierActive;
}

bool Window::isUsingSwapGroups() {
    return _useSwapGroups;
}

unsigned int Window::swapGroupFrameNumber() {
    unsigned int frameNumber = 0;
#ifdef WIN32
    if (_isBarrierActive && glfwExtensionSupported("WGL_NV_swap_group")) {
        HDC hDC = wglGetCurrentDC();
        wglQueryFrameCountNV(hDC, &frameNumber);
    }
#endif // WIN32
    return frameNumber;
}

config::Window createScalableConfiguration([[maybe_unused]]
                                           const config::Window::Scalable& scalable)
{
    config::Window res;

#ifdef SGCT_HAS_SCALABLE
    EasyBlendSDK_Mesh mesh;
    std::string p = scalable.mesh.string();
    EasyBlendSDKError err = EasyBlendSDK_Initialize(
        p.c_str(),
        &mesh,
        EasyBlendSDK_SDKMODE_ClientData,
        [](EasyBlendSDK_Mesh*, void*) { return static_cast<unsigned long>(0); }
    );
    if (err != EasyBlendSDK_ERR_S_OK) {
        throw Error(
            sgct::Error::Component::Config,
            1104,
            std::format(
                "Could not read ScalableMesh '{}' with error: {}", scalable.mesh, err
            )
        );
    }

    res.isDecorated = false;
    res.draw2D = false;
    res.pos = ivec2 { 0, 0 };
    res.size = ivec2 { static_cast<int>(mesh.Xres), static_cast<int>(mesh.Yres) };
    res.scalable = scalable;

    if (mesh.Projection == EasyBlendSDK_PROJECTION_Perspective) {
        // If the projection type in the mesh file is PERSPECTIVE, we can extract the
        // field-of-view values out of the files and create a PlanarProjection with them

        // Set projection values
        config::PlanarProjection proj;
        proj.fov = {
            .down = static_cast<float>(mesh.Frustum.BottomAngle),
            .left = static_cast<float>(mesh.Frustum.LeftAngle),
            .right = static_cast<float>(mesh.Frustum.RightAngle),
            .up = static_cast<float>(mesh.Frustum.TopAngle)
        };
        double heading = 0.0;
        double pitch = 0.0;
        double roll = 0.0;
        EasyBlendSDK_GetHeadingPitchRoll(heading, pitch, roll, &mesh);
        // Inverting some values as EasyBlend and OpenSpace use a different left-handed vs
        // right-handed coordinate system
        const glm::quat q = glm::quat(glm::vec3(
            glm::radians(-pitch),
            glm::radians(-heading),
            glm::radians(roll)
        ));
        proj.orientation = quat(q.x, q.y, q.z, q.w);
        proj.offset = vec3{
            static_cast<float>(mesh.Frustum.XOffset),
            static_cast<float>(mesh.Frustum.YOffset),
            static_cast<float>(mesh.Frustum.ZOffset)
        };

        res.viewports = {
            config::Viewport {.projection = proj }
        };
    }
    else if (mesh.Projection == EasyBlendSDK_PROJECTION_Orthographic) {
        // If the projection is orthographic, we need to create a fisheye projection
        // instead

        res.resolution = ivec2{
            scalable.orthographicResolution.value_or(4096),
            scalable.orthographicResolution.value_or(4096)
        };
        config::FisheyeProjection proj = {
            .quality = scalable.orthographicQuality.value_or(2048),
            .background = vec4{ 0.f, 0.f, 0.f, 1.f }
        };

        res.viewports = {
            config::Viewport {
                .projection = proj
            }
        };

    }
    else {
        throw Error(
            sgct::Error::Component::Config,
            1104,
            std::format(
                "Could not read ScalableMesh '{}' with error: Unknown projection type {}",
                scalable.mesh, mesh.Projection
            )
        );
    }

    EasyBlendSDK_Uninitialize(&mesh);
#else // ^^^^ SGCT_HAS_SCALABLE // !SGCT_HAS_SCALABLE vvvv
    assert(false);
#endif // SGCT_HAS_SCALABLE

    return res;
}

Window::Window(const config::Window& window)
    : _name(window.name.value_or(""))
    , _tags(window.tags)
    , _id(window.id)
    , _hideMouseCursor(window.hideMouseCursor.value_or(false))
    , _takeScreenshot(window.takeScreenshot.value_or(true))
    , _hasAlpha(window.alpha.value_or(false))
    , _hasCallDraw2DFunction(window.draw2D.value_or(true))
    , _hasCallDraw3DFunction(window.draw3D.value_or(true))
    , _isFullScreen(window.isFullScreen.value_or(false))
    , _shouldAutoiconify(window.shouldAutoiconify.value_or(false))
    , _isFloating(window.isFloating.value_or(false))
    , _shouldRenderWhileHidden(window.alwaysRender.value_or(false))
    , _nAASamples(window.msaa.value_or(1))
    , _useFXAA(window.useFxaa.value_or(false))
    , _isDecorated(window.isDecorated.value_or(true))
    , _isResizable(window.isResizable.value_or(true))
    , _blitWindowId(window.blitWindowId.value_or(-1))
    , _monitorIndex(window.monitor.value_or(0))
    , _mirrorX(window.mirrorX.value_or(false))
    , _mirrorY(window.mirrorY.value_or(false))
    , _noError(window.noError.value_or(false))
    , _isVisible(!window.isHidden.value_or(false))
    , _stereoMode(convert(window.stereo.value_or(config::Window::StereoMode::NoStereo)))
    , _windowPos(window.pos)
    , _windowRes(window.size)
    , _framebufferRes(window.size)
    , _aspectRatio(static_cast<float>(window.size.x) / static_cast<float>(window.size.y))
#ifdef SGCT_HAS_SPOUT
    , _spoutEnabled(window.spout.has_value() && window.spout->enabled)
    , _spoutName(window.spout ? window.spout->name.value_or("") : "")
#endif // SGCT_HAS_SPOUT
    , _internalColorFormat(colorBitDepthToColorFormat(
        window.bufferBitDepth.value_or(config::Window::ColorBitDepth::Depth8)
    ))
    , _colorDataType(colorBitDepthToDataType(
        window.bufferBitDepth.value_or(config::Window::ColorBitDepth::Depth8)
    ))
    , _bytesPerColor(colorBitDepthToBytesPerColor(
        window.bufferBitDepth.value_or(config::Window::ColorBitDepth::Depth8)
    ))
{
    ZoneScoped;

    if (window.resolution) {
        ZoneScopedN("Resolution");

        setFramebufferResolution(*window.resolution);
        setFixResolution(true);
    }

#ifdef SGCT_HAS_NDI
    if (window.ndi && window.ndi->enabled) {
        _ndiName = window.ndi->name.value_or(_ndiName);
        _ndiGroups = window.ndi->groups.value_or(_ndiGroups);

        NDIlib_send_create_t createDesc;
        if (!_ndiName.empty()) {
            createDesc.p_ndi_name = _ndiName.c_str();
        }
        if (!_ndiGroups.empty()) {
            createDesc.p_groups = _ndiGroups.c_str();
        }

        _ndiHandle = NDIlib_send_create(&createDesc);
        if (!_ndiHandle) {
            Log::Error("Error creating NDI sender");
        }

        ivec2 res = window.resolution.value_or(window.size);
        _videoFrame.xres = res.x;
        _videoFrame.yres = res.y;
        _videoFrame.FourCC = NDIlib_FourCC_type_RGBX;
        // We have a negative stride to account for the fact that OpenGL textures have
        // their y-axis flipped compared to DirectX textures
        _videoFrame.line_stride_in_bytes = -res.x * 4;
        _videoFrame.frame_rate_N = 60000; // 60 fps
        _videoFrame.frame_rate_D = 1000;  // 60 fps
        _videoFrame.picture_aspect_ratio =
            static_cast<float>(res.x) / static_cast<float>(res.y);
        _videoFrame.frame_format_type = NDIlib_frame_format_type_progressive;
        _videoFrame.timecode = 0;

        _videoBufferPing.resize(res.x * res.y * 4);
        _videoBufferPong.resize(res.x * res.y * 4);
    }
#endif // SGCT_HAS_NDI

#ifdef SGCT_HAS_SCALABLE
    if (window.scalable.has_value()) {
        _scalableMesh.path = window.scalable->mesh;
    }
#endif // SGCT_HAS_SCALABLE

    for (const config::Viewport& viewport : window.viewports) {
        auto vp = std::make_unique<Viewport>(viewport, *this);
        addViewport(std::move(vp));
    }
}

void Window::openWindow(GLFWwindow* share, bool isLastWindow) {
    ZoneScoped;

    {
        ZoneScopedN("Set GLFW settings");
        glfwWindowHint(GLFW_DEPTH_BITS, 32);
        glfwWindowHint(GLFW_DECORATED, _isDecorated ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_RESIZABLE, _isResizable ? GLFW_TRUE : GLFW_FALSE);

        const int antiAliasingSamples = _nAASamples;
        glfwWindowHint(GLFW_SAMPLES, antiAliasingSamples > 1 ? antiAliasingSamples : 0);

        glfwWindowHint(GLFW_AUTO_ICONIFY, _shouldAutoiconify ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_FLOATING, _isFloating ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

        glfwWindowHint(GLFW_CONTEXT_RELEASE_BEHAVIOR, GLFW_RELEASE_BEHAVIOR_NONE);
        if (_noError) {
            glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GLFW_TRUE);
        }
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif // __APPLE__
        if (!_isVisible) {
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        }
    }

    if (_stereoMode == StereoMode::Active) {
        glfwWindowHint(GLFW_STEREO, GLFW_TRUE);
        Log::Info(std::format("Window {}: Enabling quadbuffered rendering", _id));
    }

    GLFWmonitor* mon = nullptr;
    if (_isFullScreen) {
        ZoneScopedN("Fullscreen Settings");
        int count = 0;
        GLFWmonitor** monitors = glfwGetMonitors(&count);

        if (_monitorIndex > 0 && _monitorIndex < count) {
            mon = monitors[_monitorIndex];
        }
        else {
            mon = glfwGetPrimaryMonitor();
            if (_monitorIndex >= count) {
                Log::Info(std::format(
                    "Window({}): Invalid monitor index ({}). Computer has {} monitors",
                    _id, _monitorIndex, count
                ));
            }
        }

        const GLFWvidmode* currentMode = glfwGetVideoMode(mon);
        if (!_windowRes) {
            _windowRes = ivec2{ currentMode->width, currentMode->height };
        }

        glfwWindowHint(GLFW_RED_BITS, currentMode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, currentMode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, currentMode->blueBits);
    }

    {
        ZoneScopedN("glfwCreateWindow");
        assert(_windowRes);
        _windowHandle = glfwCreateWindow(_windowRes->x, _windowRes->y, "SGCT", mon, share);
        glfwSetWindowUserPointer(_windowHandle, this);
        if (_windowHandle == nullptr) {
            throw Err(8000, "Error opening GLFW window");
        }
    }

    {
        ZoneScopedN("glfwMakeContextCurrent");
        _sharedHandle = share != nullptr ? share : _windowHandle;
        glfwMakeContextCurrent(_windowHandle);
    }

    // Mac for example scales the window size != frame buffer size
    glm::ivec2 bufferSize;
    {
        ZoneScopedN("glfwGetFramebufferSize");
        glfwGetFramebufferSize(_windowHandle, &bufferSize[0], &bufferSize[1]);
    }

    _scale.x = static_cast<float>(bufferSize.x) / static_cast<float>(_windowRes->x);
    _scale.y = static_cast<float>(bufferSize.y) / static_cast<float>(_windowRes->y);
    if (!_useFixResolution) {
        _framebufferRes.x = bufferSize.x;
        _framebufferRes.y = bufferSize.y;
    }

    // Swap interval:
    //  -1 = adaptive sync
    //   0 = vertical sync off
    //   1 = wait for vertical sync
    //   2 = fix when using swapgroups in xp and running half the framerate

    // If we would set multiple windows to use vsync, we would get a framerate of (monitor
    // refreshrate)/(number of windows), which is something that might really slow down a
    // multi-monitor application. Setting last window to the requested interval, which
    // does mean all other windows will respect the last window in the pipeline.
    glfwSwapInterval(isLastWindow ? Engine::instance().settings().swapInterval : 0);

    // if client, disable mouse pointer
    if (_hideMouseCursor || !Engine::instance().isMaster()) {
        glfwSetInputMode(_windowHandle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }

    _hasFocus = glfwGetWindowAttrib(_windowHandle, GLFW_FOCUSED) == GLFW_TRUE;

    const bool captureBackBuffer = Engine::instance().settings().captureBackBuffer;
    int bytesPerColor = captureBackBuffer ? 1 : _bytesPerColor;
    unsigned int colorDataType = captureBackBuffer ? GL_UNSIGNED_BYTE : _colorDataType;
    if (!useRightEyeTexture()) {
        _screenCaptureLeftOrMono = std::make_unique<ScreenCapture>(
            *this,
            ScreenCapture::EyeIndex::Mono,
            bytesPerColor,
            colorDataType,
            _hasAlpha
        );
    }
    else {
        _screenCaptureLeftOrMono = std::make_unique<ScreenCapture>(
            *this,
            ScreenCapture::EyeIndex::StereoLeft,
            bytesPerColor,
            colorDataType,
            _hasAlpha
        );
        _screenCaptureRight = std::make_unique<ScreenCapture>(
            *this,
            ScreenCapture::EyeIndex::Mono,
            bytesPerColor,
            colorDataType,
            _hasAlpha
        );
    }
    _finalFBO = std::make_unique<OffScreenBuffer>(_internalColorFormat);

    if (!_isFullScreen) {
        if (_windowPos) {
            glfwSetWindowPos(_windowHandle, _windowPos->x, _windowPos->y);
        }
        glfwSetWindowSizeCallback(
            _windowHandle,
            [](GLFWwindow* window, int width, int height) {
                Window* win = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
                width = std::max(width, 1);
                height = std::max(height, 1);
                
                // In case this callback gets triggered from elsewhere than SGCT's
                // glfwPollEvents, we want to make sure the actual resizing is deferred to
                // the end of the frame. This can happen if some other library pulls
                // events from the operating system for example by calling
                // nextEventMatchingMask (MacOS) or PeekMessage (Windows). If we were to
                // set the actual resolution directly, we may render half a frame with
                // resolution A and the other half with resolution b, which is undefined
                // behaviour. _pendingWindowRes is checked in Window::updateResolution,
                // which is called from Engine's render loop after glfwPollEvents.
                win->_pendingWindowRes = ivec2 { width, height };
            }
        );
        glfwSetFramebufferSizeCallback(
            _windowHandle,
            [](GLFWwindow* window, int width, int height) {
                Window* win = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
                width = std::max(width, 1);
                height = std::max(height, 1);
                win->setFramebufferResolution(sgct::ivec2{ width, height });
            }
        );
        glfwSetWindowFocusCallback(
            _windowHandle,
            [](GLFWwindow* window, int state) {
                Window* win = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
                win->_hasFocus = (state == GLFW_TRUE);
            }
        );
    }

    const std::string title = std::format(
        "SGCT node: {} ({}: {})",
        ClusterManager::instance().thisNode().address(),
        (NetworkManager::instance().isComputerServer() ? "server" : "client"),
        _id
    );

    glfwSetWindowTitle(_windowHandle, _name.empty() ? title.c_str() : _name.c_str());

    {
        // swap the buffers and update the window
        ZoneScopedN("glfwSwapBuffers");
        glfwSwapBuffers(_windowHandle);
    }
}

void Window::closeWindow() {
    ZoneScoped;

#ifdef SGCT_HAS_SCALABLE
    if (_scalableMesh.sdk) {
        EasyBlendSDK_Uninitialize(
            reinterpret_cast<EasyBlendSDK_Mesh*>(_scalableMesh.sdk)
        );
        delete _scalableMesh.sdk;
        _scalableMesh.sdk = nullptr;
    }
#endif // SGCT_HAS_SCALABLE

#ifdef SGCT_HAS_SPOUT
    if (_spoutHandle) {
        _spoutHandle->ReleaseSender();
        _spoutHandle->Release();
    }
#endif // SGCT_HAS_SPOUT

#ifdef SGCT_HAS_NDI
    if (_ndiHandle) {
        // One of the two buffers might be still in flight so we have to flush the
        // pipeline before we can destroy the video or we might risk NDI accessing dead
        // memory and crashing
        NDIlib_send_send_video_async_v2(_ndiHandle, nullptr);
        NDIlib_send_destroy(_ndiHandle);
    }
#endif // SGCT_HAS_NDI

    makeSharedContextCurrent();

    Log::Info(std::format("Deleting screen capture data for window {}", _id));
    _screenCaptureLeftOrMono = nullptr;
    _screenCaptureRight = nullptr;

    // delete FBO stuff
    if (_finalFBO) {
        Log::Info(std::format("Releasing OpenGL buffers for window {}", _id));
        _finalFBO = nullptr;
        destroyFBOs();
    }

    Log::Info(std::format("Deleting VBOs for window {}", _id));
    glDeleteBuffers(1, &_vbo);
    _vbo = 0;

    Log::Info(std::format("Deleting VAOs for window {}", _id));
    glDeleteVertexArrays(1, &_vao);
    _vao = 0;

    _fboQuad.deleteProgram();
    _overlay.deleteProgram();
    _stereo.deleteProgram();
    if (_fxaa) {
        _fxaa->shader.deleteProgram();
    }

    // Current handle must be set at the end to properly destroy the window
    makeOpenGLContextCurrent();

    _viewports.clear();

    glfwSetWindowSizeCallback(_windowHandle, nullptr);
    glfwSetFramebufferSizeCallback(_windowHandle, nullptr);
    glfwSetWindowFocusCallback(_windowHandle, nullptr);
    glfwSetWindowIconifyCallback(_windowHandle, nullptr);

#ifdef WIN32
    if (_useSwapGroups && glfwExtensionSupported("WGL_NV_swap_group")) {
        HDC hDC = wglGetCurrentDC();
        wglBindSwapBarrierNV(1, 0); // un-bind
        wglJoinSwapGroupNV(hDC, 0); // un-join
    }
#endif // WIN32
}

void Window::initialize() {
    ZoneScoped;

    createTextures();
    createVBOs();

    _finalFBO->createFBO(_framebufferRes.x, _framebufferRes.y, _nAASamples);

    Log::Debug(std::format(
        "Window {}: FBO initiated successfully. Number of samples: {}",
        _id, _finalFBO->isMultiSampled() ? _nAASamples : 1
    ));

    const ivec2 res =
        Engine::instance().settings().captureBackBuffer ?
        windowResolution() :
        framebufferResolution();

    if (_screenCaptureLeftOrMono) {
        _screenCaptureLeftOrMono->resize(res);
    }

    if (_screenCaptureRight) {
        _screenCaptureRight->resize(res);
    }

    loadShaders();

#ifdef SGCT_HAS_SPOUT
    if (_spoutEnabled) {
        _spoutName = _spoutName.empty() ? "OpenSpace" : _spoutName;

        _spoutHandle = GetSpout();
        bool success = false;
        if (_spoutHandle) {
            success = _spoutHandle->CreateSender(
                _spoutName.c_str(),
                _framebufferRes.x,
                _framebufferRes.y
            );
        }
        if (!success) {
            Log::Error(std::format("Error creating SPOUT handle for {}", _spoutName));
        }
    }
#endif // SGCT_HAS_SPOUT

    for (const std::unique_ptr<Viewport>& vp : _viewports) {
        const vec2 viewportSize = vec2{
            _framebufferRes.x * vp->size().x,
            _framebufferRes.y * vp->size().y
        };
        vp->initialize(
            viewportSize,
            _stereoMode != StereoMode::NoStereo,
            _internalColorFormat,
            GL_BGRA,
            _colorDataType,
            _nAASamples
        );
    }
}

void Window::initializeContextSpecific() {
    ZoneScoped;

    makeOpenGLContextCurrent();
    std::for_each(_viewports.begin(), _viewports.end(), std::mem_fn(&Viewport::loadData));
    _hasAnyMasks = std::any_of(
        _viewports.cbegin(),
        _viewports.cend(),
        [](const std::unique_ptr<Viewport>& vp) {
            return vp->hasBlendMaskTexture() || vp->hasBlackLevelMaskTexture();
        }
    );

#ifdef SGCT_HAS_SCALABLE
    if (!_scalableMesh.path.empty()) {
        std::string path = _scalableMesh.path.string();
        _scalableMesh.sdk = new EasyBlendSDK_Mesh;
        [[maybe_unused]] EasyBlendSDKError err = EasyBlendSDK_Initialize(
            path.c_str(),
            reinterpret_cast<EasyBlendSDK_Mesh*>(_scalableMesh.sdk)
        );
        // We already loaded the path in the constructor and verified that it works
        assert(err == EasyBlendSDK_ERR_S_OK);
    }
#endif // SGCT_HAS_SCALABLE
}

void Window::updateResolutions() {
    ZoneScoped;

    if (_pendingWindowRes) {
        _windowRes = *_pendingWindowRes;
        _windowResChanged = true;
        float ratio =
            static_cast<float>(_windowRes->x) / static_cast<float>(_windowRes->y);

        // Set field of view of each of this window's viewports to match new aspect ratio,
        // adjusting only the horizontal (x) values
        for (const std::unique_ptr<Viewport>& vp : _viewports) {
            vp->updateFovToMatchAspectRatio(_aspectRatio, ratio);
            Log::Debug(std::format(
                "Update aspect ratio in viewport ({} -> {})", _aspectRatio, ratio
            ));
        }
        _aspectRatio = ratio;

        // Redraw window
        glfwSetWindowSize(_windowHandle, _windowRes->x, _windowRes->y);

        Log::Debug(std::format(
            "Resolution changed to {}x{} in window {}", _windowRes->x, _windowRes->y, _id
        ));
        _pendingWindowRes = std::nullopt;

#ifdef SGCT_HAS_NDI
        if (!_useFixResolution) {
            _videoFrame.xres = _windowRes->x;
            _videoFrame.yres = _windowRes->y;
            _videoFrame.picture_aspect_ratio =
                static_cast<float>(_windowRes->x) / static_cast<float>(_windowRes->y);
            // We have a negative stride to account for the fact that OpenGL textures have
            // their y-axis flipped compared to DirectX textures
            _videoFrame.line_stride_in_bytes = -_windowRes->x * 4;

            // We need to send an "empty" frame as otherwise NDI might be in the middle of
            // sending out one of the buffers that we are resizing. This extra call will
            // force a synchronization
            NDIlib_send_send_video_async_v2(_ndiHandle, nullptr);
            _videoBufferPing.resize(_windowRes->x * _windowRes->y * 4);
            _videoBufferPong.resize(_windowRes->x * _windowRes->y * 4);

        }
#endif // SGCT_HAS_NDI
    }

    if (_pendingFramebufferRes) {
        _framebufferRes = *_pendingFramebufferRes;

        Log::Debug(std::format(
            "Framebuffer resolution changed to {}x{} for window {}",
            _framebufferRes.x, _framebufferRes.y, _id
        ));

        _pendingFramebufferRes = std::nullopt;
    }
}

void Window::update() {
    ZoneScoped;

    if (!_isVisible || !isWindowResized()) {
        return;
    }
    makeOpenGLContextCurrent();

    resizeFBOs();

    const ivec2 res =
        Engine::instance().settings().captureBackBuffer ?
        windowResolution() :
        framebufferResolution();
    if (_screenCaptureLeftOrMono) {
        _screenCaptureLeftOrMono->resize(res);
    }
    if (_screenCaptureRight) {
        _screenCaptureRight->resize(res);
    }

    // resize non linear projection buffers
    for (const std::unique_ptr<Viewport>& vp : _viewports) {
        if (vp->hasSubViewports()) {
            const vec2 viewport = vec2{
                _framebufferRes.x * vp->size().x,
                _framebufferRes.y * vp->size().y
            };
            vp->nonLinearProjection()->update(viewport);
        }
    }
}

void Window::draw() {
    ZoneScopedN("Render window");

    if (!(isVisible() || isRenderingWhileHidden())) [[unlikely]] {
        return;
    }

    // Render Left/Mono non-linear projection viewports to cubemap
    for (const std::unique_ptr<Viewport>& vp : viewports()) {
        ZoneScopedN("Render viewport");

        if (!vp->hasSubViewports()) {
            continue;
        }

        NonLinearProjection* nonLinearProj = vp->nonLinearProjection();
        if (_stereoMode == Window::StereoMode::NoStereo) {
            // for mono viewports frustum mode can be selected by user or config
            nonLinearProj->renderCubemap(vp->eye());
        }
        else {
            nonLinearProj->renderCubemap(FrustumMode::StereoLeft);
        }
    }

    // Render left/mono regular viewports to FBO
    // if any stereo type (except passive) then set frustum mode to left eye
    if (_stereoMode == Window::StereoMode::NoStereo) {
        renderViewports(FrustumMode::Mono, Eye::MonoOrLeft);

        // if we are not rendering in stereo, we are done
        return;
    }
    else {
        renderViewports(FrustumMode::StereoLeft, Eye::MonoOrLeft);
    }

    // Render right non-linear projection viewports to cubemap
    for (const std::unique_ptr<Viewport>& vp : viewports()) {
        ZoneScopedN("Render Cubemap");
        if (!vp->hasSubViewports()) {
            continue;
        }
        NonLinearProjection* p = vp->nonLinearProjection();
        p->renderCubemap(FrustumMode::StereoRight);
    }

    // Render right regular viewports to FBO
    // use a single texture for side-by-side and top-bottom stereo modes
    if (_stereoMode >= Window::StereoMode::SideBySide) {
        renderViewports(FrustumMode::StereoRight, Eye::MonoOrLeft);
    }
    else {
        renderViewports(FrustumMode::StereoRight, Eye::Right);
    }
}

void Window::renderFBOTexture() {
    ZoneScoped;

    if (!_isVisible) [[unlikely]] {
        return;
    }

    OffScreenBuffer::unbind();

    makeOpenGLContextCurrent();

    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const FrustumMode frustum =
        (stereoMode() == Window::StereoMode::Active) ?
        FrustumMode::StereoLeft :
        FrustumMode::Mono;

    const ivec2 size = ivec2{
        static_cast<int>(std::ceil(scale().x * windowResolution().x)),
        static_cast<int>(std::ceil(scale().y * windowResolution().y))
    };

    glViewport(0, 0, size.x, size.y);
    setAndClearBuffer(*this, BufferMode::BackBufferBlack, frustum);

    bool maskShaderSet = false;
    const std::vector<std::unique_ptr<Viewport>>& vps = _viewports;
    if (_stereoMode > Window::StereoMode::Active &&
        _stereoMode < Window::StereoMode::SideBySide)
    {
        _stereo.bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _frameBufferTextures.leftEye);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _frameBufferTextures.rightEye);

        std::for_each(vps.begin(), vps.end(), std::mem_fn(&Viewport::renderWarpMesh));
    }
    else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _frameBufferTextures.leftEye);

        _fboQuad.bind();
        maskShaderSet = true;

        glUniform1i(
            glGetUniformLocation(_fboQuad.id(), "flipX"),
            _mirrorX ? 1 : 0
        );
        glUniform1i(
            glGetUniformLocation(_fboQuad.id(), "flipY"),
            _mirrorY ? 1 : 0
        );

        std::for_each(vps.begin(), vps.end(), std::mem_fn(&Viewport::renderWarpMesh));

        // render right eye in active stereo mode
        if (_stereoMode == Window::StereoMode::Active) {
            glViewport(0, 0, size.x, size.y);

            // clear buffers
            setAndClearBuffer(
                *this,
                BufferMode::BackBufferBlack,
                FrustumMode::StereoRight
            );

            glBindTexture(GL_TEXTURE_2D, _frameBufferTextures.rightEye);
            std::for_each(vps.begin(), vps.end(), std::mem_fn(&Viewport::renderWarpMesh));
        }
    }

    // render mask (mono)
    if (_hasAnyMasks) {
        if (!maskShaderSet) {
            _fboQuad.bind();

            glUniform1i(glGetUniformLocation(_fboQuad.id(), "flipX"), _mirrorX ? 1 : 0);
            glUniform1i(glGetUniformLocation(_fboQuad.id(), "flipY"), _mirrorY ? 1 : 0);
        }

        glDrawBuffer(GL_BACK);
        glReadBuffer(GL_BACK);
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_BLEND);

        // Result = (Color * BlendMask) * (1-BlackLevel) + BlackLevel
        // render blend masks
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        for (const std::unique_ptr<Viewport>& vp : _viewports) {
            ZoneScopedN("Render Viewport");

            if (vp->hasBlendMaskTexture() && vp->isEnabled()) {
                glBindTexture(GL_TEXTURE_2D, vp->blendMaskTextureIndex());
                vp->renderMaskMesh();
            }
            if (vp->hasBlackLevelMaskTexture() && vp->isEnabled()) {
                glBindTexture(GL_TEXTURE_2D, vp->blackLevelMaskTextureIndex());
                vp->renderMaskMesh();
            }
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    ShaderProgram::unbind();
    glDisable(GL_BLEND);

#ifdef SGCT_HAS_SPOUT
    if (_spoutHandle) {
        // Share the window via Spout
        glBindTexture(GL_TEXTURE_2D, _frameBufferTextures.leftEye);
        glCopyTexImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            _internalColorFormat,
            _framebufferRes.x,
            _framebufferRes.y,
            0
        );

        const bool s = _spoutHandle->SendTexture(
            _frameBufferTextures.leftEye,
            static_cast<GLuint>(GL_TEXTURE_2D),
            _framebufferRes.x,
            _framebufferRes.y
        );
        if (!s) {
            Log::Error(std::format("Error sending Spout texture for '{}'", _spoutName));
        }
    }
#endif // SGCT_HAS_SPOUT

#ifdef SGCT_HAS_NDI
    if (_ndiHandle) {
        // Download the texture data from the GPU
        glBindTexture(GL_TEXTURE_2D, _frameBufferTextures.leftEye);
        glCopyTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            0,
            0,
            _framebufferRes.x,
            _framebufferRes.y,
            0
        );
        glGetTexImage(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            _currentVideoBuffer->data()
        );

        assert(_videoFrame.xres == _framebufferRes.x);
        assert(_videoFrame.yres == _framebufferRes.y);
        // We are using a negative line stride to correct for the y-axis flip going from
        // OpenGL to DirectX.  So our start point has to be the beginning of the *last*
        // line of the image as NDI then steps backwards through the image to send it to
        // the receiver.
        // So we start at data() (=0), move to the end (+size) and then backtrack one line
        // (- -line_stride = +line_stride)
        _videoFrame.p_data = reinterpret_cast<uint8_t*>(
            _currentVideoBuffer->data() + _currentVideoBuffer->size() +
            _videoFrame.line_stride_in_bytes
        );

        NDIlib_send_send_video_async_v2(_ndiHandle, &_videoFrame);
        // Switch the current buffer
        _currentVideoBuffer =
            _currentVideoBuffer == &_videoBufferPing ?
            &_videoBufferPong :
            &_videoBufferPing;
    }
#endif // SGCT_HAS_NDI
}

void Window::swapBuffers(bool takeScreenshot) {
    if (!(_isVisible || _shouldRenderWhileHidden)) {
        return;
    }

    makeOpenGLContextCurrent();

    if (takeScreenshot) {
        ZoneScopedN("Take Screenshot");
        if (Engine::instance().settings().captureBackBuffer) {
            if (_screenCaptureLeftOrMono) {
                _screenCaptureLeftOrMono->saveScreenCapture(
                    0,
                    _stereoMode == StereoMode::Active ?
                        ScreenCapture::CaptureSource::LeftBackBuffer :
                        ScreenCapture::CaptureSource::BackBuffer
                );
            }
            if (_screenCaptureRight && _stereoMode == StereoMode::Active) {
                _screenCaptureLeftOrMono->saveScreenCapture(
                    0,
                    ScreenCapture::CaptureSource::RightBackBuffer
                );
            }
        }
        else {
            if (_screenCaptureLeftOrMono) {
                _screenCaptureLeftOrMono->saveScreenCapture(_frameBufferTextures.leftEye);
            }
            if (_screenCaptureRight && _stereoMode > StereoMode::NoStereo &&
                _stereoMode < Window::StereoMode::SideBySide)
            {
                _screenCaptureRight->saveScreenCapture(_frameBufferTextures.rightEye);
            }
        }
    }

    // swap
    _windowResChanged = false;

#ifdef SGCT_HAS_SCALABLE
    if (_scalableMesh.sdk) {
        EasyBlendSDK_TransformInputToOutput(
            reinterpret_cast<EasyBlendSDK_Mesh*>(_scalableMesh.sdk)
        );
    }
#endif // SGCT_HAS_SCALABLE

    {
        ZoneScopedN("glfwSwapBuffers");
        glfwSwapBuffers(_windowHandle);
    }
}

void Window::makeOpenGLContextCurrent() {
    ZoneScoped;

    if (_activeContext == _windowHandle) {
        // glfwMakeContextCurrent is expensive even if we don't change the context
        return;
    }
    _activeContext = _windowHandle;
    glfwMakeContextCurrent(_windowHandle);
}

bool Window::needsCompatibilityProfile() const {
    return !_scalableMesh.path.empty();
}

bool Window::shouldTakeScreenshot() const {
    return _takeScreenshot;
}

vec2 Window::scale() const {
    return _scale;
}

float Window::aspectRatio() const {
    return _aspectRatio;
}

ivec2 Window::windowResolution() const {
    assert(_windowRes);
    return *_windowRes;
}

GLFWwindow* Window::windowHandle() const {
    return _windowHandle;
}

void Window::addViewport(std::unique_ptr<Viewport> vpPtr) {
    _viewports.push_back(std::move(vpPtr));
}

const std::vector<std::unique_ptr<Viewport>>& Window::viewports() const {
    return _viewports;
}

void Window::updateFrustums(float nearClip, float farClip) {
    ZoneScoped;

    for (const std::unique_ptr<Viewport>& vp : _viewports) {
        if (vp->isTracked()) {
            // if not tracked update, otherwise this is done on the fly
            continue;
        }

        vp->calculateFrustum(FrustumMode::Mono, nearClip, farClip);
        vp->calculateFrustum(FrustumMode::StereoLeft, nearClip, farClip);
        vp->calculateFrustum(FrustumMode::StereoRight, nearClip, farClip);
    }
}

void Window::renderScreenQuad() const {
    TracyGpuZone("Render Screen Quad")

    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void Window::setFramebufferResolution(ivec2 resolution) {
    if (!_useFixResolution) {
        _pendingFramebufferRes = std::move(resolution);
    }
}

ivec2 Window::framebufferResolution() const {
    return _framebufferRes;
}

bool Window::isWindowResized() const {
    return _windowResChanged;
}

bool Window::isFocused() const {
    return _hasFocus;
}

int Window::id() const {
    return _id;
}

void Window::setName(std::string name) {
    _name = std::move(name);
}

const std::string& Window::name() const {
    return _name;
}

void Window::setTags(std::vector<std::string> tags) {
    _tags = std::move(tags);
}

bool Window::hasTag(std::string_view tag) const {
    return std::find(_tags.cbegin(), _tags.cend(), tag) != _tags.cend();
}

void Window::setVisible(bool state) {
    if (state != _isVisible && _windowHandle) {
        if (state) {
            glfwShowWindow(_windowHandle);
        }
        else {
            glfwHideWindow(_windowHandle);
        }
    }
    _isVisible = state;
}

bool Window::isVisible() const {
    return _isVisible;
}

void Window::setRenderWhileHidden(bool state) {
    _shouldRenderWhileHidden = state;
}

bool Window::isRenderingWhileHidden() const {
    return _shouldRenderWhileHidden;
}

void Window::setFullscreen(bool fullscreen) {
    _isFullScreen = fullscreen;
}

void Window::setAutoiconify(bool shouldAutoiconify) {
    _shouldAutoiconify = shouldAutoiconify;
}

void Window::setFloating(bool floating) {
    _isFloating = floating;
}

void Window::setTakeScreenshot(bool takeScreenshot) {
    _takeScreenshot = takeScreenshot;
}

void Window::setWindowDecoration(bool state) {
    _isDecorated = state;
}

void Window::setWindowResizable(bool state) {
    _isResizable = state;
}

void Window::setFixResolution(bool state) {
    _useFixResolution = state;
}

bool Window::isFixResolution() const {
    return _useFixResolution;
}

void Window::setHorizFieldOfView(float hFovDeg) {
    // Set field of view of each of this window's viewports to match new horiz/vert
    // aspect ratio, adjusting only the horizontal (x) values
    for (const std::unique_ptr<Viewport>& vp : _viewports) {
        vp->setHorizontalFieldOfView(hFovDeg);
    }
    Log::Debug(std::format("HFOV changed to {} for window {}", hFovDeg, _id));
}

float Window::horizFieldOfViewDegrees() const {
    return _viewports[0]->horizontalFieldOfViewDegrees();
}

void Window::setCallDraw2DFunction(bool state) {
    _hasCallDraw2DFunction = state;
}

void Window::setCallDraw3DFunction(bool state) {
    _hasCallDraw3DFunction = state;
}

void Window::setStereoMode(StereoMode sm) {
    _stereoMode = sm;
    loadShaders();
}

Window::StereoMode Window::stereoMode() const {
    return _stereoMode;
}

bool Window::isStereo() const {
    return _stereoMode != StereoMode::NoStereo;
}

void Window::createTextures() {
    ZoneScoped;
    TracyGpuZone("Create Textures");

    GLint max = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);
    if (_framebufferRes.x > max || _framebufferRes.y > max) {
        Log::Error(std::format(
            "Window {}: Requested framebuffer too big (Max: {})", _id, max
        ));
        return;
    }

    // Create left and right color & depth textures; don't allocate the right eye image if
    // stereo is not used create a postFX texture for effects
    generateTexture(_frameBufferTextures.leftEye, TextureType::Color);
    if (useRightEyeTexture()) {
        generateTexture(_frameBufferTextures.rightEye, TextureType::Color);
    }
    if (Engine::instance().settings().useDepthTexture) {
        generateTexture(_frameBufferTextures.depth, TextureType::Depth);
    }
    if (_useFXAA) {
        generateTexture(_frameBufferTextures.intermediate, TextureType::Color);
    }
    if (Engine::instance().settings().useNormalTexture) {
        generateTexture(_frameBufferTextures.normals, TextureType::Normal);
    }
    if (Engine::instance().settings().usePositionTexture) {
        generateTexture(_frameBufferTextures.positions, TextureType::Position);
    }

    Log::Debug(std::format("Targets initialized successfully for window {}", _id));
}

void Window::generateTexture(unsigned int& id, Window::TextureType type) {
    ZoneScoped;
    TracyGpuZone("Generate Textures");

    glDeleteTextures(1, &id);
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    // Determine the internal texture format, the texture format, and the pixel type
    const std::tuple<GLenum, GLenum, GLenum> formats =
        [this](Window::TextureType t) -> std::tuple<GLenum, GLenum, GLenum>
    {
        switch (t) {
            case TextureType::Color:
                return { _internalColorFormat, GL_BGRA, _colorDataType };
            case TextureType::Depth:
                return { GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT };
            case TextureType::Normal:
            case TextureType::Position:
                return { GL_RGB32F, GL_RGB, GL_FLOAT };
            default:
                throw std::logic_error("Unhandled case label");
        }
    }(type);

    const ivec2 res = _framebufferRes;
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        std::get<0>(formats),
        res.x,
        res.y,
        0,
        std::get<1>(formats),
        std::get<2>(formats),
        nullptr
    );
    Log::Debug(std::format("{}x{} texture generated for window {}", res.x, res.y, id));

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
}

void Window::resizeFBOs() {
    if (_useFixResolution) {
        return;
    }

    makeSharedContextCurrent();
    destroyFBOs();
    createTextures();

    _finalFBO->resizeFBO(_framebufferRes.x, _framebufferRes.y, _nAASamples);

    if (!_finalFBO->isMultiSampled()) {
        _finalFBO->bind();
        _finalFBO->attachColorTexture(_frameBufferTextures.leftEye, GL_COLOR_ATTACHMENT0);
        OffScreenBuffer::unbind();
    }
}

void Window::destroyFBOs() {
    glDeleteTextures(1, &_frameBufferTextures.leftEye);
    _frameBufferTextures.leftEye = 0;
    glDeleteTextures(1, &_frameBufferTextures.rightEye);
    _frameBufferTextures.rightEye = 0;
    glDeleteTextures(1, &_frameBufferTextures.depth);
    _frameBufferTextures.depth = 0;
    glDeleteTextures(1, &_frameBufferTextures.intermediate);
    _frameBufferTextures.intermediate = 0;
    glDeleteTextures(1, &_frameBufferTextures.positions);
    _frameBufferTextures.positions = 0;
}

bool Window::useRightEyeTexture() const {
    return _stereoMode != StereoMode::NoStereo && _stereoMode < StereoMode::SideBySide;
}

void Window::renderViewports(FrustumMode frustum, Eye eye) const {
    ZoneScoped;

    _finalFBO->bind();
    if (_finalFBO->isMultiSampled()) {
        return;
    }

    // update attachments
    _finalFBO->attachColorTexture(frameBufferTextureEye(eye), GL_COLOR_ATTACHMENT0);

    if (Engine::instance().settings().useDepthTexture) {
        _finalFBO->attachDepthTexture(_frameBufferTextures.depth);
    }

    if (Engine::instance().settings().useNormalTexture) {
        _finalFBO->attachColorTexture(_frameBufferTextures.normals, GL_COLOR_ATTACHMENT1);
    }

    if (Engine::instance().settings().usePositionTexture) {
        _finalFBO->attachColorTexture(
            _frameBufferTextures.positions,
            GL_COLOR_ATTACHMENT2
        );
    }

    const Window::StereoMode sm = stereoMode();
    // render all viewports for selected eye
    for (const std::unique_ptr<Viewport>& vp : viewports()) {
        if (!vp->isEnabled()) {
            continue;
        }

        // if passive stereo or mono
        if (sm == Window::StereoMode::NoStereo) {
            // @TODO (abock, 2019-12-04) Not sure about this one; the frustum is set in
            // the calling function based on the stereo mode already and we are
            // overwriting it here
            frustum = vp->eye();
        }

        if (vp->isTracked()) {
            vp->calculateFrustum(
                frustum,
                Engine::instance().nearClipPlane(),
                Engine::instance().farClipPlane()
            );
        }
        if (vp->hasSubViewports()) {
            if (_hasCallDraw3DFunction) {
                vp->nonLinearProjection()->render(*vp, frustum);
            }
        }
        else {
            // check if we want to blit the previous window before we do anything else
            if (_blitWindowId >= 0) {
                const std::vector<std::unique_ptr<Window>>& wins =
                    Engine::instance().windows();
                auto it = std::find_if(
                    wins.cbegin(),
                    wins.cend(),
                    [id = _blitWindowId](const std::unique_ptr<Window>& w) {
                        return w->id() == id;
                    }
                );
                assert(it != wins.cend());
                blitWindowViewport(**it, *vp, frustum);
            }

            if (_hasCallDraw3DFunction) {
                // run scissor test to prevent clearing of entire buffer
                vp->setupViewport(frustum);
                glEnable(GL_SCISSOR_TEST);
                glClearColor(0.f, 0.f, 0.f, 0.f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glDisable(GL_SCISSOR_TEST);

                if (Engine::instance().drawFunction()) {
                    ZoneScopedN("[SGCT] Draw");
                    const RenderData renderData = {
                        *this,
                        *vp,
                        frustum,
                        ClusterManager::instance().sceneTransform(),
                        vp->projection(frustum).viewMatrix(),
                        vp->projection(frustum).projectionMatrix(),
                        vp->projection(frustum).viewProjectionMatrix() *
                            ClusterManager::instance().sceneTransform(),
                        framebufferResolution()
                    };
                    Engine::instance().drawFunction()(renderData);
                }
            }
        }
    }

    // If we did not render anything, make sure we clear the screen at least
    if (!_hasCallDraw3DFunction && _blitWindowId == -1) {
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    else if (_blitWindowId != -1) {
        const std::vector<std::unique_ptr<Window>>& wins = Engine::instance().windows();
        auto it = std::find_if(
            wins.cbegin(),
            wins.cend(),
            [id = _blitWindowId](const std::unique_ptr<Window>& w) {
                return w->id() == id;
            }
        );
        assert(it != wins.cend());
        const Window& srcWin = **it;

        if (!srcWin.isVisible() && !srcWin.isRenderingWhileHidden()) {
            glClearColor(0.f, 0.f, 0.f, 0.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    // for side-by-side or top-bottom mode, do postfx/blit only after rendering right eye
    const bool isSplitScreen = (sm >= Window::StereoMode::SideBySide);
    if (!isSplitScreen || frustum != FrustumMode::StereoLeft) {
        ZoneScopedN("PostFX/Blit");

        // copy AA-buffer to "regular" / non-AA buffer

        if (_finalFBO->isMultiSampled()) {
            // bind separate read and draw buffers to prepare blit operation
            _finalFBO->bindBlit();

            // update attachments
            _finalFBO->attachColorTexture(
                frameBufferTextureEye(eye),
                GL_COLOR_ATTACHMENT0
            );

            if (Engine::instance().settings().useDepthTexture) {
                _finalFBO->attachDepthTexture(_frameBufferTextures.depth);
            }

            if (Engine::instance().settings().useNormalTexture) {
                _finalFBO->attachColorTexture(
                    _frameBufferTextures.normals,
                    GL_COLOR_ATTACHMENT1
                );
            }

            if (Engine::instance().settings().usePositionTexture) {
                _finalFBO->attachColorTexture(
                    _frameBufferTextures.positions,
                    GL_COLOR_ATTACHMENT2
                );
            }
            _finalFBO->blit();
        }


        if (_useFXAA) {
            assert(_fxaa);

            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            // bind target FBO
            _finalFBO->attachColorTexture(
                frameBufferTextureEye(eye),
                GL_COLOR_ATTACHMENT0
            );

            const ivec2 framebufferSize = framebufferResolution();
            glViewport(0, 0, framebufferSize.x, framebufferSize.y);
            glClearColor(0.f, 0.f, 0.f, 0.f);
            glClear(GL_COLOR_BUFFER_BIT);

            glActiveTexture(GL_TEXTURE0);

            glBindTexture(GL_TEXTURE_2D, _frameBufferTextures.intermediate);

            _fxaa->shader.bind();
            glUniform1f(_fxaa->sizeX, static_cast<float>(framebufferSize.x));
            glUniform1f(_fxaa->sizeY, static_cast<float>(framebufferSize.y));

            renderScreenQuad();
            ShaderProgram::unbind();
        }

        render2D(frustum);
        if (isSplitScreen) {
            // render left eye info and graph to render 2D items after post fx
            render2D(FrustumMode::StereoLeft);
        }
    }

    glDisable(GL_BLEND);
}

void Window::render2D(FrustumMode frustum) const {
    ZoneScoped;

    for (const std::unique_ptr<Viewport>& vp : _viewports) {
        if (!vp->isEnabled()) {
            continue;
        }

        vp->setupViewport(frustum);

        // if viewport has overlay
        if (vp->hasOverlayTexture()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, vp->overlayTextureIndex());
            _overlay.bind();
            renderScreenQuad();
        }

        if (Engine::instance().statisticsRenderer()) {
            Engine::instance().statisticsRenderer()->render(*this, *vp);
        }

        // Check if we should call the use defined draw2D function
        if (Engine::instance().draw2DFunction() && _hasCallDraw2DFunction) {
            ZoneScopedN("[SGCT] Draw 2D");
            const RenderData renderData = {
                *this,
                *vp,
                frustum,
                ClusterManager::instance().sceneTransform(),
                vp->projection(frustum).viewMatrix(),
                vp->projection(frustum).projectionMatrix(),
                vp->projection(frustum).viewProjectionMatrix() *
                    ClusterManager::instance().sceneTransform(),
                framebufferResolution()
            };
            Engine::instance().draw2DFunction()(renderData);
        }
    }
}

void Window::blitWindowViewport(const Window& prevWindow, const Viewport& viewport,
                                FrustumMode mode) const
{
    ZoneScoped;

    assert(prevWindow.id() != id());

    // run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);
    viewport.setupViewport(mode);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    _overlay.bind();

    glActiveTexture(GL_TEXTURE0);
    const unsigned int tex = [&prevWindow](FrustumMode v) {
        switch (v) {
            case FrustumMode::Mono:
                return prevWindow._frameBufferTextures.leftEye;
            case FrustumMode::StereoLeft:
                return prevWindow._frameBufferTextures.rightEye;
            case FrustumMode::StereoRight:
                return prevWindow._frameBufferTextures.intermediate;
            default:
                throw std::logic_error("Missing case label");
        }
    }(mode);
    glBindTexture(GL_TEXTURE_2D, tex);

    renderScreenQuad();
    ShaderProgram::unbind();
}

void Window::createVBOs() {
    ZoneScoped;
    TracyGpuZone("Create VBOs");

    constexpr std::array<const float, 36> QuadVerts = {
    //     x     y     z      u    v      r    g    b    a
        -1.f, -1.f, -1.f,   0.f, 0.f,   1.f, 1.f, 1.f, 1.f,
         1.f, -1.f, -1.f,   1.f, 0.f,   1.f, 1.f, 1.f, 1.f,
        -1.f,  1.f, -1.f,   0.f, 1.f,   1.f, 1.f, 1.f, 1.f,
         1.f,  1.f, -1.f,   1.f, 1.f,   1.f, 1.f, 1.f, 1.f
    };

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    glBufferData(GL_ARRAY_BUFFER, 36 * sizeof(float), QuadVerts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), nullptr);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        9 * sizeof(float),
        reinterpret_cast<void*>(3 * sizeof(float))
    );

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        4,
        GL_FLOAT,
        GL_FALSE,
        9 * sizeof(float),
        reinterpret_cast<void*>(5 * sizeof(float))
    );

    glBindVertexArray(0);
}

void Window::loadShaders() {
    ZoneScoped;
    TracyGpuZone("Load Shaders");

    {
        ZoneScopedN("Quad Shader");
        _fboQuad = ShaderProgram("FBOQuadShader");
        _fboQuad.addVertexShader(shaders::BaseVert);
        _fboQuad.addFragmentShader(shaders::BaseFrag);
        _fboQuad.createAndLinkProgram();
        _fboQuad.bind();
        glUniform1i(glGetUniformLocation(_fboQuad.id(), "tex"), 0);
    }

    {
        ZoneScopedN("Overlay Shader");
        _overlay = ShaderProgram("OverlayShader");
        _overlay.addVertexShader(shaders::BaseVert);
        _overlay.addFragmentShader(shaders::OverlayFrag);
        _overlay.createAndLinkProgram();
        _overlay.bind();
        glUniform1i(glGetUniformLocation(_overlay.id(), "tex"), 0);
    }

    if (_useFXAA) {
        ZoneScopedN("FXAA shader");

        _fxaa = FXAAShader();
        _fxaa->shader = ShaderProgram("FXAAShader");
        _fxaa->shader.addVertexShader(shaders::FXAAVert);
        _fxaa->shader.addFragmentShader(shaders::FXAAFrag);
        _fxaa->shader.createAndLinkProgram();
        _fxaa->shader.bind();

        const int id = _fxaa->shader.id();
        _fxaa->sizeX = glGetUniformLocation(id, "rt_w");
        const ivec2 framebufferSize = framebufferResolution();
        glUniform1f(_fxaa->sizeX, static_cast<float>(framebufferSize.x));

        _fxaa->sizeY = glGetUniformLocation(id, "rt_h");
        glUniform1f(_fxaa->sizeY, static_cast<float>(framebufferSize.y));

        glUniform1f(glGetUniformLocation(id, "FXAA_SUBPIX_TRIM"), 1.f / 4.f);
        glUniform1f(glGetUniformLocation(id, "FXAA_SUBPIX_OFFSET"), 1.f / 2.f);
        glUniform1i(glGetUniformLocation(id, "tex"), 0);
    }


    if (_stereoMode > StereoMode::Active && _stereoMode < StereoMode::SideBySide) {
        ZoneScopedN("Stereo shader");
        // reload shader program if it exists
        _stereo.deleteProgram();

        const std::string_view stereoVertShader = shaders::BaseVert;
        const std::string_view stereoFragShader = [](sgct::Window::StereoMode mode) {
            using SM = StereoMode;
            switch (mode) {
                case SM::AnaglyphRedCyan: return shaders::AnaglyphRedCyanFrag;
                case SM::AnaglyphAmberBlue: return shaders::AnaglyphAmberBlueFrag;
                case SM::AnaglyphRedCyanWimmer: return shaders::AnaglyphRedCyanWimmerFrag;
                case SM::Checkerboard: return shaders::CheckerBoardFrag;
                case SM::CheckerboardInverted: return shaders::CheckerBoardInvertedFrag;
                case SM::VerticalInterlaced: return shaders::VerticalInterlacedFrag;
                case SM::VerticalInterlacedInverted:
                    return shaders::VerticalInterlacedInvertedFrag;
                case SM::Dummy: return shaders::DummyStereoFrag;
                default: throw std::logic_error("Unhandled case label");
            }
        }(_stereoMode);

        _stereo = ShaderProgram("StereoShader");
        _stereo.addVertexShader(stereoVertShader);
        _stereo.addFragmentShader(stereoFragShader);
        _stereo.createAndLinkProgram();
        _stereo.bind();
        glUniform1i(glGetUniformLocation(_stereo.id(), "leftTex"), 0);
        glUniform1i(glGetUniformLocation(_stereo.id(), "rightTex"), 1);
    }

    ShaderProgram::unbind();
}

unsigned int Window::frameBufferTextureEye(Eye eye) const {
    switch (eye) {
        case Eye::MonoOrLeft: return _frameBufferTextures.leftEye;
        case Eye::Right:      return _frameBufferTextures.rightEye;
        default:              throw std::logic_error("Missing case label");
    }
}

} // namespace sgct
