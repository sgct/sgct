/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__ENGINE__H__
#define __SGCT__ENGINE__H__

#include <sgct/ogl_headers.h>

#include <sgct/action.h>
#include <sgct/config.h>
#include <sgct/keys.h>
#include <sgct/mouse.h>
#include <sgct/joystick.h>
#include <sgct/shaderprogram.h>
#include <sgct/fisheyeprojection.h>
#include <sgct/messagehandler.h>
#include <sgct/screencapture.h>
#include <sgct/settings.h>
#include <sgct/sphericalmirrorprojection.h>
#include <sgct/spoutoutputprojection.h>
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
    class NetworkManager;
    class Node;
    class StatisticsRenderer;
    class Touch;
} // namespace core

config::Cluster loadCluster(std::optional<std::string> path);

/**
 * The Engine class is the central part of sgct and handles most of the callbacks,
 * rendering, network handling, input devices etc.
 * 
 * The figure below illustrates when different callbacks (gray and blue boxes) are called
 * in the renderloop. The blue boxes illustrates internal processess.
 *
 * \image html render_diagram.jpg
 * \image latex render_diagram.eps "Render diagram" width=7cm
 */
class Engine {
    // needs to access draw callbacks
    friend class core::FisheyeProjection;
    friend class core::SphericalMirrorProjection;
    friend class core::SpoutOutputProjection;

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

    struct Statistics {
        static inline const int HistoryLength = 512;

        std::array<double, HistoryLength> frametimes;
        std::array<double, HistoryLength> drawTimes;
        std::array<double, HistoryLength> syncTimes;
        std::array<double, HistoryLength> loopTimeMin;
        std::array<double, HistoryLength> loopTimeMax;
    };

    static void create(const Configuration& arg);
    static void destroy();

    /// \return the static pointer to the Engine instance
    static Engine* instance();

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
    bool init(RunMode rm, config::Cluster cluster);

    /// Terminates SGCT.
    void terminate();

    /// This is SGCT's renderloop where rendering & synchronization takes place.
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
    float getNearClippingPlane() const;

    /// \return the far clipping plane distance in meters
    float getFarClippingPlane() const;

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
     * Set the exit key that will kill SGCT or abort certain SGCT functions. Default value
     * is: sgct::key:ESC. To diable shutdown or escaping SGCT then use: sgct::key::Unknown
     */
    void setExitKey(int key);

    /**
     * This functions updates the frustum of all viewports on demand. However if the
     * viewport is tracked this is done on the fly.
     */
    void updateFrustums();

    /**
     * \return the active draw texture if frame buffer objects are used,
     *         otherwise GL_FALSE
     */
    unsigned int getCurrentDrawTexture() const;

    /**
     * \return the active depth texture if depth texture rendering is enabled through
     *         Settings and if frame buffer objects are used otherwise GL_FALSE
     */
    unsigned int getCurrentDepthTexture() const;

    /**
     * \return the active normal texture if normal texture rendering is enabled through
     *         Settings and if frame buffer objects are used otherwise GL_FALSE
     */
    unsigned int getCurrentNormalTexture() const;

    /**
     * \return the active position texture if position texture rendering is enabled
     *         through Settings and if frame buffer objects are used otherwise GL_FALSE
     */
    unsigned int getCurrentPositionTexture() const;

    /// \return the resolution in pixels for the active window's framebuffer
    glm::ivec2 getCurrentResolution() const;

    /// \return the index of the focus window. If no window has focus, 0 is returned.
    int getFocusedWindowIndex() const;

    /**
     * Set if the info text should be visible or not
     *
     * \param state of the info text rendering
     */
    void setDisplayInfoVisibility(bool state);

    void setStatsGraphVisibility(bool state);

    /**
     * Take a RGBA screenshot and save it as a PNG file. If stereo rendering is enabled
     * then two screenshots will be saved per frame, one for the left eye and one for the
     * right eye.
     *
     * To record frames for a movie simply call this function every frame you wish to
     * record. The read to disk is multi-threaded and maximum number of threads can be set
     * using:
     *   - numberOfCaptureThreads command line argument.
     */
    void takeScreenshot();

