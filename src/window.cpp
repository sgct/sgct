/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/window.h>

#include <sgct/callbackdata.h>
#include <sgct/clustermanager.h>
#include <sgct/config.h>
#include <sgct/engine.h>
#include <sgct/error.h>
#include <sgct/format.h>
#include <sgct/log.h>
#include <sgct/math.h>
#include <sgct/networkmanager.h>
#include <sgct/node.h>
#include <sgct/openxr.h>
#include <sgct/profiling.h>
#include <sgct/projection/nonlinearprojection.h>
#include <sgct/statisticsrenderer.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <format>
#include <functional>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <utility>

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
#include <SpoutLibrary/SpoutLibrary.h>
#endif // SGCT_HAS_SPOUT

#ifdef WIN32
#include <Windows.h>
#endif // WIN32

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define Err(code, msg) Error(Error::Component::Window, code, msg)

namespace {
    void check(VkResult result, const std::string& what) {
        if (result != VK_SUCCESS) {
            throw sgct::Error(
                sgct::Error::Component::Window, 4500,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    }

    // The FXAA shader of the OpenGL renderer. The texture coordinates of the neighbors
    // are computed here instead of in the vertex shader. The other attachments of the
    // intermediate target are copied unchanged
    constexpr std::string_view FxaaFrag = R"(
  layout (location = 0) in vec2 in_texCoords;
  layout (location = 0) out vec4 out_color;
#if HAS_NORMALS
  layout (location = 1) out vec4 out_normal;
#endif
#if HAS_POSITIONS
  layout (location = 2) out vec4 out_position;
#endif

  layout (set = 0, binding = 0) uniform sampler2D tex;
#if HAS_NORMALS
  layout (set = 0, binding = 1) uniform sampler2D normalTex;
#endif
#if HAS_POSITIONS
  layout (set = 0, binding = 2) uniform sampler2D positionTex;
#endif
#if HAS_DEPTH
  layout (set = 0, binding = 3) uniform sampler2D depthTex;
#endif

  layout (push_constant) uniform Constants {
    layout (offset = 64) float rt_w;
    float rt_h;
    // fxaaSubPixTrim: Controls removal of sub-pixel aliasing
    //   1/2 - low removal
    //   1/3 - medium removal
    //   1/4 - default removal
    //   1/8 - high removal
    //     0 - complete removal
    float fxaaSubPixTrim;
    float fxaaSubpixOffset;
  } pc;

  // FxaaEdgeThreshold: The minimum amount of local contrast required to apply algorithm
  //   1/3 - too little
  //   1/4 - low quality
  //   1/8 - high quality
  //   1/16 - overkill
  const float FxaaEdgeThreshold = 1.0 / 8.0;

  // FxaaEdgeThresholdMin: Trims the algorithm from processing dark.
  //   1/32 - visible limit
  //   1/16 - high quality
  //   1/12 - upper limit (start of visible unfiltered edges)
  const float FxaaEdgeThresholdMin = 1.0 / 16.0;

  const float FxaaSpanMax = 8.0;

  vec4 fxaa() {
    vec2 texCoords = in_texCoords;
    vec2 texel = vec2(1.0 / pc.rt_w, 1.0 / pc.rt_h);
    vec2 offsetNW = texCoords + pc.fxaaSubpixOffset * vec2(-texel.x, -texel.y);
    vec2 offsetNE = texCoords + pc.fxaaSubpixOffset * vec2( texel.x, -texel.y);
    vec2 offsetSW = texCoords + pc.fxaaSubpixOffset * vec2(-texel.x,  texel.y);
    vec2 offsetSE = texCoords + pc.fxaaSubpixOffset * vec2( texel.x,  texel.y);

    vec3 rgbNW = textureLod(tex, offsetNW, 0.0).rgb;
    vec3 rgbNE = textureLod(tex, offsetNE, 0.0).rgb;
    vec3 rgbSW = textureLod(tex, offsetSW, 0.0).rgb;
    vec3 rgbSE = textureLod(tex, offsetSE, 0.0).rgb;
    vec3 rgbM  = textureLod(tex, texCoords, 0.0).rgb;

    const vec3 Luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, Luma);
    float lumaNE = dot(rgbNE, Luma);
    float lumaSW = dot(rgbSW, Luma);
    float lumaSE = dot(rgbSE, Luma);
    float lumaM  = dot( rgbM, Luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    float range = lumaMax - lumaMin;
    // local contrast check, for not processing homogeneous areas
    if (range < max(FxaaEdgeThresholdMin, lumaMax * FxaaEdgeThreshold)) {
      return vec4(rgbM, 1.0);
    }

    vec2 dir = vec2(
      -((lumaNW + lumaNE) - (lumaSW + lumaSE)),
      (lumaNW + lumaSW) - (lumaNE + lumaSE)
    );

    const float FxaaReduceMin = 1.0 / 128.0;
    float dirReduce = max(
      (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * pc.fxaaSubPixTrim),
      FxaaReduceMin
    );

    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(
      vec2(FxaaSpanMax,  FxaaSpanMax),
      max(vec2(-FxaaSpanMax, -FxaaSpanMax), dir * rcpDirMin)
    ) / vec2(pc.rt_w, pc.rt_h);

    vec3 rgbA = 0.5 * (
      textureLod(tex, texCoords + dir * (1.0 / 3.0 - 0.5), 0.0).rgb +
      textureLod(tex, texCoords + dir * (2.0 / 3.0 - 0.5), 0.0).rgb
    );
    vec3 rgbB = rgbA * 0.5 + (1.0 / 4.0) * (
      textureLod(tex, texCoords + dir * (0.0 / 3.0 - 0.5), 0.0).rgb +
      textureLod(tex, texCoords + dir * (3.0 / 3.0 - 0.5), 0.0).rgb
    );
    float lumaB = dot(rgbB, Luma);

    if ((lumaB < lumaMin) || (lumaB > lumaMax))  {
      return vec4(rgbA, 1.0);
    }
    else {
      return vec4(rgbB, 1.0);
    }
  }

  void main() {
    out_color = fxaa();

    ivec2 texelCoords = ivec2(gl_FragCoord.xy);
#if HAS_NORMALS
    out_normal = texelFetch(normalTex, texelCoords, 0);
#endif
#if HAS_POSITIONS
    out_position = texelFetch(positionTex, texelCoords, 0);
#endif
#if HAS_DEPTH
    gl_FragDepth = texelFetch(depthTex, texelCoords, 0).r;
#endif
  }
)";

    sgct::Window::StereoMode convert(sgct::config::Window::StereoMode mode) {
        using StereoMode = sgct::config::Window::StereoMode;
        switch (mode) {
            case StereoMode::NoStereo:
                return sgct::Window::StereoMode::NoStereo;
            case StereoMode::Active:
                return sgct::Window::StereoMode::Active;
            case StereoMode::AnaglyphRedCyan:
                return sgct::Window::StereoMode::AnaglyphRedCyan;
            case StereoMode::AnaglyphAmberBlue:
                return sgct::Window::StereoMode::AnaglyphAmberBlue;
            case StereoMode::AnaglyphRedCyanWimmer:
                return sgct::Window::StereoMode::AnaglyphRedCyanWimmer;
            case StereoMode::Checkerboard:
                return sgct::Window::StereoMode::Checkerboard;
            case StereoMode::CheckerboardInverted:
                return sgct::Window::StereoMode::CheckerboardInverted;
            case StereoMode::VerticalInterlaced:
                return sgct::Window::StereoMode::VerticalInterlaced;
            case StereoMode::VerticalInterlacedInverted:
                return sgct::Window::StereoMode::VerticalInterlacedInverted;
            case StereoMode::Dummy:
                return sgct::Window::StereoMode::Dummy;
            case StereoMode::SideBySide:
                return sgct::Window::StereoMode::SideBySide;
            case StereoMode::SideBySideInverted:
                return sgct::Window::StereoMode::SideBySideInverted;
            case StereoMode::TopBottom:
                return sgct::Window::StereoMode::TopBottom;
            case StereoMode::TopBottomInverted:
                return sgct::Window::StereoMode::TopBottomInverted;
            default:
                throw std::logic_error("Unhandled case label");
        }
    }

    sgct::ivec2 bakeSize(std::optional<sgct::ivec2> size, int monitorIndex) {
        if (size.has_value()) {
            return *size;
        }

        constexpr float ScaleFactor = 2.f / 3.f;
        constexpr float AspectRatio = 1920.f / 1080.f;

        int count;
        GLFWmonitor** monitors = glfwGetMonitors(&count);
        monitorIndex = std::clamp(monitorIndex, 0, count - 1);

        const GLFWvidmode* mode = glfwGetVideoMode(monitors[monitorIndex]);
        return sgct::ivec2(
            static_cast<int>(mode->width * ScaleFactor),
            static_cast<int>((mode->width / AspectRatio) * ScaleFactor)
        );
    }

    sgct::mat4 convert(const glm::mat4& matrix) {
        sgct::mat4 result;
        std::memcpy(result.values.data(), glm::value_ptr(matrix), sizeof(float[16]));
        return result;
    }

    sgct::mat4 viewMatrix(const sgct::Window& window, const sgct::Viewport& viewport,
                          sgct::FrustumMode frustum)
    {
#ifdef SGCT_HAS_OPENXR
        if (window.isOpenXREnabled() && sgct::openxr::isHMDActive()) {
            return convert(
                sgct::openxr::eyeToHeadTransform(frustum) * sgct::openxr::poseMatrix()
            );
        }
#endif // SGCT_HAS_OPENXR
        return viewport.projection(frustum).viewMatrix();
    }

    sgct::mat4 projectionMatrix(const sgct::Window& window,
                                const sgct::Viewport& viewport, sgct::FrustumMode frustum)
    {
#ifdef SGCT_HAS_OPENXR
        if (window.isOpenXREnabled() && sgct::openxr::isHMDActive()) {
            return convert(sgct::openxr::eyeProjectionMatrix(
                frustum,
                sgct::Engine::instance().nearClipPlane(),
                sgct::Engine::instance().farClipPlane()
            ));
        }
#endif // SGCT_HAS_OPENXR
        return viewport.projection(frustum).projectionMatrix();
    }

    sgct::mat4 viewProjectionMatrix(const sgct::Window& window,
                                    const sgct::Viewport& viewport,
                                    sgct::FrustumMode frustum)
    {
#ifdef SGCT_HAS_OPENXR
        if (window.isOpenXREnabled() && sgct::openxr::isHMDActive()) {
            return convert(sgct::openxr::currentViewProjectionMatrix(frustum));
        }
#endif // SGCT_HAS_OPENXR
        return viewport.projection(frustum).viewProjectionMatrix();
    }

    sgct::ivec2 renderBufferSize(const sgct::Window& window, sgct::FrustumMode) {
        return window.framebufferResolution();
    }

    sgct::vec2 viewportRenderSize(const sgct::Window& window,
                                  const sgct::Viewport& viewport)
    {
        const sgct::ivec2 res = window.framebufferResolution();
        return sgct::vec2{
            res.x * viewport.size().x,
            res.y * viewport.size().y
        };
    }

#ifdef SGCT_HAS_OPENXR
    sgct::ivec2 openXRFramebufferResolution(const sgct::Window& window) {
        if (!window.isOpenXREnabled() || !sgct::openxr::isHMDActive()) {
            return window.framebufferResolution();
        }

        const sgct::ivec2 left = sgct::openxr::eyeResolution(
            sgct::FrustumMode::StereoLeft
        );
        const sgct::ivec2 right = sgct::openxr::eyeResolution(
            sgct::FrustumMode::StereoRight
        );
        const int eyeWidth = std::max(left.x, right.x);
        const int eyeHeight = std::max(left.y, right.y);
        if (eyeWidth == 0 || eyeHeight == 0) {
            return window.framebufferResolution();
        }

        switch (window.stereoMode()) {
            case sgct::Window::StereoMode::SideBySide:
            case sgct::Window::StereoMode::SideBySideInverted:
                return sgct::ivec2{ eyeWidth * 2, eyeHeight };
            case sgct::Window::StereoMode::TopBottom:
            case sgct::Window::StereoMode::TopBottomInverted:
                return sgct::ivec2{ eyeWidth, eyeHeight * 2 };
            case sgct::Window::StereoMode::NoStereo:
                return left;
            default:
                return sgct::ivec2{ eyeWidth, eyeHeight };
        }
    }
#endif // SGCT_HAS_OPENXR

} // namespace

namespace sgct {

bool Window::_useSwapGroups = false;
bool Window::_isBarrierActive = false;
GLFWwindow* Window::_sharedHandle = nullptr;

void Window::initNvidiaSwapGroups() {
    // The swap groups and barriers of WGL_NV_swap_group correspond to the present barrier
    // of VK_NV_present_barrier, which the swapchains join once the barrier is active
    if (vulkan::Context::instance().supportsPresentBarrier()) {
        Log::Info("Joining the Nvidia present barrier");
        _useSwapGroups = true;
        _isBarrierActive = true;
    }
    else {
        Log::Warning("The Nvidia present barrier (VK_NV_present_barrier) is not supported");
        _useSwapGroups = false;
        _isBarrierActive = false;
    }
}

void Window::resetSwapGroupFrameNumber() {
    // The present barrier does not have a frame counter
}

void Window::setBarrier(bool state) {
    // The windows recreate their swapchains with the new state in their next draw
    if (_useSwapGroups && state != _isBarrierActive) {
        Log::Info(std::format("{} Nvidia present barrier", state ? "Enabling" : "Disabling"));
        _isBarrierActive = state;
    }
}

bool Window::isBarrierActive() {
    return _isBarrierActive;
}

bool Window::isUsingSwapGroups() {
    return _useSwapGroups;
}

unsigned int Window::swapGroupFrameNumber() {
    return 0;
}

config::Window createScalableConfiguration(
                                [[maybe_unused]] const config::Window::Scalable& scalable,
                                                             const config::Window& window)
{
    // Take the values from the already provided window configuration
    config::Window res = window;

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

    res.isDecorated = res.isDecorated.value_or(false);
    res.draw2D = res.draw2D.value_or(false);

    res.pos = res.pos.value_or(ivec2{ 0, 0 });
    res.size =  ivec2 { static_cast<int>(mesh.Xres), static_cast<int>(mesh.Yres) };
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
        proj.offset = vec3 {
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

        res.resolution = ivec2 {
            scalable.orthographicResolution.value_or(4096),
            scalable.orthographicResolution.value_or(4096)
        };
        config::FisheyeProjection proj = {
            .quality = scalable.orthographicQuality.value_or(2048),
            .background = vec4 { 0.f, 0.f, 0.f, 1.f }
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
    , _bufferBitDepth(window.bufferBitDepth)
    , _isDecorated(window.isDecorated.value_or(true))
    , _isResizable(window.isResizable.value_or(true))
    , _blitWindowId(window.blitWindowId.value_or(-1))
    , _monitorIndex(window.monitor.value_or(0))
    , _mirrorX(window.mirrorX.value_or(false))
    , _mirrorY(window.mirrorY.value_or(false))
    , _noError(window.noError.value_or(false))
    , _useOpenXR(window.openxr.has_value() && window.openxr->enabled)
    , _isVisible(!window.isHidden.value_or(false))
    , _stereoMode(convert(window.stereo.value_or(config::Window::StereoMode::NoStereo)))
    , _windowPos(window.pos)
    , _windowSize(bakeSize(window.size, _monitorIndex))
    , _framebufferRes(bakeSize(window.size, _monitorIndex))
    , _aspectRatio(static_cast<float>(_windowSize.x) / static_cast<float>(_windowSize.y))
#ifdef SGCT_HAS_SPOUT
    , _spout {
        .enabled = window.spout.has_value() && window.spout->enabled,
        .name = window.spout ? window.spout->name.value_or("") : ""
    }
#endif // SGCT_HAS_SPOUT
{
    ZoneScoped;

    if (window.resolution) {
        ZoneScopedN("Resolution");

        setFramebufferResolution(*window.resolution);
        setFixResolution(true);
    }

#ifdef SGCT_HAS_NDI
    if (window.ndi && window.ndi->enabled) {
        _ndi.name = window.ndi->name.value_or(_ndi.name);
        _ndi.groups = window.ndi->groups.value_or(_ndi.groups);

        NDIlib_send_create_t createDesc;
        if (!_ndi.name.empty()) {
            createDesc.p_ndi_name = _ndi.name.c_str();
        }
        if (!_ndi.groups.empty()) {
            createDesc.p_groups = _ndi.groups.c_str();
        }

        _ndi.handle = NDIlib_send_create(&createDesc);
        if (!_ndi.handle) {
            Log::Error("Error creating NDI sender");
        }

        // The resolution, FourCC, and stride of the frames depend on the swapchain,
        // which is created in initialize, so they are set in createStreamBuffers
        _ndi.videoFrame.frame_rate_N = 60000; // 60 fps
        _ndi.videoFrame.frame_rate_D = 1000;  // 60 fps
        _ndi.videoFrame.frame_format_type = NDIlib_frame_format_type_progressive;
        _ndi.videoFrame.timecode = 0;
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

Window::~Window() = default;

void Window::openWindow(bool isLastWindow) {
    ZoneScoped;

    {
        ZoneScopedN("Set GLFW settings");
        // SGCT renders through Vulkan, so no OpenGL context is created for the window.
        // The GLFW window is only used to obtain a native handle for the Vulkan surface
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_DECORATED, _isDecorated ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_RESIZABLE, _isResizable ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_AUTO_ICONIFY, _shouldAutoiconify ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_FLOATING, _isFloating ? GLFW_TRUE : GLFW_FALSE);
        if (!_isVisible) {
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        }
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
        _windowSize = ivec2{ currentMode->width, currentMode->height };

        glfwWindowHint(GLFW_RED_BITS, currentMode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, currentMode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, currentMode->blueBits);
    }

    {
        ZoneScopedN("glfwCreateWindow");
        // No OpenGL context is created (GLFW_NO_API), so there is no shared context to pass
        _windowHandle = glfwCreateWindow(
            _windowSize.x,
            _windowSize.y,
            "SGCT",
            mon,
            nullptr
        );
        glfwSetWindowUserPointer(_windowHandle, this);
        if (_windowHandle == nullptr) {
            throw Err(8000, "Error opening GLFW window");
        }
    }

    glfwSetWindowIconifyCallback(
        _windowHandle,
        [](GLFWwindow* window, int iconified) {
            Window* win = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            win->_isIconified = iconified != 0;
        }
    );

    _sharedHandle = _windowHandle;

    glm::ivec2 bufferSize = glm::ivec2(0);
    {
        ZoneScopedN("glfwGetFramebufferSize");
        glfwGetFramebufferSize(_windowHandle, &bufferSize[0], &bufferSize[1]);
    }

    _scale.x = static_cast<float>(bufferSize.x) / static_cast<float>(_windowSize.x);
    _scale.y = static_cast<float>(bufferSize.y) / static_cast<float>(_windowSize.y);
    if (!_useFixResolution) {
        _framebufferRes.x = bufferSize.x;
        _framebufferRes.y = bufferSize.y;
    }

    // The swap interval is applied to the last window only, as waiting for the vertical
    // sync in every window would divide the frame rate by the number of windows. It is
    // implemented through the present mode of the swapchain
    _isLastWindow = isLastWindow;
    if (_isLastWindow && Engine::instance().settings().swapInterval > 1) {
        // Skipping vertical syncs cannot be expressed through the present modes
        Log::Warning(std::format(
            "Swap interval {} is not supported, waiting for every vertical sync instead",
            Engine::instance().settings().swapInterval
        ));
    }

    // if client, disable mouse pointer
    if (_hideMouseCursor || !Engine::instance().isMaster()) {
        glfwSetInputMode(_windowHandle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }

    _hasFocus = glfwGetWindowAttrib(_windowHandle, GLFW_FOCUSED) == GLFW_TRUE;

    // Whether active stereo has a right eye target is only known once the swapchain has
    // been created, so it has captures for both eyes
    if (!needsRightEyeTarget() && _stereoMode != StereoMode::Active) {
        _screenCaptureLeftOrMono = std::make_unique<ScreenCapture>(
            *this,
            ScreenCapture::EyeIndex::Mono,
            _hasAlpha
        );
    }
    else {
        _screenCaptureLeftOrMono = std::make_unique<ScreenCapture>(
            *this,
            ScreenCapture::EyeIndex::StereoLeft,
            _hasAlpha
        );
        _screenCaptureRight = std::make_unique<ScreenCapture>(
            *this,
            ScreenCapture::EyeIndex::StereoRight,
            _hasAlpha
        );
    }

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
                // `glfwPollEvents`, we want to make sure the actual resizing is deferred
                // to the end of the frame. This can happen if some other library pulls
                // events from the operating system for example by calling PeekMessage
                // (Windows). If we were to set the actual resolution directly, we may
                // render half a frame with resolution A and the other half with
                // resolution b, which is undefined behavior. _pendingWindowRes is checked
                // in Window::updateResolution, which is called from Engine's render loop
                // after glfwPollEvents
                win->_pendingWindowSize = ivec2 { width, height };
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
    if (_spout.handle) {
        _spout.handle->ReleaseSender();
        _spout.handle->Release();
    }
#endif // SGCT_HAS_SPOUT

#ifdef SGCT_HAS_NDI
    if (_ndi.handle) {
        // One of the two buffers might be still in flight so we have to flush the
        // pipeline before we can destroy the video or we might risk NDI accessing dead
        // memory and crashing
        NDIlib_send_send_video_async_v2(_ndi.handle, nullptr);
        NDIlib_send_destroy(_ndi.handle);
    }
#endif // SGCT_HAS_NDI

    Log::Info(std::format("Deleting screen capture data for window {}", _id));
    _screenCaptureLeftOrMono = nullptr;
    _screenCaptureRight = nullptr;

    // Release the Vulkan resources. Wait for the GPU to finish with the swapchain images,
    // the framebuffer's depth image, and the offscreen images, then destroy the
    // framebuffer before the swapchain since the framebuffers reference the swapchain
    // image views
    vulkan::Context& ctx = vulkan::Context::instance();
    if (_swapchain) {
        check(vkDeviceWaitIdle(ctx.device()), "vkDeviceWaitIdle");
    }
    _composite = nullptr;
    _overlayPass = nullptr;
    _fxaaPass = nullptr;
    _intermediate = nullptr;
    _offscreenRight = nullptr;
    _offscreen = nullptr;
    if (_linearSampler != VK_NULL_HANDLE) {
        vkDestroySampler(ctx.device(), _linearSampler, nullptr);
        _linearSampler = VK_NULL_HANDLE;
    }
    _framebuffer = nullptr;
    _swapchain = nullptr;
    freeCommandBuffers();

    // Destroys the viewports' correction mesh buffers, which the GPU is done with
    _viewports.clear();

    glfwSetWindowSizeCallback(_windowHandle, nullptr);
    glfwSetFramebufferSizeCallback(_windowHandle, nullptr);
    glfwSetWindowFocusCallback(_windowHandle, nullptr);
    glfwSetWindowIconifyCallback(_windowHandle, nullptr);
}

void Window::initialize() {
    ZoneScoped;

    recreateSwapchain();

    // The cube maps of the non-linear projections have the format of the render target,
    // so that the pipelines of the draw callback can render into them
    for (const std::unique_ptr<Viewport>& vp : _viewports) {
        vp->initialize(
            viewportRenderSize(*this, *vp),
            _stereoMode != StereoMode::NoStereo,
            _sceneFormat
        );
    }

    // The content of the other window is used in the same frame, so it has to be drawn
    // before this window
    if (_blitWindowId >= 0) {
        const std::vector<std::unique_ptr<Window>>& windows =
            ClusterManager::instance().thisNode().windows();
        auto source = std::find_if(
            windows.cbegin(),
            windows.cend(),
            [id = _blitWindowId](const std::unique_ptr<Window>& w) { return w->id() == id; }
        );
        auto self = std::find_if(
            windows.cbegin(),
            windows.cend(),
            [this](const std::unique_ptr<Window>& w) { return w.get() == this; }
        );
        if (source > self) {
            Log::Warning(std::format(
                "Window {}: The content of window {} can only be copied if that window "
                "is specified first", _id, _blitWindowId
            ));
        }
    }

    // Loads the overlay and mask textures and the correction meshes; the warp meshes map
    // the offscreen image onto the window in the composite pass
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

#ifdef SGCT_HAS_OPENXR
    if (isOpenXREnabled() && sgct::openxr::isHMDActive()) {
        const ivec2 resolution = openXRFramebufferResolution(*this);
        if (resolution != _framebufferRes) {
            _framebufferRes = resolution;
            _windowResChanged = true;
            _useFixResolution = true;
            Log::Info(std::format(
                "OpenXR framebuffer resolution changed to {}x{} for window {}",
                _framebufferRes.x, _framebufferRes.y, _id
            ));
            _pendingFramebufferRes = std::nullopt;
        }
    }
#endif // SGCT_HAS_OPENXR

    if (_pendingWindowSize) {
        _windowSize = *_pendingWindowSize;
        _windowResChanged = true;
        float ratio =
            static_cast<float>(_windowSize.x) / static_cast<float>(_windowSize.y);

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
        glfwSetWindowSize(_windowHandle, _windowSize.x, _windowSize.y);

        Log::Debug(std::format(
            "Resolution changed to {}x{} in window {}", _windowSize.x, _windowSize.y, _id
        ));
        _pendingWindowSize = std::nullopt;
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

    if (!isVisible() || !isWindowResized()) {
        return;
    }

    recreateSwapchain();

    // Adapt the non-linear projections to the new aspect ratio of their viewports
    for (const std::unique_ptr<Viewport>& vp : _viewports) {
        if (vp->hasSubViewports()) {
            vp->nonLinearProjection()->update(viewportRenderSize(*this, *vp));
        }
    }
}

void Window::draw() {
    ZoneScopedN("Render window");

    _isRecording = false;
    _frameRecorded = false;

    vulkan::Context& ctx = vulkan::Context::instance();

    if (!shouldRenderFrame()) [[unlikely]] {
        // Wait for the frames that are still in flight. Per-frame resources, such as the
        // segments of a vulkan::UniformRing, assume that every window finished the frames
        // it submitted a while ago, which this window would otherwise not guarantee while
        // it does not acquire new images
        std::array<VkFence, vulkan::Swapchain::MaxFramesInFlight> fences;
        for (uint32_t i = 0; i < vulkan::Swapchain::MaxFramesInFlight; i++) {
            fences[i] = _swapchain->inFlightFence(i);
        }
        check(
            vkWaitForFences(
                ctx.device(),
                static_cast<uint32_t>(fences.size()),
                fences.data(),
                VK_TRUE,
                std::numeric_limits<uint64_t>::max()
            ),
            "vkWaitForFences"
        );
        return;
    }

    _isOffscreenRendered = false;

    // The stereo mode and the swap barrier can change at runtime, which needs a swapchain
    // with different layers or present barrier
    if (_swapchainStereo != (_stereoMode == StereoMode::Active) ||
        _swapchainPresentBarrier != (_useSwapGroups && _isBarrierActive))
    {
        recreateSwapchain();
    }

    // The framebuffer resolution can change without the swapchain being recreated (for
    // example when the framebuffer size callback arrives after the window was resized),
    // and the stereo mode can change at runtime
    const ivec2 res = framebufferResolution();
    if (static_cast<int>(_offscreen->extent().width) != std::max(res.x, 1) ||
        static_cast<int>(_offscreen->extent().height) != std::max(res.y, 1) ||
        needsRightEyeTarget() != (_offscreenRight != nullptr))
    {
        recreateOffscreen();
    }

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    try {
        // Waits for the previous frame that used this in-flight slot to finish, then
        // acquires a swapchain image. Throws if the swapchain is out of date
        _imageIndex = _swapchain->acquireImage(_frameIndex);

        // The fence was just waited on (so it is signaled); reset it so the submit in
        // swapBuffers can signal it again
        VkFence inFlightFence = _swapchain->inFlightFence(_frameIndex);
        check(vkResetFences(ctx.device(), 1, &inFlightFence), "vkResetFences");

        cmd = _commandBuffers[_frameIndex];
        check(vkResetCommandBuffer(cmd, 0), "vkResetCommandBuffer");
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        check(vkBeginCommandBuffer(cmd, &beginInfo), "vkBeginCommandBuffer");
        _isRecording = true;

        if (_timestampPool != VK_NULL_HANDLE) {
            // The previous frame in this slot has finished, as its fence was waited on
            // while acquiring the image
            const uint32_t first = 2 * _frameIndex;
            if (_hasTimestamps[_frameIndex]) {
                std::array<uint64_t, 2> ts = {};
                const VkResult queryResult = vkGetQueryPoolResults(
                    ctx.device(),
                    _timestampPool,
                    first,
                    2,
                    sizeof(ts),
                    ts.data(),
                    sizeof(uint64_t),
                    VK_QUERY_RESULT_64_BIT
                );
                if (queryResult == VK_SUCCESS && ts[1] >= ts[0]) {
                    const double period = ctx.limits().timestampPeriod;
                    _gpuFrameTime = static_cast<double>(ts[1] - ts[0]) * period * 1e-9;
                }
                _hasTimestamps[_frameIndex] = false;
            }
            vkCmdResetQueryPool(cmd, _timestampPool, first, 2);
            vkCmdWriteTimestamp(
                cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, _timestampPool, first
            );
        }

        // The cube maps of all eyes are rendered first, as they cannot be rendered while
        // the render pass of an offscreen target is active
        renderCubemaps(cmd);

#ifdef TRACY_ENABLE
        TracyVkZone(_tracyContexts[_frameIndex], cmd, "Scene");
#endif // TRACY_ENABLE

        if (_stereoMode == StereoMode::NoStereo) {
            recordTarget(cmd, *_offscreen, { FrustumMode::Mono });
        }
        else if (_stereoMode >= StereoMode::SideBySide) {
            // Both eyes are rendered into separate halves of the same target, so they
            // share a single render pass (beginning a second one would clear the left eye)
            recordTarget(
                cmd,
                *_offscreen,
                { FrustumMode::StereoLeft, FrustumMode::StereoRight }
            );
        }
        else {
            recordTarget(cmd, *_offscreen, { FrustumMode::StereoLeft });

            // Active stereo only has a right eye target if the swapchain supports
            // stereoscopic presentation; otherwise only its left eye is rendered
            if (_offscreenRight) {
                recordTarget(cmd, *_offscreenRight, { FrustumMode::StereoRight });
            }
        }
        _isOffscreenRendered = true;
    }
    catch (const Error&) {
        // A real Vulkan error (thrown by `check`); let it propagate
        throw;
    }
    catch (const vulkan::SwapchainOutOfDate&) {
        // The swapchain is out of date (thrown by `acquireImage`); recreate it and drop
        // this frame. The command buffer may still be in the recording state, so return
        // it to the initial state before the recreation frees it
        if (cmd != VK_NULL_HANDLE) {
            vkResetCommandBuffer(cmd, 0);
        }
        _isRecording = false;
        recreateSwapchain();
    }
}

void Window::recordTarget(VkCommandBuffer cmd, const vulkan::OffscreenBuffer& target,
                          std::initializer_list<FrustumMode> frustums)
{
    // The whole target is cleared, which also covers the areas that no viewport renders
    // into (the OpenGL path cleared each viewport with a scissor test instead)
    if (_intermediate) {
        // The 2D pass is drawn after the FXAA, so that the text is not filtered
        _intermediate->begin(cmd);
        for (FrustumMode frustum : frustums) {
            recordScene(cmd, *_intermediate, frustum);
        }
        vkCmdEndRenderPass(cmd);

        target.begin(cmd);
        recordFxaa(cmd, target);
    }
    else {
        target.begin(cmd);
        for (FrustumMode frustum : frustums) {
            recordScene(cmd, target, frustum);
        }
    }
    for (FrustumMode frustum : frustums) {
        record2D(cmd, target, frustum);
    }
    vkCmdEndRenderPass(cmd);
}

void Window::recordFxaa(VkCommandBuffer cmd, const vulkan::OffscreenBuffer& target) {
    ZoneScopedN("FXAA");

    // The intermediate target has the same size and orientation as the target, so the
    // viewport is not flipped
    const VkExtent2D extent = target.extent();
    VkViewport viewport = {};
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.maxDepth = 1.f;
    VkRect2D scissor = {};
    scissor.extent = extent;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    struct {
        std::array<vec4, 4> corners;
        float width;
        float height;
        float subPixTrim;
        float subPixOffset;
    } constants = {
        {
            vec4{ -1.f, -1.f, 0.f, 0.f },
            vec4{ -1.f,  1.f, 0.f, 1.f },
            vec4{  1.f, -1.f, 1.f, 0.f },
            vec4{  1.f,  1.f, 1.f, 1.f }
        },
        static_cast<float>(extent.width),
        static_cast<float>(extent.height),
        // The values that the OpenGL renderer used
        1.f / 4.f,
        1.f / 2.f
    };

    auto image = [this](VkImageView view) {
        VkDescriptorImageInfo info = {};
        info.sampler = _linearSampler;
        info.imageView = view;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        return info;
    };
    const std::array<VkDescriptorImageInfo, 4> images = {
        image(_intermediate->colorImageView()),
        image(_intermediate->normalsImageView()),
        image(_intermediate->positionsImageView()),
        image(_intermediate->depthImageView())
    };

    _fxaaPass->bind(cmd, &constants);
    _fxaaPass->setTextures(cmd, images);
    _fxaaPass->drawQuad(cmd);
}

const Window* Window::blitSource() const {
    if (_blitWindowId < 0) {
        return nullptr;
    }

    // Only a window that is drawn before this one (in the same frame) can be copied
    const std::vector<std::unique_ptr<Window>>& windows =
        ClusterManager::instance().thisNode().windows();
    for (const std::unique_ptr<Window>& w : windows) {
        if (w.get() == this) {
            return nullptr;
        }
        if (w->id() == _blitWindowId) {
            return w->_isOffscreenRendered ? w.get() : nullptr;
        }
    }
    return nullptr;
}

void Window::renderCubemaps(VkCommandBuffer cmd) {
    ZoneScoped;

    if (!_hasCallDraw3DFunction || !Engine::instance().drawFunction()) {
        return;
    }

    auto render = [this, cmd](const Viewport& vp, FrustumMode frustum) {
        if (vp.isTracked()) {
            const_cast<Viewport&>(vp).calculateFrustum(
                frustum,
                Engine::instance().nearClipPlane(),
                Engine::instance().farClipPlane()
            );
        }
        // The cube map faces are copied for the Spout/NDI streaming once the frame's
        // in-flight fence is signaled
        vp.nonLinearProjection()->renderCubemap(
            cmd, frustum, _frameIndex, _imageIndex,
            _swapchain->inFlightFence(_frameIndex)
        );
    };

    for (const std::unique_ptr<Viewport>& vp : _viewports) {
        if (!vp->isEnabled() || !vp->hasSubViewports()) {
            continue;
        }

        if (_stereoMode == StereoMode::NoStereo) {
            // For mono viewports the frustum mode can be selected by the user or config
            render(*vp, vp->eye());
        }
        else {
            render(*vp, FrustumMode::StereoLeft);
            // Active stereo only renders the left eye (see draw)
            if (_stereoMode != StereoMode::Active) {
                render(*vp, FrustumMode::StereoRight);
            }
        }
    }
}

namespace {
    // The pixel rectangle uses OpenGL's bottom-left origin, whereas Vulkan's framebuffer
    // coordinates start at the top-left. The viewport is flipped (negative height) so
    // that NDC y = +1 maps to the top of the viewport
    std::pair<VkViewport, VkRect2D> viewportArea(const sgct::Viewport& vp,
                                                 sgct::FrustumMode frustum,
                                                 int targetHeight)
    {
        const sgct::ivec4 rect = vp.pixelRect(frustum);
        VkViewport viewport = {};
        viewport.x = static_cast<float>(rect.x);
        viewport.y = static_cast<float>(targetHeight - rect.y);
        viewport.width = static_cast<float>(rect.z);
        viewport.height = -static_cast<float>(rect.w);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        VkRect2D scissor = {};
        scissor.offset = {
            std::max(rect.x, 0),
            std::max(targetHeight - rect.y - rect.w, 0)
        };
        scissor.extent = {
            static_cast<uint32_t>(std::max(rect.z, 0)),
            static_cast<uint32_t>(std::max(rect.w, 0))
        };
        return std::pair(viewport, scissor);
    }
} // namespace

void Window::recordScene(VkCommandBuffer cmd, const vulkan::OffscreenBuffer& target,
                         FrustumMode frustum)
{
    ZoneScoped;

    const int targetHeight = static_cast<int>(target.extent().height);

    // For mono viewports the frustum mode can be selected by the user or config
    auto viewportFrustum = [this, frustum](const Viewport& vp) {
        return _stereoMode == StereoMode::NoStereo ? vp.eye() : frustum;
    };

    const bool hasDraw = _hasCallDraw3DFunction && Engine::instance().drawFunction();
    const Window* source = blitSource();
    if (hasDraw || source) {
        for (const std::unique_ptr<Viewport>& vp : _viewports) {
            if (!vp->isEnabled()) {
                continue;
            }

            const FrustumMode vpFrustum = viewportFrustum(*vp);
            const auto [viewport, scissor] = viewportArea(*vp, vpFrustum, targetHeight);
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            // The content of the other window is drawn first, below the scene
            if (source && !vp->hasSubViewports()) {
                const vulkan::OffscreenBuffer* image = source->offscreenTarget(
                    vpFrustum == FrustumMode::StereoRight ? Eye::Right : Eye::MonoOrLeft
                );
                _overlayPass->draw(cmd, image->colorImageView(), _linearSampler);
            }
            if (!hasDraw) {
                continue;
            }

            // The frustums of the non-linear projections were calculated when their cube
            // maps were rendered
            if (vp->isTracked() && !vp->hasSubViewports()) {
                vp->calculateFrustum(
                    vpFrustum,
                    Engine::instance().nearClipPlane(),
                    Engine::instance().farClipPlane()
                );
            }

            // A non-linear projection maps its cube map onto the viewport instead of
            // calling the draw callback directly
            if (vp->hasSubViewports()) {
                vp->nonLinearProjection()->render(cmd, *vp, vpFrustum, scissor);
                continue;
            }

            ZoneScopedN("[SGCT] Draw");
            const RenderData renderData = {
                *this,
                *vp,
                vpFrustum,
                ClusterManager::instance().sceneTransform(),
                viewMatrix(*this, *vp, vpFrustum),
                projectionMatrix(*this, *vp, vpFrustum),
                viewProjectionMatrix(*this, *vp, vpFrustum) *
                    ClusterManager::instance().sceneTransform(),
                renderBufferSize(*this, vpFrustum),
                cmd,
                _frameIndex,
                _imageIndex,
                Engine::instance().currentFrameNumber(),
                target.extent(),
                viewport,
                scissor
            };
            Engine::instance().drawFunction()(renderData);
        }
    }
}

void Window::record2D(VkCommandBuffer cmd, const vulkan::OffscreenBuffer& target,
                      FrustumMode frustum)
{
    ZoneScoped;

    const int targetHeight = static_cast<int>(target.extent().height);

    // For mono viewports the frustum mode can be selected by the user or config
    auto viewportFrustum = [this, frustum](const Viewport& vp) {
        return _stereoMode == StereoMode::NoStereo ? vp.eye() : frustum;
    };

    // The 2D pass draws the overlays, statistics, and the draw2D callback on top of the
    // scene of all viewports
    for (const std::unique_ptr<Viewport>& vp : _viewports) {
        if (!vp->isEnabled()) {
            continue;
        }

        const FrustumMode vpFrustum = viewportFrustum(*vp);
        const auto [viewport, scissor] = viewportArea(*vp, vpFrustum, targetHeight);
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        if (vp->hasOverlayTexture()) {
            _overlayPass->draw(cmd, *vp->overlayTexture());
        }

        if (Engine::instance().statisticsRenderer()) {
            Engine::instance().statisticsRenderer()->render(*this, *vp);
        }

        if (Engine::instance().draw2DFunction() && _hasCallDraw2DFunction) {
            ZoneScopedN("[SGCT] Draw 2D");
            const RenderData renderData = {
                *this,
                *vp,
                vpFrustum,
                ClusterManager::instance().sceneTransform(),
                viewMatrix(*this, *vp, vpFrustum),
                projectionMatrix(*this, *vp, vpFrustum),
                viewProjectionMatrix(*this, *vp, vpFrustum) *
                    ClusterManager::instance().sceneTransform(),
                renderBufferSize(*this, vpFrustum),
                cmd,
                _frameIndex,
                _imageIndex,
                Engine::instance().currentFrameNumber(),
                target.extent(),
                viewport,
                scissor
            };
            Engine::instance().draw2DFunction()(renderData);
        }
    }
}

#if defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)
bool Window::streamingEnabled() const {
#if defined(SGCT_HAS_SPOUT)
    if (_spout.handle) {
        return true;
    }
#endif // SGCT_HAS_SPOUT
#if defined(SGCT_HAS_NDI)
    if (_ndi.handle) {
        return true;
    }
#endif // SGCT_HAS_NDI
    return false;
}

void Window::createStreamBuffers() {
    if (!streamingEnabled()) {
        return;
    }

    vulkan::Context& ctx = vulkan::Context::instance();
    const VkExtent2D extent = _swapchain->extent();
    const VkFormat format = _swapchain->format();
    const VkDeviceSize size =
        static_cast<VkDeviceSize>(extent.width) * extent.height * 4;

    // NDI may still be reading the old readback buffers from a previous asynchronous
    // send, so flush it before they are destroyed
#ifdef SGCT_HAS_NDI
    if (_ndi.handle) {
        NDIlib_send_send_video_async_v2(_ndi.handle, nullptr);
    }
#endif // SGCT_HAS_NDI

    _streamFormat = format;

    // (Re)create the readback ring. The buffers are host-visible and persistently
    // mapped, so the pixels can be read directly once the copy's fence is signaled
    for (StreamBuffer& sb : _streamBuffers) {
        sb.buffer = std::make_unique<vulkan::Buffer>(
            ctx, size, vulkan::Buffer::Type::Readback
        );
        sb.hasData = false;
    }
    _streamFrame = 0;

    // Spout expects RGBA pixels, so if the swapchain is BGRA allocate a temporary
    // buffer for the conversion
    const bool isBgra =
        format == VK_FORMAT_B8G8R8A8_UNORM || format == VK_FORMAT_B8G8R8A8_SRGB;
    if (isBgra) {
        _streamRgba.resize(static_cast<size_t>(size));
    }
    else {
        _streamRgba.clear();
    }

#ifdef SGCT_HAS_SPOUT
    if (_spout.enabled) {
        _spout.name = _spout.name.empty() ? "OpenSpace" : _spout.name;
        if (!_spout.handle) {
            _spout.handle = GetSpout();
            if (_spout.handle &&
                !_spout.handle->CreateSender(
                    _spout.name.c_str(), extent.width, extent.height
                ))
            {
                Log::Error(std::format(
                    "Window {}: Error creating the Spout sender '{}'", _id, _spout.name
                ));
            }
        }
        else {
            _spout.handle->UpdateSender(_spout.name.c_str(), extent.width, extent.height);
        }
    }
#endif // SGCT_HAS_SPOUT

#ifdef SGCT_HAS_NDI
    if (_ndi.handle) {
        // The readback is top-down, so the stride is positive
        _ndi.videoFrame.xres = extent.width;
        _ndi.videoFrame.yres = extent.height;
        _ndi.videoFrame.line_stride_in_bytes = static_cast<int>(extent.width) * 4;
        _ndi.videoFrame.picture_aspect_ratio =
            static_cast<float>(extent.width) / static_cast<float>(extent.height);
        _ndi.videoFrame.FourCC =
            isBgra ? NDIlib_FourCC_type_BGRA : NDIlib_FourCC_type_RGBA;
    }
#endif // SGCT_HAS_NDI
}

void Window::recordStreamCopy(VkCommandBuffer cmd) {
    if (!streamingEnabled()) {
        return;
    }

    const VkExtent2D extent = _swapchain->extent();
    StreamBuffer& sb = _streamBuffers[_streamFrame % 3];
    const VkImage image = _swapchain->image(_imageIndex);

    // The image was last written by the composite pass, whose render pass makes its
    // writes available to the transfer stage
    VkImageMemoryBarrier toTransfer = {};
    toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toTransfer.srcAccessMask = 0;
    toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    toTransfer.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.image = image;
    toTransfer.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &toTransfer
    );

    // Copy the left eye (the first layer of the swapchain image)
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { extent.width, extent.height, 1 };
    vkCmdCopyImageToBuffer(
        cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, sb.buffer->handle(), 1, &region
    );

    // Return the image to its layout for the present and make the copied pixels visible
    // to the host once the frame's fence is signaled
    VkImageMemoryBarrier fromTransfer = toTransfer;
    fromTransfer.srcAccessMask = 0;
    fromTransfer.dstAccessMask = 0;
    fromTransfer.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    fromTransfer.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkMemoryBarrier toHost = {};
    toHost.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    toHost.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toHost.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        1, &toHost,
        0, nullptr,
        1, &fromTransfer
    );

    // The copy completes when the frame's in-flight fence is signaled
    sb.fence = _swapchain->inFlightFence(_frameIndex);
    sb.hasData = true;
}

void Window::sendStreamFrame() {
    if (!streamingEnabled()) {
        return;
    }

    // The frame to send is the one recorded in the previous frame
    StreamBuffer& sb = _streamBuffers[(_streamFrame + 2) % 3];
    _streamFrame++;
    if (!sb.hasData) {
        return;
    }
    sb.hasData = false;

    vulkan::Context& ctx = vulkan::Context::instance();
    // Wait for the copy of the previous frame to have finished
    check(
        vkWaitForFences(
            ctx.device(), 1, &sb.fence, VK_TRUE, std::numeric_limits<uint64_t>::max()
        ),
        "vkWaitForFences"
    );

    const uint8_t* data = static_cast<const uint8_t*>(sb.buffer->map());
    const VkExtent2D extent = _swapchain->extent();
    const bool isBgra =
        _streamFormat == VK_FORMAT_B8G8R8A8_UNORM ||
        _streamFormat == VK_FORMAT_B8G8R8A8_SRGB;

#ifdef SGCT_HAS_SPOUT
    if (_spout.handle) {
        const uint8_t* spoutData = data;
        if (isBgra) {
            // Spout expects RGBA pixels, so convert the BGRA readback
            uint8_t* rgba = _streamRgba.data();
            const size_t nPixels = static_cast<size_t>(extent.width) * extent.height;
            for (size_t i = 0; i < nPixels; i++) {
                rgba[i * 4 + 0] = data[i * 4 + 2];
                rgba[i * 4 + 1] = data[i * 4 + 1];
                rgba[i * 4 + 2] = data[i * 4 + 0];
                rgba[i * 4 + 3] = data[i * 4 + 3];
            }
            spoutData = rgba;
        }
        if (!_spout.handle->SendImage(spoutData, extent.width, extent.height)) {
            Log::Error(std::format(
                "Window {}: Error sending the Spout image for '{}'", _id, _spout.name
            ));
        }
    }
#endif // SGCT_HAS_SPOUT

#ifdef SGCT_HAS_NDI
    if (_ndi.handle) {
        // NDI converts the BGRA (or RGBA) pixels on its own thread, so the readback
        // buffer can be passed directly; it stays valid until the next asynchronous send
        _ndi.videoFrame.p_data = const_cast<uint8_t*>(data);
        _ndi.videoFrame.timestamp = 0;
        NDIlib_send_send_video_async_v2(_ndi.handle, &_ndi.videoFrame);
    }
#endif // SGCT_HAS_NDI
}
#endif // defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)

void Window::renderComposite() {
    ZoneScoped;

    if (!_isRecording) {
        return;
    }
    _isRecording = false;

    VkCommandBuffer cmd = _commandBuffers[_frameIndex];
    const VkExtent2D extent = _swapchain->extent();

#ifdef TRACY_ENABLE
    TracyVkZone(_tracyContexts[_frameIndex], cmd, "Composite");
#endif // TRACY_ENABLE

    using Mode = vulkan::CompositePass::Mode;
    const Mode mode = [](StereoMode sm) {
        switch (sm) {
            case StereoMode::AnaglyphRedCyan:       return Mode::AnaglyphRedCyan;
            case StereoMode::AnaglyphAmberBlue:     return Mode::AnaglyphAmberBlue;
            case StereoMode::AnaglyphRedCyanWimmer: return Mode::AnaglyphRedCyanWimmer;
            case StereoMode::Checkerboard:          return Mode::Checkerboard;
            case StereoMode::CheckerboardInverted:  return Mode::CheckerboardInverted;
            case StereoMode::VerticalInterlaced:    return Mode::VerticalInterlaced;
            case StereoMode::VerticalInterlacedInverted:
                return Mode::VerticalInterlacedInverted;
            case StereoMode::Dummy:                 return Mode::Dummy;
            // No stereo, active stereo (whose layers show one eye each), and the
            // split-screen modes, whose left eye target already contains both eyes
            default:                                return Mode::Mono;
        }
    }(_stereoMode);

    // With stereoscopic presentation, the first layer of the swapchain image shows the
    // left eye and the second one the right eye
    for (uint32_t layer = 0; layer < _swapchain->layerCount(); layer++) {
        VkClearValue clearValues[2] = {};
        clearValues[0].color = {{ 0.f, 0.f, 0.f, 1.f }};
        clearValues[1].depthStencil = { 1.f, 0 };
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = _framebuffer->renderPass();
        renderPassInfo.framebuffer =
            _framebuffer->framebuffer(_imageIndex * _swapchain->layerCount() + layer);
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = extent;
        renderPassInfo.clearValueCount = 2;
        renderPassInfo.pClearValues = clearValues;
        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // The warp meshes are defined in OpenGL normalized device coordinates (y pointing
        // up), so the viewport covering the whole swapchain image is flipped
        VkViewport viewport = {};
        viewport.x = 0.f;
        viewport.y = static_cast<float>(extent.height);
        viewport.width = static_cast<float>(extent.width);
        viewport.height = -static_cast<float>(extent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;
        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vulkan::CompositePass::PushConstants constants;
        constants.flipX = _mirrorX ? 1 : 0;
        constants.flipY = _mirrorY ? 1 : 0;
        constants.mode = layer == 1 ? Mode::Right : mode;
        constants.targetHeight = static_cast<int32_t>(extent.height);
        _composite->bind(cmd, constants);

        for (const std::unique_ptr<Viewport>& vp : _viewports) {
            vp->renderWarpMesh(cmd);
        }

        // The masks are multiplied with the composited image:
        // result = color * blendMask * blackLevelMask
        if (_hasAnyMasks) {
            _composite->bindMask(cmd, constants);
            for (const std::unique_ptr<Viewport>& vp : _viewports) {
                if (!vp->isEnabled()) {
                    continue;
                }
                if (vp->hasBlendMaskTexture()) {
                    _composite->setMaskTexture(cmd, *vp->blendMaskTexture());
                    vp->renderMaskMesh(cmd);
                }
                if (vp->hasBlackLevelMaskTexture()) {
                    _composite->setMaskTexture(cmd, *vp->blackLevelMaskTexture());
                    vp->renderMaskMesh(cmd);
                }
            }
        }

        vkCmdEndRenderPass(cmd);
    }

    // The command buffer is ended and submitted in swapBuffers, after the screenshots
    // have been recorded
    _frameRecorded = true;
}

void Window::swapBuffers(bool takeScreenshot) {
    // Nothing to submit if the frame was skipped or dropped (e.g. because the swapchain
    // was out of date when acquiring the image). A recorded frame is always submitted,
    // even if the window was hidden in the meantime, as draw has reset its fence
    if (!_frameRecorded) {
        return;
    }
    _frameRecorded = false;

    vulkan::Context& ctx = vulkan::Context::instance();
    VkCommandBuffer cmd = _commandBuffers[_frameIndex];

    // Record the copies of the screenshots. They are saved once the frame has finished
    std::vector<ScreenCapture*> captures;
    if (takeScreenshot) {
        ZoneScopedN("Take Screenshot");
        if (Engine::instance().settings().captureBackBuffer) {
            if (!_swapchain->supportsCapture()) {
                Log::Error(std::format(
                    "Window {}: The swapchain does not support screenshots of the back "
                    "buffer", _id
                ));
            }
            else if (_screenCaptureLeftOrMono &&
                     _screenCaptureLeftOrMono->record(
                         cmd,
                         _swapchain->image(_imageIndex),
                         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                         _swapchain->extent(),
                         _swapchain->format()
                     ))
            {
                captures.push_back(_screenCaptureLeftOrMono.get());
            }
            // The right eye of stereoscopic presentation is in the second layer
            if (_swapchain->supportsCapture() && _swapchain->layerCount() == 2 &&
                _screenCaptureRight &&
                _screenCaptureRight->record(
                    cmd,
                    _swapchain->image(_imageIndex),
                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    _swapchain->extent(),
                    _swapchain->format(),
                    1
                ))
            {
                captures.push_back(_screenCaptureRight.get());
            }
        }
        else {
            if (_screenCaptureLeftOrMono &&
                _screenCaptureLeftOrMono->record(
                    cmd,
                    _offscreen->colorImage(),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    _offscreen->extent(),
                    _offscreen->colorFormat()
                ))
            {
                captures.push_back(_screenCaptureLeftOrMono.get());
            }
            if (_screenCaptureRight && _offscreenRight &&
                _screenCaptureRight->record(
                    cmd,
                    _offscreenRight->colorImage(),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    _offscreenRight->extent(),
                    _offscreenRight->colorFormat()
                ))
            {
                captures.push_back(_screenCaptureRight.get());
            }
        }
    }

    // Record the copy of the composited image for the Spout/NDI streaming. It is sent
    // one frame later, once this frame's fence has been signaled
#if defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)
    recordStreamCopy(cmd);
#endif

    if (_timestampPool != VK_NULL_HANDLE) {
        vkCmdWriteTimestamp(
            cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, _timestampPool, 2 * _frameIndex + 1
        );
        _hasTimestamps[_frameIndex] = true;
    }

#ifdef TRACY_ENABLE
    // TracyVkCollect records vkCmdResetQueryPool into the command buffer, so it must run
    // while the buffer is still in the recording state (before vkEndCommandBuffer) and
    // outside of a render pass
    TracyVkCollect(_tracyContexts[_frameIndex], cmd);
#endif // TRACY_ENABLE

    check(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");

    // Swap
    _windowResChanged = false;

#ifdef SGCT_HAS_SCALABLE
    if (_scalableMesh.sdk) {
        EasyBlendSDK_TransformInputToOutput(
            reinterpret_cast<EasyBlendSDK_Mesh*>(_scalableMesh.sdk)
        );
    }
#endif // SGCT_HAS_SCALABLE

    // Submit the command buffer recorded in draw. It waits on the image-available
    // semaphore (signaled by acquireImage) and signals the render-finished semaphore
    // (waited on by present); the in-flight fence is signaled when the submit completes
    try {
        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSemaphore imageAvailable = _swapchain->imageAvailableSemaphore(_frameIndex);
        VkSemaphore renderFinished = _swapchain->renderFinishedSemaphore(_imageIndex);
        VkFence inFlightFence = _swapchain->inFlightFence(_frameIndex);
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &imageAvailable;
        submitInfo.pWaitDstStageMask = &waitStage;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_commandBuffers[_frameIndex];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinished;
        VkResult result = vkQueueSubmit(ctx.queue(), 1, &submitInfo, inFlightFence);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            throw vulkan::SwapchainOutOfDate();
        }
        check(result, "vkQueueSubmit");

        if (!captures.empty()) {
            // The screenshots need the copies to have finished
            check(
                vkWaitForFences(
                    ctx.device(),
                    1,
                    &inFlightFence,
                    VK_TRUE,
                    std::numeric_limits<uint64_t>::max()
                ),
                "vkWaitForFences"
            );
            std::for_each(captures.begin(), captures.end(), std::mem_fn(&ScreenCapture::save));
        }

        // Send the pixels of the previous frame to Spout/NDI. Its copy was recorded in
        // the previous frame, whose fence has been signaled by now
#if defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)
        sendStreamFrame();
#endif

        // Send the cube map faces of the previous frame to their Spout/NDI senders
        for (const std::unique_ptr<Viewport>& vp : _viewports) {
            if (vp->hasSubViewports()) {
                vp->nonLinearProjection()->sendStreamFrames();
            }
        }

        // Present the acquired image; present waits on the render-finished semaphore
        _swapchain->present(_imageIndex);
    }
    catch (const vulkan::SwapchainOutOfDate&) {
        // The swapchain went out of date between acquire and present (e.g. the window
        // was moved to a monitor with a different DPI, which changes the surface extent
        // without a window-size callback). Recreate it and drop this frame
        recreateSwapchain();
        return;
    }

    // Advance to the next in-flight slot
    _frameIndex = (_frameIndex + 1) % vulkan::Swapchain::MaxFramesInFlight;
}

bool Window::shouldRenderFrame() const {
    return isRenderingWhileHidden() || (isVisible() && !_isIconified);
}

void Window::recreateSwapchain() {
    vulkan::Context& ctx = vulkan::Context::instance();

    // Wait for the GPU to finish with the old resources (the swapchain images, the
    // framebuffer's depth image, and the offscreen images) before destroying them
    check(vkDeviceWaitIdle(ctx.device()), "vkDeviceWaitIdle");

    // Any partially recorded frame refers to the command buffers that are freed here
    _isRecording = false;
    _frameRecorded = false;

    // Destroy the framebuffer before the swapchain since the framebuffers reference the
    // swapchain image views
    _offscreenRight = nullptr;
    _offscreen = nullptr;
    _framebuffer = nullptr;
    _swapchain = nullptr;
    freeCommandBuffers();

    _swapchain = std::make_unique<vulkan::Swapchain>(ctx, _windowHandle);
    _swapchainStereo = _stereoMode == StereoMode::Active;
    _swapchainPresentBarrier = _useSwapGroups && _isBarrierActive;
    _swapchain->setStereo(_swapchainStereo);
    _swapchain->setPresentBarrier(_swapchainPresentBarrier);
    // The swap interval only applies to the last window; the others do not wait for the
    // vertical sync but replace their pending image instead (or wait for it if the
    // surface cannot do that)
    if (!_isLastWindow) {
        _swapchain->setPresentMode(VK_PRESENT_MODE_MAILBOX_KHR);
    }
    else if (Engine::instance().settings().swapInterval == 0) {
        _swapchain->setPresentMode(VK_PRESENT_MODE_IMMEDIATE_KHR);
    }
    else {
        _swapchain->setPresentMode(VK_PRESENT_MODE_FIFO_KHR);
    }
    _swapchain->create();
    createCommandBuffers();

    if (_swapchainStereo && _swapchain->layerCount() == 1 && !_hasWarnedAboutStereo) {
        Log::Warning(std::format(
            "Window {}: The surface does not support stereoscopic presentation, so only "
            "the left eye is rendered", _id
        ));
        _hasWarnedAboutStereo = true;
    }
    if (_swapchainPresentBarrier && !_swapchain->usesPresentBarrier()) {
        Log::Warning(std::format(
            "Window {}: The surface does not support the present barrier", _id
        ));
    }

    // One framebuffer per layer of each swapchain image
    _framebuffer = std::make_unique<vulkan::Framebuffer>(
        ctx, _swapchain->format(), _swapchain->imageViews(), _swapchain->extent()
    );

    if (_linearSampler == VK_NULL_HANDLE) {
        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.maxLod = 0.f;
        check(
            vkCreateSampler(ctx.device(), &samplerInfo, nullptr, &_linearSampler),
            "vkCreateSampler"
        );
    }

    // The composite pipeline depends on the swapchain's render pass
    if (!_composite) {
        _composite = std::make_unique<vulkan::CompositePass>(ctx);
    }
    _composite->setRenderPass(_framebuffer->renderPass());
    if (!_overlayPass) {
        _overlayPass = std::make_unique<vulkan::OverlayPass>(ctx);
    }

    recreateOffscreen();

    // The readback ring of the Spout/NDI streaming has the size of the swapchain images
#if defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)
    createStreamBuffers();
#endif
}

void Window::recreateOffscreen() {
    vulkan::Context& ctx = vulkan::Context::instance();

    // In-flight frames and the composite pass' descriptor set reference the old images
    check(vkDeviceWaitIdle(ctx.device()), "vkDeviceWaitIdle");
    _fxaaPass = nullptr;
    _intermediate = nullptr;
    _offscreenRight = nullptr;
    _offscreen = nullptr;
    _isOffscreenRendered = false;

    _sceneFormat = chooseSceneFormat();

    // The scene is rendered at the framebuffer resolution, which can differ from the size
    // of the swapchain images (e.g. with a fixed resolution); the composite pass scales it
    const ivec2 res = framebufferResolution();
    const VkExtent2D extent = {
        static_cast<uint32_t>(std::max(res.x, 1)),
        static_cast<uint32_t>(std::max(res.y, 1))
    };
    _offscreen = std::make_unique<vulkan::OffscreenBuffer>(ctx, extent, _sceneFormat);
    if (needsRightEyeTarget()) {
        _offscreenRight =
            std::make_unique<vulkan::OffscreenBuffer>(ctx, extent, _sceneFormat);
    }

    if (_useFXAA) {
        _intermediate =
            std::make_unique<vulkan::OffscreenBuffer>(ctx, extent, _sceneFormat);

        vulkan::ProjectionPass::Settings settings;
        settings.fragmentShader = std::format(
            "#version 460\n"
            "#define HAS_NORMALS {}\n"
            "#define HAS_POSITIONS {}\n"
            "#define HAS_DEPTH {}\n"
            "{}",
            _sceneFormat.hasNormals ? 1 : 0,
            _sceneFormat.hasPositions ? 1 : 0,
            _sceneFormat.hasDepthTexture ? 1 : 0,
            FxaaFrag
        );
        settings.pushConstantSize = 80;
        settings.samplerCount = 4;
        settings.colorAttachmentCount = _sceneFormat.colorAttachmentCount();
        settings.samples = _sceneFormat.samples;
        settings.writesAllAttachments = true;
        settings.writesDepth = _sceneFormat.hasDepthTexture;
        _fxaaPass = std::make_unique<vulkan::ProjectionPass>(
            ctx,
            _offscreen->renderPass(),
            settings
        );
    }

    // The composite pass always samples both inputs, so the left eye is bound twice if
    // there is no separate right eye target
    const vulkan::OffscreenBuffer& right = _offscreenRight ? *_offscreenRight : *_offscreen;
    _composite->setInputs(_offscreen->colorImageView(), right.colorImageView());

    // All offscreen targets have compatible render passes, so one pipeline serves them
    _overlayPass->setRenderPass(_offscreen->renderPass(), _sceneFormat);

    // The cube maps of the non-linear projections need to match a new render target
    // format. Before the projections are initialized, their format is undefined
    for (const std::unique_ptr<Viewport>& vp : _viewports) {
        NonLinearProjection* projection = vp->nonLinearProjection();
        if (projection && projection->sceneFormat().color != VK_FORMAT_UNDEFINED &&
            projection->sceneFormat() != _sceneFormat)
        {
            projection->initialize(_sceneFormat);
        }
    }
}

vulkan::SceneFormat Window::chooseSceneFormat() const {
    using CBD = config::Window::ColorBitDepth;
    vulkan::Context& ctx = vulkan::Context::instance();

    vulkan::SceneFormat format;

    // The 8-bit targets use the format of the swapchain, so that the composite does not
    // convert the colors
    const CBD depth = _bufferBitDepth.value_or(CBD::Depth8);
    VkFormat requested = [](CBD d, VkFormat swapchain) {
        switch (d) {
            case CBD::Depth8:       return swapchain;
            case CBD::Depth16:      return VK_FORMAT_R16G16B16A16_UNORM;
            case CBD::Depth16Float: return VK_FORMAT_R16G16B16A16_SFLOAT;
            case CBD::Depth32Float: return VK_FORMAT_R32G32B32A32_SFLOAT;
            // The integer formats can neither be blended nor filtered, which the 2D pass
            // and the composite need
            case CBD::Depth16Int:
            case CBD::Depth16UInt:  return VK_FORMAT_R16G16B16A16_SFLOAT;
            case CBD::Depth32Int:
            case CBD::Depth32UInt:  return VK_FORMAT_R32G32B32A32_SFLOAT;
            default: throw std::logic_error("Unhandled case label");
        }
    }(depth, _swapchain->format());
    if (depth == CBD::Depth16Int || depth == CBD::Depth16UInt ||
        depth == CBD::Depth32Int || depth == CBD::Depth32UInt)
    {
        Log::Warning(std::format(
            "Window {}: Integer buffers cannot be composited, so a floating point buffer "
            "with the same size is used", _id
        ));
    }

    VkFormatProperties properties = {};
    vkGetPhysicalDeviceFormatProperties(ctx.physicalDevice(), requested, &properties);
    constexpr VkFormatFeatureFlags Required =
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT |
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    if ((properties.optimalTilingFeatures & Required) != Required) {
        Log::Warning(std::format(
            "Window {}: The buffer bit depth is not supported by the device, so 8 bits "
            "are used", _id
        ));
        requested = _swapchain->format();
    }
    format.color = requested;

    format.samples = vulkan::supportedSampleCount(ctx, _nAASamples);
    if (static_cast<int>(format.samples) < _nAASamples) {
        Log::Warning(std::format(
            "Window {}: {} samples are not supported, using {}",
            _id, _nAASamples, static_cast<int>(format.samples)
        ));
    }

    const Engine::Settings& settings = Engine::instance().settings();
    format.hasNormals = settings.useNormalTexture;
    format.hasPositions = settings.usePositionTexture;
    format.hasDepthTexture = settings.useDepthTexture;
    return format;
}

bool Window::needsRightEyeTarget() const {
    // Active stereo has a right eye if the swapchain supports stereoscopic presentation
    if (_stereoMode == StereoMode::Active) {
        return _swapchain && _swapchain->layerCount() == 2;
    }
    return _stereoMode > StereoMode::Active && _stereoMode < StereoMode::SideBySide;
}

void Window::createCommandBuffers() {
    vulkan::Context& ctx = vulkan::Context::instance();

    _commandBuffers.resize(vulkan::Swapchain::MaxFramesInFlight);
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = ctx.commandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = vulkan::Swapchain::MaxFramesInFlight;
    check(vkAllocateCommandBuffers(ctx.device(), &allocInfo, _commandBuffers.data()),
        "vkAllocateCommandBuffers");

    // Two timestamps per in-flight frame measure the GPU time of the frames
    if (ctx.limits().timestampComputeAndGraphics) {
        VkQueryPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        poolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        poolInfo.queryCount = 2 * vulkan::Swapchain::MaxFramesInFlight;
        check(
            vkCreateQueryPool(ctx.device(), &poolInfo, nullptr, &_timestampPool),
            "vkCreateQueryPool"
        );
    }
    _hasTimestamps = {};

#ifdef TRACY_ENABLE
    // One Tracy GPU context per in-flight command buffer, so the GPU zones of each
    // frame are collected against the command buffer that recorded them
    _tracyContexts.resize(_commandBuffers.size());
    for (size_t i = 0; i < _commandBuffers.size(); i++) {
        Log::Info(std::format(
            "Creating Tracy GPU context {} (physdev={}, device={}, queue={}, cmdbuf={})",
            i,
            reinterpret_cast<uintptr_t>(ctx.physicalDevice()),
            reinterpret_cast<uintptr_t>(ctx.device()),
            reinterpret_cast<uintptr_t>(ctx.queue()),
            reinterpret_cast<uintptr_t>(_commandBuffers[i])
        ));
        _tracyContexts[i] = TracyVkContext(
            ctx.physicalDevice(), ctx.device(), ctx.queue(), _commandBuffers[i]
        );
        Log::Info(std::format("Created Tracy GPU context {} = {}", i,
            reinterpret_cast<uintptr_t>(_tracyContexts[i])));
        TracyVkContextName(_tracyContexts[i], "SGCT", 4);
    }
#endif // TRACY_ENABLE
}

void Window::freeCommandBuffers() {
    if (_commandBuffers.empty()) {
        return;
    }

    vulkan::Context& ctx = vulkan::Context::instance();
    vkFreeCommandBuffers(
        ctx.device(),
        ctx.commandPool(),
        static_cast<uint32_t>(_commandBuffers.size()),
        _commandBuffers.data()
    );
    _commandBuffers.clear();

#ifdef TRACY_ENABLE
    for (TracyVkCtx tracyCtx : _tracyContexts) {
        TracyVkDestroy(tracyCtx);
    }
    _tracyContexts.clear();
#endif // TRACY_ENABLE

    if (_timestampPool != VK_NULL_HANDLE) {
        vkDestroyQueryPool(ctx.device(), _timestampPool, nullptr);
        _timestampPool = VK_NULL_HANDLE;
    }
    _hasTimestamps = {};
}

bool Window::shouldTakeScreenshot() const {
    return _takeScreenshot;
}

double Window::gpuFrameTime() const {
    return _gpuFrameTime;
}

bool Window::isOpenXREnabled() const {
    return _useOpenXR;
}

vec2 Window::scale() const {
    return _scale;
}

float Window::aspectRatio() const {
    return _aspectRatio;
}

ivec2 Window::windowSize() const {
    return _windowSize;
}

GLFWwindow* Window::windowHandle() const {
    return _windowHandle;
}

VkRenderPass Window::renderPass() const {
    // The scene is rendered into the offscreen target, so its render pass is the one the
    // application pipelines must be created with
    return _offscreen ? _offscreen->renderPass() : VK_NULL_HANDLE;
}

VkFormat Window::renderTargetFormat() const {
    return _sceneFormat.color;
}

const vulkan::SceneFormat& Window::sceneFormat() const {
    return _sceneFormat;
}

VkCommandBuffer Window::currentCommandBuffer() const {
    return _isRecording ? _commandBuffers[_frameIndex] : VK_NULL_HANDLE;
}

VkCommandBuffer Window::frameCommandBuffer() const {
    return _frameRecorded ? _commandBuffers[_frameIndex] : VK_NULL_HANDLE;
}

const vulkan::OffscreenBuffer* Window::offscreenTarget(Eye eye) const {
    if (eye == Eye::Right && _offscreenRight) {
        return _offscreenRight.get();
    }
    return _offscreen.get();
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
            // If not tracked update, otherwise this is done on the fly
            continue;
        }

        vp->calculateFrustum(FrustumMode::Mono, nearClip, farClip);
        vp->calculateFrustum(FrustumMode::StereoLeft, nearClip, farClip);
        vp->calculateFrustum(FrustumMode::StereoRight, nearClip, farClip);
    }
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
    // Set field of view of each of this window's viewports to match new horiz/vert aspect
    // ratio, adjusting only the horizontal (x) values
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
    // The offscreen targets are adapted to the new mode at the beginning of the next draw
    _stereoMode = sm;
}

Window::StereoMode Window::stereoMode() const {
    return _stereoMode;
}

bool Window::isStereo() const {
    return _stereoMode != StereoMode::NoStereo;
}

} // namespace sgct
