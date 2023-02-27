/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__ENGINE__H__
#define __SGCT__ENGINE__H__

#include <sgct/actions.h>
#include <sgct/callbackdata.h>
#include <sgct/config.h>
#include <sgct/frustum.h>
#include <sgct/joystick.h>
#include <sgct/keys.h>
#include <sgct/modifiers.h>
#include <sgct/mouse.h>
#include <sgct/window.h>
#include <array>
#include <functional>
#include <optional>
#include <thread>

namespace sgct {

struct Configuration;
class Node;
class StatisticsRenderer;

/**
 * Loads the cluster information from the provided \p path. The \p path is a
 * configuration file and should be an absolute path or relative to the current working
 * directory. If no path is provided, a default setup consisting of a FOV-based rendering
 * with a 1280x720 window with is loaded instead.
 *
 * \param path The path to the configuration that should be loaded
 * \return The loaded Cluster object that contains all of the information from the file
 * 
 * \pre The \p path, if it is provided, must be an existing file
 * \exception std::runtime_error This exception is thrown whenever an unrecoverable error
 *            occurs while trying to load the provided path. This error is never raised
 *            when providing no path
 */
config::Cluster loadCluster(std::optional<std::string> path = std::nullopt);

/**
 * Returns the number of seconds since the program start. The resultion of this
 * counter is usually the best available counter from the operating system.
 *
 * \return The number of seconds since the program started
 */
double time();

/**
 * The Engine class is the central part of SGCT and handles most of the callbacks,
 * rendering, network handling, input devices, etc.
 */
class Engine {
public:
    /**
     * Structure with all statistics gathered about different frametimes. The newest value
     * is always at the front of the different arrays, the remaining values being sorted
     * by the frame in which they occured. These values are only collected while the
     * statistics are being 
     */
    struct Statistics {
        /// For how many frames are the history values collected before the oldest values
        /// are replaced
        static constexpr int HistoryLength = 128;

        /// The times that contain the entire time spending processing the frames
        std::array<double, HistoryLength> frametimes = {};

        /// The amount of time spend rendering the 2D and 3D components of the frame
        std::array<double, HistoryLength> drawTimes = {};

        /// The amount of time spend synchronizing the state between master and clients
        std::array<double, HistoryLength> syncTimes = {};

        /// The lowest time recorded for network communication between master and clients
        std::array<double, HistoryLength> loopTimeMin = {};

        /// The highest time recorded for network communication between master and clients
        std::array<double, HistoryLength> loopTimeMax = {};

        /// \return The frame time (delta time) in seconds
        double dt() const;

        /// \return The average frame time (delta time) in seconds
        double avgDt() const;

        /// \return the minimum frame time (delta time) in the averaging window (seconds)
        double minDt() const;

        /// \return the maximum frame time (delta time) in the averaging window (seconds)
        double maxDt() const;
    };

    /**
     * This struct holds all of the callback functions that can be used by the client
     * library to be called during the different times of the frame.
     */
    struct Callbacks {
        /// This function is called before the window is created (before OpenGL context is
        /// created). At this stage the configuration file has been read and network
        /// is initialized.
        std::function<void()> preWindow;

        /// This function is called once before the starting the render loop and after
        /// creation of the OpenGL context.
        std::function<void(GLFWwindow*)> initOpenGL;

        /// This function is called before the synchronization stage.
        std::function<void()> preSync;

        /// This function is called once per frame after sync but before draw stage.
        std::function<void()> postSyncPreDraw;

        /// This function draws the scene and could be called several times per frame
        /// as it's called once per viewport and once per eye if stereoscopy is used.
        std::function<void(const RenderData&)> draw;

        /// This function is be called after overlays and post effects has been drawn and
        /// can used to render text and HUDs that will not be filtered or antialiased.
        std::function<void(const RenderData&)> draw2D;

        /// This function is called after the draw stage but before the OpenGL buffer
        /// swap.
        std::function<void()> postDraw;

        /// This is called before all SGCT components will be destroyed.
        std::function<void()> cleanup;