    ///  Set the screenshot number (file index)
    void setScreenShotNumber(unsigned int number);

    ///  \return the current screenshot number (file index)
    unsigned int getScreenShotNumber() const;

    /**
     * This function sets the initOGL callback. The Engine will then use the callback only
     * once before the starting the render loop. Textures, Models, Buffers, etc. can be
     * loaded/allocated here.
     */
    void setInitOGLFunction(std::function<void(void)> fn);

    /**
     * This callback is called before the window is created (before OpenGL context is
     * created). At this stage the config file has been read and network initialized.
     * Therefore it's suitable for loading master or slave specific data.
     */
    void setPreWindowFunction(std::function<void()> fn);

    /**
     * This function sets the pre-sync callback. The Engine will then use the callback
     * before the sync stage. In the callback set the variables that will be shared.
     */
    void setPreSyncFunction(std::function<void()> fn);

    /**
     * This function sets the post-sync-pre-draw callback. The Engine will then use the
     * callback after the sync stage but before the draw stage. Compared to the draw
     * callback the post-sync-pre-draw callback is called only once per frame. In this
     * callback synchronized variables can be applied or simulations depending on
     * synchronized input can run.
     */
    void setPostSyncPreDrawFunction(std::function<void()> fn);

    /**
     * This function sets the draw callback. It's possible to have several draw functions
     * and change the callback on the fly preferably in a stage before the draw like the
     * post-sync-pre-draw stage or the pre-sync stage. The draw callback can be called
     * several times per frame since it's called once for every viewport and once for
     * every eye if stereoscopy is used.
     */
    void setDrawFunction(std::function<void()> fn);

    /**
     * This function sets the draw 2D callback. This callback will be called after
     * overlays and post effects has been drawn. This makes it possible to render text and
     * HUDs that will not be filtered and antialiased.
     */
    void setDraw2DFunction(std::function<void()> fn);

    /**
     * This function sets the post-draw callback. The Engine will then use the callback
     * after the draw stage but before the OpenGL buffer swap. Compared to the draw
     * callback the post-draw callback is called only once per frame. In this callback
     * data/buffer swaps can be made.
     */
    void setPostDrawFunction(std::function<void()> fn);

