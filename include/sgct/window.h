/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__WINDOW__H__
#define __SGCT__WINDOW__H__

#include <sgct/sgctexports.h>
#include <sgct/shaderprogram.h>
#include <sgct/viewport.h>
#include <optional>
#include <string_view>
#include <vector>

struct GLFWmonitor;
struct GLFWwindow;

namespace sgct::config { struct Window; }

namespace sgct {

class BaseViewport;
class OffScreenBuffer;
class ScreenCapture;

/**
 * Helper class for window data.
 */
class SGCT_EXPORT Window {
public:
    /**
     * Different stereo modes used for rendering.
     */
    enum class StereoMode {
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

    enum class ColorBitDepth {
        Depth8,
        Depth16,
        Depth16Float,
        Depth32Float,
        Depth16Int,
        Depth32Int,
        Depth16UInt,
        Depth32UInt
    };

    enum class Eye { MonoOrLeft, Right };

    /**
     * The different texture indexes in window buffers.
     */
    enum class TextureIndex {
        LeftEye = 0,
        RightEye,
        Intermediate,
        Depth,
        Normals,
        Positions
    };

    void applyWindow(const config::Window& window);

    /**
     * Init Nvidia swap groups if supported by hardware. Supported hardware is NVidia
     * Quadro graphics card + sync card or AMD/ATI FireGL graphics card + sync card.
     */
    static void initNvidiaSwapGroups();

    /**
     * Force a restore of the shared OpenGL context.
     */
    static void resetSwapGroupFrameNumber();
    static void setBarrier(bool state);
    static bool isBarrierActive();
    static bool isUsingSwapGroups();
    static bool isSwapGroupMaster();
    static unsigned int swapGroupFrameNumber();

    static void makeSharedContextCurrent();

    Window();
    ~Window();

