/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__WINDOW__H__
#define __SGCT__WINDOW__H__

#include <sgct/offscreenbuffer.h>
#include <sgct/screencapture.h>
#include <sgct/viewport.h>
#include <glm/glm.hpp>
#include <optional>
#include <vector>

struct GLFWmonitor;
struct GLFWwindow;

namespace sgct {

namespace config { struct Window; }
namespace core { class BaseViewport; }

/// Helper class for window data.
class Window {
public:
    /// Different stereo modes used for rendering
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

    /// The different texture indexes in window buffers
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
     * Init Nvidia swap groups if supported by hardware. Supported hardware is Nvidia
     * Quadro graphics card + sync card or AMD/ATI FireGL graphics card + sync card.
     */
    static void initNvidiaSwapGroups();

    /// Force a restore of the shared OpenGL context
    static void resetSwapGroupFrameNumber();
    static void setBarrier(bool state);
    static bool isBarrierActive();
    static bool isUsingSwapGroups();
    static bool isSwapGroupMaster();
    static unsigned int swapGroupFrameNumber();

    static void makeSharedContextCurrent();

    explicit Window(int id);

    void close();
    void init();

    /// Init window buffers such as textures, FBOs, VAOs, VBOs and PBOs
    void initOGL();

    /// Init context specific data such as viewport corrections/warping meshes
    void initContextSpecificOGL();

    /**
     * Don't use this function if you want to set the window resolution. Use
     * setWindowResolution(const int x, const int y) instead. This function is called
     * within SGCT when the window is created.
     */
    void initWindowResolution(glm::ivec2 resolution);

    /// Swap previous data and current data. This is done at the end of the render loop.
    void swap(bool takeScreenshot);
    void updateResolutions();

    /// \return true if frame buffer is resized and window is visible.
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

    /// Name this window
    void setName(std::string name);

    /// Tag this window. Tags are seperated by comma
    void setTags(std::vector<std::string> tags);

    /**
     * Set the visibility state of this window. If a window is hidden the rendering for
     * that window will be paused unless it's forced to render while hidden by using
     * setRenderWhileHidden().
     */
    void setVisible(bool state);

    /**
     * Set if window should render while hidden. Normally a window pauses the rendering if
     * it's hidden.
     */
    void setRenderWhileHidden(bool state);
    
    /// Set the focued flag for this window (should not be done by user)
    void setFocused(bool state);

    /**
     * Set the window title.
     *
     * \param title The title of the window.
     */
    void setWindowTitle(const char* title);

    /**
     * Sets the window resolution.
     *
     * \param x The width of the window in pixels.
     * \param y The height of the window in pixels.
     */
    void setWindowResolution(glm::ivec2 resolution);

    /**
     * Sets the framebuffer resolution. These parameters will only be used if a fixed
     * resolution is used that is different from the window resolution.
     *
     * \param x The width of the frame buffer in pixels.
     * \param y The height of the frame buffer in pixels.
     */
    void setFramebufferResolution(glm::ivec2 resolution);

    /**
     * Set this window's position in screen coordinates.
     *
     * \param x horizontal position in pixels
     * \param y vertical position in pixels
     */
    void setWindowPosition(glm::ivec2 positions);

    /// Set if fullscreen mode should be used
    void setWindowMode(bool fullscreen);

    /// Set if the window should float (be on top / topmost)
    void setFloating(bool floating);

    /// Set if the window is double buffered (can only be set before window creation)
    void setDoubleBuffered(bool doubleBuffered);

    /// Set if window borders should be visible
    void setWindowDecoration(bool state);

    /// Set which monitor that should be used for fullscreen mode
    void setFullScreenMonitorIndex(int index);

    /// Force the framebuffer to a fixed size which may be different from the window size
    void setFixResolution(bool state);
    void setHorizFieldOfView(float hFovDeg);

    /// Set if FXAA should be used.
    void setUseFXAA(bool state);

