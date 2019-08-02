/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__ENGINE__H__
#define __SGCT__ENGINE__H__

#include <sgct/NetworkManager.h>
#include <sgct/ClusterManager.h>
#include <sgct/SGCTMutexManager.h>
#include <sgct/Statistics.h>
#include <sgct/ReadConfig.h>
#include <sgct/ShaderProgram.h>
#include <sgct/FisheyeProjection.h>
#include <sgct/SphericalMirrorProjection.h>
#include <sgct/SpoutOutputProjection.h>
#include <sgct/Touch.h>

#define MAX_UNIFORM_LOCATIONS 16
#define NUMBER_OF_SHADERS 8

/*! \namespace sgct
\brief SGCT namespace contains the most basic functionality of the toolkit
*/
namespace sgct {
/*!
The Engine class is the central part of sgct and handles most of the callbacks, rendering, network handling, input devices etc.

The figure below illustrates when different callbacks (gray and blue boxes) are called in the renderloop. The blue boxes illustrates internal processess.

\image html render_diagram.jpg
\image latex render_diagram.eps "Render diagram" width=7cm
*/
class Engine {
    friend class sgct_core::FisheyeProjection; //needs to access draw callbacks
    friend class sgct_core::SphericalMirrorProjection; //needs to access draw callbacks
    friend class sgct_core::SpoutOutputProjection; //needs to access draw callbacks

//all enums
public:
    
    //! The different run modes used by the init function
    enum RunMode { 
        /// The default mode using fixed OpenGL pipeline (compability mode)
        Default_Mode = 0,
        /// This option is using a fixed OpenGL pipeline that allows mixing legacy and modern OpenGL
        OpenGL_Compablity_Profile,
        /// This option is using a programmable OpenGL 3.3 pipeline using a core profile
        OpenGL_3_3_Core_Profile,
        /// This option is using a programmable OpenGL 4.0 pipeline using a core profile
        OpenGL_4_0_Core_Profile,
        /// This option is using a programmable OpenGL 4.1 pipeline using a core profile
        OpenGL_4_1_Core_Profile,
        /// This option is using a programmable OpenGL 4.2 pipeline using a core profile
        OpenGL_4_2_Core_Profile,
        /// This option is using a programmable OpenGL 4.3 pipeline using a core profile
        OpenGL_4_3_Core_Profile,
        /// This option is using a programmable OpenGL 4.4 pipeline using a core profile
        OpenGL_4_4_Core_Profile,
        /// This option is using a programmable OpenGL 4.5 pipeline using a core profile
        OpenGL_4_5_Core_Profile,
        /// This option is using a programmable OpenGL 4.6 pipeline using a core profile
        OpenGL_4_6_Core_Profile,

        /// This option is using a programmable OpenGL 4.1 pipeline using a core profile and debug feedback
        OpenGL_4_1_Debug_Core_Profile,
        /// This option is using a programmable OpenGL 4.2 pipeline using a core profile and debug feedback
        OpenGL_4_2_Debug_Core_Profile,
        /// This option is using a programmable OpenGL 4.3 pipeline using a core profile and debug feedback
        OpenGL_4_3_Debug_Core_Profile,
        /// This option is using a programmable OpenGL 4.4 pipeline using a core profile and debug feedback
        OpenGL_4_4_Debug_Core_Profile,
        /// This option is using a programmable OpenGL 4.5 pipeline using a core profile and debug feedback
        OpenGL_4_5_Debug_Core_Profile,
        /// This option is using a programmable OpenGL 4.6 pipeline using a core profile and debug feedback
        OpenGL_4_6_Debug_Core_Profile

    };
    //! The different texture indexes in window buffers
    enum TextureIndexes { LeftEye = 0, RightEye, Intermediate, FX1, FX2, Depth, Normals, Positions };
    enum RenderTarget { WindowBuffer, NonLinearBuffer };
    enum ViewportTypes { MainViewport, SubViewport };

public:
    static Engine* instance();

    Engine(int& argc, char**& argv);
    Engine(std::vector<std::string>& arg);
    ~Engine();

    bool init(RunMode rm = Default_Mode);
    void terminate();
    void render();
    void setConfigurationFile(std::string configFilePath);
    
