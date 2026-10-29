/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__WINDOW__H__
#define __SGCT__WINDOW__H__

#include <sgct/sgctexports.h>

#include <sgct/definitions.h>
#include <sgct/profiling.h>
#include <sgct/screencapture.h>
#include <sgct/viewport.h>
#include <sgct/vulkan/compositepass.h>
#include <sgct/vulkan/overlaypass.h>
#include <sgct/vulkan/buffer.h>
#include <sgct/vulkan/framebuffer.h>
#include <sgct/vulkan/offscreenbuffer.h>
#include <sgct/vulkan/projectionpass.h>
#include <sgct/vulkan/sceneformat.h>
#include <sgct/vulkan/swapchain.h>
#include <vulkan/vulkan.h>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#ifdef SGCT_HAS_NDI
#include <Processing.NDI.Lib.h>
#include <Processing.NDI.Send.h>
#include <Processing.NDI.Structs.h>
#endif // SGCT_HAS_NDI

struct GLFWwindow;

#ifdef SGCT_HAS_SPOUT
struct SPOUTLIBRARY;
using SPOUTHANDLE = SPOUTLIBRARY*;
#endif // SGCT_HAS_SPOUT

namespace sgct {

namespace config { struct Window; }

class SGCT_EXPORT Window {
public:
    /**
     * Different stereo modes used for rendering.
     */
    enum class StereoMode : uint8_t {
        NoStereo = 0,
        Active,
        AnaglyphRedCyan,
        AnaglyphAmberBlue,
        AnaglyphRedCyanWimmer,
        Checkerboard,
        CheckerboardInverted,
        VerticalInterlaced,
        VerticalInterlacedInverted,
        Dummy,
        SideBySide,
        SideBySideInverted,
        TopBottom,
        TopBottomInverted
    };

    /**
     * Init Nvidia swap groups if supported by hardware. Supported hardware is NVidia
     * Quadro graphics card + sync card or AMD/ATI FireGL graphics card + sync card.
     */
    static void initNvidiaSwapGroups();

    static void resetSwapGroupFrameNumber();
    static void setBarrier(bool state);
    static bool isBarrierActive();
    static bool isUsingSwapGroups();
    static unsigned int swapGroupFrameNumber();

    Window(const config::Window& window);
    ~Window();

    /**
     * This function is used to open the window. It creates the GLFW window that backs the
     * Vulkan surface; the swapchain itself is created later in #initialize, once the
     * shared Vulkan context exists.
     *
     * \param isLastWindow Whether this is the last window. This is required to know as we
     *        only want to set the vsync setting once per node
     */
    void openWindow(bool isLastWindow);

    void closeWindow();

    /**
     * Initialize the window's Vulkan resources: the swapchain, the offscreen render
     * targets, and the pipelines that render into them.
     */
    void initialize();

    void updateResolutions();

    void update();

    void draw();

    /**
     * Records the composite of the warp meshes (and the masks) into the swapchain
     * image. Must be called after #draw, while the frame's command buffer is still
     * being recorded, and before #swapBuffers.
     */
    void renderComposite();

    /**
     * Swap previous data and current data. This is done at the end of the render loop.
     */
    void swapBuffers(bool takeScreenshot);

    bool shouldTakeScreenshot() const;

    /**
     * \return The time in seconds that the GPU took to execute the last completed frame of
     *         this window, or 0 if no frame has completed yet or the device does not
     *         support timestamps
     */
    double gpuFrameTime() const;
    bool isOpenXREnabled() const;

    /**
     * \return Get the scale value (relation between pixel and point size). Normally this
     *         value is 1.f but 2.f on some retina computers
     */
    vec2 scale() const;

    /**
     * \return The aspect ratio of the window
     */
    float aspectRatio() const;

    /**
     * \return Get the window size
     */
    ivec2 windowSize() const;

    /**
     * \return The pointer to GLFW window
     */
    GLFWwindow* windowHandle() const;

    /**
     * \return The render pass of the current framebuffer, or VK_NULL_HANDLE if the
     *         framebuffer has not been created yet
     */
    VkRenderPass renderPass() const;

    /**
     * \return The color format of the render targets that #renderPass renders into
     */
    VkFormat renderTargetFormat() const;

    /**
     * \return The format of the render targets that #renderPass renders into. Pipelines
     *         for the draw callbacks have to be created for it (see
     *         vulkan::Pipeline::Settings::setSceneFormat)
     */
    const vulkan::SceneFormat& sceneFormat() const;