    /**
     * Use quad buffer (hardware stereoscopic rendering). This function can only be used
     * before the window is created. The quad buffer feature is only supported on
     * professional CAD graphics cards such as Nvidia Quadro or AMD/ATI FireGL.
     */
    void setUseQuadbuffer(bool state);

    /// Set if the specifed Draw2D function pointer should be called for this window.
    void setCallDraw2DFunction(bool state);

    /// Set if the specifed Draw3D function pointer should be called for this window.
    void setCallDraw3DFunction(bool state);

    /// Set if the the contents of the previous window should be blitted to this window
    void setBlitPreviousWindow(bool state);

    /// Set the number of samples used in multisampled anti-aliasing
    void setNumberOfAASamples(int samples);

    /**
     * Set the stereo mode. Set this mode in your init callback or during runtime in the
     * post-sync-pre-draw callback. GLSL shaders will be recompliled if needed.
     */
    void setStereoMode(StereoMode sm);

    /**
     * Set if fisheye alpha state. Should only be set using XML config of before calling
     * Engine::init.
     */
    void setAlpha(bool state);

    /// Set the color bit depth of the FBO and Screencapture.
    void setColorBitDepth(ColorBitDepth cbd);

    /// \return true if full screen rendering is enabled
    bool isFullScreen() const;

    /// \return true if window is floating/allways on top/topmost
    bool isFloating() const;

    /// \return true if window is double-buffered
    bool isDoubleBuffered() const;

    /// \return this window's focused flag
    bool isFocused() const;

    /// \return if the window is visible or not
    bool isVisible() const;

    /// \return true if the window is set to render while hidden
    bool isRenderingWhileHidden() const;

    /// \return If the frame buffer has a fix resolution this function returns true
    bool isFixResolution() const;

    /// \return true if any kind of stereo is enabled
    bool isStereo() const;

    /// \return true if this window is resized
    bool isWindowResized() const;

    /// \return the name of this window
    const std::string& name() const;

    /// \return true if a specific tag exists
    bool hasTag(const std::string& tag) const;

    /// \return this window's id
    int id() const;

    /**
     * Get a frame buffer texture. If the texture doesn't exists then it will be created.
     *
     * \param index Index or Engine::TextureIndex enum
     * \return texture index of selected frame buffer texture
     */
    unsigned int frameBufferTexture(TextureIndex index);

    /**
     * This function returns the screen capture pointer if it's set otherwise nullptr.
     *
     * \param eye can either be 0 (left) or 1 (right)
     * \return pointer to screen capture pointer
     */
    core::ScreenCapture* screenCapturePointer(Eye eye) const;

    /// \return the number of samples used in multisampled anti-aliasing
    int numberOfAASamples() const;

    /// \return the stereo mode
    StereoMode stereoMode() const;

    /**
     * Get the dimensions of the final FBO. Regular viewport rendering renders directly to
     * this FBO but a fisheye renders first a cubemap and then to the final FBO. Post
     * effects are rendered using these dimensions.
     */
    glm::ivec2 finalFBODimensions() const;

    /// Returns pointer to FBO container
    core::OffScreenBuffer* fbo() const;

    /// \return pointer to GLFW window
    GLFWwindow* windowHandle() const;

    /// \return a reference to a specific viewport
    core::Viewport& viewport(int index);

    /// \return a const reference to a specific viewport
    const core::Viewport& viewport(int index) const;

    /// \return the viewport count for this window
    int numberOfViewports() const;

    /// Enable alpha clear color and 4-component screenshots
    bool hasAlpha() const;

    /// Get the color bit depth of the FBO and Screencapture.
    ColorBitDepth colorBitDepth() const;

    /// Get FOV of viewport[0]
    float horizFieldOfViewDegrees() const;
    
    /// \return Get the window resolution.
    glm::ivec2 resolution() const;

    /// \return Get the frame buffer resolution.
    glm::ivec2 framebufferResolution() const;