        /// This function is called to encode all shared data that is sent to the
        /// connected nodes in a clustered setup.
        std::function<std::vector<std::byte>()> encode;

        /// This function is called by decode all shared data sent to us from the master
        /// The parameter is the block of data that contains the data to be decoded.
        std::function<void(const std::vector<std::byte>&)> decode;

        /// This function is called when a TCP message is received.
        std::function<void(const char*, int)> externalDecode;

        /// This function is called when the connection status changes.
        std::function<void(bool)> externalStatus;

        /// This function is called when a TCP message is received.
        std::function<void(void*, int, int, int)> dataTransferDecode;

        /// This function is called when the connection status changes.
        std::function<void(bool, int)> dataTransferStatus;

        /// This function is called when data is successfully sent.
        std::function<void(int, int)> dataTransferAcknowledge;

        /// This function sets the keyboard callback (GLFW wrapper) for all windows.
        std::function<void(Key, Modifier, Action, int, Window*)> keyboard;

        /// All windows are connected to this callback.
        std::function<void(unsigned int, int, Window*)> character;

        /// This function sets the mouse button callback (GLFW wrapper) for all windows.
        std::function<void(MouseButton, Modifier, Action, Window*)> mouseButton;

        /// All windows are connected to this callback.
        std::function<void(double, double, Window*)> mousePos;

        /// All windows are connected to this callback.
        std::function<void(double, double, Window*)> mouseScroll;

        /// Drop files to any window. All windows are connected to this callback.
        std::function<void(int, const char**)> drop;
    };

    /**
     * Returns the global Engine object that is created through the Engine::create
     * function. This function must only be called after the Engine::create function has
     * been called successfully.
     * 
     * \return The global Engine object responsible for this application
     * \throw std::logic_error This error is thrown if this function is called before the
     *        Engine::create function is called or after the Engine::destroy function was
     *        called
     */
    static Engine& instance();

    /**
     * Creates the singleton Engine that is accessible through the Engine::instance
     * function. This function can only be called while no Engine instance exists, which
     * means that either it has to be the first call to this function or the
     * Engine::destroy function was called in between.
     *
     * \param cluster The configuration object for the config::Cluster that contains the
     *        information about how many nodes should exist, how many windows each node
     *        should contain and all other potential objects. The result of this is, in
     *        general, loaded from a configuration file
     * \param callbacks The list of callbacks that should be registered and called during
     *        the frame and their correct times. All callbacks are optional
     * \param arg The parameters that were set by the user from the commandline that can
     *        override some of the behavior from the configuration
     */
    static void create(config::Cluster cluster, Callbacks callbacks,
        const Configuration& arg);

    /**
     * Destroys the singleton Engine instance that was created by Engine::create and that
     * is accessible through the Engine::instance function. If this function is called
     * without a valid singleton existing, it is a no-op.
     */
    static void destroy();

    /**
     * Signals to SGCT that the application should be terminated at the end of the next
     * frame.
     */
    void terminate();

    /**
     * This function starts the SGCT render loop in which the rendering, synchronization,
     * event handling, and everything else happens. Control will only return from this
     * function after the program is terminated for any reason or if a non-recoverable
     * error has occurred
     */
    void exec();

    /**
     * Returns the Engine::Statistics object that contains all collected information about
     * the frametimes, drawtimes, and other frame-based statistics. The reference returned
     * by this function is valid until the Engine::destroy function is called.
     *
     * \return The Engine::Statistics object containing all of the statistics information
     */
    const Statistics& statistics() const;

    /**
     * Returns the distance to the near clipping plane in meters.
     * 
     * \return The distance to the near clipping plane in meters
     */
    float nearClipPlane() const;

    /**
     * Returns the distance to the far clipping plane in meters.
     *
     * \return The distance to the far clipping plane in meters
     */
    float farClipPlane() const;

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
     * This functions updates the frustum of all viewports. If a viewport is tracked, this
     * is done on the fly.
     */
    void updateFrustums();

