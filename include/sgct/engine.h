/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__ENGINE__H__
#define __SGCT__ENGINE__H__

#include <sgct/config.h>
#include <sgct/frustum.h>
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
    class Touch;
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
    enum class RunMode { 
        /// Using a programmable OpenGL 3.3 pipeline using a core profile
        Default_Mode = 0,
        OpenGL_3_3_Core_Profile = 0,
        /// Using a programmable OpenGL 4.0 pipeline using a core profile
        OpenGL_4_0_Core_Profile,
        /// Using a programmable OpenGL 4.1 pipeline using a core profile
        OpenGL_4_1_Core_Profile,
        /// Using a programmable OpenGL 4.2 pipeline using a core profile
        OpenGL_4_2_Core_Profile,
        /// Using a programmable OpenGL 4.3 pipeline using a core profile
        OpenGL_4_3_Core_Profile,
        /// Using a programmable OpenGL 4.4 pipeline using a core profile
        OpenGL_4_4_Core_Profile,
        /// Using a programmable OpenGL 4.5 pipeline using a core profile
        OpenGL_4_5_Core_Profile,
        /// Using a programmable OpenGL 4.6 pipeline using a core profile
        OpenGL_4_6_Core_Profile,

        /// Using a programmable OpenGL 4.1 pipeline using a core debug profile
        OpenGL_4_1_Debug_Core_Profile,
        /// Using a programmable OpenGL 4.2 pipeline using a core debug profile
        OpenGL_4_2_Debug_Core_Profile,
        /// Using a programmable OpenGL 4.3 pipeline using a core debug profile
        OpenGL_4_3_Debug_Core_Profile,
        /// Using a programmable OpenGL 4.4 pipeline using a core debug profile
        OpenGL_4_4_Debug_Core_Profile,
        /// Using a programmable OpenGL 4.5 pipeline using a core debug profile
        OpenGL_4_5_Debug_Core_Profile,
        /// Using a programmable OpenGL 4.6 pipeline using a core debug profile
        OpenGL_4_6_Debug_Core_Profile
    };

    enum class RenderTarget { WindowBuffer, NonLinearBuffer };

    struct Statistics {
        static inline const int HistoryLength = 512;

        Statistics();

        std::array<double, HistoryLength> frametimes;
        std::array<double, HistoryLength> drawTimes;
        std::array<double, HistoryLength> syncTimes;
        std::array<double, HistoryLength> loopTimeMin;
        std::array<double, HistoryLength> loopTimeMax;
    };

    static Engine& instance();
    static void create(const Configuration& arg);
    static void destroy();

    /**
     * Engine initiation that:
     * 1. Parse the configuration file
     * 2. Set up the network communication
     * 3. Create window(s)
     * 4. Set up OpenGL
     *   4.1 Create textures
     *   4.2 Init FBOs
     *   4.3 Init VBOs
     *   4.4 Init PBOs
     *
     * \param rm The optional run mode.
     * \param cluster The cluster setup that should be used for this SGCT run
     */
    void init(RunMode rm, config::Cluster cluster);

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

    /// \return the active draw texture
    unsigned int getCurrentDrawTexture() const;

    /// \return the active depth texture if depth texture rendering is enabled
    unsigned int getCurrentDepthTexture() const;

    /// \return the active normal texture if normal texture rendering is enabled
    unsigned int getCurrentNormalTexture() const;

    /// \return the active position texture if position texture rendering is enabled
    unsigned int getCurrentPositionTexture() const;

    /// \return the resolution in pixels for the active window's framebuffer
    glm::ivec2 getCurrentResolution() const;

    /// \return the index of the focus window. If no window has focus, 0 is returned
    int getFocusedWindowIndex() const;

    /// Sets if the info text should be visible or not.
    void setDisplayInfoVisibility(bool state);

    /// Sets if the statistics graph should be rendered or not
    void setStatsGraphVisibility(bool state);

    /**
     * Take an RGBA screenshot and save it as a PNG file. If stereo rendering is enabled
     * then two screenshots will be saved per frame, one for the left eye and one for the
     * right eye.
     *
     * To record frames for a movie simply call this function every frame you wish to
     * record. The read to disk is multi-threaded and maximum number of threads can be set
     * using:
     *   -number-capture-threads command line argument.
     */
    void takeScreenshot();

    /// Set the screenshot number (file index)
    void setScreenShotNumber(unsigned int number);

    /// \return the current screenshot number (file index)
    unsigned int getScreenShotNumber() const;

    /**
     * This function sets the initOGL callback. The Engine will then use the callback only
     * once before the starting the render loop.
     */
    void setInitOGLFunction(std::function<void(void)> fn);

    /**
     * This callback is called before the window is created (before OpenGL context is
     * created). At this stage the config file has been read and network initialized.
     */
    void setPreWindowFunction(std::function<void()> fn);

    /**
     * This function sets the pre-sync callback. The Engine will then use the callback
     * before the sync stage.
     */
    void setPreSyncFunction(std::function<void()> fn);

    /**
     * This function sets the post-sync-pre-draw callback. The Engine will then use the
     * callback after the sync stage but before the draw stage. Compared to the draw
     * callback the post-sync-pre-draw callback is called only once per frame.
     */
    void setPostSyncPreDrawFunction(std::function<void()> fn);

    /**
     * This function sets the draw callback. It's possible to have several draw functions
     * and change the callback on the fly preferably in a stage before the draw step, for
     * example the post-sync-pre-draw stage or the pre-sync stage. The draw callback could
     * be called several times per frame since it's called once for every viewport and
     * once for every eye if stereoscopy is used.
     */
    void setDrawFunction(std::function<void()> fn);

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
     * This function sets the draw 2D callback. This callback will be called after
     * overlays and post effects has been drawn. This makes it possible to render text and
     * HUDs that will not be filtered and antialiased.
     */
    void setDraw2DFunction(std::function<void()> fn);

    /**
     * This function sets the post-draw callback. The Engine will then use the callback
     * after the draw stage but before the OpenGL buffer swap. Compared to the draw
     * callback the post-draw callback is called only once per frame.
     */
    void setPostDrawFunction(std::function<void()> fn);

    /**
     * This function sets the clean up callback which will be called in the Engine
     * destructor before all sGCT components (like window, OpenGL context, network, etc.)
     * will be destroyed.
     */
    void setCleanUpFunction(std::function<void()> fn);

    /**
     * This functions sets the encoding callback that is called by SGCT to encode all
     * shared data that is sent to the connected nodes in a clustered setup.
     */
    void setEncodeFunction(std::function<void()> fn);

    /**
     * This functions sets the decoding callback that is called by SGCT to encode all
     * shared data that is sent to the connected nodes in a clustered setup.
     */
    void setDecodeFunction(std::function<void()> fn);

    /**
     * This function sets the keyboard callback (GLFW wrapper) where the four parameters
     * are: int key, int scancode, int action, int mods. Modifier keys can be a
     * combination of sgct::modifier::Shift, sgct::modifier::Control, sgct::modifier::Alt,
     * or sgct::modifier::Super. All windows are connected to this callback.
     *
     * \param fn is the std function of a keyboard callback function
     */
    void setKeyboardCallbackFunction(
        std::function<void(int key, int scanCode, int action, int modifiers)> fn);

    /// All windows are connected to this callback.
    void setCharCallbackFunction(
        std::function<void(unsigned int unicode, int modifiers)> fn);

    /**
     * This function sets the mouse button callback (GLFW wrapper) where the two
     * parameters are: int button, int action. Button id's are listed in the table below.
     * Action can either be sgct::action::Press or sgct::action::Release. All windows are
     * connected to this callback.
     *
     * \param fn is the std function to a mouse button callback function
     */
    void setMouseButtonCallbackFunction(
        std::function<void(int button, int action, int modifiers)> fn);

    /// All windows are connected to this callback.
    void setMousePosCallbackFunction(std::function<void(double x, double y)> fn);

    /// All windows are connected to this callback.
    void setMouseScrollCallbackFunction(std::function<void(double x, double y)> fn);

    /// Drop files to any window. All windows are connected to this callback.
    void setDropCallbackFunction(std::function<void(int count, const char** paths)> fn);

    void setTouchCallbackFunction(std::function<void(const core::Touch* touches)> fn);

    /**
     * This callback must be set before Engine::init is called. Parameters to the callback
     * are: Image pointer for image data, window index, eye index, download type
     *
     * \param fn is the function pointer to a screenshot callback for custom frame
     *        capture & export
     */
    void setScreenShotCallback(std::function<void(core::Image*, size_t,
        core::ScreenCapture::EyeIndex, GLenum type)> fn);

    /**
     * This function sets the external control message callback which will be called when
     * a TCP message is received. The TCP listner is enabled in the XML configuration file
     * in the Cluster tag by externalControlPort, where the portnumber is an integer
     * preferably above 20000.
     *
     * \param fn is the function pointer to an external control message callback.
     *        arguments: const char * buffer, int buffer length
     *
     * All TCP messages must be separated by carriage return (CR) followed by a newline
     * (NL). Look at this [tutorial
     * (https://c-student.itn.liu.se/wiki/develop:sgcttutorials:externalguicsharp) for
     * more info.
     */
    void setExternalControlCallback(
        std::function<void(const char* buffer, int length)> fn);

    /**
     * This function sets the external control status callback which will be called when
     * the connection status changes (connect or disconnect).
     *
     * \param fn is the std function of an external control status callback
     */
    void setExternalControlStatusCallback(std::function<void(bool connected)> fn);

    /**
     * This function sets the OpenGL context creation callback which will be called
     * directly after all SGCT windows are created. This enables the user to create
     * additional OpenGL context for multithreaded OpenGL.
     *
     * \param fn is the std funtion of an OpenGL context (GLFW window) creation callback
     */
    void setContextCreationCallback(std::function<void(GLFWwindow* window)> fn);

    /**
     * This function sets the data transfer message callback which will be called when a
     * TCP message is received. The TCP listner is enabled in the XML configuration file
     * in the Node tag by dataTransferPort, where the portnumber is an integer preferably
     * above 20000.
     *
     * \param fn is the std function of a data transfer callback
     */
    void setDataTransferCallback(
        std::function<void(void* buffer, int length, int packageId, int clientId)> fn);
    
    /**
     * This function sets the data transfer status callback which will be called when the
     * connection status changes (connect or disconnect).
     *
     * \param fn is the std function of a data transfer status callback
     */
    void setDataTransferStatusCallback(
        std::function<void(bool connected, int clientId)> fn);

    /**
     * This function sets the data transfer acknowledge callback which will be called when
     * the data is successfully sent.
     *
     * \param fn is the std function of a data transfer acknowledge callback
     */
    void setDataAcknowledgeCallback(std::function<void(int packageId, int clientId)> fn);

    /**
     * This function sends a message to the external control interface.
     *
     * \param data a pointer to the data buffer
     * \param length is the number of bytes of data that will be sent
     */
    void sendMessageToExternalControl(const void* data, int length);

    /**
     * This function sends a message to the external control interface.
     *
     * \param msg the message string that will be sent
     */
    void sendMessageToExternalControl(const std::string& msg);

    /// Check if the external control is connected.
    bool isExternalControlConnected() const;

    /**
     * Set the buffer size for the external control communication buffer. This size must
     * be equal or larger than the receive buffer size.
     */
    void setExternalControlBufferSize(unsigned int newSize);

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
     * Compression levels 1-9.
     *  -1 = Default compression
     *   0 = No compression
     *   1 = Best speed
     *   9 = Best compression
     */
    void setDataTransferCompression(bool state, int level = 1);

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
    void invokeDecodeCallbackForDataTransfer(void* receivedData, int receivedLength,
        int packageId, int clientId);

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
     * Set the mouse position.
     *
     * \param winIndex specifies which window's input to set
     * \param xPos x screen coordinate
     * \param yPos y screen coordinate
     */
    static void setMousePos(int winIndex, double xPos, double yPos);
    
    /**
     * Set the mouse cursor/pointer visibility.
     *
     * \param winIndex specifies which window's input to set
     * \param state set to true if mouse cursor should be visible
     */
    static void setMouseCursorVisibility(int winIndex, bool state);

    /**
     * Returns the name of the requested joystick
     *
     * \param joystick is the joystick id. Available IDs are in the joystick.h
     */
    static const char* getJoystickName(int joystick);

    /**
     * \param joystick the joystick id: Available IDs are in the joystick.h
     * \param numOfValues is the number of analog axes
     * \return the analog float values (array)
     */
    static const float* getJoystickAxes(int joystick, int* numOfValues);

    /**
     * \param joystick the joystick id: Available IDs are in the joystick.h
     * \param numOfValues is the number of buttons
     * \return the button values (array)
     */
    static const unsigned char* getJoystickButtons(int joystick, int* numOfValues);

    /// \return a reference to this node (running on this computer).
    const sgct::core::Node& getThisNode() const;

    /// \return a pointer to a specified window by index on this node.
    Window& getWindow(int index) const;

    /// \return the number of windows for this node.
    int getNumberOfWindows() const;

    /// \return a pointer to the current window that is being rendered
    Window& getCurrentWindow() const;

    /// \return an index to the current window that is beeing rendered
    int getCurrentWindowIndex() const;

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
     * Get the active FBO buffer size. Each window has its own buffer plus any additional
     * non-linear projection targets.
     */
    glm::ivec2 getCurrentDrawBufferSize() const;

    /**
     * Get all the available FBO buffer sizes. Each window has its own buffer plus any
     * additional non-linear projection targets.
     */
    const std::vector<glm::ivec2>& getDrawBufferResolutions() const;

    /// \return the active render target.
    RenderTarget getCurrentRenderTarget() const;

    /**
     * Returns the active viewport information in pixels (only valid inside in the draw
     * callback function)
     */
    glm::ivec4 getCurrentViewportPixelCoords() const;

    /**
     * Specifies the sync parameters to be used in the rendering loop.
     *
     * \param printMessage If <code>true</code> a message is print waiting for a frame
     *                     every second
     * \param timeout The timeout that a master and slaves will wait for each other
     *                in seconds
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
    enum class BufferMode { BackBuffer = 0, BackBufferBlack, RenderToTexture };

    static Engine* _instance;

    Engine(const Configuration& config);

    /// Engine destructor destructs GLFW and releases resources/memory.
    ~Engine();

    /// Initiates network communication.
    void initNetwork();

    /// Create and initiate a window.
    void initWindows();

    /// Initiates OpenGL.
    void initOGL();

    /**
     * Locks the rendering thread for synchronization. Locks the slaves until data is
     * successfully received.
     */
    void frameLockPreStage();

    /**
     * Locks the rendering thread for synchronization. Locks master until slaves are ready
     * to swap buffers.
     * sync.
     */
    void frameLockPostStage();

    /// This function renders basic text info and statistics on screen.
    void renderDisplayInfo();

    void updateDrawBufferResolutions();

    /**
     * This function enters the correct viewport, frustum, stereo mode and calls the draw
     * callback.
     */
    void draw();

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
     * This function clears and sets the appropriate buffer from:
     *   - Back buffer
     *   - Left back buffer
     *   - Right back buffer
     *
     * \param mode is the one of the following:
     *   - Backbuffer (transparent)
     *   - Backbuffer (black)
     *   - RenderToTexture
     */
    void setAndClearBuffer(BufferMode mode);

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

    std::function<void()> _drawFn;
    std::function<void()> _preSyncFn;
    std::function<void()> _postSyncPreDrawFn;
    std::function<void()> _postDrawFn;
    std::function<void()> _preWindowFn;
    std::function<void()> _initOpenGLFn;
    std::function<void()> _cleanUpFn;
    std::function<void()> _draw2DFn;
    std::function<void(const char*, int)> _externalDecodeCallbackFn;
    std::function<void(bool)> _externalStatusCallbackFn;
    std::function<void(void*, int, int, int)> _dataTransferDecodeCallbackFn;
    std::function<void(bool, int)> _dataTransferStatusCallbackFn;
    std::function<void(int, int)> _dataTransferAcknowledgeCallbackFn;
    std::function<
        void(core::Image*, size_t, core::ScreenCapture::EyeIndex, GLenum)
    > _screenShotFn;
    std::function<void(GLFWwindow*)> _contextCreationFn;
    
    float _nearClippingPlaneDist = 0.1f;
    float _farClippingPlaneDist = 100.f;
    glm::vec4 _clearColor = glm::vec4(0.f, 0.f, 0.f, 1.f);

    core::Frustum::Mode _currentFrustumMode = core::Frustum::Mode::MonoEye;
    glm::ivec4 _currentViewportCoords = glm::ivec4(0, 0, 640, 480);
    std::vector<glm::ivec2> _drawBufferResolutions;
    size_t _currentDrawBufferIndex = 0;

    int _currentWindowIndex = 0;

    struct {
        double prevTimestamp = 0.0;
        unsigned int lastAvgFrameCounter = 0;
        double lastAvgTimestamp = 0.0;
    } stats;

    Statistics _statistics;
    std::unique_ptr<core::StatisticsRenderer> _statisticsRenderer;

    struct {
        int main = 0;
        int sub = 0;
    } _currentViewportIndex;
    RenderTarget _currentRenderTarget = RenderTarget::WindowBuffer;

    bool _showInfo = false;
    bool _showGraph = false;
    bool _checkOpenGLCalls = false;
    bool _checkFBOs = false;
    bool _takeScreenshot = false;
    bool _shouldTerminate = false;
    bool _renderingOffScreen = false;
    bool _helpMode = false;

    bool _printSyncMessage = true;
    float _syncTimeout = 60.f;

    struct {
        ShaderProgram fboQuad;
        ShaderProgram fxaa;
        ShaderProgram overlay;
    } _shader;

    struct {
        int monoTex = -1;
        int overlayTex = -1;
        int sizeX = -1;
        int sizeY = -1;
        int fxaaSubPixTrim = -1;
        int fxaaSubPixOffset = -1;
        int fxaaTexture = -1;
    } _shaderLoc;

    std::unique_ptr<std::thread> _thread;

    bool _isRunning = true;

    unsigned int _frameCounter = 0;
    unsigned int _shotCounter = 0;

    RunMode _runMode = RunMode::Default_Mode;
    core::NetworkManager::NetworkMode _networkMode =
        core::NetworkManager::NetworkMode::Remote;

    unsigned int _timeQueryBegin = 0;
    unsigned int _timeQueryEnd = 0;
};

} // namespace sgct

#endif // __SGCT__ENGINE__H__
