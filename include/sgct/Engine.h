/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__ENGINE__H__
#define __SGCT__ENGINE__H__

#include <sgct/SGCTConfig.h>
#include <sgct/ShaderProgram.h>
#include <sgct/FisheyeProjection.h>
#include <sgct/ScreenCapture.h>
#include <sgct/SphericalMirrorProjection.h>
#include <sgct/SpoutOutputProjection.h>

namespace sgct_core {
class Image;
class NetworkManager;
class SGCTNode;
class ReadConfig;
class Statistics;
class Touch;
} // namespace sgct_core

/**
 * \namespace sgct
 * \brief SGCT namespace contains the most basic functionality of the toolkit
 */
namespace sgct {

class PostFX;
class SGCTTrackingManager;
class SGCTWindow;

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
    friend class sgct_core::FisheyeProjection; //needs to access draw callbacks
    friend class sgct_core::SphericalMirrorProjection; //needs to access draw callbacks
    friend class sgct_core::SpoutOutputProjection; //needs to access draw callbacks

public:
    /// The different run modes used by the init function
    enum class RunMode { 
        /// The default mode using fixed OpenGL pipeline (compability mode)
        Default_Mode = 0,
        /// Using a fixed OpenGL pipeline that allows mixing legacy and modern OpenGL
        OpenGL_Compatibility_Profile,
        /// Using a programmable OpenGL 3.3 pipeline using a core profile
        OpenGL_3_3_Core_Profile,
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
    enum TextureIndexes {
        LeftEye = 0,
        RightEye,
        Intermediate,
        FX1,
        FX2,
        Depth,
        Normals,
        Positions
    };

    /// \returns the static pointer to the engine instance
    static Engine* instance();

    /**
     * This is the only valid constructor that also initiates
     * [GLFW](http://www.glfw.org/). Command line parameters are used to load a
     * configuration file and settings. Note that parameter with one '\-' are followed by
     * arguments but parameters with '\-\-' are just options without arguments.
     *
     * Parameter     | Description
     * ------------- | -------------
     * -config <filename> | set xml confiuration file
     * -logPath <filepath> | set log file path
     * --help | display help message and exit
     * -local <integer> | set which node in configuration that is the localhost (index
     *                    starts at 0)
     * --client | run the application as client (only available when running as local)
     * --slave | run the application as client (only available when running as local)
     * --debug | set the notify level of messagehandler to debug
     * --Firm-Sync | enable firm frame sync
     * --Loose-Sync | disable firm frame sync
     * --Ignore-Sync | disable frame sync
     * -notify <integer> | set the notify level used in the MessageHandler
     *                     (0 = highest priority)
     * --No-FBO | disable frame buffer objects (some stereo modes, Multi-Window rendering,
     *            FXAA and fisheye rendering will be disabled)
     * --Capture-PNG | use png images for screen capture (default)
     * --Capture-TGA | use tga images for screen capture
     * -MSAA <integer> | Enable MSAA as default (argument must be a power of two)
     * --FXAA | Enable FXAA as default
     * --gDebugger | Force textures to be generated using glTexImage2D instead of
     *               glTexStorage2D
     * -numberOfCaptureThreads <integer> | set the maximum amount of threads that should
     *                                     be used during framecapture (default 8)
     */
    Engine(std::vector<std::string>& arg);

    /// Engine destructor destructs GLFW and releases resources/memory.
    ~Engine();

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
     * \param configurationFile The configuration file that should be loaded. This
     *        overwrites the value passed from the commandline
     */
    bool init(RunMode rm = RunMode::Default_Mode, std::string configurationFile = "");

    /// Terminates SGCT.
    void terminate();

    /// This is SGCT's renderloop where rendering & synchronization takes place.
    void render();

    /// \returns the frame time (delta time) in seconds
    double getDt() const;

    /// \returns the average frames per second
    double getAvgFPS() const;

    /// \returns the average frame time (delta time) in seconds
    double getAvgDt() const;

    /// \returns the minimum frame time (delta time) in the averaging window (seconds)
    double getMinDt() const;
    
    /// \returns the maximum frame time (delta time) in the averaging window (seconds)
    double getMaxDt() const;