    /**
     * \return The command buffer that is currently being recorded for this window, or
     *         VK_NULL_HANDLE if the window is not recording. While the draw and draw2D
     *         callbacks are invoked, it is the same as RenderData::commandBuffer
     */
    VkCommandBuffer currentCommandBuffer() const;

    /**
     * \return The command buffer of the frame that has been recorded by #draw and
     *         #renderComposite but not yet submitted by #swapBuffers, or VK_NULL_HANDLE
     *         if there is no such frame. Commands that are recorded into it are executed
     *         after the composite pass
     */
    VkCommandBuffer frameCommandBuffer() const;

    /**
     * \return The offscreen target that the scene of \p eye is rendered into, or
     *         `nullptr` if the window is not initialized. The left eye target contains
     *         both eyes for the split-screen stereo modes, and it is also returned for
     *         the right eye if the stereo mode has no separate right eye target. The
     *         color image is in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL after #draw
     */
    const vulkan::OffscreenBuffer* offscreenTarget(Eye eye) const;

    void addViewport(std::unique_ptr<Viewport> vpPtr);
    const std::vector<std::unique_ptr<Viewport>>& viewports() const;

    void updateFrustums(float nearClip, float farClip);

    /**
     * Sets the framebuffer resolution. These parameters will only be used if a fixed
     * resolution is used that is different from the window resolution.
     *
     * \param resolution The width and height of the frame buffer in pixels
     */
    void setFramebufferResolution(ivec2 resolution);

    /**
     * Get the dimensions of the offscreen render target. Regular viewport rendering
     * renders directly into this target, but a fisheye renders first a cubemap and then
     * into the target. Post effects are rendered using these dimensions.
     */
    ivec2 framebufferResolution() const;

    /**
     * \return `true` if this window is resized
     */
    bool isWindowResized() const;

    /**
     * \return `this` window's focused flag
     */
    bool isFocused() const;

    /**
     * \return This window's id
     */
    int id() const;

    /**
     * Name this window.
     */
    void setName(std::string name);

    /**
     * \return The name of this window
     */
    const std::string& name() const;

    /**
     * Tag this window. Tags are seperated by comma.
     */
    void setTags(std::vector<std::string> tags);

    /**
     * \return `true` if a specific tag exists
     */
    bool hasTag(std::string_view tag) const;

    /**
     * Set the visibility state of this window. If a window is hidden the rendering for
     * that window will be paused unless it's forced to render while hidden by using
     * #setRenderWhileHidden.
     */
    void setVisible(bool state);

    /**
     * \return `true` if the window is visible or not
     */
    bool isVisible() const;

    /**
     * Set if window should render while hidden. Normally a window pauses the rendering if
     * it's hidden.
     */
    void setRenderWhileHidden(bool state);

    /**
     * \return `true` if the window is set to render while hidden
     */
    bool isRenderingWhileHidden() const;

    /**
     * Set if fullscreen mode should be used.
     */
    void setFullscreen(bool fullscreen);

    /**
     * Sets whether a full screen window should automatically iconify when losing focus.
     */
    void setAutoiconify(bool shouldAutoiconify);

    /**
     * Set if the window should float (be on top / topmost).
     */
    void setFloating(bool floating);

    /**
     * Set if the window should participate in screenshot taking.
     */
    void setTakeScreenshot(bool takeScreenshot);

    /**
     * Set if window borders should be visible.
     */
    void setWindowDecoration(bool state);

    /**
     * Set if the window should be resizable.
     */
    void setWindowResizable(bool state);

    /**
     * Force the framebuffer to a fixed size which may be different from the window size.
     */
    void setFixResolution(bool state);

    /**
     * \return If the frame buffer has a fix resolution this function returns `true`
     */
    bool isFixResolution() const;

    void setHorizFieldOfView(float hFovDeg);

    /**
     * Get FOV of viewport[0]
     */
    float horizFieldOfViewDegrees() const;

    /**
     * Set if the specified Draw2D function pointer should be called for this window.
     */
    void setCallDraw2DFunction(bool state);

    /**
     * Set if the specified Draw3D function pointer should be called for this window.
     */
    void setCallDraw3DFunction(bool state);

    /**
     * Set the stereo mode. Set this mode in your init callback or during runtime in the
     * post-sync-pre-draw callback. GLSL shaders will be recompliled if needed.
     */
    void setStereoMode(StereoMode sm);