    /*!
        \returns the static pointer to the engine instance
    */

    double getDt();
    double getAvgFPS();
    double getAvgDt();
    double getMinDt();
    double getMaxDt();
    double getDtStandardDeviation();
    double getDrawTime();
    double getSyncTime();

    /*!
        \returns the clear color as 4 floats (RGBA)
    */
    const float * getClearColor() { return mClearColor; }
    
    /*!
        \returns the near clipping plane distance in meters
    */
    const float& getNearClippingPlane() const { return mNearClippingPlaneDist; }
    /*!
        \returns the far clipping plane distance in meters
    */
    const float& getFarClippingPlane() const { return mFarClippingPlaneDist; }

    void setNearAndFarClippingPlanes(float nearClippingPlane, float farClippingPlane);
    void setEyeSeparation(float eyeSeparation);
    void setClearColor(float red, float green, float blue, float alpha);
    void setExitKey(int key);
    void setExitWaitTime(double time);
    void updateFrustums();
    void addPostFX( PostFX & fx );
    unsigned int getCurrentDrawTexture();
    unsigned int getCurrentDepthTexture();
    unsigned int getCurrentNormalTexture();
    unsigned int getCurrentPositionTexture();
    int getCurrentXResolution();
    int getCurrentYResolution();
    std::size_t getFocusedWindowIndex();

    /*!
        \param state of the wireframe rendering
    */
    void setWireframe(bool state) { mShowWireframe = state; }
    /*!
        Set if the info text should be visible or not

        \param state of the info text rendering
    */
    void setDisplayInfoVisibility(bool state) { mShowInfo = state; }

    /*!
        Set if the statistics graph should be visible or not

        \param state of the statistics graph rendering
    */
    void setStatsGraphVisibility(bool state) { mShowGraph = state; }

    /*!
        Take a RGBA screenshot and save it as a PNG file. If stereo rendering is enabled then two screenshots will be saved per frame, one for the left eye and one for the right eye.
        To record frames for a movie simply call this function every frame you wish to record. The read to disk is multi-threaded and maximum number of threads can be set using:
        -numberOfCaptureThreads command line argument.
    */
    void takeScreenshot() { mTakeScreenshot = true; }
    void setScreenShotNumber(unsigned int number);
    unsigned int getScreenShotNumber();
    void invokeScreenShotCallback1(sgct_core::Image * imPtr, std::size_t winIndex, sgct_core::ScreenCapture::EyeIndex ei, unsigned int type);
    void invokeScreenShotCallback2(unsigned char * imPtr, std::size_t winIndex, sgct_core::ScreenCapture::EyeIndex ei, unsigned int type);
    void setScreenShotCallback(void(*fnPtr)(sgct_core::Image *, std::size_t, sgct_core::ScreenCapture::EyeIndex, unsigned int type));
    void setScreenShotCallback(void(*fnPtr)(unsigned char *, std::size_t, sgct_core::ScreenCapture::EyeIndex, unsigned int type));

    std::size_t createTimer( double millisec, void(*fnPtr)(std::size_t) );
    void stopTimer(std::size_t id);

    //set callback functions
    void setInitOGLFunction( void(*fnPtr)(void) );
    void setPreWindowFunction( void(*fnPtr)(void) );
    void setPreSyncFunction( void(*fnPtr)(void) );
    void setPostSyncPreDrawFunction( void(*fnPtr)(void) );
    void setClearBufferFunction( void(*fnPtr)(void) );
    void setDrawFunction( void(*fnPtr)(void) );
    void setDraw2DFunction( void(*fnPtr)(void) );
    void setPostDrawFunction( void(*fnPtr)(void) );
    void setCleanUpFunction( void(*fnPtr)(void) );
    