    Window(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(const Window&) = delete;
    Window& operator=(Window&&) = delete;

    void close();

    /**
     * Initialize window buffers such as textures, FBOs, VAOs, VBOs and PBOs.
     */
    void initOGL();

    /**
     * Initialize context specific data such as viewport corrections/warping meshes.
     */
    void initContextSpecificOGL();

    /**
     * Swap previous data and current data. This is done at the end of the render loop.
     */
    void swap(bool takeScreenshot);
    void updateResolutions();

    void update();

    /**
     * This function is used internally within sgct to open the window.
     *
     * \param share The context that is shared between windows. Might be nullptr
     * \param isLastWindow Whether this is the last window. This is required to know as we
     *        only want to set the vsync setting once per node
     */
    void openWindow(GLFWwindow* share, bool isLastWindow);

    void makeOpenGLContextCurrent();

    /**
     * Name this window.
     */
    void setName(std::string name);

    /**
     * Tag this window. Tags are seperated by comma.
     */
    void setTags(std::vector<std::string> tags);

    /**
     * Set the visibility state of this window. If a window is hidden the rendering for
     * that window will be paused unless it's forced to render while hidden by using
     * #setRenderWhileHidden.
     */
    void setVisible(bool state);

    /**
     * Set if window should render while hidden. Normally a window pauses the rendering if
     * it's hidden.
     */
    void setRenderWhileHidden(bool state);

    /**
     * Set the focued flag for this window (should not be done by user).
     */
    void setFocused(bool state);

    /**
     * Set the window title.
     *
     * \param title The title of the window
     */
    void setWindowTitle(const char* title);

    /**
     * Sets the window resolution.
     *
     * \param resolution The width and height of the window in pixels
     */
    void setWindowResolution(ivec2 resolution);

    /**
     * Sets the framebuffer resolution. These parameters will only be used if a fixed
     * resolution is used that is different from the window resolution.
     *
     * \param resolution The width and height of the frame buffer in pixels
     */
    void setFramebufferResolution(ivec2 resolution);

    /**
     * Set this window's position in screen coordinates.
     *
     * \param positions The horizontal and vertical position in pixels
    */
    void setWindowPosition(ivec2 positions);

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
     * Set if the window is double buffered (can only be set before window creation).
     */
    void setDoubleBuffered(bool doubleBuffered);

    /**
     * Set if window borders should be visible.
     */
    void setWindowDecoration(bool state);

    /**
     * Set if the window should be resizable.
     */
    void setWindowResizable(bool state);

    /**
     * Set which monitor that should be used for fullscreen mode.
     */
    void setFullScreenMonitorIndex(int index);

    /**
     * Force the framebuffer to a fixed size which may be different from the window size.
     */
    void setFixResolution(bool state);
    void setHorizFieldOfView(float hFovDeg);

    /**
     * Set if FXAA should be used.
     */
    void setUseFXAA(bool state);

    /**
     * Use quad buffer (hardware stereoscopic rendering). This function can only be used
     * before the window is created. The quad buffer feature is only supported on
     * professional CAD graphics cards such as Nvidia Quadro or AMD/ATI FireGL.
     */
    void setUseQuadbuffer(bool state);

    /**
     * Set if the specifed Draw2D function pointer should be called for this window.
     */
    void setCallDraw2DFunction(bool state);

    /**
     * Set if the specifed Draw3D function pointer should be called for this window.
     */
    void setCallDraw3DFunction(bool state);

    /**
     * Set the id of the window that should be blitted to this window.
     */
    void setBlitWindowId(int id);

    /**
     * Set the number of samples used in multisampled anti-aliasing.
     */
    void setNumberOfAASamples(int samples);

    /**
     * Set the stereo mode. Set this mode in your init callback or during runtime in the
     * post-sync-pre-draw callback. GLSL shaders will be recompliled if needed.
     */
    void setStereoMode(StereoMode sm);

    /**
     * \return `true` if full screen rendering is enabled
     */
    bool isFullScreen() const;

    /**
     * \return `true` if full screen windows should automatically iconify when losing
     *         focus
     */
    bool shouldAutoiconify() const;

    /**
     * \return `true` if window is floating/allways on top/topmost
     */
    bool isFloating() const;

    /**
     * \return `true` if window is double-buffered
     */
    bool isDoubleBuffered() const;

    /**
     * \return `this` window's focused flag
     */
    bool isFocused() const;

    /**
     * \return `true` if the window is visible or not
     */
    bool isVisible() const;

    /**
     * \return `true` if the window is set to render while hidden
     */
    bool isRenderingWhileHidden() const;

    /**
     * \return If the frame buffer has a fix resolution this function returns `true`
     */
    bool isFixResolution() const;

    /**
     * \return `true` if any kind of stereo is enabled
     */
    bool isStereo() const;

    /**
     * \return `true` if this window is resized
     */
    bool isWindowResized() const;

    /**
     * \return The name of this window
     */
    const std::string& name() const;

    /**
     * \return `true` if a specific tag exists
     */
    bool hasTag(std::string_view tag) const;

    /**
     * \return This window's id
     */
    int id() const;

    /**
     * Get a frame buffer texture. If the texture doesn't exists then it will be created.
     *
     * \param index Index or Engine::TextureIndex enum
     * \return The texture index of selected frame buffer texture
     */
    unsigned int frameBufferTexture(TextureIndex index);

    /**
     * This function returns the screen capture pointer if it's set otherwise nullptr.
     *
     * \param eye Can either be 0 (left) or 1 (right)
     * \return Pointer to screen capture pointer
     */
    ScreenCapture* screenCapturePointer(Eye eye) const;

    /**
     * \return The number of samples used in multisampled anti-aliasing
     */
    int numberOfAASamples() const;

    /**
     * \return The stereo mode
     */
    StereoMode stereoMode() const;

    /**
     * Get the dimensions of the final FBO. Regular viewport rendering renders directly to
     * this FBO but a fisheye renders first a cubemap and then to the final FBO. Post
     * effects are rendered using these dimensions.
     */
    ivec2 finalFBODimensions() const;

    /**
     * Returns pointer to FBO container.
     */
    OffScreenBuffer* fbo() const;

    /**
     * \return The pointer to GLFW window
     */
    GLFWwindow* windowHandle() const;

    const std::vector<std::unique_ptr<Viewport>>& viewports() const;

    /**
     * Get FOV of viewport[0]
     */
    float horizFieldOfViewDegrees() const;

    /**
     * \return Get the window resolution
     */
    ivec2 resolution() const;

    /**
     * \return Get the frame buffer resolution
     */
    ivec2 framebufferResolution() const;

    /**
     * \return Get the initial window resolution
     */
    ivec2 initialResolution() const;

    /**
     * \return Get the scale value (relation between pixel and point size). Normally this
     *         value is 1.f but 2.f on some retina computers.
     */
    vec2 scale() const;

    /**
     * \return The aspect ratio of the window
     */
    float aspectRatio() const;

    /**
     * \return Get the frame buffer bytes per color component (BPCC) count
     */
    int framebufferBPCC() const;

    void renderScreenQuad() const;

    void addViewport(std::unique_ptr<Viewport> vpPtr);

    /**
     * \return `true` if any masks are used
     */
    bool hasAnyMasks() const;

    /**
     * \return `true` if FXAA should be used
     */
    bool useFXAA() const;

    void bindStereoShaderProgram(unsigned int leftTex, unsigned int rightTex) const;

    bool shouldCallDraw2DFunction() const;
    bool shouldCallDraw3DFunction() const;
    int blitWindowId() const;

private:
    enum class TextureType { Color, Depth, Normal, Position };

    void initWindowResolution(ivec2 resolution);

    void initScreenCapture();

    /**
     * This function creates textures that will act as FBO targets.
     */
    void createTextures();
    void generateTexture(unsigned int& id, TextureType type);

    /**
     * This function creates FBOs. This is done in the initOGL function.
     */
    void createFBOs();

    /**
     * This function resizes the FBOs when the window is resized to achive 1:1 mapping.
     */
    void resizeFBOs();

    void destroyFBOs();

    /**
     * Create vertex buffer objects used to render framebuffer quad.
     */
    void createVBOs();
    void loadShaders();
    bool useRightEyeTexture() const;

    std::string _name;
    std::vector<std::string> _tags;

    bool _isVisible = true;
    bool _shouldRenderWhileHidden = false;
    bool _hasFocus = false;
    bool _useFixResolution = false;
    bool _isWindowResolutionSet = false;
    bool _hasCallDraw2DFunction = true;
    bool _hasCallDraw3DFunction = true;
    int _blitWindowId = -1;
    bool _useQuadBuffer = false;
    bool _isFullScreen = false;
    bool _shouldAutoiconify = false;
    bool _hideMouseCursor = false;
    bool _isFloating = false;
    bool _isDoubleBuffered = true;
    bool _setWindowPos = false;
    bool _isDecorated = true;
    bool _isResizable = true;
    bool _isMirrored = false;
    ivec2 _framebufferRes = ivec2{ 512, 256 };
    ivec2 _windowInitialRes = ivec2{ 640, 480 };
    std::optional<ivec2> _pendingWindowRes;
    std::optional<ivec2> _pendingFramebufferRes;
    ivec2 _windowRes = ivec2{ 640, 480 };
    ivec2 _windowPos = ivec2{ 0, 0 };
    ivec2 _windowResOld = ivec2{ 640, 480 };
    int _monitorIndex = 0;
    GLFWwindow* _windowHandle = nullptr;
    float _aspectRatio = 1.f;
    vec2 _scale = vec2{ 0.f, 0.f };

    bool _useFXAA = false;

    ColorBitDepth _bufferColorBitDepth = ColorBitDepth::Depth8;
    unsigned int _internalColorFormat = 0x8814; // = GL_RGBA32F
    unsigned int _colorDataType = 0x1406; // = GL_FLOAT
    int _bytesPerColor = 4;

    struct {
        unsigned int leftEye = 0;
        unsigned int rightEye = 0;
        unsigned int depth = 0;
        unsigned int intermediate = 0;
        unsigned int normals = 0;
        unsigned int positions = 0;
    } _frameBufferTextures;

    std::unique_ptr<ScreenCapture> _screenCaptureLeftOrMono;
    std::unique_ptr<ScreenCapture> _screenCaptureRight;

    StereoMode _stereoMode = StereoMode::NoStereo;
    int _nAASamples = 1;
    int _id = -1;

    unsigned int _vao = 0;
    unsigned int _vbo = 0;

    struct {
        ShaderProgram shader;
        int leftTexLoc = -1;
        int rightTexLoc = -1;
    } _stereo;

    bool _hasAnyMasks = false;

    std::vector<std::unique_ptr<Viewport>> _viewports;
    std::unique_ptr<OffScreenBuffer> _finalFBO;

    static GLFWwindow* _sharedHandle;
    static bool _useSwapGroups;
    static bool _isBarrierActive;
    static bool _isSwapGroupMaster;
};

} // namespace sgct

#endif // __SGCT__WINDOW__H__