    /// \returns the standard devitation of the delta time in seconds
    double getDtStandardDeviation() const;

    /// \returns the draw time in seconds
    double getDrawTime() const;

    /// \returns the sync time (time waiting for other nodes and network) in seconds
    double getSyncTime() const;

    /// \returns the clear color as 4 floats (RGBA)
    glm::vec4 getClearColor() const;
    
    /// \returns the near clipping plane distance in meters
    float getNearClippingPlane() const;

    /// \returns the far clipping plane distance in meters
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
     * \param red the red color component
     * \param green the green color component
     * \param blue the blue color component
     * \param alpha the alpha color component
     */
    void setClearColor(float red, float green, float blue, float alpha);

    /**
     * Set the exit key that will kill SGCT or abort certain SGCT functions. Default value
     * is: SGCT_KEY_ESC. To diable shutdown or escaping SGCT then use: SGCT_KEY_UNKNOWN
     *
     * \param key can be either an uppercase printable ISO 8859-1 (Latin 1) character
     * (e.g. 'A', '3' or '.'), or a special key identifier described in
     * setKeyboardCallbackFunction description.
     */
    void setExitKey(int key);

    /**
     * This functions updates the frustum of all viewports on demand. However if the
     * viewport is tracked this is done on the fly.
     */
    void updateFrustums();

    /// Add a post effect to all windows.
    void addPostFX(PostFX& fx);

    /**
     * \return the active draw texture if frame buffer objects are used,
     * otherwise GL_FALSE
     */
    unsigned int getCurrentDrawTexture() const;

    /**
     * \return the active depth texture if depth texture rendering is enabled through
     * SGCTSettings and if frame buffer objects are used otherwise GL_FALSE
     */
    unsigned int getCurrentDepthTexture() const;

    /**
     * \return the active normal texture if normal texture rendering is enabled through
     * SGCTSettings and if frame buffer objects are used otherwise GL_FALSE
     */
    unsigned int getCurrentNormalTexture() const;

    /**
     * \return the active position texture if position texture rendering is enabled
     * through SGCTSettings and if frame buffer objects are used otherwise GL_FALSE
     */
    unsigned int getCurrentPositionTexture() const;

    /// \return the resolution in pixels for the active window's framebuffer
    glm::ivec2 getCurrentResolution() const;

    /// \return the index of the focus window. If no window has focus, 0 is returned.
    size_t getFocusedWindowIndex() const;

    /// \param state of the wireframe rendering
    void setWireframe(bool state);

    /**
     * Set if the info text should be visible or not
     *
     * \param state of the info text rendering
     */
    void setDisplayInfoVisibility(bool state);

    /**
     * Set if the statistics graph should be visible or not
     *
     * \param state of the statistics graph rendering
     */
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

    /// Don't use this. This function is called internally in SGCT.
    void invokeScreenShotCallback(sgct_core::Image* imPtr, size_t winIndex,
        sgct_core::ScreenCapture::EyeIndex ei, unsigned int type);

    /**
     * Create a timer that counts down and call the given callback when finished. The
     * timer runs only on the master and is not precies since it is triggered in end of
     * the renderloop.
     *
     * \param millisec is the countdown time
     * \param fnPtr is the function pointer to a timer callback (the argument will be the
     * timer handle/id).
     *
     * \return Handle/id to the created timer
     */
    size_t createTimer(double millisec, void(*fnPtr)(size_t));

    /**
     * Stops the specified timer.
     *
     * \param id/handle to a timer
     */
    void stopTimer(size_t id);

    void setCleanUpFunction(void(*fnPtr)(void));
    
    // arguments: bool connected
    void setExternalControlStatusCallback(void(*fnPtr)(bool));

    /**
     * This function sets the initOGL callback. The Engine will then use the callback only
     * once before the starting the render loop. Textures, Models, Buffers, etc. can be
     * loaded/allocated here.
     *
     * \param fn is the std function of an OpenGL initiation callback
     */
    void setInitOGLFunction(std::function<void(void)> fn);

