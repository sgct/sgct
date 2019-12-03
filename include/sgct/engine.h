/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__ENGINE__H__
#define __SGCT__ENGINE__H__

#include <sgct/actions.h>
#include <sgct/config.h>
#include <sgct/frustum.h>
#include <sgct/joystick.h>
#include <sgct/keys.h>
#include <sgct/modifiers.h>
#include <sgct/mouse.h>
#include <sgct/networkmanager.h>
#include <sgct/screencapture.h>
#include <sgct/shadermanager.h>
#include <sgct/window.h>
#include <array>
#include <functional>
#include <optional>

struct GLFWwindow;

namespace sgct {

struct Configuration;
class PostFX;
class TrackingManager;
class Window;

namespace core {
    class Image;
    class Node;
    class StatisticsRenderer;
    class User;
} // namespace core

config::Cluster loadCluster(std::optional<std::string> path);

/**
 * The Engine class is the central part of sgct and handles most of the callbacks,
 * rendering, network handling, input devices etc.
 */
class Engine {
public:
    /// The different run modes used by the init function
    enum class Profile { 
        OpenGL_3_3_Core,
        OpenGL_4_0_Core,
        OpenGL_4_1_Core,
        OpenGL_4_2_Core,
        OpenGL_4_3_Core,
        OpenGL_4_4_Core,
        OpenGL_4_5_Core,
        OpenGL_4_6_Core
    };

    enum class RenderTarget { WindowBuffer, NonLinearBuffer };

    struct Statistics {
        static inline const int HistoryLength = 512;

        std::array<double, HistoryLength> frametimes = {};
        std::array<double, HistoryLength> drawTimes = {};
        std::array<double, HistoryLength> syncTimes = {};
        std::array<double, HistoryLength> loopTimeMin = {};
        std::array<double, HistoryLength> loopTimeMax = {};
    };

    struct Callbacks {
        /// This function is called before the window is created (before OpenGL context is
        /// created). At this stage the configuration file has been read and network
        /// is initialized.
        std::function<void()> preWindow;

        /// This function is called directly after all SGCT windows are created. This
        // enables the user to create additional OpenGL context for multithreaded OpenGL.
        std::function<void(GLFWwindow*)> contextCreation;

        /// This function is called once before the starting the render loop and after
        /// creation of the OpenGL context.
        std::function<void()> initOpenGL;

        /// This function is called before the synchronization stage
        std::function<void()> preSync;

        /// This function is called once per frame after sync but before draw stage
        std::function<void()> postSyncPreDraw;

        /// This function draws the scene and could be called several times per frame
        /// as it's called once per viewport and once per eye if stereoscopy is used.
        std::function<void()> draw;

        /// This function is be called after overlays and post effects has been drawn and
        /// can used to render text and HUDs that will not be filtered or antialiased.
        std::function<void()> draw2D;

        /// This function is called after the draw stage but before the OpenGL buffer swap
        std::function<void()> postDraw;

        /// This is called before all SGCT components will be destroyed
        std::function<void()> cleanUp;

        /// This function is called to encode all shared data that is sent to the
        /// connected nodes in a clustered setup.
        std::function<void()> encode;

        /// This function is called by decode all shared data sent to us from the master
        std::function<void()> decode;

        /// This function is called when a TCP message is received
        std::function<void(const char*, int)> externalDecode;

        /// This function is called when the connection status changes
        std::function<void(bool)> externalStatus;

        /// This function is called when a TCP message is received
        std::function<void(void*, int, int, int)> dataTransferDecode;

        /// This function is called when the connection status changes
        std::function<void(bool, int)> dataTransferStatus;

        /// This function is called when data is successfully sent
        std::function<void(int, int)> dataTransferAcknowledge;

        /// This function sets the keyboard callback (GLFW wrapper) for all windows
        std::function<void(Key, Modifier, Action, int)> keyboard;

        /// All windows are connected to this callback.
        /// @TODO (abock, 2019-12-02) Check if we really want to keep/need this
        std::function<void(unsigned int, int)> character;

        /// This function sets the mouse button callback (GLFW wrapper) for all windows
        std::function<void(MouseButton, Modifier, Action)> mouseButton;

        /// All windows are connected to this callback.
        std::function<void(double, double)> mousePos;

        /// All windows are connected to this callback.
        std::function<void(double, double)> mouseScroll;

        /// Drop files to any window. All windows are connected to this callback.
        std::function<void(int, const char**)> drop;
    };