    /**
     * This function sets the clean up callback which will be called in the Engine
     * destructor before all sgct components (like window, OpenGL context, network, etc.)
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
     * combination of SGCT_MOD_SHIFT, SGCT_MOD_CONTROL, SGCT_MOD_ALT, SGCT_MOD_SUPER. All
     * windows are connected to this callback.
     *
     * \param fn is the std function of a keyboard callback function
     *
     * Name          | Description
     * ------------- | -------------
     * SGCT_KEY_UNKNOWN  | Unknown
     * SGCT_KEY_SPACE  | Space
     * SGCT_KEY_APOSTROPHE | Apostrophe
     * SGCT_KEY_COMMA | Comma
     * SGCT_KEY_MINUS | Minus
     * SGCT_KEY_PERIOD | Period
     * SGCT_KEY_SLASH | Slash
     * SGCT_KEY_0 | 0
     * SGCT_KEY_1 | 1
     * SGCT_KEY_2 | 2
     * SGCT_KEY_3 | 3
     * SGCT_KEY_4 | 4
     * SGCT_KEY_5 | 5
     * SGCT_KEY_6 | 6
     * SGCT_KEY_7 | 7
     * SGCT_KEY_8 | 8
     * SGCT_KEY_9 | 9
     * SGCT_KEY_SEMICOLON | Semicolon
     * SGCT_KEY_EQUAL | Equal
     * SGCT_KEY_A | A
     * SGCT_KEY_B | B
     * SGCT_KEY_C | C
     * SGCT_KEY_D | D
     * SGCT_KEY_E | E
     * SGCT_KEY_F | F
     * SGCT_KEY_G | G
     * SGCT_KEY_H | H
     * SGCT_KEY_I | I
     * SGCT_KEY_J | J
     * SGCT_KEY_K | K
     * SGCT_KEY_L | L
     * SGCT_KEY_M | M
     * SGCT_KEY_N | N
     * SGCT_KEY_O | O
     * SGCT_KEY_P | P
     * SGCT_KEY_Q | Q
     * SGCT_KEY_R | R
     * SGCT_KEY_S | S
     * SGCT_KEY_T | T
     * SGCT_KEY_U | U
     * SGCT_KEY_V | V
     * SGCT_KEY_W | W
     * SGCT_KEY_X | X
     * SGCT_KEY_Y | Y
     * SGCT_KEY_Z | Z
     * SGCT_KEY_LEFT_BRACKET | Left bracket
     * SGCT_KEY_BACKSLASH | backslash
     * SGCT_KEY_RIGHT_BRACKET | Right bracket
     * SGCT_KEY_GRAVE_ACCENT | Grave accent
     * SGCT_KEY_WORLD_1 | World 1
     * SGCT_KEY_WORLD_2 | World 2
     * SGCT_KEY_ESC | Escape
     * SGCT_KEY_ESCAPE | Escape
     * SGCT_KEY_ENTER | Enter
     * SGCT_KEY_TAB | Tab
     * SGCT_KEY_BACKSPACE | Backspace
     * SGCT_KEY_INSERT | Insert
     * SGCT_KEY_DEL | Delete
     * SGCT_KEY_DELETE | Delete
     * SGCT_KEY_RIGHT | Right
     * SGCT_KEY_LEFT | Left
     * SGCT_KEY_DOWN | Down
     * SGCT_KEY_UP | Up
     * SGCT_KEY_PAGEUP | Page up
     * SGCT_KEY_PAGEDOWN | Page down
     * SGCT_KEY_PAGE_UP | Page up
     * SGCT_KEY_PAGE_DOWN | Page down
     * SGCT_KEY_HOME | Home
     * SGCT_KEY_END | End
     * SGCT_KEY_CAPS_LOCK | Caps lock
     * SGCT_KEY_SCROLL_LOCK | Scroll lock
     * SGCT_KEY_NUM_LOCK | Num lock
     * SGCT_KEY_PRINT_SCREEN | Print screen
     * SGCT_KEY_PAUSE | Pause
     * SGCT_KEY_F1 | F1
     * SGCT_KEY_F2 | F2
     * SGCT_KEY_F3 | F3
     * SGCT_KEY_F4 | F4
     * SGCT_KEY_F5 | F5
     * SGCT_KEY_F6 | F6
     * SGCT_KEY_F7 | F7
     * SGCT_KEY_F8 | F8
     * SGCT_KEY_F9 | F9
     * SGCT_KEY_F10 | F10
     * SGCT_KEY_F11 | F11
     * SGCT_KEY_F12 | F12
     * SGCT_KEY_F13 | F13
     * SGCT_KEY_F14 | F14
     * SGCT_KEY_F15 | F15
     * SGCT_KEY_F16 | F16
     * SGCT_KEY_F17 | F17
     * SGCT_KEY_F18 | F18
     * SGCT_KEY_F19 | F19
     * SGCT_KEY_F20 | F20
     * SGCT_KEY_F21 | F21
     * SGCT_KEY_F22 | F22
     * SGCT_KEY_F23 | F23
     * SGCT_KEY_F24 | F24
     * SGCT_KEY_F25 | F25
     * SGCT_KEY_KP_0 | Keypad 0
     * SGCT_KEY_KP_1 | Keypad 1
     * SGCT_KEY_KP_2 | Keypad 2
     * SGCT_KEY_KP_3 | Keypad 3
     * SGCT_KEY_KP_4 | Keypad 4
     * SGCT_KEY_KP_5 | Keypad 5
     * SGCT_KEY_KP_6 | Keypad 6
     * SGCT_KEY_KP_7 | Keypad 7
     * SGCT_KEY_KP_8 | Keypad 8
     * SGCT_KEY_KP_9 | Keypad 9
     * SGCT_KEY_KP_DECIMAL| Keypad decimal
     * SGCT_KEY_KP_DIVIDE | Keypad divide
     * SGCT_KEY_KP_MULTIPLY | Keypad multiply
     * SGCT_KEY_KP_SUBTRACT | Keypad subtract
     * SGCT_KEY_KP_ADD | Keypad add
     * SGCT_KEY_KP_ENTER | Keypad enter
     * SGCT_KEY_KP_EQUAL | Keypad equal
     * SGCT_KEY_LSHIFT | Left shift
     * SGCT_KEY_LEFT_SHIFT | Left shift
     * SGCT_KEY_LCTRL | Left control
     * SGCT_KEY_LEFT_CONTROL | Left control
     * SGCT_KEY_LALT | Left alt
     * SGCT_KEY_LEFT_ALT | Left alt
     * SGCT_KEY_LEFT_SUPER | Left super
     * SGCT_KEY_RSHIFT | Right shift
     * SGCT_KEY_RIGHT_SHIFT | Right shift
     * SGCT_KEY_RCTRL | Right control
     * SGCT_KEY_RIGHT_CONTROL | Right control
     * SGCT_KEY_RALT | Right alt
     * SGCT_KEY_RIGHT_ALT | Right alt
     * SGCT_KEY_RIGHT_SUPER | Right super
     * SGCT_KEY_MENU | Menu
     * SGCT_KEY_LAST | Last key index
     */
    void setKeyboardCallbackFunction(
        std::function<void(int key, int scanCode, int action, int modifiers)> fn);