    /**
     * \return The stereo mode
     */
    StereoMode stereoMode() const;

    /**
     * \return `true` if any kind of stereo is enabled
     */
    bool isStereo() const;

private:
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Vulkan swapchain lifecycle
    void recreateSwapchain();
    void createCommandBuffers();
    void freeCommandBuffers();
    bool shouldRenderFrame() const;

    // Recreates the offscreen render targets at the framebuffer resolution and points
    // the composite pass at them
    void recreateOffscreen();

    // Whether the stereo mode renders the right eye into a separate offscreen target
    // that the composite pass combines with the left eye
    bool needsRightEyeTarget() const;

    // The format of the offscreen targets for the window's settings
    vulkan::SceneFormat chooseSceneFormat() const;

    // Records the rendering of the provided frustums into the offscreen target: the
    // scene (through the FXAA target if FXAA is used) followed by the 2D pass
    void recordTarget(VkCommandBuffer cmd, const vulkan::OffscreenBuffer& target,
        std::initializer_list<FrustumMode> frustums);

    // Records the copy of the intermediate target into the active render pass of the
    // offscreen target with FXAA
    void recordFxaa(VkCommandBuffer cmd, const vulkan::OffscreenBuffer& target);

    // The window whose content is copied into the viewports (see blitWindowId), if it
    // has rendered the current frame already
    const Window* blitSource() const;

    // Records the rendering of the cube maps of the viewports with non-linear
    // projections, for all eyes that the stereo mode renders. Must be recorded outside
    // of a render pass
    void renderCubemaps(VkCommandBuffer cmd);

#if defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)
    // Spout/NDI streaming of the composited window. createStreamBuffers allocates the
    // readback ring (and the temporary RGBA buffer for Spout), recordStreamCopy records
    // the copy of the composited left-eye image into the ring, and sendStreamFrame sends
    // the pixels of the previous frame once its copy has finished
    bool streamingEnabled() const;
    void createStreamBuffers();
    void recordStreamCopy(VkCommandBuffer cmd);
    void sendStreamFrame();
#endif // defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)

    // Records the scene of all viewports for the provided frustum into the render pass
    // that is active on the offscreen render target
    void recordScene(VkCommandBuffer cmd, const vulkan::OffscreenBuffer& target,
        FrustumMode frustum);

    // Records the overlays, statistics, and draw2D callbacks of all viewports for the
    // provided frustum into the render pass that is active on the offscreen target
    void record2D(VkCommandBuffer cmd, const vulkan::OffscreenBuffer& target,
        FrustumMode frustum);

    std::string _name;
    std::vector<std::string> _tags;
    int8_t _id = -1;

    bool _hideMouseCursor;
    bool _takeScreenshot;
    bool _hasAlpha;
    bool _hasCallDraw2DFunction;
    bool _hasCallDraw3DFunction;
    bool _isFullScreen;
    bool _shouldAutoiconify;
    bool _isFloating;
    bool _shouldRenderWhileHidden;
    uint8_t _nAASamples;
    bool _useFXAA;
    std::optional<config::Window::ColorBitDepth> _bufferBitDepth;
    bool _isLastWindow = false;
    bool _isDecorated;
    bool _isResizable;
    bool _isMirrored;
    int8_t _blitWindowId;
    uint8_t _monitorIndex;
    bool _mirrorX;
    bool _mirrorY;
    bool _noError;
    bool _useOpenXR;
    bool _isVisible;
    bool _isIconified = false;
    StereoMode _stereoMode;
    std::optional<ivec2> _windowPos;
    ivec2 _windowSize;
    ivec2 _framebufferRes;