    /// \return Get the initial window resolution.
    glm::ivec2 initialResolution() const;

    /**
     * \return Get the scale value (relation between pixel and point size). Normally this
     *         value is 1.f but 2.f on some retina computers.
     */
    glm::vec2 scale() const;

    /// \return the aspect ratio of the window 
    float aspectRatio() const;

    /// \return Get the frame buffer bytes per color component (BPCC) count.
    int framebufferBPCC() const;

    void renderScreenQuad() const;

    void addViewport(std::unique_ptr<core::Viewport> vpPtr);

    /// \return true if any masks are used
    bool hasAnyMasks() const;

    /// \return true if FXAA should be used
    bool useFXAA() const;

    void bindStereoShaderProgram(unsigned int leftTex, unsigned int rightTex) const;

    bool shouldCallDraw2DFunction() const;
    bool shouldCallDraw3DFunction() const;
    bool shouldBlitPreviousWindow() const;

private:
    enum class TextureType { Color, Depth, Normal, Position };

    void initScreenCapture();
    /// This function creates textures that will act as FBO targets.
    void createTextures();
    void generateTexture(unsigned int& id, TextureType type);

    /// This function creates FBOs. This is done in the initOGL function.
    void createFBOs();

    /// This function resizes the FBOs when the window is resized to achive 1:1 mapping
    void resizeFBOs();

    void destroyFBOs();

    /// Create vertex buffer objects used to render framebuffer quad
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
    bool _shouldBitPreviousWindow = false;
    bool _useQuadBuffer = false;
    bool _isFullScreen = false;
    bool _isFloating = false;
    bool _isDoubleBuffered = true;
    bool _setWindowPos = false;
    bool _isDecorated = true;
    bool _hasAlpha = false;
    glm::ivec2 _framebufferRes = glm::ivec2(512, 256);
    glm::ivec2 _windowInitialRes = glm::ivec2(640, 480);
    std::optional<glm::ivec2> _pendingWindowRes;
    std::optional<glm::ivec2> _pendingFramebufferRes;
    glm::ivec2 _windowRes = glm::ivec2(640, 480);
    glm::ivec2 _windowPos = glm::ivec2(0, 0);
    glm::ivec2 _windowResOld = glm::ivec2(640, 480);
    int _monitorIndex = 0;
    GLFWwindow* _windowHandle = nullptr;
    float _aspectRatio = 1.f;
    glm::vec2 _scale = glm::vec2(0.f, 0.f);

    bool _useFXAA = false;

    ColorBitDepth _bufferColorBitDepth = ColorBitDepth::Depth8;
    GLenum _internalColorFormat;
    GLenum _colorFormat;
    GLenum _colorDataType;
    int _bytesPerColor;

    struct {
        unsigned int leftEye = 0;
        unsigned int rightEye = 0;
        unsigned int depth = 0;
        unsigned int intermediate = 0;
        unsigned int normals = 0;
        unsigned int positions = 0;
    } _frameBufferTextures;

    std::unique_ptr<core::ScreenCapture> _screenCaptureLeftOrMono;
    std::unique_ptr<core::ScreenCapture> _screenCaptureRight;

    StereoMode _stereoMode = StereoMode::NoStereo;
    int _nAASamples = 1;
    int _id;

    unsigned int _vao = 0;
    unsigned int _vbo = 0;

    struct {
        ShaderProgram shader;
        int leftTexLoc = -1;
        int rightTexLoc = -1;
    } _stereo;

    bool _hasAnyMasks = false;

    std::vector<std::unique_ptr<core::Viewport>> _viewports;
    std::unique_ptr<core::OffScreenBuffer> _finalFBO;

    static GLFWwindow* _sharedHandle;
    static bool _useSwapGroups;
    static bool _isBarrierActive;
    static bool _isSwapGroupMaster;
};

} // namespace sgct

#endif // __SGCT__WINDOW__H__