    /// All windows are connected to this callback.
    void setCharCallbackFunction(
        std::function<void(unsigned int unicode, int modifiers)> fn);

    /**
     * This function sets the mouse button callback (GLFW wrapper) where the two
     * parameters are: int button, int action. Button id's are listed in the table below.
     * Action can either be SGCT_PRESS or SGCT_RELEASE. All windows are connected to this
     * callback.
     *
     * \param fn is the function pointer to a mouse button callback function
     *
     * Name          | Description
     * ------------- | -------------
     * SGCT_MOUSE_BUTTON_LEFT | Left button
     * SGCT_MOUSE_BUTTON_RIGHT | Right button
     * SGCT_MOUSE_BUTTON_MIDDLE | Middle button
     * SGCT_MOUSE_BUTTON_1 | Button 1
     * SGCT_MOUSE_BUTTON_2 | Button 2
     * SGCT_MOUSE_BUTTON_3 | Button 3
     * SGCT_MOUSE_BUTTON_4 | Button 4
     * SGCT_MOUSE_BUTTON_5 | Button 5
     * SGCT_MOUSE_BUTTON_6 | Button 6
     * SGCT_MOUSE_BUTTON_7 | Button 7
     * SGCT_MOUSE_BUTTON_8 | Button 8
     * SGCT_MOUSE_BUTTON_LAST | Last mouse button index
     */
    void setMouseButtonCallbackFunction(
        std::function<void(int button, int action, int modifiers)> fn);

    /// All windows are connected to this callback.
    void setMousePosCallbackFunction(std::function<void(double x, double y)> fn);

    /// All windows are connected to this callback.
    void setMouseScrollCallbackFunction(std::function<void(double x, double y)> fn);

    /// Drop files to any window. All windows are connected to this callback.
    void setDropCallbackFunction(std::function<void(int count, const char** paths)> fn);

    void setTouchCallbackFunction(
        std::function<void(const sgct::core::Touch* touches)> fn);

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