    /**
     * This callback is called before the window is created (before OpenGL context is
     * created). At this stage the config file has been read and network initialized.
     * Therefore it's suitable for loading master or slave specific data.
     *
     * \param fn is the std function of a pre window creation callback
     */
    void setPreWindowFunction(std::function<void(void)> fn);

    /**
     * This function sets the pre-sync callback. The Engine will then use the callback
     * before the sync stage. In the callback set the variables that will be shared.
     *
     * \param fn is the function pointer to a pre-sync callback
     */
    void setPreSyncFunction(std::function<void(void)> fn);

    /**
     * This function sets the post-sync-pre-draw callback. The Engine will then use the
     * callback after the sync stage but before the draw stage. Compared to the draw
     * callback the post-sync-pre-draw callback is called only once per frame. In this
     * callback synchronized variables can be applied or simulations depending on
     * synchronized input can run.
     *
     * \param fn is the std function of a post-sync-pre-draw callback
     */
    void setPostSyncPreDrawFunction(std::function<void(void)> fn);

    /**
     * This function sets the clear buffer callback which will override the default clear
     * buffer function.
     *
     * \param fnPtr is the function pointer to a clear buffer function callback
     *
     *
\code
void sgct::Engine::clearBuffer() {
    const float * colorPtr = sgct::Engine::instance()->getClearColor();
    glClearColor(colorPtr[0], colorPtr[1], colorPtr[2], colorPtr[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
\endcode
     */
    void setClearBufferFunction(std::function<void(void)> fn);

    /**
     * This function sets the draw callback. It's possible to have several draw functions
     * and change the callback on the fly preferably in a stage before the draw like the
     * post-sync-pre-draw stage or the pre-sync stage. The draw callback can be called
     * several times per frame since it's called once for every viewport and once for
     * every eye if stereoscopy is used.
     *
     * \param fn is the std function to a draw callback
     */
    void setDrawFunction(std::function<void(void)> fn);

    /**
     * This function sets the draw 2D callback. This callback will be called after
     * overlays and post effects has been drawn. This makes it possible to render text and
     * HUDs that will not be filtered and antialiased.
     *
     * \param fn is the function pointer to a draw 2D callback
     */
    void setDraw2DFunction(std::function<void(void)> fn);

    /**
     * This function sets the post-draw callback. The Engine will then use the callback
     * after the draw stage but before the OpenGL buffer swap. Compared to the draw
     * callback the post-draw callback is called only once per frame. In this callback
     * data/buffer swaps can be made.
     *
     * \param fn is the std function of a post-draw callback
     */
    void setPostDrawFunction(std::function<void(void)> fn);

    /**
     * This function sets the clean up callback which will be called in the Engine
     * destructor before all sgct components (like window, OpenGL context, network, etc.)
     * will be destroyed.
     *
     * \param fn is the std function pointer of a clean up function callback
     */
    void setCleanUpFunction(std::function<void(void)> fn);

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
        std::function<void(const sgct_core::Touch* touches)> fn);

    /**
     *  This callback must be set before Engine::init is called. Parameters to the
     * callback are: Image pointer for image data, window index, eye index, download type
     *
     * \param fn is the function pointer to a screenshot callback for custom frame
     *        capture & export
     */
    void setScreenShotCallback(std::function<void(sgct_core::Image*, size_t,
        sgct_core::ScreenCapture::EyeIndex, unsigned int type)> fn);


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
     * Don't use this. This function is called from SGCTNetwork and will invoke the
     * external network callback when messages are received.
     */
    void invokeDecodeCallbackForExternalControl(const char* receivedData,
        int receivedLength, int clientId);

    /**
     *  Don't use this. This function is called from SGCTNetwork and will invoke the
     * external network update callback when connection is connected/disconnected.
     */
    void invokeUpdateCallbackForExternalControl(bool connected);

    // data transfer functions
    /**
     * Compression levels 1-9.
     *   -1 = Default compression
     *   0 = No compression
     * 1 = Best speed
     * 9 = Best compression
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
     * Don't use this. This function is called from SGCTNetwork and will invoke the data
     * transfer callback when messages are received.
     */
    void invokeDecodeCallbackForDataTransfer(void* receivedData, int receivedLength,
        int packageId, int clientId);