    void setKeyboardCallbackFunction( void(*fnPtr)(int, int) ); //arguments: int key, int action
    void setKeyboardCallbackFunction( void(*fnPtr)(int, int, int, int) ); //arguments: int key, int scancode, int action, int mods
    void setCharCallbackFunction( void(*fnPtr)(unsigned int) ); //arguments: unsigned int unicode character
    void setCharCallbackFunction( void(*fnPtr)(unsigned int, int) ); //arguments: unsigned int unicode character, int mods
    void setMouseButtonCallbackFunction( void(*fnPtr)(int, int, int) ); //arguments: int button, int action, int mods
    void setMousePosCallbackFunction( void(*fnPtr)(double, double) ); //arguments: double x, double y
    void setMouseScrollCallbackFunction( void(*fnPtr)(double, double) ); //arguments: double xoffset, double yoffset
    void setDropCallbackFunction(void(*fnPtr)(int, const char**) ); //arguments: int count, const char ** list of path strings
    void setTouchCallbackFunction(void(*fnPtr)(const sgct_core::Touch*)); //arguments: current touch points

    void setExternalControlCallback(void(*fnPtr)(const char *, int)); //arguments: const char * buffer, int buffer length
    void setExternalControlStatusCallback(void(*fnPtr)(bool)); //arguments: const bool & connected
    void setContextCreationCallback(void(*fnPtr)(GLFWwindow*)); //arguments: glfw window share

    void setDataTransferCallback(void(*fnPtr)(void *, int, int, int)); //arguments: const char * buffer, int buffer length, int package id, int client
    void setDataTransferStatusCallback(void(*fnPtr)(bool, int)); //arguments: const bool & connected, int client
    void setDataAcknowledgeCallback(void(*fnPtr)(int, int)); //arguments: int package id, int client

    void setInitOGLFunction(std::function<void(void)> fn);
    void setPreWindowFunction(std::function<void(void)> fn);
    void setPreSyncFunction(std::function<void(void)> fn);
    void setPostSyncPreDrawFunction(std::function<void(void)> fn);
    void setClearBufferFunction(std::function<void(void)> fn);
    void setDrawFunction(std::function<void(void)> fn);
    void setDraw2DFunction(std::function<void(void)> fn);
    void setPostDrawFunction(std::function<void(void)> fn);
    void setCleanUpFunction(std::function<void(void)> fn);
    
    void setKeyboardCallbackFunction(std::function<void(int, int)> fn); //arguments: int key, int action
    void setKeyboardCallbackFunction(std::function<void(int, int, int, int)> fn); //arguments: int key, int scancode, int action, int mods
    void setCharCallbackFunction(std::function<void(unsigned int)> fn); //arguments: unsigned int unicode character
    void setCharCallbackFunction(std::function<void(unsigned int, int)> fn); //arguments: unsigned int unicode character, int mods
    void setMouseButtonCallbackFunction(std::function<void(int, int, int)> fn); //arguments: int button, int action, int mods
    void setMousePosCallbackFunction(std::function<void(double, double)> fn); //arguments: double x, double y
    void setMouseScrollCallbackFunction(std::function<void(double, double)> fn); //arguments: double xoffset, double yoffset
    void setDropCallbackFunction(std::function<void(int, const char**)> fn); //arguments: int count, const char ** list of path strings
    void setTouchCallbackFunction(std::function<void(const sgct_core::Touch*)> fn); //arguments: current touch points

    void setExternalControlCallback(std::function<void(const char *, int)> fn); //arguments: const char * buffer, int buffer length
    void setExternalControlStatusCallback(std::function<void(bool)> fn); //arguments: const bool & connected
    void setContextCreationCallback(std::function<void(GLFWwindow*)> fn); //arguments: glfw window share

    void setDataTransferCallback(std::function<void(void *, int, int, int)> fn); //arguments: const char * buffer, int buffer length, int package id, int client
    void setDataTransferStatusCallback(std::function<void(bool, int)> fn); //arguments: const bool & connected, int client
    void setDataAcknowledgeCallback(std::function<void(int, int)> fn); //arguments: int package id, int client

    //external control network functions
    void sendMessageToExternalControl(const void * data, int length);
    void sendMessageToExternalControl(const std::string& msg);
    bool isExternalControlConnected();
    void setExternalControlBufferSize(unsigned int newSize);
    void invokeDecodeCallbackForExternalControl(const char * receivedData, int receivedlength, int clientId);
    void invokeUpdateCallbackForExternalControl(bool connected);