    static Engine& instance();

    /**
     * \param cluster The cluster setup that should be used for this SGCT run
     * \param callbacks The list of callbacks that should be installed
     */
    static void create(config::Cluster cluster, Callbacks callbacks,
        const Configuration& arg, Profile profile = Profile::OpenGL_3_3_Core);
    static void destroy();

    /// Terminates SGCT
    void terminate();

    /// This is SGCT's renderloop where rendering & synchronization takes place
    void render();

    /// Returns the statistic object containing all information about the frametimes, etc
    const Statistics& getStatistics() const;

    /// \return the frame time (delta time) in seconds
    double getDt() const;

    /// \return the average frames per second
    double getAvgFPS() const;

    /// \return the average frame time (delta time) in seconds
    double getAvgDt() const;

    /// \return the minimum frame time (delta time) in the averaging window (seconds)
    double getMinDt() const;
    
    /// \return the maximum frame time (delta time) in the averaging window (seconds)
    double getMaxDt() const;

    /// \return the standard devitation of the delta time in seconds
    double getDtStandardDeviation() const;

    /// \return the clear color as 4 floats (RGBA)
    glm::vec4 getClearColor() const;
    
    /// \return the near clipping plane distance in meters
    float getNearClipPlane() const;

    /// \return the far clipping plane distance in meters
    float getFarClipPlane() const;

    /**
     * Set the near and far clipping planes. This operation recalculates all frustums for
     * all viewports.
     *
     * \param nearClippingPlane near clipping plane in meters
     * \param farClippingPlane far clipping plane in meters
     */
    void setNearAndFarClippingPlanes(float nearClippingPlane, float farClippingPlane);

    /**
     * Set the eye separation (interocular distance) for all users. This operation
     * recalculates all frustums for all viewports.
     *
     * \param eyeSeparation eye separation in meters
     */
    void setEyeSeparation(float eyeSeparation);

    /**
     * Set the clear color (background color).
     *
     * \param color the clear color
     */
    void setClearColor(glm::vec4 color);

    /**
     * This functions updates the frustum of all viewports on demand. However if the
     * viewport is tracked this is done on the fly.
     */
    void updateFrustums();

    /// \return the active draw textureo
    unsigned int getCurrentDrawTexture() const;

    /// \return the resolution in pixels for the active window's framebuffer
    glm::ivec2 getCurrentResolution() const;

    /// \return the index of the focus window. If no window has focus, 0 is returned
    int getFocusedWindowIndex() const;

    /// Sets if the statistics graph should be rendered or not
    void setStatsGraphVisibility(bool state);

    /**
     * Take an RGBA screenshot and save it as a PNG file. If stereo rendering is enabled
     * then two screenshots will be saved per frame, one for each eyeo.
     *
     * To record frames for a movie simply call this function every frame you wish to
     * record. The read to disk is multi-threaded.
     */
    void takeScreenshot();

    /// Set the screenshot number (file index)
    void setScreenShotNumber(unsigned int number);

    /// \return the current screenshot number (file index)
    unsigned int getScreenShotNumber() const;

    /**
     * This function returns the currently assigned draw function to be used in internal
     * classes that need to repeatedly call this. In general, there is no need for
     * external applications to store the draw function, but they are free to do so. Be
     * aware that the user (i.e. you) is allowed to change the draw function at any time.
     *
     * \return The currently bound draw function
     */
    const std::function<void()>& getDrawFunction() const;

    /**
     * This function sends a message to the external control interface.
     *
     * \param data a pointer to the data buffer
     * \param length is the number of bytes of data that will be sent
     */
    void sendMessageToExternalControl(const void* data, int length);

    /// Check if the external control is connected.
    bool isExternalControlConnected() const;

    /**
     * Don't use this. This function is called from Network and will invoke the external
     * network callback when messages are received.
     */
    void invokeDecodeCallbackForExternalControl(const char* receivedData,
        int receivedLength, int clientId);

    /**
     * Don't use this. This function is called from Network and will invoke the external
     * network update callback when connection is connected/disconnected.
     */
    void invokeUpdateCallbackForExternalControl(bool connected);

    /**
     * This function sends data between nodes.
     *
     * \param data a pointer to the data buffer
     * \param length is the number of bytes of data that will be sent
     * \param packageId is the identification id of this specific package
     */
    void transferDataBetweenNodes(const void* data, int length, int packageId);

