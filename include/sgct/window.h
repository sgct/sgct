/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__WINDOW__H__
#define __SGCT__WINDOW__H__

#include <sgct/engine.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/postfx.h>
#include <sgct/screencapture.h>
#include <sgct/viewport.h>
#include <glm/glm.hpp>
#include <vector>

struct GLFWmonitor;
struct GLFWwindow;

namespace sgct {

namespace config { struct Window; }
namespace core {
    class BaseViewport;
    class ScreenCapture;
} // namespace core

/**
 * Helper class for window data.
 */
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

    enum class Context { Shared = 0, Window, Unset };

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
        FX1,
        FX2,
        Depth,
        Normals,
        Positions
    };

    void applyWindow(const config::Window& window);

    /**
     * Init Nvidia swap groups if they are supported by hardware. Supported hardware is
     * Nvidia Quadro graphics card + sync card or AMD/ATI FireGL graphics card + sync
     * card.
     */
    static void initNvidiaSwapGroups();

    /// Force a restore of the shared OpenGL context
    static void restoreSharedContext();
    static void resetSwapGroupFrameNumber();
    static void setBarrier(bool state);
    static bool isBarrierActive();
    static bool isUsingSwapGroups();
    static bool isSwapGroupMaster();
    static unsigned int getSwapGroupFrameNumber();

    explicit Window(int id);

    void close();
    void init();

    /// Init window buffers such as textures, FBOs, VAOs, VBOs and PBOs.
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
    bool update();

    /**
     * This function is used internally within sgct to open the window.
     *
     * /return True if window was created successfully.
     */
    bool openWindow(GLFWwindow* share, int lastWindowIdx);

    /**
     * Set this window's OpenGL context or the shared context as current. This function
     * keeps track of which context is in use and only set the context to current if it's
     * not.
     */
    void makeOpenGLContextCurrent(Context context);

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

    /// Set the iconified flag for this window (should not be done by user)
    void setIconified(bool state);

    /**
     * Set the window title.
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
     * This might be useful in fullscreen mode on Apples retina displays to force 1080p
     * resolution or similar.
     *
     * \param x The width of the frame buffer in pixels.
     * \param y The height of the frame buffer in pixels.
     */
    void setFramebufferResolution(glm::ivec2 resolution);

    /**
     * Set this window's position in screen coordinates
     *
     * \param x horizontal position in pixels
     * \param y vertical position in pixels
     */
    void setWindowPosition(glm::ivec2 positions);

    /// Set if fullscreen mode should be used
    void setWindowMode(bool fullscreen);

    /// Set if the window should float (be on top / topmost)
    void setFloating(bool floating);

    /**
     * Set if the window should be double buffered (can only be set before window is
     * created)
     */
    void setDoubleBuffered(bool doubleBuffered);

    /// Set if window borders should be visible
    void setWindowDecoration(bool state);

    /// Set which monitor that should be used for fullscreen mode
    void setFullScreenMonitorIndex(int index);

    /**
     * Force the framebuffer to have a fixed size which may be different from the window
     * size.
     */
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
     * Set the which viewport that is the current. This is done from the Engine and end
     * users shouldn't change this
     */
    void setCurrentViewport(size_t index);

    /**
     * Set the which viewport that is the current. This is done internally from SGCT and
     * end users shouldn't change this
     */
    void setCurrentViewport(core::BaseViewport* vp);

    /**
     * Set if fisheye alpha state. Should only be set using XML config of before calling
     * Engine::init.
     */
    void setAlpha(bool state);

    /// Set monitor gamma (works only if fullscreen)
    void setGamma(float gamma);

    /// Set monitor contrast in range [0.5, 1.5] (works only if fullscreen)
    void setContrast(float contrast);

    /// Set monitor brightness in range [0.5, 1.5] (works only if fullscreen)
    void setBrightness(float brightness);

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

    /// \return this window's inconify flag 
    bool isIconified() const;

    /// \return if the window is visible or not
    bool isVisible() const;

    /// \return true if the window is set to render while hidden
    bool isRenderingWhileHidden() const;

    /// \return If the frame buffer has a fix resolution this function returns true.
    bool isFixResolution() const;

    /// \return true if any kind of stereo is enabled
    bool isStereo() const;

    /// \return true if this window is resized
    bool isWindowResized() const;

    /// \return the name of this window
    const std::string& getName() const;

    /// \return the tags of this window
    const std::vector<std::string>& getTags() const;

    /// \return true if a specific tag exists
    bool hasTag(const std::string& tag) const;

    /// \return this window's id
    int getId() const;

    /**
     * Get a frame buffer texture. If the texture doesn't exists then it will be created.
     *
     * \param index Index or Engine::TextureIndex enum
     * \return texture index of selected frame buffer texture
     */
    unsigned int getFrameBufferTexture(Engine::TextureIndex index);

    /**
     * This function returns the screen capture pointer if it's set otherwise nullptr.
     *
     * \param eye can either be 0 (left) or 1 (right)
     * \return pointer to screen capture ptr
     */
    core::ScreenCapture* getScreenCapturePointer(Eye eye) const;

    /// \return the number of samples used in multisampled anti-aliasing
    int getNumberOfAASamples() const;

    /**
     * Returns the stereo mode. The value can be compared to the
     * sgct::core::ClusterManager::StereoMode enum
     */
    StereoMode getStereoMode() const;

    /**
     * Get the dimensions of the final FBO. Regular viewport rendering renders directly to
     * this FBO but a fisheye renders first a cubemap and then to the final FBO. Post
     * effects are rendered using these dimensions.
     */
    glm::ivec2 getFinalFBODimensions() const;

    /// Returns pointer to FBO container
    core::OffScreenBuffer* getFBO() const;

    /// \return pointer to GLFW monitor
    GLFWmonitor* getMonitor() const;

    /// \return pointer to GLFW window
    GLFWwindow* getWindowHandle() const;

    /// \return a pointer to the viewport that is beeing rendered to at the moment
    core::BaseViewport* getCurrentViewport() const;

    /// \return a reference to a specific viewport
    core::Viewport& getViewport(int index);

    /// \return a reference to a specific viewport, checking validity of reference
    core::Viewport& getViewport(int index, bool& validReference);

    /// \return a const reference to a specific viewport
    const core::Viewport& getViewport(int index) const;

    /// \return a const reference to a specific viewport, checking validity of reference
    const core::Viewport& getViewport(int index, bool& validReference) const;

    /// Get the current viewport data in pixels.
    glm::ivec4 getCurrentViewportPixelCoords() const;

    /// \return the viewport count for this window
    int getNumberOfViewports() const;

    /// Enable alpha clear color and 4-component screenshots
    bool hasAlpha() const;

    /// Get monitor gamma value (works only if fullscreen)
    float getGamma() const;

    /// Get monitor contrast value (works only if fullscreen)
    float getContrast() const;

    /// Get monitor brightness value (works only if fullscreen)
    float getBrightness() const;

    /// Get the color bit depth of the FBO and Screencapture.
    ColorBitDepth getColorBitDepth() const;

    /// Get FOV of viewport[0]
    float getHorizFieldOfViewDegrees() const;
    
    /// \return the pointer to a specific post effect
    PostFX& getPostFX(int index);

    /// \return the number of post effects
    int getNumberOfPostFXs() const;

    /// \return Get the window resolution.
    glm::ivec2 getResolution() const;

    /// \return Get the frame buffer resolution.
    glm::ivec2 getFramebufferResolution() const;

    /// \return Get the initial window resolution.
    glm::ivec2 getInitialResolution() const;

    /**
     * \return Get the scale value (relation between pixel and point size). Normally this
     *         value is 1.f but 2.f on some retina computers.
     */
    glm::vec2 getScale() const;

    /// \return the aspect ratio of the window 
    float getAspectRatio() const;

    /// \return Get the frame buffer bytes per color component (BPCC) count.
    int getFramebufferBPCC() const;

    void bindVAO() const;
    void bindVBO() const;
    void unbindVBO() const;
    void unbindVAO() const;

    /// Add a post effect for this window
    void addPostFX(PostFX fx);
    void addViewport(std::unique_ptr<core::Viewport> vpPtr);

    /// \return true if any masks are used
    bool hasAnyMasks() const;

    /// \return true if FXAA should be used
    bool useFXAA() const;

    /// \return true if PostFX pass should be used
    bool usePostFX() const;

    void bindStereoShaderProgram() const;
    int getStereoShaderLeftTexLoc() const;
    int getStereoShaderRightTexLoc() const;

    bool shouldCallDraw2DFunction() const;
    bool shouldCallDraw3DFunction() const;
    bool shouldBlitPreviousWindow() const;