    /**
     * Don't use this. This function is called from SGCTNetwork and will invoke the data
     * transfer callback when connection is connected/disconnected.
     */
    void invokeUpdateCallbackForDataTransfer(bool connected, int clientId);

    /**
     * Don't use this. This function is called from SGCTNetwork and will invoke the data
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
     *
     * \return SGCT_PRESS or SGCT_RELEASE
     */
    static int getKey(size_t winIndex, int key);

    /**
     * Checks if specified mouse button has been pressed.
     *
     * \param winIndex specifies which window to poll
     * \param button specifies which button to check
     *
     * \return SGCT_PRESS or SGCT_RELEASE
     */
    static int getMouseButton(size_t winIndex, int button);
    
    /**
     * Get the mouse position.
     *
     * \param winIndex specifies which window to poll
     * \param xPos x screen coordinate
     * \param yPos y screen coordinate
     */
    static void getMousePos(size_t winIndex, double* xPos, double* yPos);

    /**
     * Set the mouse position.
     *
     * \param winIndex specifies which window's input to set
     * \param xPos x screen coordinate
     * \param yPos y screen coordinate
     */
    static void setMousePos(size_t winIndex, double xPos, double yPos);
    
    /**
     * Set the mouse cursor/pointer visibility.
     *
     * \param winIndex specifies which window's input to set
     * \param state set to true if mouse cursor should be visible
     */
    static void setMouseCursorVisibility(size_t winIndex, bool state);

    /**
     * \param joystick is the joystick id. Availible id's:
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
     *
     * \param numOfValues is the number of analog axes
     *
     * \return the analog float values (array)
     */
    static const float* getJoystickAxes(int joystick, int* numOfValues);

    /**
     * \param joystick the joystick id: Availibe id's are specified here: getJoystickName
     *
     * \param numOfValues is the number of buttons
     *
     * \return the button values (array)
     */
    static const unsigned char* getJoystickButtons(int joystick, int* numOfValues);

    /// Returns a pointer to this node (running on this computer).
    const sgct_core::SGCTNode* getThisNode(size_t index) const;

    /// Returns a pointer to a specified window by index on this node.
    SGCTWindow& getWindow(size_t index) const;

    /// Returns the number of windows for this node.
    size_t getNumberOfWindows() const;

    /// Returns a pointer to the current window that is beeing rendered
    SGCTWindow& getCurrentWindow() const;

    /// Returns an index to the current window that is beeing rendered
    size_t getCurrentWindowIndex() const;

    /// Returns a pointer to the user (VR observer position) object
    static sgct_core::SGCTUser& getDefaultUser();

    /// Returns a pointer to the tracking manager pointer
    static SGCTTrackingManager& getTrackingManager();

    /**
     * This functions checks for OpenGL errors and prints them using the MessageHandler
     * (to commandline). Avoid this function in the render loop for release code since it
     * can reduce performance.
     *
     * \returns true if no errors occured
     */
    static bool checkForOGLErrors();

    /// Returns true if this node is the master
    bool isMaster() const;

    /// Returns true if on-screen info is rendered.
    bool isDisplayInfoRendered() const;

    /**
     * Returns true if render target is off-screen (FBO) or false if render target is the
     * frame buffer.
     */
    bool isRenderingOffScreen() const;

    /**
     * Returns the active frustum mode which can be one of the following:
     *   - Mono
     *   - Stereo Left
     *   - Stereo Right
     */
    sgct_core::Frustum::FrustumMode getCurrentFrustumMode() const;

    /**
     * Returns the active projection matrix (only valid inside in the draw callback
     * function)
     */
    const glm::mat4& getCurrentProjectionMatrix() const;

    /*!
        Returns the active view matrix (only valid inside in the draw callback function)
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

    /**
     * Return true if OpenGL pipeline is fixed (OpenGL 1-2) or false if OpenGL pipeline is
     * programmable (OpenGL 3-4)
     */
    bool isOGLPipelineFixed() const;

    bool isOpenGLCompatibilityMode() const;

    /// Get the GLSL version string that matches the run mode setting
    const std::string& getGLSLVersion() const;

    /**
     * Get the active viewport size in pixels.
     */
    glm::ivec2 getCurrentViewportSize() const;