    /**
     * Don't use this. This function is called from Network and will invoke the data
     * transfer callback when messages are received.
     */
    void invokeDecodeCallbackForDataTransfer(void* d, int len, int package, int id);

    /**
     * Don't use this. This function is called from Network and will invoke the data
     * transfer callback when connection is connected/disconnected.
     */
    void invokeUpdateCallbackForDataTransfer(bool connected, int clientId);

    /**
     * Don't use this. This function is called from Network and will invoke the data
     * transfer callback when data is successfully sent.
     */
    void invokeAcknowledgeCallbackForDataTransfer(int packageId, int clientId);

    /// Get the time from program start in seconds
    static double getTime();

    /**
     * Checks the keyboard if the specified key has been pressed.
     *
     * \param winIndex specifies which window to poll
     * \param key specifies which key to check
     * \return SGCT_PRESS or SGCT_RELEASE
     */
    static int getKey(int winIndex, int key);

    /**
     * Checks if specified mouse button has been pressed.
     *
     * \param winIndex specifies which window to poll
     * \param button specifies which button to check
     * \return SGCT_PRESS or SGCT_RELEASE
     */
    static int getMouseButton(int winIndex, int button);
    
    /**
     * Get the mouse position.
     *
     * \param winIndex specifies which window to poll
     * \param xPos x screen coordinate
     * \param yPos y screen coordinate
     */
    static void getMousePos(int winIndex, double* xPos, double* yPos);

    /**
     * Returns the name of the requested joystick
     *
     * \param joystick is the joystick id. Available IDs are in the joystick.h
     */
    static const char* getJoystickName(Joystick joystick);

    /**
     * \param joystick the joystick id: Available IDs are in the joystick.h
     * \param numOfValues is the number of analog axes
     * \return the analog float values (array)
     */
    static const float* getJoystickAxes(Joystick joystick, int* numOfValues);

    /**
     * \param joystick the joystick id: Available IDs are in the joystick.h
     * \param numOfValues is the number of buttons
     * \return the button values (array)
     */
    static const unsigned char* getJoystickButtons(Joystick joystick, int* numOfValues);

    /// \return a reference to this node (running on this computer).
    const sgct::core::Node& getThisNode() const;

    /// \return a pointer to a specified window by index on this node.
    Window& getWindow(int index) const;

    /// \return the number of windows for this node.
    int getNumberOfWindows() const;

    /// \return a pointer to the current window that is being rendered
    Window& getCurrentWindow() const;

    /// \return a pointer to the user (observer position) object
    static core::User& getDefaultUser();

    /// \return true if this node is the master
    bool isMaster() const;

    /**
     * \return the active frustum mode which can be one of the following:
     *   - Mono
     *   - Stereo Left
     *   - Stereo Right
     */
    core::Frustum::Mode getCurrentFrustumMode() const;

    /**
     * \return the projection matrix (only valid inside in the draw callback function)
     */
    const glm::mat4& getCurrentProjectionMatrix() const;

    /**
     * \return the view matrix (only valid inside in the draw callback function)
     */
    const glm::mat4& getCurrentViewMatrix() const;

    /**
     * Returns the scene transform specified in the XML configuration, default is a
     * identity matrix
     */
    const glm::mat4& getModelMatrix() const;

    /**
     * Returns the active VP = Projection * View matrix (only valid inside in the draw
     * callback function)
     */
    const glm::mat4& getCurrentViewProjectionMatrix() const;

    /**
     * Returns the active MVP = Projection * View * Model matrix (only valid inside in the
     * draw callback function)
     */
    glm::mat4 getCurrentModelViewProjectionMatrix() const;
    
    /**
     * Returns the active MV = View * Model matrix (only valid inside in the draw callback
     * function)
     */
    glm::mat4 getCurrentModelViewMatrix() const;

    /// Returns the current frame number
    unsigned int getCurrentFrameNumber() const;

    /// Get the active viewport size in pixels.
    glm::ivec2 getCurrentViewportSize() const;

    /**
     * Specifies the sync parameters to be used in the rendering loop.
     *
     * \param printMessage If <code>true</code> a message is print waiting for a frame
     *        every second
     * \param timeout The timeout that the master and clients will wait for in seconds
     */
    void setSyncParameters(bool printMessage = true, float timeout = 60.f);

    /**
     * Set up the current viewport, sets the framebuffer resolutions, windowing and
     * scissoring in OpenGL. This is a function that will be called by internal classes of
     * SGCT and in general does not have to be called by any external application using
     * this library.
     */
    void enterCurrentViewport();

private:
    static Engine* _instance;