    //data transfer functions
    void setDataTransferCompression(bool state, int level = 1);
    void transferDataBetweenNodes(const void * data, int length, int packageId);
    void transferDataToNode(const void * data, int length, int packageId, std::size_t nodeIndex);
    void invokeDecodeCallbackForDataTransfer(void * receivedData, int receivedlength, int packageId, int clientd);
    void invokeUpdateCallbackForDataTransfer(bool connected, int clientId);
    void invokeAcknowledgeCallbackForDataTransfer(int packageId, int clientId);

    //GLFW wrapped functions
    static double getTime();
    static int getKey( std::size_t winIndex, int key );
    static int getMouseButton( std::size_t winIndex, int button );
    static void getMousePos( std::size_t winIndex, double * xPos, double * yPos );
    static void setMousePos( std::size_t winIndex, double xPos, double yPos );
    static void setMouseCursorVisibility( std::size_t winIndex, bool state );
    static const char * getJoystickName( int joystick );
    static const float * getJoystickAxes( int joystick, int * numOfValues);
    static const unsigned char * getJoystickButtons( int joystick, int * numOfValues);
    static void sleep(double secs);

    /*!
        Returns a pointer to this node (running on this computer).
    */
    inline const sgct_core::SGCTNode * getThisNodePtr(std::size_t index) const { return mThisNode; }

    /*!
        Returns a pointer to a specified window by index on this node.
    */
    inline sgct::SGCTWindow * getWindowPtr(std::size_t index) { return mThisNode->getWindowPtr(index); }

    /*!
        Returns the number of windows for this node.
    */
    inline std::size_t getNumberOfWindows() { return mThisNode->getNumberOfWindows(); }

    /*!
        Returns a pointer to the current window that is beeing rendered
    */
    inline sgct::SGCTWindow * getCurrentWindowPtr() { return mThisNode->getCurrentWindowPtr(); }

    /*!
        Returns an index to the current window that is beeing rendered
    */
    inline std::size_t getCurrentWindowIndex() { return mThisNode->getCurrentWindowIndex(); }

    /*!
        Returns a pinter to the user (VR observer position) object
    */
    static sgct_core::SGCTUser * getDefaultUserPtr() { return sgct_core::ClusterManager::instance()->getDefaultUserPtr(); }

    /*!
        Returns a pointer to the tracking manager pointer
    */
    static sgct::SGCTTrackingManager * getTrackingManager() { return sgct_core::ClusterManager::instance()->getTrackingManagerPtr(); }
    static bool checkForOGLErrors();

    /*!
        Returns true if this node is the master
    */
    inline bool isMaster() { return mNetworkConnections->isComputerServer(); }

    /*!
        Returns true if on-screen info is rendered.
    */
    inline bool isDisplayInfoRendered() { return mShowInfo; }

    /*!
        Returns true if render target is off screen (FBO) or false if render target is the frame buffer.
    */
    inline bool isRenderingOffScreen() { return mRenderingOffScreen; }

    /*!
        Returns the active frustum mode which can be one of the following:
        - Mono
        - Stereo Left
        - Stereo Right
    */
    inline const sgct_core::Frustum::FrustumMode & getCurrentFrustumMode() { return mCurrentFrustumMode; }

    /*!
        Returns the active projection matrix (only valid inside in the draw callback function)
    */
    inline const glm::mat4 & getCurrentProjectionMatrix() { return getCurrentWindowPtr()->getCurrentViewport()->getProjection(mCurrentFrustumMode)->getProjectionMatrix(); }

    /*!
        Returns the active view matrix (only valid inside in the draw callback function)
    */
    inline const glm::mat4 & getCurrentViewMatrix() { return getCurrentWindowPtr()->getCurrentViewport()->getProjection(mCurrentFrustumMode)->getViewMatrix(); }

    /*!
        Returns the scene transform specified in the XML configuration, default is a identity matrix
    */
    inline const glm::mat4 & getModelMatrix() { return sgct_core::ClusterManager::instance()->getSceneTransform(); }

    /*!
        Returns the active VP = Projection * View matrix (only valid inside in the draw callback function)
    */
    inline const glm::mat4 & getCurrentViewProjectionMatrix() { return getCurrentWindowPtr()->getCurrentViewport()->getProjection(mCurrentFrustumMode)->getViewProjectionMatrix(); }