    /**
     * Return the Window that currently has the focus. If no SGCT window has focus, a
     * `nullptr` is returned.
     *
     * \return The focus window or `nullptr` if no such window exists
     */
    /// \return the index of the focus window. If no window has focus, nullptr is returned
    const Window* focusedWindow() const;

    /**
     * Determines whether the graph displaying the rendering stats is being displayed or
     * not.
     *
     * \param value The new desired state as to whether the statistics are being shown
     */
    void setStatsGraphVisibility(bool value);

    /**
     * Takes an RGBA screenshot and saves it as a PNG file. If stereo rendering is enabled
     * then two screenshots will be saved per frame, one for each eye. The filename for
     * each image is the window title with an incremental counter appended to it. Each
     * successive call of this function will increment the counter. If it is desired to
     * reset the counter, see Engine::setScreenshotNumber
     *
     * To record frames for a movie simply call this function every frame you wish to
     * record. The write to disk is multi-threaded.
     *
     * \param windowIds If the vector is empty, screenshots of all windows will be taken,
     *        otherwise only the Window ids that appear in the vector will be used for
     *        screenshots and Window ids that do not appear in the list are ignored
     */
    void takeScreenshot(std::vector<int> windowIds = std::vector<int>());

    /**
     * Sets the number that the next screenshot will recieve with the next call of
     * Engine::takeScreenshot.
     *
     * \param number The next screenshot number
     */
    void setScreenshotNumber(unsigned int number);

    /**
     * Returns the number the next screenshot will receive upon the next call of
     * Engine::takeScreenshot. This counter can be reset through
     * Engine::setScreenshotNumber.
     *
     * \return The number the next screenshot will receive
     */
    /// \return the current screenshot number (file index)
    unsigned int screenShotNumber() const;

    /**
     * This function returns the draw function to be used in internal classes that need to
     * repeatedly call this. In general, there is no need for external applications to
     * store the draw function, but they are free to do so.
     *
     * \return The currently bound draw function
     */
    const std::function<void(const RenderData&)>& drawFunction() const;

    /**
     * Returns a reference to the node that represents this computer.
     *
     * \return A reference to this node
     */
    const Node& thisNode() const;

    /**
     * Returns a list of all windows for the current node. This vector might be empty, and
     * is valid for the lifetime of the program until the Engine::destroy function is
     * called.
     *
     * \return A list of all windows for the current node
     */
    const std::vector<std::unique_ptr<Window>>& windows() const;

    /**
     * Returns a pointer to the default user, i.e. the observer position.
     *
     * \return A pointer to the default user
     */
    static User& defaultUser();

    /**
     * Returns if this Node is the master in a clustered environment. This function will
     * also return `true` if the Node is not part of a clustered environment.
     *
     * \return `true` if this Node is the master in a clustered environment or not part of
     *         a cluster; `false` otherwise
     */
    bool isMaster() const;

    /**
     * Returns the number of the current frame. The frame number is a monotonically
     * increasing number that will never be repeated.
     *
     * \return The current framenumber
     */
    unsigned int currentFrameNumber() const;

    /**
     * Specifies the sync parameters to be used in the rendering loop.
     *
     * \param printMessage If `true` a message is print waiting for a frame every second
     * \param timeout The timeout that the master and clients will wait for in seconds
     */
    void setSyncParameters(bool printMessage = true, float timeout = 60.f);

    /**
     * Set up the current viewport, the framebuffer resolutions, windowing, and scissoring
     * in OpenGL. This is a function that is called by internal classes of SGCT and in
     * general does not have to be called by any external application using this library.
     *
     * \param window The Window object for which the viewport should be set
     * \param viewport The viewport of the \p window that should be set
     * \param frustum The frustum of the BaseViewport that should be set
     */
    void setupViewport(const Window& window, const BaseViewport& viewport,
        Frustum::Mode frustum);

private:
    /**
     * The global singleton instance of this Engine class. This instance is created
     * through the Engine::create function, accessed through the Engine::instance
     * function, and removed through the Engine::destroy function.
     */ 
    static Engine* _instance;

