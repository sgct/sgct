/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__WINDOW__H__
#define __SGCT__WINDOW__H__

#include <sgct/sgctexports.h>
#include <sgct/shaderprogram.h>
#include <sgct/viewport.h>
#include <functional>
#include <optional>
#include <string_view>
#include <vector>

struct GLFWwindow;

namespace sgct {

namespace config { struct Window; }

class OffScreenBuffer;
class ScreenCapture;

class SGCT_EXPORT Window {
public:
    /// Different stereo modes used for rendering.
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

    Window(const config::Window& window);

    static void makeSharedContextCurrent();

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
    static unsigned int swapGroupFrameNumber();

    ~Window() = default;

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
    void swapBuffers(bool takeScreenshot);
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

    void draw();

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
     * Set the window title.
     *
     * \param title The title of the window
     */
    void setWindowTitle(std::string title);

    /**
     * Sets the framebuffer resolution. These parameters will only be used if a fixed
     * resolution is used that is different from the window resolution.
     *
     * \param resolution The width and height of the frame buffer in pixels
     */
    void setFramebufferResolution(ivec2 resolution);

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
     * Set which monitor that should be used for fullscreen mode.
     */
    void setFullScreenMonitorIndex(int index);

    /**
     * Force the framebuffer to a fixed size which may be different from the window size.
     */
    void setFixResolution(bool state);
    void setHorizFieldOfView(float hFovDeg);

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

    unsigned int frameBufferTextureEye(Eye eye) const;
    unsigned int frameBufferTextureIntermediate() const;
    unsigned int frameBufferTextureDepth() const;
    unsigned int frameBufferTextureNormals() const;
    unsigned int frameBufferTexturePositions() const;

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
     * \return Get the scale value (relation between pixel and point size). Normally this
     *         value is 1.f but 2.f on some retina computers.
     */
    vec2 scale() const;

    /**
     * \return The aspect ratio of the window
     */
    float aspectRatio() const;

    void renderScreenQuad() const;

    void addViewport(std::unique_ptr<Viewport> vpPtr);


    bool shouldTakeScreenshot() const;

    // Returns true if this window has any settings that require a fallback on an OpenGL
    // compatibility profile
    bool needsCompatibilityProfile() const;

    /**
     * Draw viewport overlays if there are any. This function renders stats, OSD and overlays of the provided \p window and using
      * the provided \p frustum.
     *
     * \param window The Window object for which the overlays should be drawn
     * \param frustum The frustum for which the overlay should be drawn
     */
    void render2D(FrustumMode frustum) const;

    /**
     * Draw geometry and bind FBO as texture in screenspace (ortho mode). The geometry can
     * be a simple quad or a geometry correction and blending mesh.
     *
     * \param window The Window whose geometry should be drawn
     */
    void renderFBOTexture();

    void updateFrustums(float nearClip, float farClip);

private:
    enum class TextureType { Color, Depth, Normal, Position };

    Window(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(const Window&) = delete;
    Window& operator=(Window&&) = delete;

    void initWindowResolution(ivec2 resolution);

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

    /**
     * Causes all of the viewports of the provided \p window be rendered with the
     * \p frustum into the texture behind the provided \p ti texture index.
     *
     * \param window The window whose viewports should be rendered
     * \param frustum The frustum that should be used to render the viewports
     * \param eye The eye that should be rendered
     */
    void renderViewports(FrustumMode frustum, Eye eye) const;

    /**
     * This function combines a texture and a shader into a new texture while applying
     * fast anti-aliasing (FXAA).
     *
     * \param window The Window object for which the FXAA operation should be performed
     * \param eye The eye that should be rendered
     */
    void renderFXAA(Eye eye) const;


    /**
     * This function copies/render the result from the previous window same viewport (if
     * it exists) into this window.
     *
     * \param prevWindow The source window whose content should be copied into the
     *        \p window
     * \param window The destination into which the contents of the \p prevWindow is
     *        copied
     * \param viewport The viewport of the window that should be compied
     * \param mode The frustum that should be used to copy the window contents
     *
     * \pre The \p prevWindow and \p window must be different Window objects
     */
    void blitWindowViewport(const Window& prevWindow, const Viewport& viewport,
        FrustumMode mode) const;

    std::string _name;
    int _id = -1;
    std::vector<std::string> _tags;

    bool _hideMouseCursor;
    bool _takeScreenshot;
    bool _hasCallDraw2DFunction;
    bool _hasCallDraw3DFunction;
    bool _isFullScreen;
    bool _shouldAutoiconify;
    bool _isFloating;
    bool _shouldRenderWhileHidden;
    int _nAASamples;
    bool _useFXAA;
    bool _isDecorated;
    bool _isResizable;
    bool _isMirrored;
    int _blitWindowId;
    int _monitorIndex;
    bool _mirrorX;
    bool _mirrorY;
    bool _noError;
    bool _isVisible;
    StereoMode _stereoMode;
    std::optional<ivec2> _windowPos;

    bool _hasFocus = false;
    bool _useFixResolution = false;
    ivec2 _framebufferRes = ivec2{ 512, 256 };
    std::optional<ivec2> _pendingWindowRes;
    bool _windowResChanged = false;
    std::optional<ivec2> _pendingFramebufferRes;
    std::optional<ivec2> _windowRes;
    GLFWwindow* _windowHandle = nullptr;
    float _aspectRatio = 1.f;
    vec2 _scale = vec2{ 0.f, 0.f };


    const unsigned int _internalColorFormat;
    const unsigned int _colorDataType;
    const int _bytesPerColor = 4;

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


    unsigned int _vao = 0;
    unsigned int _vbo = 0;

    ShaderProgram _fboQuad;
    ShaderProgram _overlay;

    struct {
        ShaderProgram shader;
        int leftTexLoc = -1;
        int rightTexLoc = -1;
    } _stereo;

    struct FXAAShader {
        ShaderProgram shader;
        int sizeX = -1;
        int sizeY = -1;
        int subPixTrim = -1;
        int subPixOffset = -1;
    };
    std::optional<FXAAShader> _fxaa;


    bool _hasAnyMasks = false;

    std::vector<std::unique_ptr<Viewport>> _viewports;
    std::unique_ptr<OffScreenBuffer> _finalFBO;

    static GLFWwindow* _sharedHandle;
    static bool _useSwapGroups;
    static bool _isBarrierActive;


    // ScalableMesh
    struct {
        void* sdk = nullptr;
        std::string path;
    } _scalableMesh;
};

} // namespace sgct

#endif // __SGCT__WINDOW__H__