    /*!
        Returns the active MVP = Projection * View * Model matrix (only valid inside in the draw callback function)
    */
    inline glm::mat4 getCurrentModelViewProjectionMatrix() { return getCurrentWindowPtr()->getCurrentViewport()->getProjection(mCurrentFrustumMode)->getViewProjectionMatrix()
        * sgct_core::ClusterManager::instance()->getSceneTransform(); }
    
    /*!
        Returns the active MV = View * Model matrix (only valid inside in the draw callback function)
    */
    inline glm::mat4 getCurrentModelViewMatrix() { return getCurrentWindowPtr()->getCurrentViewport()->getProjection(mCurrentFrustumMode)->getViewMatrix()
        * sgct_core::ClusterManager::instance()->getSceneTransform(); }

    /*!
        Returns the current frame number
    */
    inline unsigned int getCurrentFrameNumber() { return mFrameCounter; }

    /*!
        Return true if OpenGL pipeline is fixed (openGL 1-2) or false if OpenGL pipeline is programmable (openGL 3-4)
    */
    inline bool isOGLPipelineFixed() { return mFixedOGLPipeline; }

    /*!
        Get the run mode setting (context version and compability modes)
    */
    inline RunMode getRunMode() { return mRunMode; }

    /*!
    Get the GLSL version string that matches the run mode setting
    */
    inline std::string getGLSLVersion() { return mGLSLVersion; }

    const std::size_t getCurrentViewportIndex(ViewportTypes vp) const;
    void getCurrentViewportSize(int & x, int & y);
    void getCurrentDrawBufferSize(int & x, int & y);
    void getDrawBufferSize(const std::size_t & index, int &x, int & y);
    std::size_t getNumberOfDrawBuffers();
    const std::size_t & getCurrentDrawBufferIndex();
    const RenderTarget & getCurrentRenderTarget();
    sgct_core::OffScreenBuffer * getCurrentFBO();
    const int * getCurrentViewportPixelCoords();

    bool getWireframe() const;

    /// Specifies the sync parameters to be used in the rendering loop
    /// @param printMessage If <code>true</code> a message is print waiting for a frame
    ///                     every second
    /// @param timeout      The timeout that a master and slaves will wait for each other
    ///                     in seconds
    void setSyncParameters(bool printMessage = true, float timeout = 60.f);

private:
    enum SyncStage { PreStage = 0, PostStage };
    enum BufferMode { BackBuffer = 0, BackBufferBlack, RenderToTexture };
    enum ViewportSpace { ScreenSpace = 0, FBOSpace };
    enum ShaderIndexes { FBOQuadShader = 0, FXAAShader, OverlayShader };
    enum ShaderLocIndexes {
        MonoTex = 0,
        OverlayTex,
        SizeX, SizeY, FXAA_SUBPIX_TRIM, FXAA_SUBPIX_OFFSET, FXAA_Texture
    };

    bool initNetwork();
    bool initWindows();
    void initOGL();
    void clean();
    void clearAllCallbacks();

    bool frameLock(SyncStage stage);
    void calculateFPS(double timestamp);
    void parseArguments( std::vector<std::string>& arg );
    void renderDisplayInfo();
    void printNodeInfo(unsigned int nodeId);
    void enterCurrentViewport();
    void updateAAInfo(std::size_t winIndex);
    void updateDrawBufferResolutions();

    void draw();
    void drawOverlays();
    void renderFBOTexture();
    void renderPostFX(TextureIndexes ti );
    void renderViewports(TextureIndexes ti);
    void render2D();

    void drawFixedPipeline();
    void drawOverlaysFixedPipeline();
    void renderFBOTextureFixedPipeline();
    void renderPostFXFixedPipeline(TextureIndexes finalTargetIndex );

    void prepareBuffer(TextureIndexes ti);
    void updateRenderingTargets(TextureIndexes ti);
    void updateTimers(double timeStamp);
    void loadShaders();
    void setAndClearBuffer(BufferMode mode);
    void waitForAllWindowsInSwapGroupToOpen();
    void copyPreviousWindowViewportToCurrentWindowViewport(sgct_core::Frustum::FrustumMode frustumMode);