    // external control network functions

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
     * This function sends data to a specific node.
     *
     * \param data a pointer to the data buffer
     * \param length is the number of bytes of data that will be sent
     * \param packageId is the identification id of this specific package
     * \param nodeIndex is the index of a specific node
     */
    void transferDataToNode(const void* data, int length, int packageId,
        size_t nodeIndex);

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
     * \param joystick is the joystick id. Availible ids:
     *   - SGCT_JOYSTICK_1
     *   - SGCT_JOYSTICK_2
     *   - SGCT_JOYSTICK_3
     *   - SGCT_JOYSTICK_4
     *   - SGCT_JOYSTICK_5
     *   - SGCT_JOYSTICK_6
     *   - SGCT_JOYSTICK_7
     *   - SGCT_JOYSTICK_8
     *   - SGCT_JOYSTICK_9
     *   - SGCT_JOYSTICK_10
     *   - SGCT_JOYSTICK_11
     *   - SGCT_JOYSTICK_12
     *   - SGCT_JOYSTICK_13
     *   - SGCT_JOYSTICK_14
     *   - SGCT_JOYSTICK_15
     *   - SGCT_JOYSTICK_16
     *   - SGCT_JOYSTICK_LAST
     */
    static const char* getJoystickName(int joystick);

    /**
     * \param joystick the joystick id: Availibe id's are specified here: getJoystickName
     * \param numOfValues is the number of analog axes
     * \return the analog float values (array)
     */
    static const float* getJoystickAxes(int joystick, int* numOfValues);

    /**
     * \param joystick the joystick id: Availibe id's are specified here: getJoystickName
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

    /// \return a pointer to the user (VR observer position) object
    static core::User& getDefaultUser();

    /// \return a pointer to the tracking manager pointer
    static TrackingManager& getTrackingManager();

    /**
     * This functions checks for OpenGL errors and prints them using the MessageHandler
     * (to commandline). Avoid this function in the render loop for release code since it
     * can reduce performance.
     *
     * \param function The name of the function that asked for this check
     * \return true if no errors occured
     */
    static bool checkForOGLErrors(const std::string& function);

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
     * \return the active projection matrix (only valid inside in the draw callback
     *         function)
     */
    const glm::mat4& getCurrentProjectionMatrix() const;

    /**
     * \return the active view matrix (only valid inside in the draw callback function)
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
     * Returns the active viewport in pixels (only valid inside in the draw callback
     * function)
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

private:
    enum class BufferMode { BackBuffer = 0, BackBufferBlack, RenderToTexture };

    Engine(const Configuration& config);

    /// Engine destructor destructs GLFW and releases resources/memory.
    ~Engine();

    /// Initiates network communication.
    bool initNetwork();

    /// Create and initiate a window.
    bool initWindows();

    /// Initiates OpenGL.
    void initOGL();

    /**
     * Locks the rendering thread for synchronization. Locks the slaves until data is
     * successfully received.
     */
    bool frameLockPreStage();

    /**
     * Locks the rendering thread for synchronization. Locks master until slaves are ready
     * to swap buffers.
     * sync.
     */
    bool frameLockPostStage();

    /// This function renders basic text info and statistics on screen.
    void renderDisplayInfo();

    /// Set up the current viewport.
    void enterCurrentViewport();

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
    
    /// This function combines a texture and a shader into a new texture.
    void renderPostFX(TextureIndex targetIndex);

    void renderViewports(TextureIndex ti);

    /// This function renders stats, OSD and overlays.
    void render2D();

    /// This function attaches targets to FBO if FBO is in use
    void prepareBuffer(TextureIndex ti);

    /// This function updates the renderingtargets.
    void updateRenderingTargets(TextureIndex ti);

    /**
     * This function loads shaders that handles different 3D modes. The shaders are only
     * loaded once in the initOGL function.
     */
    void loadShaders();

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

    static void clearBuffer();

    static Engine* _instance;

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
        size_t main = 0;
        size_t sub = 0;
    } _currentViewportIndex;
    RenderTarget _currentRenderTarget = RenderTarget::WindowBuffer;

    bool _showInfo = false;
    bool _showGraph = false;
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

    std::unique_ptr<core::NetworkManager> _networkConnections;

    std::unique_ptr<std::thread> _thread;

    bool _isRunning = true;

    unsigned int _frameCounter = 0;
    unsigned int _shotCounter = 0;

    RunMode _runMode = RunMode::Default_Mode;
    int _exitKey = key::Escape;

    unsigned int _timeQueryBegin = 0;
    unsigned int _timeQueryEnd = 0;
};

} // namespace sgct

#endif // __SGCT__ENGINE__H__