    /**
     * The internal constructor for this class, which will be called by the
     * Engine::create function. See the Engine::create function for more documentation on
     * the parameters.
     *
     * \param cluster The cluster setup that should be used for this SGCT run
     * \param callbacks The list of callbacks that should be installed
     * \param arg Parameters that were set by the user from the commandline
     */
    Engine(config::Cluster cluster, Callbacks callbacks, const Configuration& arg);

    /// Engine destructor destructs GLFW and releases resources/memory.
    ~Engine();

    /**
     * Two-phase initialization that will setup all of the required OpenGL state and other
     * states that are necessary to run the Engine instance created by the constructor.
     */
    void initialize();

    /**
     * Creates and initializes all of the windows that are specified for the current Node.
     * This function will call the Callbacks::preWindow callback for each window created
     * before the OpenGL context has been created and initialized.
     *
     * \param majorVersion The major version for OpenGL that is requested
     * \param minorVersion The minor version for OpenGL that is requested
     *
     * \pre \p majorVersion must be bigger than 0
     * \pre \p minorVersion must be bigger than 0
     */
    void initWindows(int majorVersion, int minorVersion);

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

    /**
     * Draw viewport overlays if there are any.
     *
     * \param window The Window object for which the overlays should be drawn
     * \param frustum The frustum for which the overlay should be drawn
     */
    void drawOverlays(const Window& window, Frustum::Mode frustum);

    /**
     * Draw geometry and bind FBO as texture in screenspace (ortho mode). The geometry can
     * be a simple quad or a geometry correction and blending mesh.
     *
     * \param window The Window whose geometry should be drawn
     */
    void renderFBOTexture(Window& window);

    /**
     * This function combines a texture and a shader into a new texture while applying
     * fast anti-aliasing (FXAA).
     *
     * \param window The Window object for which the FXAA operation should be performed
     * \param targetIndex The texture index that should be used as the source for the FXAA
     */
    void renderFXAA(Window& window, Window::TextureIndex targetIndex);

    /**
     * Causes all of the viewports of the provided \p window be rendered with the
     * \p frustum into the texture behind the provided \p ti texture index.
     *
     * \param window The window whose viewports should be rendered
     * \param frustum The frustum that should be used to render the viewports
     * \paramm ti The Window::TextureIndex that is pointing at the target where the
     *         rendering should be placed
     */
    void renderViewports(Window& window, Frustum::Mode frustum, Window::TextureIndex ti);

    /**
     * This function renders stats, OSD and overlays of the provided \p window and using
     * the provided \p frustum.
     *
     * \param window The Window into of which the 2D rendering should be performed
     * \param frustum The frustum that should be used to render the 2D component
     */
    void render2D(const Window& window, Frustum::Mode frustum);

    /**
     * This function waits for all windows to be created on the whole cluster in order to
     * set the barrier (hardware swap-lock). Under some Nvidia drivers the stability is
     * improved by first join a swapgroup and then set the barrier then all windows in a
     * swapgroup are created.
     */
    void waitForAllWindowsInSwapGroupToOpen();

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
    void blitWindowViewport(Window& prevWindow, Window& window,
        const Viewport& viewport, Frustum::Mode mode);

    std::function<void()> _preWindowFn;
    std::function<void(GLFWwindow*)> _initOpenGLFn;
    std::function<void()> _preSyncFn;
    std::function<void()> _postSyncPreDrawFn;
    std::function<void(const RenderData&)> _drawFn;
    std::function<void(const RenderData&)> _draw2DFn;
    std::function<void()> _postDrawFn;
    std::function<void()> _cleanupFn;

    float _nearClipPlane = 0.1f;
    float _farClipPlane = 100.f;

    Statistics _statistics;
    double _statsPrevTimestamp = 0.0;
    std::unique_ptr<StatisticsRenderer> _statisticsRenderer;

    bool _createDebugContext = false;
    bool _shouldTakeScreenshot = false;
    std::vector<int> _shouldTakeScreenshotIds;
    bool _shouldTerminate = false;

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

    unsigned int _frameCounter = 0;
    unsigned int _shotCounter = 0;
};

} // namespace sgct

#endif // __SGCT__ENGINE__H__