    /**
     * Get the active FBO buffer size. Each window has its own buffer plus any additional
     * non-linear projection targets.
     */
    glm::ivec2 getCurrentDrawBufferSize() const;

    /**
     * Get the selected FBO buffer size. Each window has its own buffer plus any
     * additional non-linear projection targets.
     *
     * \param index index of selected drawbuffer
     */
    glm::ivec2 getDrawBufferSize(size_t index) const;

    size_t getNumberOfDrawBuffers() const;

    /// \return the active FBO buffer index.
    size_t getCurrentDrawBufferIndex() const;

    /// \return the active render target.
    RenderTarget getCurrentRenderTarget() const;

    /// \return the active off screen buffer. If no buffer is active nullptr is returned. 
    sgct_core::OffScreenBuffer* getCurrentFBO() const;

    /**
     * Returns the active viewport in pixels (only valid inside in the draw callback
     * function)
     */
    glm::ivec4 getCurrentViewportPixelCoords() const;

    /**
     * Get if wireframe rendering is enabled.
     *
     * \return true if wireframe is enabled otherwise false
     */
    bool getWireframe() const;

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

    /// Initiates network communication.
    bool initNetwork();

    /// Create and initiate a window.
    bool initWindows();

    /// Initiates OpenGL.
    void initOGL();

    /// Clean up all resources and release memory.
    void clean();

    /// Un-binds all callbacks.
    void clearAllCallbacks();

    /**
     * Locks the rendering thread for synchronization. Locks the slaves until data is
     * successfully received. Sync time from statistics is the time each computer waits
     * for sync.
     */
    bool frameLockPreStage();

    /**
     * Locks the rendering thread for synchronization. Locks master until slaves are ready
     * to swap buffers. Sync time from statistics is the time each computer waits for
     * sync.
     */
    bool frameLockPostStage();

    void calculateFPS(double timestamp);

    /**
     * \param arg is the list of arguments
     *
     * This function parses all SGCT arguments and removes them from the argument list.
     */
    void parseArguments(std::vector<std::string>& arg);
    
    /// This function renders basic text info and statistics on screen.
    void renderDisplayInfo();

    /**
     * Print the node info to terminal.
     *
     * \param nodeId Which node to print
     */
    void printNodeInfo(unsigned int nodeId);
    
    /// Set up the current viewport.
    void enterCurrentViewport();

    /**
     * This function updates the Anti-Aliasing (AA) settings. This function is called once
     * per second.
     */
    void updateAAInfo(const SGCTWindow& window);

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
    void renderPostFX(TextureIndexes ti);

    /// Works for fixed and programable pipeline.
    void renderViewports(TextureIndexes ti);

    /// This function renders stats, OSD and overlays.
    void render2D();

    /**
     * This function enters the correct viewport, frustum, stereo mode and calls the draw
     * callback.
     */
    void drawFixedPipeline();

    /// Draw viewport overlays if there are any.
    void drawOverlaysFixedPipeline();

    /**
     * Draw geometry and bind FBO as texture in screenspace (ortho mode). The geometry can
     * be a simple quad or a geometry correction and blending mesh.
     */
    void renderFBOTextureFixedPipeline();

    /// This function combines a texture and a shader into a new texture.
    void renderPostFXFixedPipeline(TextureIndexes finalTargetIndex);

    /// This function attaches targets to FBO if FBO is in use
    void prepareBuffer(TextureIndexes ti);

    /// This function updates the renderingtargets.
    void updateRenderingTargets(TextureIndexes ti);

    /// This function updates the timers.
    void updateTimers(double timeStamp);

    /**
     * This function loads shaders that handles different 3D modes. The shaders are only
     * loaded once in the initOGL function.
     */
    void loadShaders();