    static void clearBuffer();
    static void internal_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void internal_key_char_callback(GLFWwindow* window, unsigned int ch);
    static void internal_key_char_mods_callback(GLFWwindow* window, unsigned int ch, int mods);
    static void internal_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void internal_mouse_pos_callback(GLFWwindow* window, double xPos, double yPos);
    static void internal_mouse_scroll_callback(GLFWwindow* window, double xOffset, double yOffset);
    static void internal_glfw_error_callback(int error, const char* description);
    static void internal_drop_callback(GLFWwindow* window, int count, const char** paths);
    static void internal_touch_callback(GLFWwindow* window, GLFWtouch* touchPoints, int count);
    static void outputHelpMessage();

private:
    static Engine* mInstance;


    //function pointers
    std::function<void()> mDrawFnPtr;
    std::function<void()> mPreSyncFnPtr;
    std::function<void()> mPostSyncPreDrawFnPtr;
    std::function<void()> mPostDrawFnPtr;
    std::function<void()> mPreWindowFnPtr;
    std::function<void()> mInitOGLFnPtr;
    std::function<void()> mClearBufferFnPtr;
    std::function<void()> mCleanUpFnPtr;
    std::function<void()> mDraw2DFnPtr;
    std::function<void(const char *, int)> mExternalDecodeCallbackFnPtr;
    std::function<void(bool)> mExternalStatusCallbackFnPtr;
    std::function<void(void *, int, int, int)> mDataTransferDecodeCallbackFnPtr;
    std::function<void(bool, int)> mDataTransferStatusCallbackFnPtr;
    std::function<void(int, int)> mDataTransferAcknowledgeCallbackFnPtr;
    std::function<void(sgct_core::Image*, std::size_t, sgct_core::ScreenCapture::EyeIndex, unsigned int type)> mScreenShotFnPtr1;
    std::function<void(unsigned char *, std::size_t, sgct_core::ScreenCapture::EyeIndex, unsigned int type)> mScreenShotFnPtr2; //less latency, more advanced
    std::function<void(GLFWwindow*)> mContextCreationFnPtr;
    
    std::function<void()> mInternalDrawFn;
    std::function<void()> mInternalRenderFBOFn;
    std::function<void()> mInternalDrawOverlaysFn;
    std::function<void(TextureIndexes)> mInternalRenderPostFXFn;

    float mNearClippingPlaneDist = 0.1f;
    float mFarClippingPlaneDist = 100.f;
    float mClearColor[4] = { 0.f, 0.f, 0.f, 1.f };

    sgct_core::Frustum::FrustumMode mCurrentFrustumMode = sgct_core::Frustum::MonoEye;
    int mCurrentViewportCoords[4] = { 0, 0, 640, 480 };
    std::vector<glm::ivec2> mDrawBufferResolutions;
    size_t mCurrentDrawBufferIndex = 0;
    size_t mCurrentViewportIndex[2] = { 0, 0 };
    RenderTarget mCurrentRenderTarget = WindowBuffer;
    sgct_core::OffScreenBuffer* mCurrentOffScreenBuffer = nullptr;

    static sgct_core::Touch mCurrentTouchPoints; //< stores all touch points (oldest to newest) from last callback

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

    //objects
    ShaderProgram mShaders[NUMBER_OF_SHADERS];

    //glsl
    int mShaderLocs[MAX_UNIFORM_LOCATIONS];

    //pointers
    sgct_core::NetworkManager* mNetworkConnections = nullptr;
    sgct_core::ReadConfig* mConfig = nullptr;
    sgct_core::Statistics* mStatistics = nullptr;
    sgct_core::SGCTNode* mThisNode = nullptr;

    std::thread* mThreadPtr = nullptr;

    std::string configFilename;
    std::string mLogfilePath;
    int mRunning;
    bool mInitialized = false;
    std::string mAAInfo;

    unsigned int mFrameCounter = 0;
    unsigned int mShotCounter = 0;

    typedef struct  {
        size_t mId;
        double mLastFired;
        double mInterval;
        std::function<void(size_t)> mCallback;
    } TimerInformation;

    std::vector<TimerInformation> mTimers; //< stores all active timers
    size_t mTimerID = 0; //< the timer created next will use this ID

    RunMode mRunMode = Default_Mode;
    std::string mGLSLVersion;
    int mExitKey = GLFW_KEY_ESCAPE;
};

} // namespace sgct

#endif // __SGCT__ENGINE__H__