private:
    enum class TextureType { Color, Depth, Normal, Position };

    void initScreenCapture();
    /// This function creates textures that will act as FBO targets.
    void createTextures();
    void generateTexture(unsigned int& id, TextureType type);

    /**
     * This function creates FBOs if they are supported. This is done in the initOGL
     * function.
     */
    void createFBOs();

    /**
     * This function resizes the FBOs when the window is resized to achive 1:1 pixel-texel
     * mapping.
     */
    void resizeFBOs();

    void destroyFBOs();

    /// Create vertex buffer objects used to render framebuffer quad
    void createVBOs();
    void loadShaders();
    void updateTransferCurve();
    void updateColorBufferData();
    bool useRightEyeTexture() const;

    std::string _name;
    std::vector<std::string> _tags;

    bool _isVisible = true;
    bool _renderWhileHidden = false;
    bool _hasFocus = false;
    bool _isIconified = false;
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
    bool _hasPendingWindowRes = false;
    glm::ivec2 _pendingWindowRes = glm::ivec2(0, 0);
    bool _hasPendingFramebufferRes = false;
    glm::ivec2 _pendingFramebufferRes = glm::ivec2(0, 0);
    glm::ivec2 _windowRes = glm::ivec2(640, 480);
    glm::ivec2 _windowPos = glm::ivec2(0, 0);
    glm::ivec2 _windowResOld = glm::ivec2(640, 480);
    int _monitorIndex = 0;
    GLFWmonitor* _monitor = nullptr;
    GLFWwindow* _windowHandle = nullptr;
    float _aspectRatio = 1.f;
    float _gamma = 1.f;
    float _contrast = 1.f;
    float _brightness = 1.f;
    glm::vec2 _scale = glm::vec2(0.f, 0.f);

    bool _useFXAA;

    ColorBitDepth _bufferColorBitDepth = ColorBitDepth::Depth8;
    GLenum _internalColorFormat;
    GLenum _colorFormat;
    GLenum _colorDataType;
    int _bytesPerColor;

    struct {
        unsigned int leftEye = 0;
        unsigned int rightEye = 0;
        unsigned int depth = 0;
        unsigned int fx1 = 0;
        unsigned int fx2 = 0;
        unsigned int intermediate = 0;
        unsigned int normals = 0;
        unsigned int positions = 0;
    } _frameBufferTextures;

    std::unique_ptr<core::ScreenCapture> _screenCaptureLeftOrMono;
    std::unique_ptr<core::ScreenCapture> _screenCaptureRight;

    StereoMode _stereoMode = StereoMode::NoStereo;
    int _nAASamples;
    int _id;

    unsigned int _vao = 0;
    unsigned int _vbo = 0;

    //Shaders
    struct {
        ShaderProgram shader;
        int leftTexLoc = -1;
        int rightTexLoc = -1;
    } _stereo;

    bool _hasAnyMasks = false;

    core::BaseViewport* _currentViewport = nullptr;
    std::vector<std::unique_ptr<core::Viewport>> _viewports;
    std::vector<PostFX> _postFXPasses;
    std::unique_ptr<core::OffScreenBuffer> _finalFBO;

    static GLFWwindow* _sharedHandle;
    static GLFWwindow* _currentContextOwner;
    static bool _useSwapGroups;
    static bool _isBarrierActive;
    static bool _isSwapGroupMaster;
};

} // namespace sgct

#endif // __SGCT__WINDOW__H__