    std::optional<ivec2> _pendingWindowSize;
    bool _windowResChanged = false;
    bool _hasFocus = false;
    bool _useFixResolution = false;
    bool _hasAnyMasks = false;
    std::optional<ivec2> _pendingFramebufferRes;
    GLFWwindow* _windowHandle = nullptr;
    float _aspectRatio = 1.f;
    vec2 _scale = vec2{ 0.f, 0.f };

#ifdef SGCT_HAS_SPOUT
    struct {
        bool enabled;
        std::string name;
        SPOUTHANDLE handle = nullptr;
    } _spout;
#endif // SGCT_HAS_SPOUT

#ifdef SGCT_HAS_NDI
    struct {
        NDIlib_send_instance_t handle = nullptr;
        NDIlib_video_frame_v2_t videoFrame;
        std::string name;
        std::string groups;
    } _ndi;
#endif // SGCT_HAS_NDI

#if defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)
    // Readback ring for Spout/NDI streaming. Each frame, the composited left-eye image
    // is copied into the next buffer of the ring, and one frame later, once the copy's
    // fence has been signaled, the pixels are sent. The ring is one buffer deeper than
    // the number of frames in flight, so the GPU copy of the current frame never
    // collides with the CPU send of an older frame
    struct StreamBuffer {
        std::unique_ptr<vulkan::Buffer> buffer;
        VkFence fence = VK_NULL_HANDLE;
        bool hasData = false;
    };
    std::array<StreamBuffer, 3> _streamBuffers;
    // Monotonically increasing counter that indexes the ring
    uint64_t _streamFrame = 0;
    // The swapchain format of the readback pixels; it determines the NDI FourCC and
    // whether the Spout sender needs a BGRA to RGBA conversion
    VkFormat _streamFormat = VK_FORMAT_UNDEFINED;
    // Temporary RGBA pixels for the Spout sender, used when the swapchain is BGRA
    std::vector<uint8_t> _streamRgba;
#endif // defined(SGCT_HAS_SPOUT) || defined(SGCT_HAS_NDI)

    std::unique_ptr<ScreenCapture> _screenCaptureLeftOrMono;
    std::unique_ptr<ScreenCapture> _screenCaptureRight;

    std::vector<std::unique_ptr<Viewport>> _viewports;

    // Vulkan rendering state. The swapchain, command buffers, framebuffer, and offscreen
    // target are created in initialize (and recreated on resize). Each frame, draw
    // records the viewports into the offscreen target, renderComposite records the
    // composite of the warp meshes into the swapchain image, and swapBuffers submits and
    // presents the command buffer
    std::unique_ptr<vulkan::Swapchain> _swapchain;
    std::unique_ptr<vulkan::Framebuffer> _framebuffer;
    // The left eye (or mono) target; split-screen stereo renders both eyes into it
    std::unique_ptr<vulkan::OffscreenBuffer> _offscreen;
    // The right eye target for the stereo modes that combine two images in the composite
    std::unique_ptr<vulkan::OffscreenBuffer> _offscreenRight;
    std::unique_ptr<vulkan::CompositePass> _composite;
    // Draws the viewports' overlay textures into the offscreen targets
    std::unique_ptr<vulkan::OverlayPass> _overlayPass;
    // The format of the offscreen targets
    vulkan::SceneFormat _sceneFormat;
    // With FXAA, the scene is rendered into the intermediate target first, and the FXAA
    // pass then renders it into the offscreen target
    std::unique_ptr<vulkan::OffscreenBuffer> _intermediate;
    std::unique_ptr<vulkan::ProjectionPass> _fxaaPass;
    // A linear, clamp-to-edge sampler for the offscreen targets
    VkSampler _linearSampler = VK_NULL_HANDLE;
    // Whether the offscreen targets have been rendered in the current frame
    bool _isOffscreenRendered = false;
    // The settings that the current swapchain was requested with
    bool _swapchainStereo = false;
    bool _swapchainPresentBarrier = false;
    bool _hasWarnedAboutStereo = false;
    std::vector<VkCommandBuffer> _commandBuffers;
    // The Tracy GPU contexts, one per in-flight command buffer (only used when Tracy
    // is enabled; TracyVkCtx is a void* placeholder otherwise)
    std::vector<TracyVkCtx> _tracyContexts;
    // Set when draw has begun recording the frame's command buffer
    bool _isRecording = false;
    // Set when the frame's command buffer has been completely recorded and can be
    // submitted
    bool _frameRecorded = false;
    uint32_t _frameIndex = 0;
    uint32_t _imageIndex = 0;

    // Timestamps at the beginning and end of the command buffer of each in-flight frame,
    // which are read once the frame's fence has been waited on
    VkQueryPool _timestampPool = VK_NULL_HANDLE;
    std::array<bool, vulkan::Swapchain::MaxFramesInFlight> _hasTimestamps = {};
    double _gpuFrameTime = 0.0;

    static GLFWwindow* _sharedHandle;
    static bool _useSwapGroups;
    static bool _isBarrierActive;

    // ScalableMesh
    struct {
        void* sdk = nullptr;
        std::filesystem::path path;
    } _scalableMesh;
};

config::Window createScalableConfiguration(const config::Window::Scalable& scalable,
    const config::Window& window);

} // namespace sgct

#endif // __SGCT__WINDOW__H__