    /**
     *
     * \param mode is the one of the following:
     *   - Backbuffer (transparent)
     *   - Backbuffer (black)
     *   - RenderToTexture
     *
     * This function clears and sets the appropriate buffer from:
     *   - Back buffer
     *   - Left back buffer
     *   - Right back buffer
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
    void copyPreviousWindowViewportToCurrentWindowViewport(
        sgct_core::Frustum::FrustumMode frustumMode);

    static void clearBuffer();


    static Engine* mInstance;

    std::function<void()> mDrawFnPtr;
    std::function<void()> mPreSyncFnPtr;
    std::function<void()> mPostSyncPreDrawFnPtr;
    std::function<void()> mPostDrawFnPtr;
    std::function<void()> mPreWindowFnPtr;
    std::function<void()> mInitOGLFnPtr;
    std::function<void()> mClearBufferFnPtr;
    std::function<void()> mCleanUpFnPtr;
    std::function<void()> mDraw2DFnPtr;
    std::function<void(const char*, int)> mExternalDecodeCallbackFnPtr;
    std::function<void(bool)> mExternalStatusCallbackFnPtr;
    std::function<void(void*, int, int, int)> mDataTransferDecodeCallbackFnPtr;
    std::function<void(bool, int)> mDataTransferStatusCallbackFnPtr;
    std::function<void(int, int)> mDataTransferAcknowledgeCallbackFnPtr;
    std::function<
        void(sgct_core::Image*, size_t, sgct_core::ScreenCapture::EyeIndex, unsigned int)
    > mScreenShotFnPtr;
    std::function<void(GLFWwindow*)> mContextCreationFnPtr;
    
    std::function<void()> mInternalDrawFn;
    std::function<void()> mInternalRenderFBOFn;
    std::function<void()> mInternalDrawOverlaysFn;
    std::function<void(TextureIndexes)> mInternalRenderPostFXFn;

    float mNearClippingPlaneDist = 0.1f;
    float mFarClippingPlaneDist = 100.f;
    glm::vec4 mClearColor = glm::vec4(0.f, 0.f, 0.f, 1.f);

    sgct_core::Frustum::FrustumMode mCurrentFrustumMode = sgct_core::Frustum::MonoEye;
    glm::ivec4 mCurrentViewportCoords = glm::ivec4(0, 0, 640, 480);
    std::vector<glm::ivec2> mDrawBufferResolutions;
    size_t mCurrentDrawBufferIndex = 0;

    struct {
        size_t main = 0;
        size_t sub = 0;
    } mCurrentViewportIndex;
    RenderTarget mCurrentRenderTarget = RenderTarget::WindowBuffer;
    sgct_core::OffScreenBuffer* mCurrentOffScreenBuffer = nullptr;

    bool mShowInfo = false;
    bool mShowGraph = false;
    bool mShowWireframe = false;
    bool mTakeScreenshot = false;
    bool mTerminate = false;
    bool mRenderingOffScreen = false;
    bool mFixedOGLPipeline = true;
    bool mHelpMode = false;

    bool mPrintSyncMessage = true;
    float mSyncTimeout = 60.f;

    struct {
        ShaderProgram fboQuad;
        ShaderProgram fxaa;
        ShaderProgram overlay;
    } mShader;

    struct {
        int monoTex = -1;
        int overlayTex = -1;
        int sizeX = -1;
        int sizeY = -1;
        int fxaaSubPixTrim = -1;
        int fxaaSubPixOffset = -1;
        int fxaaTexture = -1;
    } mShaderLoc;

    std::unique_ptr<sgct_core::NetworkManager> mNetworkConnections;
    std::unique_ptr<sgct_core::Statistics> mStatistics;
    sgct_core::SGCTNode* mThisNode = nullptr;

    std::unique_ptr<std::thread> mThreadPtr;

    std::string configFilename;
    std::string mLogfilePath;
    bool mRunning;
    bool mInitialized = false;
    std::string mAAInfo;

    unsigned int mFrameCounter = 0;
    unsigned int mShotCounter = 0;

    struct TimerInformation {
        size_t mId;
        double mLastFired;
        double mInterval;
        std::function<void(size_t)> mCallback;
    };

    /// stores all active timers
    std::vector<TimerInformation> mTimers;
    /// the timer created next will use this ID
    size_t mTimerID = 0;

    RunMode mRunMode = RunMode::Default_Mode;
    std::string mGLSLVersion;
    int mExitKey = GLFW_KEY_ESCAPE;
};

} // namespace sgct

#endif // __SGCT__ENGINE__H__