    Engine(config::Cluster cluster, Callbacks callbacks, const Configuration& arg);

    /// Engine destructor destructs GLFW and releases resources/memory.
    ~Engine();

    void initialize(Profile profile);

    /// Initiates network communication.
    void initNetwork();

    /// Create and initiate a window.
    void initWindows(Profile rm);

    /**
     * Locks the rendering thread for synchronization. Locks the clients until data is
     * successfully received.
     */
    void frameLockPreStage();

    /**
     * Locks the rendering thread for synchronization. Locks master until clients are
     * ready to swap buffers.
     */
    void frameLockPostStage();

    /// Draw viewport overlays if there are any.
    void drawOverlays();

    /**
     * Draw geometry and bind FBO as texture in screenspace (ortho mode). The geometry can
     * be a simple quad or a geometry correction and blending mesh.
     */
    void renderFBOTexture();
    
    /// This function combines a texture and a shader into a new texture
    void renderPostFX(Window::TextureIndex targetIndex);

    void renderViewports(Window::TextureIndex ti);

    /// This function renders stats, OSD and overlays
    void render2D();

    /// This function attaches targets to FBO if FBO is in use
    void prepareBuffer(Window::TextureIndex ti);

    /// This function updates the renderingtargets.
    void updateRenderingTargets(Window::TextureIndex ti);

    /**
     * This function waits for all windows to be created on the whole cluster in order to
     * set the barrier (hardware swap-lock). Under some Nvidia drivers the stability is
     * improved by first join a swapgroup and then set the barrier then all windows in a
     * swapgroup are created.
     */
    void waitForAllWindowsInSwapGroupToOpen();

    /**
     * This function copies/render the result from the previous window same viewport (if
     * it exists) into this window
     */
    void blitPreviousWindowViewport(core::Frustum::Mode mode);

    // @TODO (abock, 2019-12-02) This is a workaround for the fact that something in SGCT
    // is using the callbacks during the application shutdown. The previous method was to
    // set the callbacks to nullptr, but I really want to make them constant, which
    // prevents that approach from working
    const std::function<void()> _preWindowFn;
    const std::function<void(GLFWwindow*)> _contextCreationFn;
    const std::function<void()> _initOpenGLFn;
    const std::function<void()> _preSyncFn;
    const std::function<void()> _postSyncPreDrawFn;
    const std::function<void()> _drawFn;
    const std::function<void()> _draw2DFn;
    const std::function<void()> _postDrawFn;
    const std::function<void()> _cleanUpFn;
    std::function<void(const char*, int)> _externalDecodeFn;
    std::function<void(bool)> _externalStatusFn;
    std::function<void(void*, int, int, int)> _dataTransferDecodeFn;
    std::function<void(bool, int)> _dataTransferStatusFn;
    std::function<void(int, int)> _dataTransferAcknowledgeFn;
    
    float _nearClipPlane = 0.1f;
    float _farClipPlane = 100.f;
    glm::vec4 _clearColor = glm::vec4(0.f, 0.f, 0.f, 1.f);

    core::Frustum::Mode _currentFrustumMode = core::Frustum::Mode::MonoEye;
    glm::ivec2 _currentViewportSize = glm::ivec2(640, 480);
    int _currentWindowIndex = 0;

    Statistics _statistics;
    double _statsPrevTimestamp = 0.0;
    std::unique_ptr<core::StatisticsRenderer> _statisticsRenderer;

    bool _checkOpenGLCalls = false;
    bool _createDebugContext = false;
    bool _checkFBOs = false;
    bool _takeScreenshot = false;
    bool _shouldTerminate = false;
    bool _renderingOffScreen = false;

    bool _printSyncMessage = true;
    float _syncTimeout = 60.f;

    struct FXAAShader {
        ShaderProgram shader;
        int sizeX = -1;
        int sizeY = -1;
        int subPixTrim = -1;
        int subPixOffset = -1;
    };
    std::optional<FXAAShader> _fxaa;
    ShaderProgram _fboQuad;
    ShaderProgram _overlay;

    std::unique_ptr<std::thread> _thread;

    bool _isRunning = true;

    unsigned int _frameCounter = 0;
    unsigned int _shotCounter = 0;

    core::NetworkManager::NetworkMode _networkMode =
        core::NetworkManager::NetworkMode::Remote;
};

} // namespace sgct

#endif // __SGCT__ENGINE__H__
