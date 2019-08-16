/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/Engine.h>

//#include <sgct/SGCTConfig.h>
#if INCLUDE_SGCT_TEXT
    #include <sgct/freetype.h>
    #include <sgct/FontManager.h>
#endif
#include <sgct/ClusterManager.h>
#include <sgct/MessageHandler.h>
#include <sgct/OffScreenBuffer.h>
#include <sgct/ReadConfig.h>
#include <sgct/SGCTMutexManager.h>
#include <sgct/SGCTSettings.h>
#include <sgct/SGCTUser.h>
#include <sgct/SGCTVersion.h>
#include <sgct/ShaderManager.h>
#include <sgct/SharedData.h>
#include <sgct/Statistics.h>
#include <sgct/TextureManager.h>
#include <sgct/Touch.h>
#include <sgct/Viewport.h>
#include <sgct/helpers/SGCTStringFunctions.h>
#include <sgct/shaders/SGCTInternalShaders.h>
#include <sgct/shaders/SGCTInternalShaders_modern.h>
#include <algorithm>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

//#define __SGCT_RENDER_LOOP_DEBUG__

sgct::Engine* sgct::Engine::mInstance = nullptr;
sgct_core::Touch sgct::Engine::mCurrentTouchPoints = sgct_core::Touch();

//Callback wrappers for GLFW
std::function<void(int, int)> gKeyboardCallbackFnPtr = nullptr;
std::function<void(int, int, int, int)> gKeyboardCallbackFnPtr2 = nullptr;
std::function<void(unsigned int)> gCharCallbackFnPtr = nullptr;
std::function<void(unsigned int, int)> gCharCallbackFnPtr2 = nullptr;
std::function<void(int, int, int)> gMouseButtonCallbackFnPtr = nullptr;
std::function<void(double, double)> gMousePosCallbackFnPtr = nullptr;
std::function<void(double, double)> gMouseScrollCallbackFnPtr = nullptr;
std::function<void(int, const char**)> gDropCallbackFnPtr = nullptr;
std::function<void(const sgct_core::Touch*)> gTouchCallbackFnPtr = nullptr;

void updateFrameLockLoop(void* arg);
bool sRunUpdateFrameLockLoop = true;

#ifdef GLEW_MX
GLEWContext* glewGetContext();
#endif

namespace {
    constexpr const bool UseSleepToWaitForNodes = false;
    constexpr const bool RunFrameLockCheckThread = true;
    constexpr const std::chrono::milliseconds FrameLockTimeout(100);
} // namespace

namespace sgct {

Engine* Engine::instance() {
    return mInstance;
}

/*!
This is the only valid constructor that also initiates [GLFW](http://www.glfw.org/). Command line parameters are used to load a configuration file and settings.
Note that parameter with one '\-' are followed by arguments but parameters with '\-\-' are just options without arguments.

Parameter     | Description
------------- | -------------
-config <filename> | set xml confiuration file
-logPath <filepath> | set log file path
--help | display help message and exit
-local <integer> | set which node in configuration that is the localhost (index starts at 0)
--client | run the application as client (only available when running as local)
--slave | run the application as client (only available when running as local)
--debug | set the notify level of messagehandler to debug
--Firm-Sync | enable firm frame sync
--Loose-Sync | disable firm frame sync
--Ignore-Sync | disable frame sync
-notify <integer> | set the notify level used in the MessageHandler (0 = highest priority)
--No-FBO | disable frame buffer objects (some stereo modes, Multi-Window rendering, FXAA and fisheye rendering will be disabled)
--Capture-PNG | use png images for screen capture (default)
--Capture-TGA | use tga images for screen capture
-MSAA <integer> | Enable MSAA as default (argument must be a power of two)
--FXAA | Enable FXAA as default
--gDebugger | Force textures to be genareted using glTexImage2D instead of glTexStorage2D
-numberOfCaptureThreads <integer> | set the maximum amount of threads that should be used during framecapture (default 8)

*/
Engine::Engine(int& argc, char**& argv) {
    //init pointers
    mInstance = this;

    for (unsigned int i = 0; i < MAX_UNIFORM_LOCATIONS; i++) {
        mShaderLocs[i] = -1;
    }

    setClearBufferFunction(clearBuffer);

    //parse needs to be before read config since the path to the XML is parsed here
    std::vector<std::string> arg(argc);
    for (int i = 0; i < argc; ++i) {
        arg[i] = std::string(argv[i]);
    }
    std::vector<std::string> oldArg = arg;
    parseArguments(arg);

    // remove the arguments that have been processed
    if (oldArg != arg) {
        char** newArgv = new char*[arg.size()];
        for (int i = 0; i < arg.size(); ++i) {
            char* ar = new char[arg[i].size() + 1];
            std::strcpy(ar, arg[i].c_str());
            newArgv[i] = ar;
        }

        argc = static_cast<int>(arg.size());
        argv = newArgv;
    }

    if (!mHelpMode) {
        // Initialize GLFW
        glfwSetErrorCallback(internal_glfw_error_callback);
        if (!glfwInit()) {
            mTerminate = true;
        }
    }
}

Engine::Engine(std::vector<std::string>& arg) {
    //init pointers
    mInstance = this;

    for (unsigned int i = 0; i < MAX_UNIFORM_LOCATIONS; i++) {
        mShaderLocs[i] = -1;
    }

    setClearBufferFunction(clearBuffer);

    //parse needs to be before read config since the path to the XML is parsed here
    parseArguments(arg);

    if (!mHelpMode) {
        // Initialize GLFW
        glfwSetErrorCallback(internal_glfw_error_callback);
        if (!glfwInit()) {
            mTerminate = true;
        }
    }
}


/*!
Engine destructor destructs GLFW and releases resources/memory.
*/
Engine::~Engine() {
    clean();
}

/*!
Engine initiation that:
 1. Parse the configuration file
 2. Set up the network communication
 3. Create window(s)
 4. Set up OpenGL
    4.1 Create textures
    4.2 Init FBOs
    4.3 Init VBOs
    4.4 Init PBOs

 @param rm rm is the optional run mode.
*/
bool Engine::init(RunMode rm) {
    mRunMode = rm;

    MessageHandler::instance()->print(
        MessageHandler::Level::VersionInfo,
        "%s\n", getSGCTVersion().c_str()
    );

    if (mHelpMode) {
        return false;
    }

    if (mTerminate) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Failed to init GLFW! Application will close in 5 seconds.\n"
        );
        sleep(5.0);
        return false;
    }

    mConfig = new sgct_core::ReadConfig(configFilename);
    if (!mConfig->isValid()) {
        // fatal error
        outputHelpMessage();
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Error in xml config file parsing. Application will close in 5 seconds.\n"
        );
        sleep(5.0);

        return false;
    }

    if (!initNetwork()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Network init error. Application will close in 5 seconds.\n"
        );
        sleep(5.0);
        return false;
    }

    if (!initWindows()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Window init error. Application will close in 5 seconds.\n"
        );
        sleep(5.0);
        return false;
    }

    // Window resolution may have been set when reading config.
    // However, it only sets a pending resolution, so it needs to apply it
    // using the same routine as in the end of a frame.
    for (size_t i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        mThisNode->setCurrentWindowIndex(i);
        getCurrentWindowPtr().updateResolutions();
    }

    //if a single node, skip syncing
    if (sgct_core::ClusterManager::instance()->getNumberOfNodes() == 1) {
        sgct_core::ClusterManager::instance()->setUseIgnoreSync(true);
    }

    for (size_t i=0; i < mThisNode->getNumberOfWindows(); i++) {
        if (gKeyboardCallbackFnPtr || gKeyboardCallbackFnPtr2) {
            glfwSetKeyCallback(getWindowPtr(i).getWindowHandle(), internal_key_callback);
        }
        if (gMouseButtonCallbackFnPtr) {
            glfwSetMouseButtonCallback(getWindowPtr(i).getWindowHandle(), internal_mouse_button_callback);
        }
        if (gMousePosCallbackFnPtr) {
            glfwSetCursorPosCallback(getWindowPtr(i).getWindowHandle(), internal_mouse_pos_callback);
        }
        if (gCharCallbackFnPtr) {
            glfwSetCharCallback(getWindowPtr(i).getWindowHandle(), internal_key_char_callback);
        }
        if (gCharCallbackFnPtr2) {
            glfwSetCharModsCallback(getWindowPtr(i).getWindowHandle(), internal_key_char_mods_callback);
        }
        if (gMouseScrollCallbackFnPtr) {
            glfwSetScrollCallback(getWindowPtr(i).getWindowHandle(), internal_mouse_scroll_callback);
        }
        if (gDropCallbackFnPtr) {
            glfwSetDropCallback(getWindowPtr(i).getWindowHandle(), internal_drop_callback);
        }
        if (gTouchCallbackFnPtr) {
            glfwSetTouchCallback(getWindowPtr(i).getWindowHandle(), internal_touch_callback);
        }
    }

    initOGL();

    //start sampling tracking data
    if (isMaster()) {
        getTrackingManager().startSampling();
    }

    mInitialized = true;
    return true;
}

/*!
    Terminates SGCT.
*/
void Engine::terminate() {
    mTerminate = true;
}

/*!
Initiates network communication.
*/
bool Engine::initNetwork() {
    try {
        mNetworkConnections = new sgct_core::NetworkManager(
            sgct_core::ClusterManager::instance()->getNetworkMode()
        );

    }
    catch(const char* err) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Initiating network connections failed! Error: '%s'\n", err
        );
        return false;
    }

    //check in cluster configuration which it is
    if (sgct_core::ClusterManager::instance()->getNetworkMode() ==
        sgct_core::NetworkManager::Remote)
    {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "Matching ip address to find node in configuration...\n"
        );
        mNetworkConnections->retrieveNodeId();
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "Running locally as node %d\n",
            sgct_core::ClusterManager::instance()->getThisNodeId()
        );
    }

    // If the user has provided the node id as an incorrect cmd argument then make the mThisNode invalid
    if (sgct_core::ClusterManager::instance()->getThisNodeId() >=
        static_cast<int>(sgct_core::ClusterManager::instance()->getNumberOfNodes()) ||
        sgct_core::ClusterManager::instance()->getThisNodeId() < 0)
    {
        //fatal error
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "This computer is not a part of the cluster configuration!\n"
        );
        mNetworkConnections->close();
        return false;
    }
    else {
        mThisNode = sgct_core::ClusterManager::instance()->getThisNodePtr(); //Set node pointer
    }

    //set logfile path
    if (!mLogfilePath.empty()) {
        MessageHandler::instance()->setLogPath(
            mLogfilePath.c_str(),
            sgct_core::ClusterManager::instance()->getThisNodeId()
        );
        MessageHandler::instance()->setLogToFile(true);
    }

    //Set message handler to send messages or not
    //MessageHandler::instance()->setSendFeedbackToServer( !mNetworkConnections->isComputerServer() );

    if (!mNetworkConnections->init()) {
        return false;
    }

    return true;
}

/*!
Create and initiate a window.
*/
bool Engine::initWindows() {
    if (mThisNode->getNumberOfWindows() == 0) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "No windows exist in configuration!\n"
        );
        return false;
    }

    int tmpGlfwVer[3];
    glfwGetVersion(&tmpGlfwVer[0], &tmpGlfwVer[1], &tmpGlfwVer[2]);
    MessageHandler::instance()->print(
        MessageHandler::Level::VersionInfo,
        "Using GLFW version %d.%d.%d\n",
        tmpGlfwVer[0], tmpGlfwVer[1], tmpGlfwVer[2]
    );

    switch (mRunMode) {
        case OpenGL_3_3_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 330 core";
            break;
        case OpenGL_4_0_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 400 core";
            break;
        case OpenGL_4_1_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 410 core";
            break;
        case OpenGL_4_2_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 420 core";
            break;
        case OpenGL_4_3_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 430 core";
            break;
        case OpenGL_4_4_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 440 core";
            break;
        case OpenGL_4_5_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 450 core";
            break;
        case OpenGL_4_6_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 460 core";
            break;
        case OpenGL_4_1_Debug_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 410 core";
            break;
        case OpenGL_4_2_Debug_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 420 core";
            break;
        case OpenGL_4_3_Debug_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 430 core";
            break;
        case OpenGL_4_4_Debug_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 440 core";
            break;
        case OpenGL_4_5_Debug_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 450 core";
            break;
        case OpenGL_4_6_Debug_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
#if __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 460 core";
            break;
        default:
            mGLSLVersion = "#version 120";
            break;
    }

    if (mPreWindowFnPtr != nullptr) {
        mPreWindowFnPtr();
    }

    mStatistics = new sgct_core::Statistics();
    GLFWwindow* share = nullptr;
    size_t lastWindowIdx = mThisNode->getNumberOfWindows() - 1;
    for (size_t i=0; i < mThisNode->getNumberOfWindows(); i++) {
        if (i > 0) {
            share = mThisNode->getWindowPtr(0).getWindowHandle();
        }
        
        if (!mThisNode->getWindowPtr(i).openWindow(share, lastWindowIdx)) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Failed to open window %d!\n", i
            );
            return false;
        }
    }

    GLenum err = glewInit();
    if (GLEW_OK != err) {
      //Problem: glewInit failed, something is seriously wrong.
      MessageHandler::instance()->print(
          MessageHandler::Level::Error,
          "GLEW error: %s!\n", glewGetErrorString(err)
      );
      return false;
    }
    MessageHandler::instance()->print(
        MessageHandler::Level::VersionInfo,
        "Using GLEW %s.\n", glewGetString(GLEW_VERSION)
    );

    if (!checkForOGLErrors()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error, "GLEW init triggered an OpenGL error.\n"
        );
    }

    /*
    -----------------------------------
    WINDOW/Context Creation callback
    -----------------------------------
    */
    if (mThisNode->getNumberOfWindows() > 0) {
        share = mThisNode->getWindowPtr(0).getWindowHandle();

        if (mContextCreationFnPtr != nullptr) {
            mContextCreationFnPtr(share);
        }
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error, "No windows created on this node!\n"
        );
        return false;
    }

    for (size_t i=0; i < mThisNode->getNumberOfWindows(); i++) {
        mThisNode->setCurrentWindowIndex(i);
        getCurrentWindowPtr().init();
        updateAAInfo(i);
    }

    updateDrawBufferResolutions();//init draw buffer resolution
    waitForAllWindowsInSwapGroupToOpen();

    if (RunFrameLockCheckThread) {
        if (sgct_core::ClusterManager::instance()->getNumberOfNodes() > 1) {
            mThreadPtr = new (std::nothrow) std::thread(updateFrameLockLoop, nullptr);
        }
    }

    //init swap group if enabled
    if (mThisNode->isUsingSwapGroups()) {
        SGCTWindow::initNvidiaSwapGroups();
    }

    return true;
}

/*!
Initiates OpenGL.
*/
void Engine::initOGL() {
    /*
        Set up function pointers etc. depending on if fixed or programmable pipeline is used
    */
    if (mRunMode > OpenGL_Compatibility_Profile) {
        mInternalDrawFn = [this]() { draw(); };
        mInternalRenderFBOFn = [this]() { renderFBOTexture(); };
        mInternalDrawOverlaysFn = [this]() { drawOverlays(); };
        mInternalRenderPostFXFn = [this](TextureIndexes idx) { renderPostFX(idx); };

        //force buffer objects since display lists are not supported in core opengl 3.3+
        sgct_core::ClusterManager::instance()->setMeshImplementation(
            sgct_core::ClusterManager::MeshImplementation::BufferObjects
        );
        mFixedOGLPipeline = false;
    }
    else {
        mInternalDrawFn = [this]() { drawFixedPipeline(); };
        mInternalRenderFBOFn = [this]() { renderFBOTextureFixedPipeline(); };
        mInternalDrawOverlaysFn = [this]() { drawOverlaysFixedPipeline(); };
        mInternalRenderPostFXFn = [this](TextureIndexes idx) {
            renderPostFXFixedPipeline(idx);
        };

        mFixedOGLPipeline = true;
    }

    //Get OpenGL version
    int mOpenGL_Version[3];
    mOpenGL_Version[0] = glfwGetWindowAttrib(
        getCurrentWindowPtr().getWindowHandle(),
        GLFW_CONTEXT_VERSION_MAJOR
    );
    mOpenGL_Version[1] = glfwGetWindowAttrib(
        getCurrentWindowPtr().getWindowHandle(),
        GLFW_CONTEXT_VERSION_MINOR
    );
    mOpenGL_Version[2] = glfwGetWindowAttrib(
        getCurrentWindowPtr().getWindowHandle(),
        GLFW_CONTEXT_REVISION
    );

    MessageHandler::instance()->print(
        MessageHandler::Level::VersionInfo,
        "OpenGL version %d.%d.%d %s\n",
        mOpenGL_Version[0], mOpenGL_Version[1], mOpenGL_Version[2],
        mFixedOGLPipeline ? "comp. profile" : "core profile"
    );

    MessageHandler::instance()->print(
        MessageHandler::Level::VersionInfo,
        "Vendor: %s\n", glGetString(GL_VENDOR)
    );

    MessageHandler::instance()->print(
        MessageHandler::Level::VersionInfo,
        "Renderer: %s\n", glGetString(GL_RENDERER)
    );

    if (!glfwExtensionSupported("GL_EXT_framebuffer_object") && mOpenGL_Version[0] < 2) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Warning! Frame buffer objects are not supported! "
            "A lot of features in SGCT will not work!\n"
        );
        SGCTSettings::instance()->setUseFBO(false);
    }
    else if (!glfwExtensionSupported("GL_EXT_framebuffer_multisample") && mOpenGL_Version[0] < 2) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Warning! FBO multisampling is not supported!\n"
        );
        SGCTSettings::instance()->setUseFBO(true);

        for (size_t i = 0; i < mThisNode->getNumberOfWindows(); i++) {
            mThisNode->getWindowPtr(i).setNumberOfAASamples(1);
        }
    }

    if (sgct_core::ClusterManager::instance()->getNumberOfNodes() > 1) {
        std::stringstream ss;
        ss << "_node" << sgct_core::ClusterManager::instance()->getThisNodeId();

        SGCTSettings::instance()->appendCapturePath(ss.str(), SGCTSettings::Mono);
        SGCTSettings::instance()->appendCapturePath(ss.str(), SGCTSettings::LeftStereo);
        SGCTSettings::instance()->appendCapturePath(ss.str(), SGCTSettings::RightStereo);
    }

    //init window opengl data
    getCurrentWindowPtr().makeOpenGLContextCurrent(SGCTWindow::Context::Shared);

    loadShaders();
    mStatistics->initVBO(mFixedOGLPipeline);

    if (mInitOGLFnPtr != nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Important,
            "\n---- Calling init callback ----\n"
        );
        mInitOGLFnPtr();
        MessageHandler::instance()->print(
            MessageHandler::Level::Important,
            "-------------------------------\n"
        );
    }

    //create all textures, etc
    for( size_t i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        mThisNode->setCurrentWindowIndex(i);
        getCurrentWindowPtr().initOGL(); //sets context to shared
        
        if (mScreenShotFnPtr1 != nullptr) {
            //set callback
            auto callback = [this](sgct_core::Image* img, size_t size,
                                   sgct_core::ScreenCapture::EyeIndex idx,
                                   unsigned int type)
            {
                invokeScreenShotCallback1(img, size, idx, type);
            };
            
            //left channel (Mono and Stereo_Left)
            if (getCurrentWindowPtr().getScreenCapturePointer(0) != nullptr) {
                getCurrentWindowPtr().getScreenCapturePointer(0)->setCaptureCallback(callback);
            }
            //right channel (Stereo_Right)
            if (getCurrentWindowPtr().getScreenCapturePointer(1) != nullptr) {
                getCurrentWindowPtr().getScreenCapturePointer(1)->setCaptureCallback(callback);
            }
        }
        else if (mScreenShotFnPtr2 != nullptr) {
            //set callback
            auto callback = [this](unsigned char* img, size_t size,
                                   sgct_core::ScreenCapture::EyeIndex idx,
                                   unsigned int type)
            {
                invokeScreenShotCallback2(img, size, idx, type);
            };

            //left channel (Mono and Stereo_Left)
            if (getCurrentWindowPtr().getScreenCapturePointer(0) != nullptr) {
                getCurrentWindowPtr().getScreenCapturePointer(0)->setCaptureCallback(callback);
            }
            //right channel (Stereo_Right)
            if (getCurrentWindowPtr().getScreenCapturePointer(1) != nullptr) {
                getCurrentWindowPtr().getScreenCapturePointer(1)->setCaptureCallback(callback);
            }
        }
    }

    //link all users to their viewports
    for (size_t w = 0; w < mThisNode->getNumberOfWindows(); w++) {
        SGCTWindow& winPtr = mThisNode->getWindowPtr(w);
        for (unsigned int i = 0; i < winPtr.getNumberOfViewports(); i++) {
            winPtr.getViewport(i).linkUserName();
        }
    }

    updateFrustums();

    //
    // Add fonts
    //
#if INCLUDE_SGCT_TEXT
    if (SGCTSettings::instance()->getOSDTextFontPath().empty()) {
        if (!sgct_text::FontManager::instance()->addFont(
                "SGCTFont",
                SGCTSettings::instance()->getOSDTextFontName())
            )
        {
            sgct_text::FontManager::instance()->getFont(
                "SGCTFont",
                SGCTSettings::instance()->getOSDTextFontSize()
            );
        }
    }
    else {
        std::string tmpPath = SGCTSettings::instance()->getOSDTextFontPath() +
                              SGCTSettings::instance()->getOSDTextFontName();
        if (!sgct_text::FontManager::instance()->addFont(
                "SGCTFont",
                tmpPath,
                sgct_text::FontManager::Local)
            )
        {
            sgct_text::FontManager::instance()->getFont(
                "SGCTFont",
                SGCTSettings::instance()->getOSDTextFontSize()
            );
        }
    }
#endif

    //init swap barrier is swap groups are active
    SGCTWindow::setBarrier(true);
    SGCTWindow::resetSwapGroupFrameNumber();

    for (size_t i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        mThisNode->setCurrentWindowIndex(i);

        //generate mesh (VAO and VBO)
        getCurrentWindowPtr().initContextSpecificOGL();
    }

    //check for errors
    checkForOGLErrors();

    MessageHandler::instance()->print(
        MessageHandler::Level::Important,
        "\nReady to render!\n"
    );
}

/*!
Clean up all resources and release memory.
*/
void Engine::clean() {
    MessageHandler::instance()->print(
        MessageHandler::Level::Important,
        "Cleaning up...\n"
    );

    if (mCleanUpFnPtr != nullptr) {
        if (mThisNode != nullptr && mThisNode->getNumberOfWindows() > 0) {
            mThisNode->getWindowPtr(0).makeOpenGLContextCurrent(
                SGCTWindow::Context::Shared
            );
        }
        mCleanUpFnPtr();
    }

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Clearing all callbacks...\n"
    );
    clearAllCallbacks();

    //kill thread
    if (mThreadPtr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "Waiting for frameLock thread to finish...\n"
        );

        sgct::SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::FrameSyncMutex );
        sRunUpdateFrameLockLoop = false;
        sgct::SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::FrameSyncMutex );

        mThreadPtr->join();
        delete mThreadPtr;
        mThreadPtr = nullptr;
        MessageHandler::instance()->print(MessageHandler::Level::Debug, "Done.\n");
    }

    //de-init window and unbind swapgroups...
    if (sgct_core::ClusterManager::instance()->getNumberOfNodes() > 0) {
        if (mThisNode != nullptr) {
            for (size_t i = 0; i < mThisNode->getNumberOfWindows(); i++) {
                mThisNode->getWindowPtr(i).close();
            }
        }
    }

    //close TCP connections
    if (mNetworkConnections != nullptr) {
        delete mNetworkConnections;
        mNetworkConnections = nullptr;
    }

    if ( mConfig != nullptr) {
        delete mConfig;
        mConfig = nullptr;
    }

    // Destroy explicitly to avoid memory leak messages
    // Shared contex -------------------------------------------------------------------->
    if (mThisNode != nullptr && mThisNode->getNumberOfWindows() > 0) {
        mThisNode->getWindowPtr(0).makeOpenGLContextCurrent(
            SGCTWindow::Context::Shared
        );
    }
    if (mStatistics != nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Deleting stats data...\n"
        );
        delete mStatistics;
        mStatistics = nullptr;
    }

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Destroying shader manager and internal shaders...\n"
    );
    ShaderManager::destroy();
    for (size_t i = 0; i < NUMBER_OF_SHADERS; i++) {
        mShaders[i].deleteProgram();
    }

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Destroying texture manager...\n"
    );
    TextureManager::destroy();

#if INCLUDE_SGCT_TEXT
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Destroying font manager...\n"
    );
    sgct_text::FontManager::destroy();
#endif

    //Window specific context ----------------------------------------------------------->
    if (mThisNode != nullptr && mThisNode->getNumberOfWindows() > 0) {
        mThisNode->getWindowPtr(0).makeOpenGLContextCurrent(
            SGCTWindow::Context::Window
        );
    }
    
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Destroying shared data...\n"
    );
    SharedData::destroy();
    
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Destroying cluster manager...\n"
    );
    sgct_core::ClusterManager::destroy();
    
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Destroying settings...\n"
    );
    SGCTSettings::destroy();

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Destroying message handler...\n"
    );
    MessageHandler::destroy();

    std::cout << "Destroying mutexes...\n" << std::endl;
    SGCTMutexManager::destroy();

    // Close window and terminate GLFW
    std::cout << std::endl << "Terminating glfw...";
    glfwTerminate();
    std::cout << " Done." << std::endl;
}

/*!
Un-binds all callbacks.
*/
void Engine::clearAllCallbacks() {
    mDrawFnPtr = nullptr;
    mDraw2DFnPtr = nullptr;
    mPreSyncFnPtr = nullptr;
    mPostSyncPreDrawFnPtr = nullptr;
    mPostDrawFnPtr = nullptr;
    mInitOGLFnPtr = nullptr;
    mPreWindowFnPtr = nullptr;
    mClearBufferFnPtr = nullptr;
    mCleanUpFnPtr = nullptr;
    mExternalDecodeCallbackFnPtr = nullptr;
    mExternalStatusCallbackFnPtr = nullptr;
    mDataTransferDecodeCallbackFnPtr = nullptr;
    mDataTransferStatusCallbackFnPtr = nullptr;
    mDataTransferAcknowledgeCallbackFnPtr = nullptr;
    mContextCreationFnPtr = nullptr;
    mScreenShotFnPtr1 = nullptr;
    mScreenShotFnPtr2 = nullptr;

    mInternalDrawFn = nullptr;
    mInternalRenderFBOFn = nullptr;
    mInternalDrawOverlaysFn = nullptr;
    mInternalRenderPostFXFn = nullptr;

    //global
    gKeyboardCallbackFnPtr = nullptr;
    gKeyboardCallbackFnPtr2 = nullptr;
    gCharCallbackFnPtr = nullptr;
    gMouseButtonCallbackFnPtr = nullptr;
    gMousePosCallbackFnPtr = nullptr;
    gMouseScrollCallbackFnPtr = nullptr;
    gDropCallbackFnPtr = nullptr;
    gTouchCallbackFnPtr = nullptr;

    for (unsigned int i = 0; i < mTimers.size(); i++) {
        mTimers[i].mCallback = nullptr;
    }
}

/*!
Locks the rendering thread for synchronization. The two stages are:

1. PreStage, locks the slaves until data is successfully received
2. PostStage, locks master until slaves are ready to swap buffers

Sync time from statistics is the time each computer waits for sync.
*/
bool Engine::frameLock(sgct::Engine::SyncStage stage) {
    if (stage == SyncStage::PreStage) {
        double t0 = glfwGetTime();
        // from server to clients
        mNetworkConnections->sync(
            sgct_core::NetworkManager::SyncMode::SendDataToClients,
            mStatistics
        ); 
        mStatistics->setSyncTime(static_cast<float>(glfwGetTime() - t0));

        //run only on clients/slaves
        if (!sgct_core::ClusterManager::instance()->getIgnoreSync() &&
            !mNetworkConnections->isComputerServer()) 
        {
            // not server
            t0 = glfwGetTime();
            while (mNetworkConnections->isRunning() && mRunning) {
                if (mNetworkConnections->isSyncComplete()) {
                    break;
                }

                if (UseSleepToWaitForNodes) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                else {
                    std::unique_lock<std::mutex> lk(
                        SGCTMutexManager::instance()->getMutexPtr(
                            SGCTMutexManager::FrameSyncMutex
                        )
                    );
                    sgct_core::NetworkManager::gCond.wait(lk);
                    //SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::FrameSyncMutex );
                    //sgct_core::NetworkManager::gCond.wait(std::unique_lock<std::mutex>() );
                    //SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::FrameSyncMutex );
                }
                
                //for debugging
                sgct_core::SGCTNetwork* conn;
                if (glfwGetTime() - t0 > 1.0)  {
                    // more than a second
                    conn = mNetworkConnections->getSyncConnectionByIndex(0);
                    if (mPrintSyncMessage && !conn->isUpdated()) {
                        MessageHandler::instance()->print(
                            MessageHandler::Level::Info,
                            "Slave: waiting for master... send frame %d != previous recv "
                            "frame %d\n\tNvidia swap groups: %s\n\tNvidia swap barrier: "
                            "%s\n\tNvidia universal frame number: %u\n\tSGCT frame "
                            "number: %u\n",
                            conn->getSendFrame(),
                            conn->getRecvFrame(sgct_core::SGCTNetwork::Previous),
                            getCurrentWindowPtr().isUsingSwapGroups() ? "enabled" : "disabled",
                            getCurrentWindowPtr().isBarrierActive() ? "enabled" : "disabled",
                            getCurrentWindowPtr().getSwapGroupFrameNumber(),
                            mFrameCounter
                        );
                    }
                    
                    if (glfwGetTime() - t0 > mSyncTimeout) {
                        // more than a minute
                        MessageHandler::instance()->print(
                            MessageHandler::Level::Error,
                            "Slave: no sync signal from master after %.1f seconds! "
                            "Exiting...",
                            mSyncTimeout
                        );
                        return false;
                    }
                }
            } //end while wait loop

            /*
                A this point all data needed for rendering a frame is received.
                Let's signal that back to the master/server.
            */
            mNetworkConnections->sync(
                sgct_core::NetworkManager::SyncMode::AcknowledgeData,
                mStatistics
            );

            mStatistics->addSyncTime(static_cast<float>(glfwGetTime() - t0));
        }//end if client
    }
    else {
        // post stage
        if (!sgct_core::ClusterManager::instance()->getIgnoreSync() &&
            mNetworkConnections->isComputerServer())//&&
            //mConfig->isMasterSyncLocked() &&
            /*localRunningMode == NetworkManager::Remote &&*/
            //!getCurrentWindowPtr()->isBarrierActive() )//post stage
        {
            double t0 = glfwGetTime();
            while (mNetworkConnections->isRunning() && mRunning &&
                   mNetworkConnections->getActiveConnectionsCount() > 0)
            {
                if (mNetworkConnections->isSyncComplete()) {
                    break;
                }

                if (UseSleepToWaitForNodes) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                else {
                    std::unique_lock<std::mutex> lk(
                        SGCTMutexManager::instance()->getMutexPtr(
                            SGCTMutexManager::FrameSyncMutex
                        )
                    );
                    sgct_core::NetworkManager::gCond.wait(lk);

                    //SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::FrameSyncMutex );
                    //sgct_core::NetworkManager::gCond.wait(std::unique_lock<std::mutex>(*SGCTMutexManager::instance()->getMutexPtr( SGCTMutexManager::FrameSyncMutex )) );
                    //SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::FrameSyncMutex );
                }

                //for debuging
                sgct_core::SGCTNetwork* conn;
                if (glfwGetTime() - t0 > 1.0) {
                    // more than a second
                    for (unsigned int i=0;
                         i < mNetworkConnections->getSyncConnectionsCount();
                         i++)
                    {
                        conn = mNetworkConnections->getConnectionByIndex(i);
                        if (mPrintSyncMessage && !conn->isUpdated()) {
                            MessageHandler::instance()->print(
                                MessageHandler::Level::Info,
                                "Waiting for slave%d: send frame %d != recv frame %d\n\t"
                                "Nvidia swap groups: %s\n\tNvidia swap barrier: %s\n\t"
                                "Nvidia universal frame number: %u\n\t"
                                "SGCT frame number: %u\n",
                                i,
                                mNetworkConnections->getConnectionByIndex(i)->getSendFrame(),
                                mNetworkConnections->getConnectionByIndex(i)->getRecvFrame(
                                    sgct_core::SGCTNetwork::Current
                                ),
                                getCurrentWindowPtr().isUsingSwapGroups() ? "enabled" : "disabled",
                                getCurrentWindowPtr().isBarrierActive() ? "enabled" : "disabled",
                                getCurrentWindowPtr().getSwapGroupFrameNumber(),
                                mFrameCounter
                            );
                        }
                    }
                    
                    if (glfwGetTime() - t0 > mSyncTimeout) {
                        // more than a minute
                        MessageHandler::instance()->print(
                            MessageHandler::Level::Error,
                            "Master: no sync signal from all slaves after %.1f seconds! "
                            "Exiting...",
                            mSyncTimeout
                        );
                        
                        return false;
                    }
                }
            } //end while
            mStatistics->addSyncTime(static_cast<float>(glfwGetTime() - t0));
        }//end if server
    }
    
    return true;
}

/*!
    This is SGCT's renderloop where rendering & synchronization takes place.
*/
void Engine::render() {
    if (!mInitialized) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Render function called before initialization."
        );
        return;
    }
    
    mRunning = GL_TRUE;

    //create openGL query objects for opengl 3.3+
    GLuint time_queries[2];
    if (!mFixedOGLPipeline) {
        getCurrentWindowPtr().makeOpenGLContextCurrent(
            SGCTWindow::Context::Shared
        );
        glGenQueries(2, time_queries);
    }

    while (mRunning) {
        mRenderingOffScreen = false;

#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(
            MessageHandler::Info,
            "Render-Loop: Updating tracking devices.\n"
        );
#endif

        //update tracking data
        if (isMaster()) {
            sgct_core::ClusterManager::instance()->getTrackingManagerPtr().updateTrackingDevices();
        }

#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Render-Loop: Running pre-sync.\n"
        );
#endif
        if (mPreSyncFnPtr != nullptr) {
            mPreSyncFnPtr();
        }

        if ( mNetworkConnections->isComputerServer()) {
#ifdef __SGCT_RENDER_LOOP_DEBUG__
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Render-Loop: Encoding data.\n"
            );
#endif
            SharedData::instance()->encode();
        }
        else {
            if ( !mNetworkConnections->isRunning())  {
                // exit if not running
                MessageHandler::instance()->print(
                    MessageHandler::Level::Error,
                    "Network disconnected! Exiting...\n"
                );
                break;
            }
        }

#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_INFO,
            "Render-Loop: Sync/framelock\n"
        );
#endif
        if (!frameLock(SyncStage::PreStage)) {
            break;
        }

#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_INFO,
            "Render-Loop: running post-sync-pre-draw\n"
        );
#endif

        // check if re-size needed of VBO and PBO
        // context switching may occur if multiple windows are used
        bool buffersNeedUpdate = false;
        for (size_t i = 0; i < mThisNode->getNumberOfWindows(); i++) {
            if (mThisNode->getWindowPtr(i).update()) {
                buffersNeedUpdate = true;
            }
        }

        if (buffersNeedUpdate) {
            updateDrawBufferResolutions();
        }
    
        mRenderingOffScreen = SGCTSettings::instance()->useFBO();
        if (mRenderingOffScreen) {
            getCurrentWindowPtr().makeOpenGLContextCurrent(
                SGCTWindow::Context::Shared
            );
        }

        //Make sure correct context is current
        if (mPostSyncPreDrawFnPtr != nullptr) {
            mPostSyncPreDrawFnPtr();
        }

        double startFrameTime = glfwGetTime();
        calculateFPS(startFrameTime); //measures time between calls

        if (!mFixedOGLPipeline && mShowGraph) {
            glQueryCounter(time_queries[0], GL_TIMESTAMP);
        }

        //--------------------------------------------------------------
        //     RENDER VIEWPORTS / DRAW
        //--------------------------------------------------------------
        mCurrentDrawBufferIndex = 0;
        size_t firstDrawBufferIndexInWindow = 0;

        for (size_t i = 0; i < mThisNode->getNumberOfWindows(); i++) {
            if (mThisNode->getWindowPtr(i).isVisible() ||
                mThisNode->getWindowPtr(i).isRenderingWhileHidden())
            {
                //store the first buffer index for each window
                firstDrawBufferIndexInWindow = mCurrentDrawBufferIndex;

                mThisNode->setCurrentWindowIndex(i);
                SGCTWindow& win = getCurrentWindowPtr();

                if (!mRenderingOffScreen) {
                    win.makeOpenGLContextCurrent(SGCTWindow::Context::Window);
                }

                SGCTWindow::StereoMode sm = win.getStereoMode();

                //--------------------------------------------------------------
                //     RENDER LEFT/MONO NON LINEAR PROJECTION VIEWPORTS TO CUBEMAP
                //--------------------------------------------------------------
                mCurrentRenderTarget = NonLinearBuffer;
                sgct_core::NonLinearProjection * nonLinearProjPtr;
                for (size_t j = 0; j < win.getNumberOfViewports(); j++) {
                    mCurrentViewportIndex[MainViewport] = j;

                    if (win.getViewport(j).hasSubViewports()) {
#ifdef __SGCT_RENDER_LOOP_DEBUG__
                        MessageHandler::instance()->print(
                            MessageHandler::NOTIFY_INFO,
                            "Render-Loop: Rendering sub-viewports\n"
                        );
#endif
                        nonLinearProjPtr = win.getViewport(j).getNonLinearProjectionPtr();
                        mCurrentOffScreenBuffer = nonLinearProjPtr->getOffScreenBuffer();

                        nonLinearProjPtr->setAlpha(getCurrentWindowPtr().getAlpha() ? 0.f : 1.f);
                        if (sm == SGCTWindow::StereoMode::NoStereo) {
                            //for mono viewports frustum mode can be selected by user or xml
                            mCurrentFrustumMode = win.getViewport(j).getEye();
                            nonLinearProjPtr->renderCubemap(&mCurrentViewportIndex[SubViewport]);
                        }
                        else {
                            mCurrentFrustumMode = sgct_core::Frustum::StereoLeftEye;
                            nonLinearProjPtr->renderCubemap(&mCurrentViewportIndex[SubViewport]);
                        }

                        //FBO index, every window and every non-linear projection has it's own FBO
                        mCurrentDrawBufferIndex++;
                    }
                }

                //--------------------------------------------------------------
                //     RENDER LEFT/MONO REGULAR VIEWPORTS TO FBO
                //--------------------------------------------------------------
                mCurrentRenderTarget = WindowBuffer;
                mCurrentOffScreenBuffer = win.getFBOPtr();

#ifdef __SGCT_RENDER_LOOP_DEBUG__
                MessageHandler::instance()->print(
                    MessageHandler::NOTIFY_INFO,
                    "Render-Loop: Rendering\n"
                );
#endif
                //if any stereo type (except passive) then set frustum mode to left eye
                if (sm == SGCTWindow::StereoMode::NoStereo) {
                    mCurrentFrustumMode = sgct_core::Frustum::MonoEye;
                    renderViewports(LeftEye);
                }
                else {
                    mCurrentFrustumMode = sgct_core::Frustum::StereoLeftEye;
                    renderViewports(LeftEye);
                }

                //FBO index, every window and every non-linear projection has it's own FBO
                mCurrentDrawBufferIndex++;

                //if stereo
                if (sm != SGCTWindow::StereoMode::NoStereo) {
                    //jump back counter to the first buffer index for current window
                    mCurrentDrawBufferIndex = firstDrawBufferIndexInWindow;

                    //--------------------------------------------------------------
                    //     RENDER RIGHT NON LINEAR PROJECTION VIEWPORTS TO CUBEMAP
                    //--------------------------------------------------------------
                    mCurrentRenderTarget = NonLinearBuffer;
                    sgct_core::NonLinearProjection * nonLinearProjPtr;
                    for (size_t j = 0; j < win.getNumberOfViewports(); j++) {
                        mCurrentViewportIndex[MainViewport] = j;

                        if (win.getViewport(j).hasSubViewports()) {
#ifdef __SGCT_RENDER_LOOP_DEBUG__
                            MessageHandler::instance()->print(
                                MessageHandler::NOTIFY_INFO,
                                "Render-Loop: Rendering sub-viewports\n"
                            );
#endif
                            nonLinearProjPtr = win.getViewport(j).getNonLinearProjectionPtr();
                            mCurrentOffScreenBuffer = nonLinearProjPtr->getOffScreenBuffer();

                            nonLinearProjPtr->setAlpha(getCurrentWindowPtr().getAlpha() ? 0.f : 1.f);
                            mCurrentFrustumMode = sgct_core::Frustum::StereoRightEye;
                            nonLinearProjPtr->renderCubemap(&mCurrentViewportIndex[SubViewport]);

                            //FBO index, every window and every non-linear projection has it's own FBO
                            mCurrentDrawBufferIndex++;
                        }
                    }

                    //--------------------------------------------------------------
                    //     RENDER RIGHT REGULAR VIEWPORTS TO FBO
                    //--------------------------------------------------------------
                    mCurrentRenderTarget = WindowBuffer;
                    mCurrentOffScreenBuffer = win.getFBOPtr();

#ifdef __SGCT_RENDER_LOOP_DEBUG__
                    MessageHandler::instance()->print(
                        MessageHandler::NOTIFY_INFO,
                        "Render-Loop: Rendering\n"
                    );
#endif
                    mCurrentFrustumMode = sgct_core::Frustum::StereoRightEye;
                    //use a single texture for side-by-side and top-bottom stereo modes
                    sm >= SGCTWindow::StereoMode::SideBySide ?
                        renderViewports(LeftEye) :
                        renderViewports(RightEye);

                    //FBO index, every window and every non-linear projection has it's own FBO
                    mCurrentDrawBufferIndex++;
                }

                //--------------------------------------------------------------
                //           DONE RENDERING VIEWPORTS TO FBO
                //--------------------------------------------------------------

#ifdef __SGCT_RENDER_LOOP_DEBUG__
                MessageHandler::instance()->print(
                    MessageHandler::Level::Info,
                    "Render-Loop: Rendering FBO quad\n"
                );
#endif
            }//end window loop
        }

        //--------------------------------------------------------------
        //           RENDER TO SCREEN
        //--------------------------------------------------------------
        for (size_t i = 0; i < mThisNode->getNumberOfWindows(); i++) {
            if (mThisNode->getWindowPtr(i).isVisible()) {
                mThisNode->setCurrentWindowIndex(i);

                mRenderingOffScreen = false;
                if (SGCTSettings::instance()->useFBO()) {
                    mInternalRenderFBOFn();
                }
            }
        }
#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Render-Loop: Running post-sync\n"
        );
#endif
        /*
            For single threaded rendering glFinish should be fine to use for frame sync.
            For multitheded usage a glFenceSync fence should be used to synchronize all GPU threads.

            example: GLsync mFence = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
            Then on each thread: glWaitSync(mFence);
        */
        //glFinish(); //wait for all rendering to finish /* ATI doesn't like this.. the 
        // framerate is halfed if it's used. */

        getCurrentWindowPtr().makeOpenGLContextCurrent(
            SGCTWindow::Context::Shared
        );

#ifdef __SGCT_DEBUG__
        //check for errors
        checkForOGLErrors();
#endif

#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Render-Loop: swap and update data\n"
        );
#endif
        
        if (!mFixedOGLPipeline && mShowGraph) {
            glQueryCounter(time_queries[1], GL_TIMESTAMP);
        }

        double endFrameTime = glfwGetTime();
        updateTimers(endFrameTime);

        //run post frame actions
        if (mPostDrawFnPtr != nullptr) {
            mPostDrawFnPtr();
        }

        //update stats
        if (mFixedOGLPipeline) {
            mStatistics->setDrawTime(static_cast<float>(endFrameTime - startFrameTime));
        }
        else {
            if (mShowGraph) {
                //double t = glfwGetTime();
                //int counter = 0;

                // wait until the query results are available
                GLint done = GL_FALSE;
                while (!done) {
                    glGetQueryObjectiv(
                        time_queries[1],
                        GL_QUERY_RESULT_AVAILABLE,
                        &done
                    );

                    //counter++;
                }

                //fprintf(stderr, "Wait: %d %lf\n", counter, (glfwGetTime() - t) * 1000.0);

                GLuint64 timerStart;
                GLuint64 timerEnd;
                // get the query results
                glGetQueryObjectui64v(time_queries[0], GL_QUERY_RESULT, &timerStart);
                glGetQueryObjectui64v(time_queries[1], GL_QUERY_RESULT, &timerEnd);

                double elapsedTime = static_cast<double>(timerEnd - timerStart) / 1000000000.0;
                mStatistics->setDrawTime(static_cast<float>(elapsedTime));
            }
        }

        if (mShowGraph) {
#ifdef __SGCT_RENDER_LOOP_DEBUG__
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Render-Loop: update stats VBOs\n"
            );
#endif
            mStatistics->update();
        }
        
#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_INFO,
            "Render-Loop: lock\n"
        );
#endif
        // master will wait for nodes render before swapping
        if (!frameLock(SyncStage::PostStage)) {
            break;
        }

#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_INFO,
            "Render-Loop: Swap buffers\n"
        );
#endif
        // Swap front and back rendering buffers
        for (size_t i = 0; i < mThisNode->getNumberOfWindows(); i++) {
            mThisNode->setCurrentWindowIndex(i);
            getCurrentWindowPtr().swap(mTakeScreenshot);
        }

        glfwPollEvents();
        for (size_t i = 0; i < mThisNode->getNumberOfWindows(); i++) {
            mThisNode->setCurrentWindowIndex(i);
            getCurrentWindowPtr().updateResolutions();
        }

        // Check if ESC key was pressed or window was closed
        mRunning = !(mThisNode->getKeyPressed(mExitKey) ||
            mThisNode->shouldAllWindowsClose() || mTerminate ||
            !mNetworkConnections->isRunning());

        //for all windows
        mFrameCounter++;
        if (mTakeScreenshot) {
            mShotCounter++;
        }
        mTakeScreenshot = false;

#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Render-Loop: End iteration\n"
        );
#endif
    }

    if (!mFixedOGLPipeline) {
        getCurrentWindowPtr().makeOpenGLContextCurrent(
            SGCTWindow::Context::Shared
        );
        glDeleteQueries(2, time_queries);
    }
}

/*!
    Set the configuration file path. Must be done before Engine::init().
*/
void Engine::setConfigurationFile(std::string configFilePath) {
    configFilename = std::move(configFilePath);
}

/*!
    This function renders basic text info and statistics on screen.
*/
void Engine::renderDisplayInfo() {
#if INCLUDE_SGCT_TEXT
    unsigned int lFrameNumber = 0;
    getCurrentWindowPtr()->getSwapGroupFrameNumber(lFrameNumber);

    glm::vec4 strokeColor = sgct_text::FontManager::instance()->getStrokeColor();
    sgct_text::FontManager::instance()->setStrokeColor(glm::vec4(0.f, 0.f, 0.f, 0.8f));

    unsigned int font_size = SGCTSettings::instance()->getOSDTextFontSize();
    font_size = static_cast<unsigned int>(
        static_cast<float>(font_size) * getCurrentWindowPtr()->getXScale()
    );
    
    sgct_text::Font* font = sgct_text::FontManager::instance()->getFont(
        "SGCTFont",
        font_size
    );

    if (font != nullptr) {
        float lineHeight = font->getHeight() * 1.59f;
        float xPos = static_cast<float>(getCurrentWindowPtr()->getXResolution()) *
                     SGCTSettings::instance()->getOSDTextXOffset();
        float yPos = static_cast<float>(getCurrentWindowPtr()->getYResolution()) *
                     SGCTSettings::instance()->getOSDTextYOffset();
        
        sgct_text::print(
            font,
            sgct_text::TopLeft,
            xPos,
            lineHeight * 6.f + yPos,
            glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
            "Node ip: %s (%s)",
            mThisNode->getAddress().c_str(),
            mNetworkConnections->isComputerServer() ? "master" : "slave"
        );

        sgct_text::print(
            font,
            sgct_text::TopLeft,
            xPos,
            lineHeight * 5.f + yPos,
            glm::vec4(0.8f,0.8f,0.f,1.f),
            "Frame rate: %.2f Hz, frame: %u",
            mStatistics->getAvgFPS(),
            mFrameCounter
        );

        sgct_text::print(
            font,
            sgct_text::TopLeft,
            xPos,
            lineHeight * 4.f + yPos,
            glm::vec4(0.8f, 0.f, 0.8f, 1.f),
            "Avg. draw time: %.2f ms",
            mStatistics->getAvgDrawTime() * 1000.f
        );

        if (isMaster()) {
            sgct_text::print(
                font,
                sgct_text::TopLeft,
                xPos,
                lineHeight * 3.f + yPos,
                glm::vec4(0.f,0.8f,0.8f,1.f),
                "Avg. sync time: %.2f ms (%d bytes, comp: %.3f)",
                mStatistics->getAvgSyncTime() * 1000.0,
                SharedData::instance()->getUserDataSize(),
                SharedData::instance()->getCompressionRatio()
            );
        }
        else {
            sgct_text::print(
                font,
                sgct_text::TopLeft,
                xPos,
                lineHeight * 3.f + yPos,
                glm::vec4(0.f, 0.8f, 0.8f, 1.f),
                "Avg. sync time: %.2f ms",
                mStatistics->getAvgSyncTime() * 1000.0
            );
        }

        bool usingSwapGroups = getCurrentWindowPtr()->isUsingSwapGroups();
        if (usingSwapGroups) {
            sgct_text::print(
                font,
                sgct_text::TopLeft,
                xPos,
                lineHeight * 2.f + yPos,
                glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
                "Swap groups: %s and barrier is %s (%s) | Frame: %d",
                getCurrentWindowPtr()->isUsingSwapGroups() ? "Enabled" : "Disabled",
                getCurrentWindowPtr()->isBarrierActive() ? "active" : "inactive",
                getCurrentWindowPtr()->isSwapGroupMaster() ? "master" : "slave",
                lFrameNumber
            );
        }
        else {
            sgct_text::print(
                font,
                sgct_text::TopLeft,
                xPos,
                lineHeight * 2.f + yPos,
                glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
                "Swap groups: Disabled"
            );
        }

        sgct_text::print(
            font,
            sgct_text::TopLeft,
            xPos,
            lineHeight * 1.f + yPos,
            glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
            "Frame buffer resolution: %d x %d",
            getCurrentWindowPtr()->getXFramebufferResolution(),
            getCurrentWindowPtr()->getYFramebufferResolution()
        );

        sgct_text::print(
            font,
            sgct_text::TopLeft,
            xPos,
            lineHeight * 0.f + yPos,
            glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
            "Anti-Aliasing: %s",
            mAAInfo.c_str()
        );

        //if active stereoscopic rendering
        if (mCurrentFrustumMode == sgct_core::Frustum::StereoLeftEye) {
            sgct_text::print(
                font,
                sgct_text::TopLeft,
                xPos,
                lineHeight * 8.f + yPos,
                glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
                "Stereo type: %s\nCurrent eye: Left",
                getCurrentWindowPtr()->getStereoModeStr().c_str()
            );
        }
        else if (mCurrentFrustumMode == sgct_core::Frustum::StereoRightEye) {
            sgct_text::print(
                font,
                sgct_text::TopLeft,
                xPos,
                lineHeight * 8.f + yPos,
                glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
                "Stereo type: %s\nCurrent eye:          Right",
                getCurrentWindowPtr()->getStereoModeStr().c_str()
            );
        }
    }

    //reset
    sgct_text::FontManager::instance()->setStrokeColor(strokeColor);
#endif
}

/*!
    This function enters the correct viewport, frustum, stereo mode and calls the draw callback.
*/
void Engine::draw() {
    //run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);
    
    enterCurrentViewport();
    
    //clear buffers
    SGCTSettings::instance()->useFBO() ?
        setAndClearBuffer(RenderToTexture) :
        setAndClearBuffer(BackBuffer);
    
    glDisable(GL_SCISSOR_TEST);

    if (mDrawFnPtr != nullptr) {
        glLineWidth(1.0);
        mShowWireframe ?
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) :
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        mDrawFnPtr();

        // restore polygon mode
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

/*!
    This function enters the correct viewport, frustum, stereo mode and calls the draw callback.
*/
void Engine::drawFixedPipeline() {
    //run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);
    
    enterCurrentViewport(); //set glViewport & glScissor
    
    //clear buffers
    SGCTSettings::instance()->useFBO() ?
        setAndClearBuffer(RenderToTexture) :
        setAndClearBuffer(BackBuffer);
    
    glDisable(GL_SCISSOR_TEST);

    glMatrixMode(GL_PROJECTION);

    sgct_core::SGCTProjection& proj =
        getCurrentWindowPtr().getCurrentViewport()->getProjection(mCurrentFrustumMode);

    glLoadMatrixf(glm::value_ptr(proj.getProjectionMatrix()));

    glMatrixMode(GL_MODELVIEW);

    glLoadMatrixf(glm::value_ptr(proj.getViewMatrix() * getModelMatrix()));

    if (mDrawFnPtr != nullptr) {
        glLineWidth(1.0);
        mShowWireframe ?
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) :
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        mDrawFnPtr();

        //restore polygon mode
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

/*!
    Draw viewport overlays if there are any.
*/
void Engine::drawOverlays() {
    for (size_t i = 0; i < getCurrentWindowPtr().getNumberOfViewports(); i++) {
        getCurrentWindowPtr().setCurrentViewport(i);

        //if viewport has overlay
        const sgct_core::Viewport& vp = getCurrentWindowPtr().getViewport(i);
        
        if (vp.hasOverlayTexture() && vp.isEnabled()) {
            // Some code (using OpenSceneGraph) can mess up the viewport settings.
            // To ensure correct mapping enter the current viewport.
            enterCurrentViewport();
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, vp.getOverlayTextureIndex());

            mShaders[OverlayShader].bind();

            glUniform1i(mShaderLocs[OverlayTex], 0);

            getCurrentWindowPtr().bindVAO();

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            // unbind
            getCurrentWindowPtr().unbindVAO();
            ShaderProgram::unbind();
        }
    }
}

/*!
    Draw viewport overlays if there are any.
*/
void Engine::drawOverlaysFixedPipeline() {
    std::size_t numberOfIterations = getCurrentWindowPtr().getNumberOfViewports();
    for (size_t i = 0; i < numberOfIterations; i++) {
        getCurrentWindowPtr().setCurrentViewport(i);

        //if viewport has overlay
        const sgct_core::Viewport& vp = getCurrentWindowPtr().getViewport(i);
        if (vp.hasOverlayTexture() && vp.isEnabled()) {
            //enter ortho mode
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glPushMatrix();
            
            /*
                Some code (using OpenSceneGraph) can mess up the viewport settings.
                To ensure correct mapping enter the current viewport.
            */
            enterCurrentViewport();

            glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
            glMatrixMode(GL_MODELVIEW);

            glPushAttrib(GL_ALL_ATTRIB_BITS);
            glDisable(GL_LIGHTING);
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glDisable(GL_CULL_FACE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glLoadIdentity();
            glColor4f(1.f, 1.f, 1.f, 1.f);

            //Open Scene Graph or the user may have changed the active texture
            glActiveTexture(GL_TEXTURE0);
            //glMatrixMode(GL_TEXTURE);
            //glLoadIdentity();
            
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, vp.getOverlayTextureIndex());

            glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
            
            getCurrentWindowPtr().bindVBO();

            glClientActiveTexture(GL_TEXTURE0);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(0));

            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(8));

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            getCurrentWindowPtr().unbindVBO();

            glPopClientAttrib();
            glPopAttrib();

            //exit ortho mode
            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
        }
    }
}

/*!
    This function attaches targets to FBO if FBO is in use
*/
void Engine::prepareBuffer(TextureIndexes ti) {
    if (SGCTSettings::instance()->useFBO()) {
        if (getCurrentWindowPtr().usePostFX()) {
            ti = Intermediate;
        }

        sgct_core::OffScreenBuffer* fbo = getCurrentWindowPtr().getFBOPtr();

        fbo->bind();
        if (!fbo->isMultiSampled()) {
            //update attachments
            fbo->attachColorTexture(getCurrentWindowPtr().getFrameBufferTexture(ti));

            if (SGCTSettings::instance()->useDepthTexture()) {
                fbo->attachDepthTexture(
                    getCurrentWindowPtr().getFrameBufferTexture(Depth)
                );
            }

            if (SGCTSettings::instance()->useNormalTexture()) {
                fbo->attachColorTexture(
                    getCurrentWindowPtr().getFrameBufferTexture(Normals),
                    GL_COLOR_ATTACHMENT1
                );
            }

            if (SGCTSettings::instance()->usePositionTexture()) {
                fbo->attachColorTexture(
                    getCurrentWindowPtr().getFrameBufferTexture(Positions),
                    GL_COLOR_ATTACHMENT2
                );
            }
        }
    }
}

/*!
    Draw geometry and bind FBO as texture in screenspace (ortho mode).
    The geometry can be a simple quad or a geometry correction and blending mesh.
*/
void Engine::renderFBOTexture() {
    //unbind framebuffer
    sgct_core::OffScreenBuffer::unBind();

    bool maskShaderSet = false;

    SGCTWindow& win = getCurrentWindowPtr();
    win.makeOpenGLContextCurrent(SGCTWindow::Context::Window);

    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //needed for shaders

    //clear buffers
    mCurrentFrustumMode = win.getStereoMode() == SGCTWindow::StereoMode::Active ?
        sgct_core::Frustum::StereoLeftEye :
        sgct_core::Frustum::MonoEye;

    glm::ivec2 size = glm::ivec2(
        glm::ceil(win.getScale() * glm::vec2(win.getResolution()))
    );
        
    glViewport(0, 0, size.x, size.y);
    setAndClearBuffer(BackBufferBlack);
    
    size_t numberOfIterations = win.getNumberOfViewports();

    sgct_core::CorrectionMesh::MeshType mt = SGCTSettings::instance()->getUseWarping() ?
        sgct_core::CorrectionMesh::WARP_MESH :
        sgct_core::CorrectionMesh::QUAD_MESH;

    SGCTWindow::StereoMode sm = win.getStereoMode();
    if (sm > SGCTWindow::StereoMode::Active &&
        sm < SGCTWindow::StereoMode::SideBySide)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, win.getFrameBufferTexture(LeftEye));

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, win.getFrameBufferTexture(RightEye));

        win.bindStereoShaderProgram();

        glUniform1i(win.getStereoShaderLeftTexLoc(), 0);
        glUniform1i(win.getStereoShaderRightTexLoc(), 1);

        for (size_t i = 0; i < win.getNumberOfViewports(); i++) {
            win.getViewport(i).renderMesh(mt);
        }
    }
    else {
        glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
        glBindTexture(GL_TEXTURE_2D, win.getFrameBufferTexture(LeftEye));

        mShaders[FBOQuadShader].bind(); //bind
        glUniform1i( mShaderLocs[MonoTex], 0);
        maskShaderSet = true;

        for (size_t i = 0; i < numberOfIterations; i++) {
            win.getViewport(i).renderMesh(mt);
        }

        // render right eye in active stereo mode
        if (win.getStereoMode() == SGCTWindow::StereoMode::Active) {
            glViewport(0, 0, size.x, size.y);
            
            //clear buffers
            mCurrentFrustumMode = sgct_core::Frustum::StereoRightEye;
            setAndClearBuffer(BackBufferBlack);

            glBindTexture(GL_TEXTURE_2D, win.getFrameBufferTexture(RightEye));
            for (size_t i = 0; i < numberOfIterations; i++) {
                win.getViewport(i).renderMesh(mt);
            }
        }
    }

    // render mask (mono)
    if (win.hasAnyMasks()) {
        if (!maskShaderSet) {
            mShaders[FBOQuadShader].bind(); //bind
            glUniform1i(mShaderLocs[MonoTex], 0);
        }
        
        glDrawBuffer(win.isDoubleBuffered() ? GL_BACK : GL_FRONT);
        glReadBuffer(win.isDoubleBuffered() ? GL_BACK : GL_FRONT);
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_BLEND);

        // ------------------------------------------------------------
        // Result = (Color * BlendMask) * (1-BlackLevel) + BlackLevel
        // ------------------------------------------------------------

        // render blend masks
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        for (std::size_t i = 0; i < numberOfIterations; i++) {
            const sgct_core::Viewport& vp = win.getViewport(i);
            if (vp.hasBlendMaskTexture() && vp.isEnabled()) {
                glBindTexture(GL_TEXTURE_2D, vp.getBlendMaskTextureIndex());
                vp.renderMesh(sgct_core::CorrectionMesh::MASK_MESH);
            }
        }

        // render black level masks
        for (size_t i = 0; i < numberOfIterations; i++) {
            const sgct_core::Viewport& vp = win.getViewport(i);
            if (vp.hasBlackLevelMaskTexture() && vp.isEnabled()) {
                glBindTexture(GL_TEXTURE_2D, vp.getBlackLevelMaskTextureIndex());

                //inverse multiply
                glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
                vp.renderMesh(sgct_core::CorrectionMesh::MASK_MESH);

                //add
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                vp.renderMesh(sgct_core::CorrectionMesh::MASK_MESH);
            }
        }

        //restore
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    ShaderProgram::unbind();

    glDisable(GL_BLEND);
}


/*!
    Draw geometry and bind FBO as texture in screenspace (ortho mode).
    The geometry can be a simple quad or a geometry correction and blending mesh.
*/
void Engine::renderFBOTextureFixedPipeline() {
    //unbind framebuffer
    sgct_core::OffScreenBuffer::unBind();
    
    SGCTWindow& win = getCurrentWindowPtr();
    win.makeOpenGLContextCurrent(SGCTWindow::Context::Window);
    
    //clear buffers
    mCurrentFrustumMode = win.getStereoMode() == SGCTWindow::StereoMode::Active ?
        sgct_core::Frustum::StereoLeftEye :
        sgct_core::Frustum::MonoEye;
    
    glm::ivec2 size = glm::ivec2(
        glm::ceil(win.getScale() * glm::vec2(win.getResolution()))
    );
    glViewport(0, 0, size.x, size.y);
    setAndClearBuffer(BackBufferBlack);

    //enter ortho mode
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glPushMatrix();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //needed for shaders

    glLoadIdentity();
    
    std::size_t numberOfIterations = win.getNumberOfViewports();

    SGCTWindow::StereoMode sm = win.getStereoMode();

    sgct_core::CorrectionMesh::MeshType mt = SGCTSettings::instance()->getUseWarping() ?
        sgct_core::CorrectionMesh::WARP_MESH :
        sgct_core::CorrectionMesh::QUAD_MESH;

    if (sm > SGCTWindow::StereoMode::Active &&
        sm < SGCTWindow::StereoMode::SideBySide)
    {
        win.bindStereoShaderProgram();

        glUniform1i(win.getStereoShaderLeftTexLoc(), 0);
        glUniform1i(win.getStereoShaderRightTexLoc(), 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, win.getFrameBufferTexture(LeftEye));
        glEnable(GL_TEXTURE_2D);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, win.getFrameBufferTexture(RightEye));
        glEnable(GL_TEXTURE_2D);

        for (size_t i = 0; i < numberOfIterations; i++) {
            win.getViewport(i).renderMesh(mt);
        }
        ShaderProgram::unbind();
    }
    else {
        glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
        glBindTexture(GL_TEXTURE_2D, win.getFrameBufferTexture(LeftEye));
        glEnable(GL_TEXTURE_2D);

        for (size_t i = 0; i < numberOfIterations; i++) {
            win.getViewport(i).renderMesh(mt);
        }

        //render right eye in active stereo mode
        if (win.getStereoMode() == SGCTWindow::StereoMode::Active) {
            glm::ivec2 res = win.getResolution();
            glViewport(0, 0, res.x, res.y);
            
            //clear buffers
            mCurrentFrustumMode = sgct_core::Frustum::StereoRightEye;
            setAndClearBuffer(BackBufferBlack);

            glBindTexture(GL_TEXTURE_2D, win.getFrameBufferTexture(RightEye));

            for (size_t i = 0; i < numberOfIterations; i++) {
                win.getViewport(i).renderMesh(mt);
            }
        }
    }

    //render mask (mono)
    if (win.hasAnyMasks()) {
        glDrawBuffer(win.isDoubleBuffered() ? GL_BACK : GL_FRONT);
        glReadBuffer(win.isDoubleBuffered() ? GL_BACK : GL_FRONT);

        //if stereo != active stereo
        glActiveTexture(GL_TEXTURE1);
        glDisable(GL_TEXTURE_2D);

        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_BLEND);

        // ------------------------------------------------------------
        // Result = (Color * BlendMask) * (1-BlackLevel) + BlackLevel
        // ------------------------------------------------------------

        //render blend masks
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        for (size_t i = 0; i < numberOfIterations; i++) {
            const sgct_core::Viewport& vp = win.getViewport(i);
            if (vp.hasBlendMaskTexture() && vp.isEnabled()) {
                glBindTexture(GL_TEXTURE_2D, vp.getBlendMaskTextureIndex());
                vp.renderMesh(sgct_core::CorrectionMesh::MASK_MESH);
            }
        }

        //render black level masks
        for (size_t i = 0; i < numberOfIterations; i++) {
            const sgct_core::Viewport& vp = win.getViewport(i);
            if (vp.hasBlackLevelMaskTexture() && vp.isEnabled()) {
                glBindTexture(GL_TEXTURE_2D, vp.getBlackLevelMaskTextureIndex());

                //inverse multiply
                glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
                vp.renderMesh(sgct_core::CorrectionMesh::MASK_MESH);

                //add
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                vp.renderMesh(sgct_core::CorrectionMesh::MASK_MESH);
            }
        }
    }
    
    glPopAttrib();

    //exit ortho mode
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

/*
    Works for fixed and programable pipeline
*/
void Engine::renderViewports(TextureIndexes ti) {
    prepareBuffer(ti); //attach FBO

    SGCTWindow::StereoMode sm = getCurrentWindowPtr().getStereoMode();
    
    //render all viewports for selected eye
    for (size_t i = 0; i < getCurrentWindowPtr().getNumberOfViewports(); i++) {
        getCurrentWindowPtr().setCurrentViewport(i);
        mCurrentViewportIndex[MainViewport] = i;
        sgct_core::Viewport& vp = getCurrentWindowPtr().getViewport(i);

        if (vp.isEnabled()) {
            //if passive stereo or mono
            if (sm == SGCTWindow::StereoMode::NoStereo) {
                mCurrentFrustumMode = vp.getEye();
            }

            if (vp.hasSubViewports()) {
                if (vp.isTracked()) {
                    vp.getNonLinearProjectionPtr()->updateFrustums(
                        mCurrentFrustumMode,
                        mNearClippingPlaneDist,
                        mFarClippingPlaneDist
                    );
                }

                if (getCurrentWindowPtr().getCallDraw3DFunction()) {
                    vp.getNonLinearProjectionPtr()->render();
                }
            }
            else {
                // no subviewports
                if (vp.isTracked()) {
                    vp.calculateFrustum(
                        mCurrentFrustumMode,
                        mNearClippingPlaneDist,
                        mFarClippingPlaneDist
                    );
                }

                //check if we want to copy the previos window into this one before we go ahead with anyting else
                if (getCurrentWindowPtr().getCopyPreviousWindowToCurrentWindow()) {
                    copyPreviousWindowViewportToCurrentWindowViewport(
                        mCurrentFrustumMode
                    );
                }

                if (getCurrentWindowPtr().getCallDraw3DFunction()) {
                    mInternalDrawFn();
                }
            }
        }
    }

    // If we did not render anything, make sure we clear the screen at least
    if (!getCurrentWindowPtr().getCallDraw3DFunction() &&
        !getCurrentWindowPtr().getCopyPreviousWindowToCurrentWindow())
    {
        SGCTSettings::instance()->useFBO() ?
            setAndClearBuffer(RenderToTexture) :
            setAndClearBuffer(BackBuffer);
    }

    if (mFixedOGLPipeline) {
        glPushAttrib(GL_ALL_ATTRIB_BITS);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    //if side-by-side and top-bottom mode only do post fx and blit only after rendered right eye
    bool split_screen_stereo = (sm >= sgct::SGCTWindow::StereoMode::SideBySide);
    if (!( split_screen_stereo &&
        mCurrentFrustumMode == sgct_core::Frustum::StereoLeftEye))
    {
        if (getCurrentWindowPtr().usePostFX()) {
            //blit buffers
            updateRenderingTargets(ti); //only used if multisampled FBOs

            mInternalRenderPostFXFn(ti);

            render2D();
            if (split_screen_stereo) {
                //render left eye info and graph so that all 2D items are rendered after post fx
                mCurrentFrustumMode = sgct_core::Frustum::StereoLeftEye;
                render2D();
            }
        }
        else {
            render2D();
            if (split_screen_stereo) {
                //render left eye info and graph so that all 2D items are rendered after post fx
                mCurrentFrustumMode = sgct_core::Frustum::StereoLeftEye;
                render2D();
            }

            updateRenderingTargets(ti); //only used if multisampled FBOs
        }
    }

    glDisable(GL_BLEND);
    if (mFixedOGLPipeline) {
        glPopAttrib();
    }
}

/*!
    This function renders stats, OSD and overlays
*/
void Engine::render2D() {
    //draw viewport overlays if any
    mInternalDrawOverlaysFn();

    //draw info & stats
    //the cubemap viewports are all the same so it makes no sense to render everything several times
    //therefore just loop one iteration in that case.
    if (!(mShowGraph || mShowInfo || mDraw2DFnPtr != nullptr)) {
        return;
    }
    std::size_t numberOfIterations = getCurrentWindowPtr().getNumberOfViewports();
    for (size_t i = 0; i < numberOfIterations; i++) {
        getCurrentWindowPtr().setCurrentViewport(i);
        mCurrentViewportIndex[MainViewport] = i;
            
        if (getCurrentWindowPtr().getCurrentViewport()->isEnabled()) {
            enterCurrentViewport();

            if (mShowGraph) {
                mStatistics->draw(
                    static_cast<float>(getCurrentWindowPtr().getFramebufferResolution().y)
                    / static_cast<float>(getCurrentWindowPtr().getResolution().y)
                );
            }
            //The text renderer enters automatically the correct viewport
            if (mShowInfo) {
                //choose specified eye from config
                if (getCurrentWindowPtr().getStereoMode() ==
                        SGCTWindow::StereoMode::NoStereo)
                {
                    mCurrentFrustumMode = getCurrentWindowPtr().getCurrentViewport()->getEye();
                }
                renderDisplayInfo();
            }

            // Check if we should call the use defined draw2D function
            if (mDraw2DFnPtr != nullptr && getCurrentWindowPtr().getCallDraw2DFunction())
            {
                mDraw2DFnPtr();
            }
        }
    }
}

/*!
    This function combines a texture and a shader into a new texture
*/
void Engine::renderPostFX(TextureIndexes finalTargetIndex) {
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    PostFX* lastFx = nullptr;

    size_t numberOfPasses = getCurrentWindowPtr().getNumberOfPostFXs();
    for (size_t i = 0; i < numberOfPasses; i++) {
        PostFX& fx = getCurrentWindowPtr().getPostFX(i);
        lastFx = &getCurrentWindowPtr().getPostFX(i);

        //set output
        if (i == (numberOfPasses - 1) && !getCurrentWindowPtr().useFXAA()) {
            //if last
            fx.setOutputTexture(
                getCurrentWindowPtr().getFrameBufferTexture(finalTargetIndex)
            );
        }
        else {
            //ping pong between the two FX buffers
            fx.setOutputTexture(
                getCurrentWindowPtr().getFrameBufferTexture((i % 2 == 0) ? FX1 : FX2)
            ); 
        }

        //set input (dependent on output)
        if (i == 0) {
            fx.setInputTexture(getCurrentWindowPtr().getFrameBufferTexture(Intermediate));
        }
        else {
            PostFX& fxPrevious = getCurrentWindowPtr().getPostFX(i - 1);
            fx.setInputTexture(fxPrevious.getOutputTexture());
        }

        fx.render();
    }
    if (getCurrentWindowPtr().useFXAA()) {
        //bind target FBO
        getCurrentWindowPtr().getFBOPtr()->attachColorTexture(
            getCurrentWindowPtr().getFrameBufferTexture(finalTargetIndex)
        );

        glm::ivec2 framebufferSize = getCurrentWindowPtr().getFramebufferResolution();
        //if for some reson the active texture has been reset
        glViewport(0, 0, framebufferSize.x, framebufferSize.y);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);

        if (lastFx) {
            glBindTexture(GL_TEXTURE_2D, lastFx->getOutputTexture());
        }
        else {
            glBindTexture(
                GL_TEXTURE_2D,
                getCurrentWindowPtr().getFrameBufferTexture(Intermediate)
            );
        }

        mShaders[FXAAShader].bind();
        glUniform1f(mShaderLocs[SizeX], static_cast<float>(framebufferSize.x));
        glUniform1f(mShaderLocs[SizeY], static_cast<float>(framebufferSize.y));
        glUniform1i(mShaderLocs[FXAA_Texture], 0);
        glUniform1f(
            mShaderLocs[FXAA_SUBPIX_TRIM],
            SGCTSettings::instance()->getFXAASubPixTrim()
        );
        glUniform1f(
            mShaderLocs[FXAA_SUBPIX_OFFSET],
            SGCTSettings::instance()->getFXAASubPixOffset()
        );

        getCurrentWindowPtr().bindVAO();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        getCurrentWindowPtr().unbindVAO();

        //unbind FXAA
        ShaderProgram::unbind();
    }
}

/*!
    This function combines a texture and a shader into a new texture
*/
void Engine::renderPostFXFixedPipeline(TextureIndexes finalTargetIndex) {
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    PostFX* lastFx = nullptr;

    size_t numberOfPasses = getCurrentWindowPtr().getNumberOfPostFXs();
    if (numberOfPasses > 0) {
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_CULL_FACE);
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
    }

    for (size_t i = 0; i < numberOfPasses; i++) {
        PostFX& fx = getCurrentWindowPtr().getPostFX(i);
        lastFx = &getCurrentWindowPtr().getPostFX(i);

        //set output
        if (i == (numberOfPasses - 1) && !getCurrentWindowPtr().useFXAA()) {
            //if last
            fx.setOutputTexture(
                getCurrentWindowPtr().getFrameBufferTexture(finalTargetIndex)
            );
        }
        else {
            //ping pong between the two FX buffers
            fx.setOutputTexture(
                getCurrentWindowPtr().getFrameBufferTexture((i % 2 == 0) ? FX1 : FX2)
            );
        }

        //set input (dependent on output)
        if (i == 0) {
            fx.setInputTexture(getCurrentWindowPtr().getFrameBufferTexture(Intermediate));
        }
        else {
            PostFX& fxPrevious = getCurrentWindowPtr().getPostFX(i - 1);
            fx.setInputTexture(fxPrevious.getOutputTexture());
        }

        fx.render();
    }

    if (numberOfPasses > 0) {
        glPopAttrib();
    }

    if (getCurrentWindowPtr().useFXAA()) {
        //bind target FBO
        getCurrentWindowPtr().getFBOPtr()->attachColorTexture(
            getCurrentWindowPtr().getFrameBufferTexture(finalTargetIndex)
        );

        //if for some reson the active texture has been reset
        glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();

        glMatrixMode(GL_MODELVIEW); //restore
        glm::ivec2 framebufferSize = getCurrentWindowPtr().getFramebufferResolution();
        glViewport(0, 0, framebufferSize.x, framebufferSize.y);
        
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);

        glPushAttrib(GL_ALL_ATTRIB_BITS);

        glEnable(GL_TEXTURE_2D);
        if (lastFx) {
            glBindTexture(GL_TEXTURE_2D, lastFx->getOutputTexture());
        }
        else {
            glBindTexture(
                GL_TEXTURE_2D,
                getCurrentWindowPtr().getFrameBufferTexture(Intermediate)
            );
        }

        glDisable(GL_CULL_FACE);
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);

        mShaders[FXAAShader].bind();
        glUniform1f(mShaderLocs[SizeX], static_cast<float>(framebufferSize.x));
        glUniform1f(mShaderLocs[SizeY], static_cast<float>(framebufferSize.y));
        glUniform1i(mShaderLocs[FXAA_Texture], 0 );
        glUniform1f(
            mShaderLocs[FXAA_SUBPIX_TRIM],
            SGCTSettings::instance()->getFXAASubPixTrim()
        );
        glUniform1f(
            mShaderLocs[FXAA_SUBPIX_OFFSET],
            SGCTSettings::instance()->getFXAASubPixOffset()
        );

        glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

        getCurrentWindowPtr().bindVBO();
        glClientActiveTexture(GL_TEXTURE0);

        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(0));

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(8));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        getCurrentWindowPtr().unbindVBO();

        ShaderProgram::unbind();

        glPopClientAttrib();
        glPopAttrib();
    }
}

/*!
    This function updates the renderingtargets.
*/
void Engine::updateRenderingTargets(TextureIndexes ti) {
    //copy AA-buffer to "regular"/non-AA buffer
    sgct_core::OffScreenBuffer* fbo = getCurrentWindowPtr().getFBOPtr();
    if (fbo->isMultiSampled()) {
        if (getCurrentWindowPtr().usePostFX()) {
            ti = Intermediate;
        }

        fbo->bindBlit(); //bind separate read and draw buffers to prepare blit operation

        //update attachments
        fbo->attachColorTexture(getCurrentWindowPtr().getFrameBufferTexture(ti));

        if (SGCTSettings::instance()->useDepthTexture()) {
            fbo->attachDepthTexture(getCurrentWindowPtr().getFrameBufferTexture(Depth));
        }

        if (SGCTSettings::instance()->useNormalTexture()) {
            fbo->attachColorTexture(
                getCurrentWindowPtr().getFrameBufferTexture(Normals),
                GL_COLOR_ATTACHMENT1
            );
        }

        if (SGCTSettings::instance()->usePositionTexture()) {
            fbo->attachColorTexture(
                getCurrentWindowPtr().getFrameBufferTexture(Positions),
                GL_COLOR_ATTACHMENT2
            );
        }

        fbo->blit();
    }
}

/*!
    This function updates the timers.
*/
void Engine::updateTimers(double timeStamp) {
    // check all timers if one of them has expired
    if (isMaster()) {
        for (size_t i = 0; i < mTimers.size(); ++i ) {
            TimerInformation& currentTimer = mTimers[i];
            const double timeSinceLastFiring = timeStamp - currentTimer.mLastFired;
            if (timeSinceLastFiring > currentTimer.mInterval) {
                currentTimer.mLastFired = timeStamp;
                currentTimer.mCallback(currentTimer.mId);
            }
        }
    }
}

/*!
    This function loads shaders that handles different 3D modes.
    The shaders are only loaded once in the initOGL function.
*/
void Engine::loadShaders() {
    //create FXAA shaders
    mShaders[FXAAShader].setName("FXAAShader");
    std::string fxaa_vert_shader;
    std::string fxaa_frag_shader;
    
    if (mFixedOGLPipeline) {
        fxaa_vert_shader = sgct_core::shaders::FXAAVert;
        fxaa_frag_shader = sgct_core::shaders::FXAAFrag;
    }
    else {
        fxaa_vert_shader = sgct_core::shaders_modern::FXAAVert;
        fxaa_frag_shader = sgct_core::shaders_modern::FXAAFrag;
    }

    //replace glsl version
    sgct_helpers::findAndReplace(fxaa_vert_shader, "**glsl_version**", getGLSLVersion());
    sgct_helpers::findAndReplace(fxaa_frag_shader, "**glsl_version**", getGLSLVersion());

    bool fxaaVertShader = mShaders[FXAAShader].addShaderSrc(
        fxaa_vert_shader,
        GL_VERTEX_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!fxaaVertShader) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Failed to load FXAA vertex shader\n"
        );
    }

    bool fxaaFragShader = mShaders[FXAAShader].addShaderSrc(
        fxaa_frag_shader,
        GL_FRAGMENT_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!fxaaFragShader) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Failed to load FXAA fragment shader\n"
        );
    }
    mShaders[FXAAShader].createAndLinkProgram();
    mShaders[FXAAShader].bind();

    mShaderLocs[SizeX] = mShaders[FXAAShader].getUniformLocation("rt_w");
    glm::ivec2 framebufferSize = getCurrentWindowPtr().getFramebufferResolution();
    glUniform1f(mShaderLocs[SizeX], static_cast<float>(framebufferSize.x));

    mShaderLocs[SizeY] = mShaders[FXAAShader].getUniformLocation("rt_h");
    glUniform1f(mShaderLocs[SizeY], static_cast<float>(framebufferSize.y));

    mShaderLocs[FXAA_SUBPIX_TRIM] = mShaders[FXAAShader].getUniformLocation("FXAA_SUBPIX_TRIM");
    glUniform1f(
        mShaderLocs[FXAA_SUBPIX_TRIM],
        SGCTSettings::instance()->getFXAASubPixTrim()
    );

    mShaderLocs[FXAA_SUBPIX_OFFSET] = mShaders[FXAAShader].getUniformLocation("FXAA_SUBPIX_OFFSET");
    glUniform1f(
        mShaderLocs[FXAA_SUBPIX_OFFSET],
        SGCTSettings::instance()->getFXAASubPixOffset()
    );

    mShaderLocs[FXAA_Texture] = mShaders[FXAAShader].getUniformLocation("tex");
    glUniform1i(mShaderLocs[FXAA_Texture], 0);

    ShaderProgram::unbind();

    /*!
        Used for overlays & mono.
    */
    if (!mFixedOGLPipeline) {
        std::string FBO_quad_vert_shader;
        std::string FBO_quad_frag_shader;
        FBO_quad_vert_shader = sgct_core::shaders_modern::BaseVert;
        FBO_quad_frag_shader = sgct_core::shaders_modern::BaseFrag;
        
        //replace glsl version
        sgct_helpers::findAndReplace(
            FBO_quad_vert_shader,
            "**glsl_version**",
            getGLSLVersion()
        );
        sgct_helpers::findAndReplace(
            FBO_quad_frag_shader,
            "**glsl_version**",
            getGLSLVersion()
        );
        
        mShaders[FBOQuadShader].setName("FBOQuadShader");
        bool fboQuadVertShader = mShaders[FBOQuadShader].addShaderSrc(
            FBO_quad_vert_shader,
            GL_VERTEX_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!fboQuadVertShader) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Failed to load FBO quad vertex shader\n"
            );
        }
        bool fboQuadFragShader = mShaders[FBOQuadShader].addShaderSrc(
            FBO_quad_frag_shader,
            GL_FRAGMENT_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!fboQuadFragShader) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Failed to load FBO quad fragment shader\n"
            );
        }
        mShaders[FBOQuadShader].createAndLinkProgram();
        mShaders[FBOQuadShader].bind();
        mShaderLocs[MonoTex] = mShaders[FBOQuadShader].getUniformLocation( "Tex" );
        glUniform1i(mShaderLocs[MonoTex], 0);
        ShaderProgram::unbind();
        
        std::string Overlay_vert_shader;
        std::string Overlay_frag_shader;
        Overlay_vert_shader = sgct_core::shaders_modern::OverlayVert;
        Overlay_frag_shader = sgct_core::shaders_modern::OverlayFrag;
        
        //replace glsl version
        sgct_helpers::findAndReplace(
            Overlay_vert_shader,
            "**glsl_version**",
            getGLSLVersion()
        );
        sgct_helpers::findAndReplace(
            Overlay_frag_shader,
            "**glsl_version**",
            getGLSLVersion()
        );
        
        mShaders[OverlayShader].setName("OverlayShader");
        bool overlayVertShader = mShaders[OverlayShader].addShaderSrc(
            Overlay_vert_shader,
            GL_VERTEX_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!overlayVertShader) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Failed to load overlay vertex shader\n"
            );
        }
        bool overlayFragShader = mShaders[OverlayShader].addShaderSrc(
            Overlay_frag_shader,
            GL_FRAGMENT_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!overlayFragShader) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Failed to load overlay fragment shader\n"
            );
        }
        mShaders[OverlayShader].createAndLinkProgram();
        mShaders[OverlayShader].bind();
        mShaderLocs[OverlayTex] = mShaders[OverlayShader].getUniformLocation("Tex");
        glUniform1i(mShaderLocs[OverlayTex], 0);
        ShaderProgram::unbind();
    }
}

/*!
    \param mode is the one of the following:

    - Backbuffer (transparent)
    - Backbuffer (black)
    - RenderToTexture

    This function clears and sets the appropriate buffer from:

    - Back buffer
    - Left back buffer
    - Right back buffer
*/
void Engine::setAndClearBuffer(BufferMode mode) {
    if (mode < RenderToTexture) {
        //Set buffer
        if (getCurrentWindowPtr().getStereoMode() != SGCTWindow::StereoMode::Active) {
            glDrawBuffer(
                getCurrentWindowPtr().isDoubleBuffered() ? GL_BACK : GL_FRONT
            );
            glReadBuffer(
                getCurrentWindowPtr().isDoubleBuffered() ? GL_BACK : GL_FRONT
            );
        }
        else if (mCurrentFrustumMode == sgct_core::Frustum::StereoLeftEye) {
            //if active left
            glDrawBuffer(
                getCurrentWindowPtr().isDoubleBuffered() ? GL_BACK_LEFT : GL_FRONT_LEFT
            );
            glReadBuffer(
                getCurrentWindowPtr().isDoubleBuffered() ? GL_BACK_LEFT : GL_FRONT_LEFT
            );
        }
        else if (mCurrentFrustumMode == sgct_core::Frustum::StereoRightEye) {
            //if active right
            glDrawBuffer(
                getCurrentWindowPtr().isDoubleBuffered() ? GL_BACK_RIGHT : GL_FRONT_RIGHT
            );
            glReadBuffer(
                getCurrentWindowPtr().isDoubleBuffered() ? GL_BACK_RIGHT : GL_FRONT_RIGHT
            );
        }
    }

    //clear
    if (mode != BackBufferBlack && mClearBufferFnPtr != nullptr) {
        mClearBufferFnPtr();
    }
    else {
        //when rendering textures to backbuffer (using fbo)
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
}

/*!
    This functions checks for OpenGL errors and prints them using the MessageHandler (to commandline).
    Avoid this function in the render loop for release code since it can reduce performance.

    \returns true if no errors occured
*/
bool Engine::checkForOGLErrors() {
    GLenum oglError = glGetError();

    switch (oglError) {
        case GL_INVALID_ENUM:
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "OpenGL error: GL_INVALID_ENUM\n"
            );
            break;
        case GL_INVALID_VALUE:
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "OpenGL error: GL_INVALID_VALUE\n"
            );
            break;
        case GL_INVALID_OPERATION:
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "OpenGL error: GL_INVALID_OPERATION\n"
            );
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "OpenGL error: GL_INVALID_FRAMEBUFFER_OPERATION\n"
            );
            break;
        case GL_STACK_OVERFLOW:
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "OpenGL error: GL_STACK_OVERFLOW\n"
            );
            break;
        case GL_STACK_UNDERFLOW:
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "OpenGL error: GL_STACK_UNDERFLOW\n"
            );
            break;
        case GL_OUT_OF_MEMORY:
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "OpenGL error: GL_OUT_OF_MEMORY\n"
            );
            break;
        case GL_TABLE_TOO_LARGE:
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "OpenGL error: GL_TABLE_TOO_LARGE\n"
            );
            break;
    }

    return oglError == GL_NO_ERROR; //returns true if no errors
}

bool Engine::isMaster() const {
    return mNetworkConnections->isComputerServer();
}

bool Engine::isDisplayInfoRendered() const {
    return mShowInfo;
}

bool Engine::isRenderingOffScreen() const {
    return mRenderingOffScreen;
}

const sgct_core::Frustum::FrustumMode& Engine::getCurrentFrustumMode() const {
    return mCurrentFrustumMode;
}

const glm::mat4& Engine::getCurrentProjectionMatrix() const {
    return getCurrentWindowPtr().getCurrentViewport()->getProjection(mCurrentFrustumMode).getProjectionMatrix();
}

const glm::mat4& Engine::getCurrentViewMatrix() const {
    return getCurrentWindowPtr().getCurrentViewport()->getProjection(mCurrentFrustumMode).getViewMatrix();
}

const glm::mat4& Engine::getModelMatrix() const {
    return sgct_core::ClusterManager::instance()->getSceneTransform();
}

const glm::mat4& Engine::getCurrentViewProjectionMatrix() const {
    return getCurrentWindowPtr().getCurrentViewport()->getProjection(mCurrentFrustumMode).getViewProjectionMatrix();
}

glm::mat4 Engine::getCurrentModelViewProjectionMatrix() const {
    return getCurrentWindowPtr().getCurrentViewport()->getProjection(mCurrentFrustumMode).getViewProjectionMatrix()
        * sgct_core::ClusterManager::instance()->getSceneTransform();
}

glm::mat4 Engine::getCurrentModelViewMatrix() const {
    return getCurrentWindowPtr().getCurrentViewport()->getProjection(mCurrentFrustumMode).getViewMatrix()
        * sgct_core::ClusterManager::instance()->getSceneTransform();
}

unsigned int Engine::getCurrentFrameNumber() const {
    return mFrameCounter;
}

bool Engine::isOGLPipelineFixed() const {
    return mFixedOGLPipeline;
}

Engine::RunMode Engine::getRunMode() const {
    return mRunMode;
}

std::string Engine::getGLSLVersion() const {
    return mGLSLVersion;
}


/*!
    This function waits for all windows to be created on the whole cluster in order to set the barrier (hardware swap-lock).
    Under some Nvida drivers the stability is improved by first join a swapgroup and then set the barrier then all windows in a swapgroup are created.
*/
void Engine::waitForAllWindowsInSwapGroupToOpen() {
    //clear the buffers initially
    for (size_t i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        mThisNode->getWindowPtr(i).makeOpenGLContextCurrent(
            SGCTWindow::Context::Window
        );
        glDrawBuffer(getCurrentWindowPtr().isDoubleBuffered() ? GL_BACK : GL_FRONT);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mThisNode->getWindowPtr(i).isDoubleBuffered() ?
            glfwSwapBuffers(mThisNode->getWindowPtr(i).getWindowHandle()) :
            glFinish();
    }
    glfwPollEvents();
    
    //Must wait until all nodes are running if using swap barrier
    if (!sgct_core::ClusterManager::instance()->getIgnoreSync() &&
        sgct_core::ClusterManager::instance()->getNumberOfNodes() > 1)
    {
        //check if swapgroups are supported
        #ifdef __WIN32__
        if (glfwExtensionSupported("WGL_NV_swap_group")) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Swap groups are supported by hardware.\n"
            );
        }
        #else
        if( glfwExtensionSupported("GLX_NV_swap_group") )
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Swap groups are supported by hardware.\n"
            );
        #endif
        else {
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Swap groups are not supported by hardware.\n"
            );
        }

        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Waiting for all nodes to connect.\n"
        );
        MessageHandler::instance()->setShowTime(false);
        
        while (mNetworkConnections->isRunning() && !mThisNode->getKeyPressed(mExitKey) &&
            !mThisNode->shouldAllWindowsClose() && !mTerminate)
        {
            MessageHandler::instance()->print(MessageHandler::Level::Info, ".");

            // Swap front and back rendering buffers
            for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
            {
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                mThisNode->getWindowPtr(i).isDoubleBuffered() ?
                    glfwSwapBuffers(mThisNode->getWindowPtr(i).getWindowHandle()) :
                    glFinish();
            }
            glfwPollEvents();

            if (mNetworkConnections->areAllNodesConnected()) {
                break;
            }

            sleep(0.1);
        }
        MessageHandler::instance()->print(MessageHandler::Level::Info, "\n");

        //wait for user to release exit key
        while (mThisNode->getKeyPressed(mExitKey)) {
            // Swap front and back rendering buffers
            // key buffers also swapped
            for (size_t i = 0; i < mThisNode->getNumberOfWindows(); i++) {
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                mThisNode->getWindowPtr(i).isDoubleBuffered() ?
                    glfwSwapBuffers(mThisNode->getWindowPtr(i).getWindowHandle()) :
                    glFinish();
            }
            glfwPollEvents();
            
            MessageHandler::instance()->print(MessageHandler::Level::Info, ".");

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        MessageHandler::instance()->print(MessageHandler::Level::Info, "\n");
        MessageHandler::instance()->setShowTime(true);
    }
}

/*!
    This functions updates the frustum of all viewports on demand. However if the viewport is tracked this is done on the fly.
*/
void Engine::updateFrustums() {
    if (mThisNode == nullptr) {
        return;
    }

    for (size_t w = 0; w < mThisNode->getNumberOfWindows(); w++) {
        SGCTWindow& win = mThisNode->getWindowPtr(w);
        for (unsigned int i = 0; i < win.getNumberOfViewports(); i++) {
            sgct_core::Viewport& vp = win.getViewport(i);
            if (!vp.isTracked())  {
                //if not tracked update, otherwise this is done on the fly
                if (vp.hasSubViewports()) {
                    vp.getNonLinearProjectionPtr()->updateFrustums(
                        sgct_core::Frustum::MonoEye,
                        mNearClippingPlaneDist,
                        mFarClippingPlaneDist
                    );

                    vp.getNonLinearProjectionPtr()->updateFrustums(
                        sgct_core::Frustum::StereoLeftEye,
                        mNearClippingPlaneDist,
                        mFarClippingPlaneDist
                    );

                    vp.getNonLinearProjectionPtr()->updateFrustums(
                        sgct_core::Frustum::StereoRightEye,
                        mNearClippingPlaneDist,
                        mFarClippingPlaneDist
                    );
                }
                else {
                    vp.calculateFrustum(
                        sgct_core::Frustum::MonoEye,
                        mNearClippingPlaneDist,
                        mFarClippingPlaneDist
                    );

                    vp.calculateFrustum(
                        sgct_core::Frustum::StereoLeftEye,
                        mNearClippingPlaneDist,
                        mFarClippingPlaneDist
                    );

                    vp.calculateFrustum(
                        sgct_core::Frustum::StereoRightEye,
                        mNearClippingPlaneDist,
                        mFarClippingPlaneDist
                    );
                }
            }
        }
    }
}

/*!
This function copies/render the result from the previous window same viewport(if it exists) into this window
*/
void Engine::copyPreviousWindowViewportToCurrentWindowViewport(
                                              sgct_core::Frustum::FrustumMode frustumMode)
{
    //Check that we have a previous window
    if (getCurrentWindowIndex() < 1) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Warning,
            "Could not copy from previous window, as this window is the first one.\n"
        );
        return;
    }

    SGCTWindow& previousWindow = getWindowPtr(getCurrentWindowIndex() - 1);

    //run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);

    enterCurrentViewport();

    //clear buffers
    SGCTSettings::instance()->useFBO() ?
        setAndClearBuffer(RenderToTexture) :
        setAndClearBuffer(BackBuffer);

    glDisable(GL_SCISSOR_TEST);

    mShaders[OverlayShader].bind();

    glUniform1i(mShaderLocs[OverlayTex], 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, previousWindow.getFrameBufferTexture(frustumMode));

    getCurrentWindowPtr().bindVAO();

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    //unbind
    getCurrentWindowPtr().unbindVAO();

    ShaderProgram::unbind();
}

/*!
    \param arg is the list of arguments

    This function parses all SGCT arguments and removes them from the argument list.
*/
void Engine::parseArguments(std::vector<std::string>& arg) {
    //parse arguments
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Parsing arguments...\n"
    );
    int i = 0;
    while (i < arg.size()) {
        if (arg[i] == "-config" && arg.size() > (i + 1)) {
            configFilename = arg[i + 1];
            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--client") {
            sgct_core::ClusterManager::instance()->setNetworkMode(
                sgct_core::NetworkManager::LocalClient
            );
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--slave") {
            sgct_core::ClusterManager::instance()->setNetworkMode(
                sgct_core::NetworkManager::LocalClient
            );
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--debug") {
            MessageHandler::instance()->setNotifyLevel(MessageHandler::Level::Debug);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--help") {
            mHelpMode = true;
            outputHelpMessage();
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-local" && arg.size() > (i + 1)) {
            sgct_core::ClusterManager::instance()->setNetworkMode(
                sgct_core::NetworkManager::LocalServer
            );
            int tmpi = -1;
            std::stringstream ss(arg[i+1]);
            ss >> tmpi;
            sgct_core::ClusterManager::instance()->setThisNodeId(tmpi);
            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-logPath") {
            //Remove unwanted chars
            std::string tmpStr = arg[i+1];
            tmpStr.erase(remove(tmpStr.begin(), tmpStr.end(), '\"'), tmpStr.end());
            std::size_t lastPos = tmpStr.length() - 1;
            
            const char last = tmpStr.at(lastPos);
            if (last == '\\' || last == '/') {
                tmpStr.erase(lastPos);
            }

            mLogfilePath = tmpStr;

            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-notify" && arg.size() > (i + 1)) {
            int tmpi = -1;
            std::stringstream ss(arg[i + 1]);
            ss >> tmpi;
            if (tmpi != -1) {
                MessageHandler::instance()->setNotifyLevel(
                    static_cast<MessageHandler::Level>(tmpi)
                );
            }
            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--Firm-Sync") {
            sgct_core::ClusterManager::instance()->setFirmFrameLockSyncStatus(true);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--Loose-Sync") {
            sgct_core::ClusterManager::instance()->setFirmFrameLockSyncStatus(false);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--Ignore-Sync") {
            sgct_core::ClusterManager::instance()->setUseIgnoreSync(true);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--No-Sync") {
            sgct_core::ClusterManager::instance()->setUseIgnoreSync(true);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--gDebugger") {
            SGCTSettings::instance()->setForceGlTexImage2D(true);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--FXAA") {
            SGCTSettings::instance()->setDefaultFXAAState(true);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-MSAA" && arg.size() > (i + 1)) {
            int tmpi = -1;
            std::stringstream ss(arg[i + 1]);
            ss >> tmpi;
            SGCTSettings::instance()->setDefaultNumberOfAASamples(tmpi);
            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--No-FBO") {
            SGCTSettings::instance()->setUseFBO(false);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--Capture-TGA") {
            SGCTSettings::instance()->setCaptureFormat("TGA");
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--Capture-PNG") {
            SGCTSettings::instance()->setCaptureFormat("PNG");
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--Capture-JPG") {
            SGCTSettings::instance()->setCaptureFormat("JPG");
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-numberOfCaptureThreads" && arg.size() > (i + 1)) {
            int tmpi = -1;
            std::stringstream ss(arg[i + 1]);
            ss >> tmpi;

            if (tmpi > 0) {
                SGCTSettings::instance()->setNumberOfCaptureThreads(tmpi);
            }

            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else {
            i++; //iterate
        }
    }

    MessageHandler::instance()->print(MessageHandler::Level::Info, "Done\n");
}

/*!
    \param fnPtr is the function pointer to a draw callback

    This function sets the draw callback. It's possible to have several draw functions and change the callback on the fly preferably in a stage before the draw like the post-sync-pre-draw stage or the pre-sync stage.
    The draw callback can be called several times per frame since it's called once for every viewport and once for every eye if stereoscopy is used.
*/
void Engine::setDrawFunction(void(*fnPtr)(void)) {
    mDrawFnPtr = fnPtr;
}

/*!
\param fn is the std function of a draw callback

@see sgct::Engine::setDrawFunction(void(*fnPtr)(void))
*/
void Engine::setDrawFunction(std::function<void(void)> fn) {
    mDrawFnPtr = std::move(fn);
}

/*!
    \param fnPtr is the function pointer to a draw 2D callback

    This function sets the draw 2D callback. This callback will be called after overlays and post effects has been drawn.
    This makes it possible to render text and HUDs that will not be filtered and antialiasied.
*/
void Engine::setDraw2DFunction(void(*fnPtr)(void)) {
    mDraw2DFnPtr = std::move(fnPtr);
}

/*!
\param fn is the std function of a draw 2D callback

This function sets the draw 2D callback. This callback will be called after overlays and post effects has been drawn.
This makes it possible to render text and HUDs that will not be filtered and antialiasied.
*/
void Engine::setDraw2DFunction(std::function<void(void)> fn) {
    mDraw2DFnPtr = std::move(fn);
}

/*!
    \param fnPtr is the function pointer to a pre-sync callback

    This function sets the pre-sync callback. The Engine will then use the callback before the sync stage.
    In the callback set the variables that will be shared.
*/
void Engine::setPreSyncFunction(void(*fnPtr)(void)) {
    mPreSyncFnPtr = fnPtr;
}

/*!
    \param fn is the std function of a pre-sync callback

    This function sets the pre-sync callback. The Engine will then use the callback before the sync stage.
    In the callback set the variables that will be shared.
*/
void Engine::setPreSyncFunction(std::function<void(void)> fn) {
    mPreSyncFnPtr = std::move(fn);
}

/*!
    \param fnPtr is the function pointer to a post-sync-pre-draw callback

    This function sets the post-sync-pre-draw callback. The Engine will then use the callback after the sync stage but before the draw stage. Compared to the draw callback the post-sync-pre-draw callback is called only once per frame.
    In this callback synchronized variables can be applied or simulations depending on synchronized input can run.
*/
void Engine::setPostSyncPreDrawFunction(void(*fnPtr)(void)) {
    mPostSyncPreDrawFnPtr = fnPtr;
}

/*!
\param fn is the std function of a post-sync-pre-draw callback

This function sets the post-sync-pre-draw callback. The Engine will then use the callback after the sync stage but before the draw stage. Compared to the draw callback the post-sync-pre-draw callback is called only once per frame.
In this callback synchronized variables can be applied or simulations depending on synchronized input can run.
*/
void Engine::setPostSyncPreDrawFunction(std::function<void(void)> fn) {
    mPostSyncPreDrawFnPtr = std::move(fn);
}

/*!
    \param fnPtr is the function pointer to a post-draw callback

    This function sets the post-draw callback. The Engine will then use the callback after the draw stage but before the OpenGL buffer swap. Compared to the draw callback the post-draw callback is called only once per frame.
    In this callback data/buffer swaps can be made.
*/
void Engine::setPostDrawFunction(void(*fnPtr)(void)) {
    mPostDrawFnPtr = fnPtr;
}

/*!
\param fn is the std function of a post-draw callback

This function sets the post-draw callback. The Engine will then use the callback after the draw stage but before the OpenGL buffer swap. Compared to the draw callback the post-draw callback is called only once per frame.
In this callback data/buffer swaps can be made.
*/
void Engine::setPostDrawFunction(std::function<void(void)> fn) {
    mPostDrawFnPtr = std::move(fn);
}

/*!
    \param fnPtr is the function pointer to a OpenGL initiation callback

    This function sets the initOGL callback. The Engine will then use the callback only once before the starting the render loop.
    Textures, Models, Buffers, etc. can be loaded/allocated here.
*/
void Engine::setInitOGLFunction(void(*fnPtr)(void)) {
    mInitOGLFnPtr = fnPtr;
}

/*!
    \param fn is the std function of an OpenGL initiation callback

    This function sets the initOGL callback. The Engine will then use the callback only once before the starting the render loop.
    Textures, Models, Buffers, etc. can be loaded/allocated here.
*/
void Engine::setInitOGLFunction(std::function<void(void)> fn) {
    mInitOGLFnPtr = std::move(fn);
}

/*!
    This callback is called before the window is created (before OpenGL context is created).
    At this stage the config file has been read and network initialized. Therefore it's suitable for loading master or slave specific data.

    \param fnPtr is the function pointer to a pre window creation callback
*/
void Engine::setPreWindowFunction(void(*fnPtr)(void)) {
    mPreWindowFnPtr = fnPtr;
}

/*!
    This callback is called before the window is created (before OpenGL context is created).
    At this stage the config file has been read and network initialized. Therefore it's suitable for loading master or slave specific data.

    \param fn is the std function of a pre window creation callback
*/
void Engine::setPreWindowFunction(std::function<void(void)> fn) {
    mPreWindowFnPtr = std::move(fn);
}

/*!
    \param fnPtr is the function pointer to a clear buffer function callback

    This function sets the clear buffer callback which will override the default clear buffer function:

    \code
    void sgct::Engine::clearBuffer(void)
    {
        const float * colorPtr = sgct::Engine::instance()->getClearColor();
        glClearColor(colorPtr[0], colorPtr[1], colorPtr[2], colorPtr[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    \endcode
*/
void Engine::setClearBufferFunction(void(*fnPtr)(void)) {
    mClearBufferFnPtr = fnPtr;
}

/*!
\param fn is the std function of a clear buffer function callback

@see sgct::Engine::setClearBufferFunction(void(*fnPtr)(void))
*/
void Engine::setClearBufferFunction(std::function<void(void)> fn) {
    mClearBufferFnPtr = std::move(fn);
}

/*!
    \param fnPtr is the function pointer to a clean up function callback

    This function sets the clean up callback which will be called in the Engine destructor before all sgct components (like window, OpenGL context, network, etc.) will be destroyed.
*/
void Engine::setCleanUpFunction(void(*fnPtr)(void)) {
    mCleanUpFnPtr = fnPtr;
}

/*!
\param fn is the std function pointer of a clean up function callback

This function sets the clean up callback which will be called in the Engine destructor before all sgct components (like window, OpenGL context, network, etc.) will be destroyed.
*/
void Engine::setCleanUpFunction(std::function<void(void)> fn) {
    mCleanUpFnPtr = std::move(fn);
}

/*!
 \param fnPtr is the function pointer to an external control message callback
 
 This function sets the external control message callback which will be called when a TCP message is received. The TCP listner is enabled in the XML configuration file in the Cluster tag by externalControlPort, where the portnumber is an integer preferably above 20000.
 Example:
 \code
 <Cluster masterAddress="127.0.0.1" externalControlPort="20500">
 \endcode
 
 All TCP messages must be separated by carriage return (CR) followed by a newline (NL). Look at this [tutorial](https://c-student.itn.liu.se/wiki/develop:sgcttutorials:externalguicsharp) for more info.
 
 */
void Engine::setExternalControlCallback(void(*fnPtr)(const char *, int)) {
    mExternalDecodeCallbackFnPtr = fnPtr;
}

/*!
\param fn is the std function of an external control message callback

@see sgct::Engine::setExternalControlCallback(void(*fnPtr)(const char *, int))
*/
void Engine::setExternalControlCallback(std::function<void(const char *, int)> fn) {
    mExternalDecodeCallbackFnPtr = std::move(fn);
}

/*!
 \param fnPtr is the function pointer to an external control status callback
 
 This function sets the external control status callback which will be called when the connection status changes (connect or disconnect).
 
 */
void Engine::setExternalControlStatusCallback(void(*fnPtr)(bool)) {
    mExternalStatusCallbackFnPtr = fnPtr;
}

/*!
\param fn is the std function of an external control status callback

This function sets the external control status callback which will be called when the connection status changes (connect or disconnect).

*/
void Engine::setExternalControlStatusCallback(std::function<void(bool)> fn) {
    mExternalStatusCallbackFnPtr = std::move(fn);
}

/*!
 \param fnPtr is the function pointer to a data transfer callback
 
 This function sets the data transfer message callback which will be called when a TCP message is received. The TCP listner is enabled in the XML configuration file in the Node tag by dataTransferPort, where the portnumber is an integer preferably above 20000.
 
 */
void Engine::setDataTransferCallback(void(*fnPtr)(void *, int, int, int)) {
    mDataTransferDecodeCallbackFnPtr = fnPtr;
}

/*!
\param fn is the std function of a data transfer callback

This function sets the data transfer message callback which will be called when a TCP message is received. The TCP listner is enabled in the XML configuration file in the Node tag by dataTransferPort, where the portnumber is an integer preferably above 20000.
*/
void Engine::setDataTransferCallback(std::function<void(void *, int, int, int)> fn) {
    mDataTransferDecodeCallbackFnPtr = std::move(fn);
}

/*!
 \param fnPtr is the function pointer to a data transfer status callback
 
 This function sets the data transfer status callback which will be called when the connection status changes (connect or disconnect).
 
 */
void Engine::setDataTransferStatusCallback(void(*fnPtr)(bool, int)) {
    mDataTransferStatusCallbackFnPtr = fnPtr;
}

/*!
\param fn is the std function of a data transfer status callback

This function sets the data transfer status callback which will be called when the connection status changes (connect or disconnect).

*/
void Engine::setDataTransferStatusCallback(std::function<void(bool, int)> fn) {
    mDataTransferStatusCallbackFnPtr = std::move(fn);
}

/*!
 \param fnPtr is the function pointer to a data transfer acknowledge callback
 
 This function sets the data transfer acknowledge callback which will be called when the data is successfully sent.
 
 */
void Engine::setDataAcknowledgeCallback(void(*fnPtr)(int, int)) {
    mDataTransferAcknowledgeCallbackFnPtr = fnPtr;
}

/*!
\param fn is the std function of a data transfer acknowledge callback

This function sets the data transfer acknowledge callback which will be called when the data is successfully sent.

*/
void Engine::setDataAcknowledgeCallback(std::function<void(int, int)> fn) {
    mDataTransferAcknowledgeCallbackFnPtr = std::move(fn);
}

/*!
\param fnPtr is the funtion pointer to an OpenGL context (GLFW window) creation callback
 
This function sets the OpenGL context creation callback which will be called directly after all SGCT windows are created. This enables the user to create additional OpenGL context for multithreaded OpenGL.
*/
void Engine::setContextCreationCallback(void(*fnPtr)(GLFWwindow*)) {
    mContextCreationFnPtr = fnPtr;
}

/*!
\param fn is the std funtion of an OpenGL context (GLFW window) creation callback

This function sets the OpenGL context creation callback which will be called directly after all SGCT windows are created. This enables the user to create additional OpenGL context for multithreaded OpenGL.
*/
void Engine::setContextCreationCallback(std::function<void(GLFWwindow*)> fn) {
    mContextCreationFnPtr = std::move(fn);
}

/*!
 \param fnPtr is the function pointer to a screenshot callback for custom frame capture & export
 This callback must be set before Engine::init is called\n
 Parameters to the callback are: Image pointer for image data, window index, eye index, download type
 */
void Engine::setScreenShotCallback(void(*fnPtr)(sgct_core::Image *, std::size_t, sgct_core::ScreenCapture::EyeIndex, unsigned int type))
{
    mScreenShotFnPtr1 = fnPtr;
    mScreenShotFnPtr2 = nullptr; //allow only one callback
}

/*!
\param fnPtr is the function pointer to a screenshot callback for custom frame capture & export
This callback must be set before Engine::init is called\n
Parameters to the callback are: raw data pointer for image data, window index, eye index, download type
*/
void Engine::setScreenShotCallback(void(*fnPtr)(unsigned char *, std::size_t, sgct_core::ScreenCapture::EyeIndex, unsigned int type))
{
    mScreenShotFnPtr2 = fnPtr;
    mScreenShotFnPtr1 = nullptr; //allow only one callback
}

/*!
    \param fnPtr is the function pointer to a keyboard callback function

    This function sets the keyboard callback (GLFW wrapper) where the two parameters are: int key, int action. Key can be a character (e.g. 'A', 'B', '5' or ',') or a special character defined in the table below. Action can either be SGCT_PRESS or SGCT_RELEASE.
    All windows are connected to this callback.

    Name          | Description
    ------------- | -------------
    SGCT_KEY_UNKNOWN  | Unknown
    SGCT_KEY_SPACE  | Space
    SGCT_KEY_APOSTROPHE | Apostrophe
    SGCT_KEY_COMMA | Comma
    SGCT_KEY_MINUS | Minus
    SGCT_KEY_PERIOD | Period
    SGCT_KEY_SLASH | Slash
    SGCT_KEY_0 | 0
    SGCT_KEY_1 | 1
    SGCT_KEY_2 | 2
    SGCT_KEY_3 | 3
    SGCT_KEY_4 | 4
    SGCT_KEY_5 | 5
    SGCT_KEY_6 | 6
    SGCT_KEY_7 | 7
    SGCT_KEY_8 | 8
    SGCT_KEY_9 | 9
    SGCT_KEY_SEMICOLON | Semicolon
    SGCT_KEY_EQUAL | Equal
    SGCT_KEY_A | A
    SGCT_KEY_B | B
    SGCT_KEY_C | C
    SGCT_KEY_D | D
    SGCT_KEY_E | E
    SGCT_KEY_F | F
    SGCT_KEY_G | G
    SGCT_KEY_H | H
    SGCT_KEY_I | I
    SGCT_KEY_J | J
    SGCT_KEY_K | K
    SGCT_KEY_L | L
    SGCT_KEY_M | M
    SGCT_KEY_N | N
    SGCT_KEY_O | O
    SGCT_KEY_P | P
    SGCT_KEY_Q | Q
    SGCT_KEY_R | R
    SGCT_KEY_S | S
    SGCT_KEY_T | T
    SGCT_KEY_U | U
    SGCT_KEY_V | V
    SGCT_KEY_W | W
    SGCT_KEY_X | X
    SGCT_KEY_Y | Y
    SGCT_KEY_Z | Z
    SGCT_KEY_LEFT_BRACKET | Left bracket
    SGCT_KEY_BACKSLASH | backslash
    SGCT_KEY_RIGHT_BRACKET | Right bracket
    SGCT_KEY_GRAVE_ACCENT | Grave accent
    SGCT_KEY_WORLD_1 | World 1
    SGCT_KEY_WORLD_2 | World 2
    SGCT_KEY_ESC | Escape
    SGCT_KEY_ESCAPE | Escape
    SGCT_KEY_ENTER | Enter
    SGCT_KEY_TAB | Tab
    SGCT_KEY_BACKSPACE | Backspace
    SGCT_KEY_INSERT | Insert
    SGCT_KEY_DEL | Delete
    SGCT_KEY_DELETE | Delete
    SGCT_KEY_RIGHT | Right
    SGCT_KEY_LEFT | Left
    SGCT_KEY_DOWN | Down
    SGCT_KEY_UP | Up
    SGCT_KEY_PAGEUP | Page up
    SGCT_KEY_PAGEDOWN | Page down
    SGCT_KEY_PAGE_UP | Page up
    SGCT_KEY_PAGE_DOWN | Page down
    SGCT_KEY_HOME | Home
    SGCT_KEY_END | End
    SGCT_KEY_CAPS_LOCK | Caps lock
    SGCT_KEY_SCROLL_LOCK | Scroll lock
    SGCT_KEY_NUM_LOCK | Num lock
    SGCT_KEY_PRINT_SCREEN | Print screen
    SGCT_KEY_PAUSE | Pause
    SGCT_KEY_F1 | F1
    SGCT_KEY_F2 | F2
    SGCT_KEY_F3 | F3
    SGCT_KEY_F4 | F4
    SGCT_KEY_F5 | F5
    SGCT_KEY_F6 | F6
    SGCT_KEY_F7 | F7
    SGCT_KEY_F8 | F8
    SGCT_KEY_F9 | F9
    SGCT_KEY_F10 | F10
    SGCT_KEY_F11 | F11
    SGCT_KEY_F12 | F12
    SGCT_KEY_F13 | F13
    SGCT_KEY_F14 | F14
    SGCT_KEY_F15 | F15
    SGCT_KEY_F16 | F16
    SGCT_KEY_F17 | F17
    SGCT_KEY_F18 | F18
    SGCT_KEY_F19 | F19
    SGCT_KEY_F20 | F20
    SGCT_KEY_F21 | F21
    SGCT_KEY_F22 | F22
    SGCT_KEY_F23 | F23
    SGCT_KEY_F24 | F24
    SGCT_KEY_F25 | F25
    SGCT_KEY_KP_0 | Keypad 0
    SGCT_KEY_KP_1 | Keypad 1
    SGCT_KEY_KP_2 | Keypad 2
    SGCT_KEY_KP_3 | Keypad 3
    SGCT_KEY_KP_4 | Keypad 4
    SGCT_KEY_KP_5 | Keypad 5
    SGCT_KEY_KP_6 | Keypad 6
    SGCT_KEY_KP_7 | Keypad 7
    SGCT_KEY_KP_8 | Keypad 8
    SGCT_KEY_KP_9 | Keypad 9
    SGCT_KEY_KP_DECIMAL| Keypad decimal
    SGCT_KEY_KP_DIVIDE | Keypad divide
    SGCT_KEY_KP_MULTIPLY | Keypad multiply
    SGCT_KEY_KP_SUBTRACT | Keypad subtract
    SGCT_KEY_KP_ADD | Keypad add
    SGCT_KEY_KP_ENTER | Keypad enter
    SGCT_KEY_KP_EQUAL | Keypad equal
    SGCT_KEY_LSHIFT | Left shift
    SGCT_KEY_LEFT_SHIFT | Left shift
    SGCT_KEY_LCTRL | Left control
    SGCT_KEY_LEFT_CONTROL | Left control
    SGCT_KEY_LALT | Left alt
    SGCT_KEY_LEFT_ALT | Left alt
    SGCT_KEY_LEFT_SUPER | Left super
    SGCT_KEY_RSHIFT | Right shift
    SGCT_KEY_RIGHT_SHIFT | Right shift
    SGCT_KEY_RCTRL | Right control
    SGCT_KEY_RIGHT_CONTROL | Right control
    SGCT_KEY_RALT | Right alt
    SGCT_KEY_RIGHT_ALT | Right alt
    SGCT_KEY_RIGHT_SUPER | Right super
    SGCT_KEY_MENU | Menu
    SGCT_KEY_LAST | Last key index

*/
void Engine::setKeyboardCallbackFunction(void(*fnPtr)(int,int)) {
    gKeyboardCallbackFnPtr = fnPtr;
}

/*!
\param fn is the std function of a keyboard callback function

@see sgct::Engine::setKeyboardCallbackFunction( void(*fnPtr)(int,int) )
*/
void Engine::setKeyboardCallbackFunction(std::function<void(int, int)> fn) {
    gKeyboardCallbackFnPtr = std::move(fn);
}

/*!
\param fnPtr is the function pointer to a keyboard callback function

 This function sets the keyboard callback (GLFW wrapper) where the four parameters are: int key, int scancode, int action, int mods. Modifier keys can be a combination of SGCT_MOD_SHIFT, SGCT_MOD_CONTROL, SGCT_MOD_ALT, SGCT_MOD_SUPER. All windows are connected to this callback.
 
 @see sgct::Engine::setKeyboardCallbackFunction( void(*fnPtr)(int,int) )
 */
void Engine::setKeyboardCallbackFunction(void(*fnPtr)(int, int, int, int)) {
    gKeyboardCallbackFnPtr2 = fnPtr;
}

/*!
\param fn is the std function of a keyboard callback function

This function sets the keyboard callback (GLFW wrapper) where the four parameters are: int key, int scancode, int action, int mods. Modifier keys can be a combination of SGCT_MOD_SHIFT, SGCT_MOD_CONTROL, SGCT_MOD_ALT, SGCT_MOD_SUPER. All windows are connected to this callback.

@see sgct::Engine::setKeyboardCallbackFunction( void(*fnPtr)(int,int) )
*/
void Engine::setKeyboardCallbackFunction(std::function<void(int, int, int, int)> fn) {
    gKeyboardCallbackFnPtr2 = std::move(fn);
}

/*!
All windows are connected to this callback.
*/
void Engine::setCharCallbackFunction(void(*fnPtr)(unsigned int)) {
    gCharCallbackFnPtr = fnPtr;
}

void Engine::setCharCallbackFunction(void(*fnPtr)(unsigned int, int)) {
    gCharCallbackFnPtr2 = fnPtr;
}

/*!
All windows are connected to this callback.
*/
void Engine::setCharCallbackFunction(std::function<void(unsigned int)> fn) {
    gCharCallbackFnPtr = std::move(fn);
}

void Engine::setCharCallbackFunction(std::function<void(unsigned int, int)> fn) {
    gCharCallbackFnPtr2 = std::move(fn);
}

/*!
    \param fnPtr is the function pointer to a mouse button callback function

    This function sets the mouse button callback (GLFW wrapper) where the two parameters are: int button, int action. Button id's are listed in the table below. Action can either be SGCT_PRESS or SGCT_RELEASE.
    All windows are connected to this callback.

    Name          | Description
    ------------- | -------------
    SGCT_MOUSE_BUTTON_LEFT | Left button
    SGCT_MOUSE_BUTTON_RIGHT | Right button
    SGCT_MOUSE_BUTTON_MIDDLE | Middle button
    SGCT_MOUSE_BUTTON_1 | Button 1
    SGCT_MOUSE_BUTTON_2 | Button 2
    SGCT_MOUSE_BUTTON_3 | Button 3
    SGCT_MOUSE_BUTTON_4 | Button 4
    SGCT_MOUSE_BUTTON_5 | Button 5
    SGCT_MOUSE_BUTTON_6 | Button 6
    SGCT_MOUSE_BUTTON_7 | Button 7
    SGCT_MOUSE_BUTTON_8 | Button 8
    SGCT_MOUSE_BUTTON_LAST | Last mouse button index

*/
void Engine::setMouseButtonCallbackFunction(void(*fnPtr)(int, int, int)) {
    gMouseButtonCallbackFnPtr = fnPtr;
}

/*!
\param fn is the std function of a mouse button callback function

@see sgct::Engine::setMouseButtonCallbackFunction( void(*fnPtr)(int, int) )
*/
void Engine::setMouseButtonCallbackFunction(std::function<void(int, int, int)> fn) {
    gMouseButtonCallbackFnPtr = std::move(fn);
}

/*!
All windows are connected to this callback.
*/
void Engine::setMousePosCallbackFunction(void(*fnPtr)(double, double)) {
    gMousePosCallbackFnPtr = fnPtr;
}

/*!
All windows are connected to this callback.
*/
void Engine::setMousePosCallbackFunction(std::function<void(double, double)> fn) {
    gMousePosCallbackFnPtr = std::move(fn);
}

/*!
All windows are connected to this callback.
*/
void Engine::setMouseScrollCallbackFunction(void(*fnPtr)(double, double)) {
    gMouseScrollCallbackFnPtr = fnPtr;
}

/*!
All windows are connected to this callback.
*/
void Engine::setMouseScrollCallbackFunction(std::function<void(double, double)> fn) {
    gMouseScrollCallbackFnPtr = std::move(fn);
}

/*!
Drop files to any window. All windows are connected to this callback.
*/
void Engine::setDropCallbackFunction(void(*fnPtr)(int, const char**)) {
    gDropCallbackFnPtr = fnPtr;
}

/*!
Drop files to any window. All windows are connected to this callback.
*/
void Engine::setDropCallbackFunction(std::function<void(int, const char**)> fn) {
    gDropCallbackFnPtr = std::move(fn);
}

/*!
fnPtr is the function pointer to a touch callback function
*/
void Engine::setTouchCallbackFunction(void(*fnPtr)(const sgct_core::Touch*)) {
    gTouchCallbackFnPtr = fnPtr;
}

/*!
fnPtr is the function pointer to a touch callback function
*/
void Engine::setTouchCallbackFunction(std::function<void(const sgct_core::Touch*)> fn) {
    gTouchCallbackFnPtr = std::move(fn);
}

void sgct::Engine::clearBuffer() {
    glm::vec4 color = Engine::instance()->getClearColor();

    glClearColor(
        color.r,
        color.g,
        color.b,
        instance()->getCurrentWindowPtr().getAlpha() ? 0.f : color.a
    );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Engine::internal_key_callback(GLFWwindow* window, int key, int scancode, int action,
                                   int mods)
{
    if (gKeyboardCallbackFnPtr != nullptr) {
        gKeyboardCallbackFnPtr(key, action);
    }
    
    if (gKeyboardCallbackFnPtr2 != nullptr) {
        gKeyboardCallbackFnPtr2(key, scancode, action, mods);
    }
}

void Engine::internal_key_char_callback(GLFWwindow* window, unsigned int ch) {
    if (gCharCallbackFnPtr != nullptr) {
        gCharCallbackFnPtr(ch);
    }
}

void Engine::internal_key_char_mods_callback(GLFWwindow* window, unsigned int ch, int mod)
{
    if (gCharCallbackFnPtr != nullptr) {
        gCharCallbackFnPtr(ch);
    }
    
    if (gCharCallbackFnPtr2 != nullptr) {
        gCharCallbackFnPtr2(ch, mod);
    }
}

void Engine::internal_mouse_button_callback(GLFWwindow* window, int button, int action,
                                            int mods)
{
    if (gMouseButtonCallbackFnPtr != nullptr) {
        gMouseButtonCallbackFnPtr(button, action, mods);
    }
}

void Engine::internal_mouse_pos_callback(GLFWwindow* window, double xPos, double yPos) {
    if (gMousePosCallbackFnPtr != nullptr) {
        gMousePosCallbackFnPtr(xPos, yPos);
    }
}

void Engine::internal_mouse_scroll_callback(GLFWwindow* window, double xOffset,
                                            double yOffset)
{
    if (gMouseScrollCallbackFnPtr != nullptr) {
        gMouseScrollCallbackFnPtr(xOffset, yOffset);
    }
}

void Engine::internal_glfw_error_callback(int error, const char* description) {
    MessageHandler::instance()->print(
        MessageHandler::Level::Error,
        "GLFW error: %s\n", description
    );
}

void Engine::internal_drop_callback(GLFWwindow* window, int count, const char** paths) {
    if (gDropCallbackFnPtr != nullptr) {
        gDropCallbackFnPtr(count, paths);
    }
}

void Engine::internal_touch_callback(GLFWwindow* window, GLFWtouch* touchPoints,
                                     int count)
{
    glm::ivec4 coords = mInstance->getCurrentWindowPtr().getCurrentViewportPixelCoords();

    mCurrentTouchPoints.processPoints(touchPoints, count, coords.z, coords.w);

    if (gTouchCallbackFnPtr && !mCurrentTouchPoints.getLatestTouchPoints().empty()) {
        gTouchCallbackFnPtr(&mCurrentTouchPoints);
    }

    mCurrentTouchPoints.setLatestPointsHandled();
}

/*!
    Print the node info to terminal.

    \param nodeId Which node to print
*/
void Engine::printNodeInfo(unsigned int nodeId) {
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "This node has index %d.\n", nodeId
    );
}

/*!
    Set up the current viewport.
*/
void Engine::enterCurrentViewport() {
    sgct_core::BaseViewport* vp = getCurrentWindowPtr().getCurrentViewport();
    
    glm::vec2 res = glm::vec2(getCurrentWindowPtr().getFramebufferResolution());
        
    mCurrentViewportCoords = glm::ivec4(
        static_cast<int>(vp->getX() * res.x),
        static_cast<int>(vp->getY() * res.y),
        static_cast<int>(vp->getXSize() * res.x),
        static_cast<int>(vp->getYSize() * res.y)
    );

    SGCTWindow::StereoMode sm = getCurrentWindowPtr().getStereoMode();
    if (sm >= SGCTWindow::StereoMode::SideBySide) {
        if (mCurrentFrustumMode == sgct_core::Frustum::StereoLeftEye) {
            switch (sm) {
                case SGCTWindow::StereoMode::SideBySide:
                    mCurrentViewportCoords[0] = mCurrentViewportCoords[0] >> 1; //x offset
                    mCurrentViewportCoords[2] = mCurrentViewportCoords[2] >> 1; //x size
                    break;
                case SGCTWindow::StereoMode::SideBySideInverted:
                    mCurrentViewportCoords[0] = (mCurrentViewportCoords[0] >> 1) + (mCurrentViewportCoords[2] >> 1); //x offset
                    mCurrentViewportCoords[2] = mCurrentViewportCoords[2] >> 1; //x size
                    break;
                case SGCTWindow::StereoMode::TopBottom:
                    mCurrentViewportCoords[1] = (mCurrentViewportCoords[1] >> 1) + (mCurrentViewportCoords[3] >> 1); //y offset
                    mCurrentViewportCoords[3] = mCurrentViewportCoords[3] >> 1; //y size
                    break;
                case SGCTWindow::StereoMode::TopBottomInverted:
                    mCurrentViewportCoords[1] = mCurrentViewportCoords[1] >> 1; //y offset
                    mCurrentViewportCoords[3] = mCurrentViewportCoords[3] >> 1; //y size
                    break;
                default:
                    break;
            }
        }
        else {
            switch (sm) {
                case SGCTWindow::StereoMode::SideBySide:
                    mCurrentViewportCoords[0] = (mCurrentViewportCoords[0] >> 1) + (mCurrentViewportCoords[2] >> 1); //x offset
                    mCurrentViewportCoords[2] = mCurrentViewportCoords[2] >> 1; //x size
                    break;
                case SGCTWindow::StereoMode::SideBySideInverted:
                    mCurrentViewportCoords[0] = mCurrentViewportCoords[0] >> 1; //x offset
                    mCurrentViewportCoords[2] = mCurrentViewportCoords[2] >> 1; //x size
                    break;
                case SGCTWindow::StereoMode::TopBottom:
                    mCurrentViewportCoords[1] = mCurrentViewportCoords[1] >> 1; //y offset
                    mCurrentViewportCoords[3] = mCurrentViewportCoords[3] >> 1; //y size
                    break;
                case SGCTWindow::StereoMode::TopBottomInverted:
                    mCurrentViewportCoords[1] = (mCurrentViewportCoords[1] >> 1) + (mCurrentViewportCoords[3] >> 1); //y offset
                    mCurrentViewportCoords[3] = mCurrentViewportCoords[3] >> 1; //y size
                    break;
                default:
                    break;
            }
        }
    }

    glViewport(
        mCurrentViewportCoords[0],
        mCurrentViewportCoords[1],
        mCurrentViewportCoords[2],
        mCurrentViewportCoords[3]
    );
    
    glScissor(
        mCurrentViewportCoords[0],
        mCurrentViewportCoords[1],
        mCurrentViewportCoords[2],
        mCurrentViewportCoords[3]
    );

    /*fprintf(stderr, "Viewport: %d %d %d %d\n",
        mCurrentViewportCoords[0],
        mCurrentViewportCoords[1],
        mCurrentViewportCoords[2],
        mCurrentViewportCoords[3]);
    
    fprintf(stderr, "Window size: %dx%d (%dx%d)\n",
            getCurrentWindowPtr()->getXResolution(),
            getCurrentWindowPtr()->getYResolution(),
            getCurrentWindowPtr()->getXFramebufferResolution(),
            getCurrentWindowPtr()->getYFramebufferResolution());*/
}

void Engine::calculateFPS(double timestamp) {
    static double lastTimestamp = glfwGetTime();
    mStatistics->setFrameTime(static_cast<float>(timestamp - lastTimestamp));
    lastTimestamp = timestamp;
    static float renderedFrames = 0.f;
    static float tmpTime = 0.f;
    renderedFrames += 1.f;
    tmpTime += mStatistics->getFrameTime();
    if ( tmpTime >= 1.f) {
        mStatistics->setAvgFPS(renderedFrames / tmpTime);
        renderedFrames = 0.f;
        tmpTime = 0.f;

        for (size_t i = 0; i < mThisNode->getNumberOfWindows(); i++) {
            if (mThisNode->getWindowPtr(i).isVisible()) {
                updateAAInfo(i);
            }
        }
    }
}

/*!
\returns the frame time (delta time) in seconds
*/
double Engine::getDt() {
    return mStatistics->getFrameTime();
}

/*!
\returns the average frames per second
*/
double Engine::getAvgFPS() {
    return mStatistics->getAvgFPS();
}

/*!
\returns the average frame time (delta time) in seconds
*/
double Engine::getAvgDt() {
    return mStatistics->getAvgFrameTime();
}

/*!
 \returns the minimum frame time (delta time) in the averaging window (seconds)
 */
double Engine::getMinDt() {
    return mStatistics->getMinFrameTime();
}

/*!
 \returns the maximum frame time (delta time) in the averaging window (seconds)
 */
double Engine::getMaxDt() {
    return mStatistics->getMaxFrameTime();
}

/*!
 \returns the standard devitation of the delta time in seconds
 */
double Engine::getDtStandardDeviation() {
    return mStatistics->getFrameTimeStandardDeviation();
}

/*!
\returns the draw time in seconds
*/
double Engine::getDrawTime() {
    return mStatistics->getDrawTime();
}

/*!
\returns the sync time (time waiting for other nodes and network) in seconds
*/
double Engine::getSyncTime() {
    return mStatistics->getSyncTime();
}

glm::vec4 Engine::getClearColor() const {
    return mClearColor;
}

float Engine::getNearClippingPlane() const {
    return mNearClippingPlaneDist;
}

float Engine::getFarClippingPlane() const {
    return mFarClippingPlaneDist;
}

/*!
    Set the near and far clipping planes. This operation recalculates all frustums for all viewports.

    @param nearClippingPlane near clipping plane in meters
    @param farClippingPlane far clipping plane in meters
*/
void Engine::setNearAndFarClippingPlanes(float nearClippingPlane, float farClippingPlane) {
    mNearClippingPlaneDist = nearClippingPlane;
    mFarClippingPlaneDist = farClippingPlane;
    updateFrustums();
}

/*!
    Set the eye separation (interocular distance) for all users. This operation recalculates all frustums for all viewports.

    @param eyeSeparation eye separation in meters
*/
void Engine::setEyeSeparation(float eyeSeparation) {
    for (size_t w = 0; w < mThisNode->getNumberOfWindows(); w++) {
        SGCTWindow& window = mThisNode->getWindowPtr(w);

        for (unsigned int i = 0; i < window.getNumberOfViewports(); i++) {
            window.getViewport(i).getUser()->setEyeSeparation(eyeSeparation);
        }
    }
    updateFrustums();
}

/*!
Set the clear color (background color).

@param red the red color component
@param green the green color component
@param blue the blue color component
@param alpha the alpha color component
*/
void Engine::setClearColor(float red, float green, float blue, float alpha) {
    mClearColor[0] = red;
    mClearColor[1] = green;
    mClearColor[2] = blue;
    mClearColor[3] = alpha;
}

/*!
Set the exit key that will kill SGCT or abort certain SGCT functions.
Default value is: SGCT_KEY_ESC. To diable shutdown or escaping SGCT then use: SGCT_KEY_UNKNOWN
\param key can be either an uppercase printable ISO 8859-1 (Latin 1) character (e.g. 'A', '3' or '.'), or
a special key identifier described in \link setKeyboardCallbackFunction \endlink description.
*/
void Engine::setExitKey(int key) {
    mExitKey = key;
}

/*!
    Add a post effect to all windows
*/
void Engine::addPostFX(PostFX& fx) {
    for (size_t i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        mThisNode->getWindowPtr(i).setUsePostFX(true);
        mThisNode->getWindowPtr(i).addPostFX(fx);
    }
}

/*!
    \Returns the active draw texture if frame buffer objects are used otherwise GL_FALSE
*/
unsigned int Engine::getCurrentDrawTexture() const {
    if (getCurrentWindowPtr().usePostFX()) {
        return getCurrentWindowPtr().getFrameBufferTexture(Intermediate);
    }
    else {
        return mCurrentFrustumMode == sgct_core::Frustum::StereoRightEye ?
            getCurrentWindowPtr().getFrameBufferTexture(RightEye) :
            getCurrentWindowPtr().getFrameBufferTexture(LeftEye);
    }
}

/*!
    \Returns the active depth texture if depth texture rendering is enabled through SGCTSettings and if frame buffer objects are used otherwise GL_FALSE
*/
unsigned int Engine::getCurrentDepthTexture() const {
    return getCurrentWindowPtr().getFrameBufferTexture(Depth);
}

/*!
\Returns the active normal texture if normal texture rendering is enabled through SGCTSettings and if frame buffer objects are used otherwise GL_FALSE
*/
unsigned int Engine::getCurrentNormalTexture() const {
    return getCurrentWindowPtr().getFrameBufferTexture(Normals);
}

/*!
\Returns the active position texture if position texture rendering is enabled through SGCTSettings and if frame buffer objects are used otherwise GL_FALSE
*/
unsigned int Engine::getCurrentPositionTexture() const {
    return getCurrentWindowPtr().getFrameBufferTexture(Positions);
}

/*!
    \Returns the resolution in pixels for the active window's framebuffer
*/
glm::ivec2 Engine::getCurrentResolution() const {
    return getCurrentWindowPtr().getFramebufferResolution();
}

/*!
    \Returns the index of the window that is focued. If no window is focused 0 is returned.
*/
std::size_t Engine::getFocusedWindowIndex() const {
    for (size_t i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        if (mThisNode->getWindowPtr(i).isFocused()) {
            return i;
        }
    }
    return 0; // no window is focued
}

void Engine::setWireframe(bool state) {
    mShowWireframe = state;
}

void Engine::setDisplayInfoVisibility(bool state) {
    mShowInfo = state;
}

void Engine::setStatsGraphVisibility(bool state) {
    mShowGraph = state;
}

void Engine::takeScreenshot() {
    mTakeScreenshot = true;
}

/*!
 Don't use this. This function is called from SGCTNetwork and will invoke the external network callback when messages are received.
 */
void Engine::invokeDecodeCallbackForExternalControl(const char* receivedData,
                                                    int receivedlength, int clientId)
{
    if (mExternalDecodeCallbackFnPtr != nullptr && receivedlength > 0) {
        mExternalDecodeCallbackFnPtr(receivedData, receivedlength);
    }
}

/*!
 Don't use this. This function is called from SGCTNetwork and will invoke the external network update callback when connection is connected/disconnected.
 */
void Engine::invokeUpdateCallbackForExternalControl(bool connected) {
    if (mExternalStatusCallbackFnPtr != nullptr) {
        mExternalStatusCallbackFnPtr(connected);
    }
}

/*!
 Don't use this. This function is called from SGCTNetwork and will invoke the data transfer callback when messages are received.
 */
void Engine::invokeDecodeCallbackForDataTransfer(void* receivedData, int receivedlength,
                                                 int packageId, int clientId)
{
    if (mDataTransferDecodeCallbackFnPtr != nullptr && receivedlength > 0) {
        mDataTransferDecodeCallbackFnPtr(
            receivedData,
            receivedlength,
            packageId,
            clientId
        );
    }
}

/*!
 Don't use this. This function is called from SGCTNetwork and will invoke the data transfer callback when connection is connected/disconnected.
 */
void Engine::invokeUpdateCallbackForDataTransfer(bool connected, int clientId) {
    if (mDataTransferStatusCallbackFnPtr != nullptr) {
        mDataTransferStatusCallbackFnPtr(connected, clientId);
    }
}

/*!
 Don't use this. This function is called from SGCTNetwork and will invoke the data transfer callback when data is successfully sent.
 */
void Engine::invokeAcknowledgeCallbackForDataTransfer(int packageId, int clientId) {
    if (mDataTransferAcknowledgeCallbackFnPtr != nullptr) {
        mDataTransferAcknowledgeCallbackFnPtr(packageId, clientId);
    }
}

/*!
    Don't use this. This function is called internally in SGCT.
*/
void Engine::invokeScreenShotCallback1(sgct_core::Image* imPtr, size_t winIndex,
                                       sgct_core::ScreenCapture::EyeIndex ei,
                                       unsigned int type)
{
    if (mScreenShotFnPtr1 != nullptr) {
        mScreenShotFnPtr1(imPtr, winIndex, ei, type);
    }
}

/*!
Don't use this. This function is called internally in SGCT.
*/
void Engine::invokeScreenShotCallback2(unsigned char* data, size_t winIndex,
                                       sgct_core::ScreenCapture::EyeIndex ei,
                                       unsigned int type)
{
    if (mScreenShotFnPtr2 != nullptr) {
        mScreenShotFnPtr2(data, winIndex, ei, type);
    }
}

/*!
    This function sends a message to the external control interface.
    \param data a pointer to the data buffer
    \param length is the number of bytes of data that will be sent
*/
void Engine::sendMessageToExternalControl(const void* data, int length) {
    if (mNetworkConnections->getExternalControlPtr() != nullptr) {
        mNetworkConnections->getExternalControlPtr()->sendData(data, length);
    }
}

/*!
 Compression levels 1-9.
 -1 = Default compression
 0 = No compression
 1 = Best speed
 9 = Best compression
 */
void Engine::setDataTransferCompression(bool state, int level) {
    mNetworkConnections->setDataTransferCompression(state, level);
}

/*!
This function sends data between nodes.
\param data a pointer to the data buffer
\param length is the number of bytes of data that will be sent
\param packageId is the identification id of this specific package
*/
void Engine::transferDataBetweenNodes(const void* data, int length, int packageId) {
    mNetworkConnections->transferData(data, length, packageId);
}

/*!
This function sends data to a specific node.
\param data a pointer to the data buffer
\param length is the number of bytes of data that will be sent
\param packageId is the identification id of this specific package
\param nodeIndex is the index of a specific node
*/
void Engine::transferDataToNode(const void* data, int length, int packageId,
                                size_t nodeIndex)
{
    mNetworkConnections->transferData(data, length, packageId, nodeIndex);
}

/*!
    This function sends a message to the external control interface.
    \param msg the message string that will be sent
*/
void Engine::sendMessageToExternalControl(const std::string& msg) {
    if (mNetworkConnections->getExternalControlPtr() != nullptr) {
        mNetworkConnections->getExternalControlPtr()->sendData(
            msg.c_str(),
            static_cast<int>(msg.size())
        );
    }
}

/*!
    Check if the external control is connected.
*/
bool Engine::isExternalControlConnected() const {
    return (mNetworkConnections->getExternalControlPtr() != nullptr &&
            mNetworkConnections->getExternalControlPtr()->isConnected());
}

/*!
    Set the buffer size for the external control communication buffer. This size must be equal or larger than the receive buffer size.
*/
void Engine::setExternalControlBufferSize(unsigned int newSize) {
    if (mNetworkConnections->getExternalControlPtr() != nullptr) {
        mNetworkConnections->getExternalControlPtr()->setBufferSize(newSize);
    }
}

/*!
    This function updates the Anti-Aliasing (AA) settings.
    This function is called once per second.
*/
void Engine::updateAAInfo(size_t winIndex) {
    if (getWindowPtr(winIndex).useFXAA()) {
        if (getWindowPtr(winIndex).getNumberOfAASamples() > 1) {
            mAAInfo = "FXAA+MSAAx" + getWindowPtr(winIndex).getNumberOfAASamples();
        }
        else {
            mAAInfo = "FXAA";
        }
    }
    else {
        //no FXAA
        if (getWindowPtr(winIndex).getNumberOfAASamples() > 1) {
            mAAInfo = "MSAAx" + getWindowPtr(winIndex).getNumberOfAASamples();
        }
        else {
            mAAInfo = "none";
        }
    }
}

void Engine::updateDrawBufferResolutions() {
    mDrawBufferResolutions.clear();

    for (size_t i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        SGCTWindow& win = getWindowPtr(i);
        
        // first add cubemap resolutions if any
        for (size_t j = 0; j < win.getNumberOfViewports(); j++) {
            if (win.getViewport(j).hasSubViewports()) {
                int cubeRes = win.getViewport(j).getNonLinearProjectionPtr()->getCubemapResolution();
                mDrawBufferResolutions.push_back(glm::ivec2(cubeRes, cubeRes));
            }
        }

        // second add window resolution
        glm::ivec2 size = win.getFinalFBODimensions();
        mDrawBufferResolutions.push_back(size);
    }
}

/*!
    Checks the keyboard if the specified key has been pressed.
    \param winIndex specifies which window to poll
    \param key specifies which key to check
    \returns SGCT_PRESS or SGCT_RELEASE
*/
int Engine::getKey(size_t winIndex, int key) {
    return glfwGetKey(mInstance->getWindowPtr(winIndex).getWindowHandle(), key);
}

/*!
    Checks if specified mouse button has been pressed.
    \param winIndex specifies which window to poll
    \param button specifies which button to check
    \returns SGCT_PRESS or SGCT_RELEASE
*/
int Engine::getMouseButton(size_t winIndex, int button) {
    return glfwGetMouseButton(
        mInstance->getWindowPtr(winIndex).getWindowHandle(),
        button
    );
}

/*!
    Get the mouse position.
    \param winIndex specifies which window to poll
    \param xPos x screen coordinate
    \param yPos y screen coordinate
*/
void Engine::getMousePos(size_t winIndex, double* xPos, double* yPos) {
    glfwGetCursorPos(mInstance->getWindowPtr(winIndex).getWindowHandle(), xPos, yPos);
}

/*!
    Set the mouse position.
    \param winIndex specifies which window's input to set
    \param xPos x screen coordinate
    \param yPos y screen coordinate
*/
void Engine::setMousePos(size_t winIndex, double xPos, double yPos) {
    glfwSetCursorPos(mInstance->getWindowPtr(winIndex).getWindowHandle(), xPos, yPos);
}

/*!
    Set the mouse cursor/pointer visibility
    \param winIndex specifies which window's input to set
    \param state set to true if mouse cursor should be visible
*/
void Engine::setMouseCursorVisibility(size_t winIndex, bool state) {
    if (state) {
        glfwSetInputMode(
            mInstance->getWindowPtr(winIndex).getWindowHandle(),
            GLFW_CURSOR,
            GLFW_CURSOR_NORMAL
        );
    }
    else {
        glfwSetInputMode(
            mInstance->getWindowPtr(winIndex).getWindowHandle(),
            GLFW_CURSOR,
            GLFW_CURSOR_HIDDEN
        );
    }
}

/*!
    \param joystick is the joystick id. Availible id's:
       - SGCT_JOYSTICK_1
       - SGCT_JOYSTICK_2
       - SGCT_JOYSTICK_3
       - SGCT_JOYSTICK_4
       - SGCT_JOYSTICK_5
       - SGCT_JOYSTICK_6
       - SGCT_JOYSTICK_7
       - SGCT_JOYSTICK_8
       - SGCT_JOYSTICK_9
       - SGCT_JOYSTICK_10
       - SGCT_JOYSTICK_11
       - SGCT_JOYSTICK_12
       - SGCT_JOYSTICK_13
       - SGCT_JOYSTICK_14
       - SGCT_JOYSTICK_15
       - SGCT_JOYSTICK_16
       - SGCT_JOYSTICK_LAST
*/
const char* Engine::getJoystickName(int joystick) {
    return glfwGetJoystickName(joystick);
}

/*!
\param joystick the joystick id: Availibe id's are specified here: \link getJoystickName \endlink
\param numOfValues is the number of analog axes
\returns the analog float values (array)
*/
const float* Engine::getJoystickAxes(int joystick, int* numOfValues) {
    return glfwGetJoystickAxes(joystick, numOfValues);
}

/*!
\param joystick the joystick id: Availibe id's are specified here: \link getJoystickName \endlink
\param numOfValues is the number of buttons
\returns the button values (array)
*/
const unsigned char* Engine::getJoystickButtons(int joystick, int* numOfValues) {
    return glfwGetJoystickButtons(joystick, numOfValues);
}

/*!
This function puts the current thread to sleep during a specified time
\param secs is the time to sleep the thread
*/
void Engine::sleep(double secs) {
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(secs * 1000)));
}

const sgct_core::SGCTNode* Engine::getThisNodePtr(size_t index) const {
    return mThisNode;
}

SGCTWindow& Engine::getWindowPtr(size_t index) const {
    return mThisNode->getWindowPtr(index);
}

size_t Engine::getNumberOfWindows() const {
    return mThisNode->getNumberOfWindows();
}

SGCTWindow& Engine::getCurrentWindowPtr() const {
    return mThisNode->getCurrentWindowPtr();
}

size_t Engine::getCurrentWindowIndex() const {
    return mThisNode->getCurrentWindowIndex();
}

sgct_core::SGCTUser* Engine::getDefaultUserPtr() {
    return sgct_core::ClusterManager::instance()->getDefaultUserPtr();
}

SGCTTrackingManager& Engine::getTrackingManager() {
    return sgct_core::ClusterManager::instance()->getTrackingManagerPtr();
}

/*!
    Create a timer that counts down and call the given callback when finished.
    The timer runs only on the master and is not precies since it is triggered in end of the renderloop.

    \param millisec is the countdown time
    \param fnPtr is the function pointer to a timer callback (the argument will be the timer handle/id).

    \returns Handle/id to the created timer
*/
size_t Engine::createTimer(double millisec, void(*fnPtr)(size_t)) {
    if (isMaster()) {
        // construct the timer object
        TimerInformation timer;
        timer.mCallback = fnPtr;
        // we want to present timers in millisec, but glfwGetTime uses seconds
        timer.mInterval = millisec / 1000.0; 
        timer.mId = mTimerID++;  // use and post-increase
        timer.mLastFired = getTime();
        mTimers.push_back(timer);
        return timer.mId;
    }
    else {
        return std::numeric_limits<size_t>::max();
    }
}

/*!
Stops the specified timer
\param id/handle to a timer
*/
void Engine::stopTimer(size_t id) {
    if (isMaster()) {
        // iterate over all timers and search for the id
        for (size_t i = 0; i < mTimers.size(); ++i) {
            const TimerInformation& currentTimer = mTimers[i];
            if (currentTimer.mId == id) {
                mTimers[i].mCallback = nullptr;
                // if the id found, delete this timer and return immediately
                mTimers.erase(mTimers.begin() + i);
                return;
            }
        }

        // if we get this far, the searched ID did not exist
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "There was no timer with id: %d", id
        );
    }
}

/*!
    Get the time from program start in seconds
*/
double Engine::getTime() {
    return glfwGetTime();
}

/*!
    Get the current viewportindex for given type: MainViewport or SubViewport
*/
size_t Engine::getCurrentViewportIndex(ViewportTypes vp) const {
    return mCurrentViewportIndex[vp];
}

/*!
Get the active viewport size in pixels.
\param x the horizontal size
\param y the vertical size
*/
void Engine::getCurrentViewportSize(int& x, int& y) const {
    x = mCurrentViewportCoords[2];
    y = mCurrentViewportCoords[3];
}

/*!
Get the active FBO buffer size. Each window has its own buffer plus any additional non-linear projection targets.
\param x the horizontal size
\param y the vertical size
*/
void Engine::getCurrentDrawBufferSize(int& x, int& y) const {
    x = mDrawBufferResolutions[mCurrentDrawBufferIndex].x;
    y = mDrawBufferResolutions[mCurrentDrawBufferIndex].y;
}

/*!
Get the selected FBO buffer size. Each window has its own buffer plus any additional non-linear projection targets.
\param index index of selected drawbuffer
\param x the horizontal size
\param y the vertical size
*/
void Engine::getDrawBufferSize(const size_t& index, int& x, int& y) const {
    if (index < mDrawBufferResolutions.size()) {
        x = mDrawBufferResolutions[index].x;
        y = mDrawBufferResolutions[index].y;
    }
    else {
        x = 0;
        y = 0;
    }
}

size_t Engine::getNumberOfDrawBuffers() const {
    return mDrawBufferResolutions.size();
}

/*!
\returns the active FBO buffer index.
*/
size_t Engine::getCurrentDrawBufferIndex() const {
    return mCurrentDrawBufferIndex;
}

/*!
\returns the active render target.
*/
const Engine::RenderTarget& Engine::getCurrentRenderTarget() const {
    return mCurrentRenderTarget;
}

/*!
\returns the active off screen buffer. If no buffer is active NULL is returned. 
*/
sgct_core::OffScreenBuffer* Engine::getCurrentFBO() const {
    return mCurrentOffScreenBuffer;
}

/*!
Returns the active viewport in pixels (only valid inside in the draw callback function)
*/
glm::ivec4 Engine::getCurrentViewportPixelCoords() const {
    const sgct_core::Viewport& vp = getCurrentWindowPtr().getViewport(
        mCurrentViewportIndex[MainViewport]
    );
    if (vp.hasSubViewports()) {
        return vp.getNonLinearProjectionPtr()->getViewportCoords();
    }
    else {
        return mCurrentViewportCoords;
    }
}

/*!
Get if wireframe rendering is enabled
\returns true if wireframe is enabled otherwise false
*/
bool Engine::getWireframe() const {
    return mShowWireframe;
}

void Engine::setSyncParameters(bool printMessage, float timeout) {
    mPrintSyncMessage = printMessage;
    mSyncTimeout = timeout;
}

/*!
 Set the screenshot number (file index)
 */
void Engine::setScreenShotNumber(unsigned int number) {
    mShotCounter = number;
}

/*!
 \returns the current screenshot number (file index)
 */
unsigned int Engine::getScreenShotNumber() const {
    return mShotCounter;
}

void sgct::Engine::outputHelpMessage() {
    fprintf( stderr, "\nParameters:\n------------------------------------\n\
\n-config <filename.xml>           \n\tSet xml confiuration file\n\
\n-logPath <filepath>              \n\tSet log file path\n\
\n--help                           \n\tDisplay help message and exit\n\
\n-local <integer>                 \n\tForce node in configuration to localhost\n\t(index starts at 0)\n\
\n--client                         \n\tRun the application as client\n\t(only available when running as local)\n\
\n--slave                          \n\tRun the application as client\n\t(only available when running as local)\n\
\n--debug                          \n\tSet the notify level of messagehandler to debug\n\
\n--Firm-Sync                      \n\tEnable firm frame sync\n\
\n--Loose-Sync                     \n\tDisable firm frame sync\n\
\n--Ignore-Sync                    \n\tDisable frame sync\n\
\n-MSAA    <integer>                  \n\tEnable MSAA as default (argument must be a power of two)\n\
\n--FXAA                           \n\tEnable FXAA as default\n\
\n-notify <integer>                \n\tSet the notify level used in the MessageHandler\n\t(0 = highest priority)\n\
\n--gDebugger                      \n\tForce textures to be genareted using glTexImage2D instead of glTexStorage2D\n\
\n--No-FBO                         \n\tDisable frame buffer objects\n\t(some stereo modes, Multi-Window rendering,\n\tFXAA and fisheye rendering will be disabled)\n\
\n--Capture-PNG                    \n\tUse png images for screen capture (default)\n\
\n--Capture-JPG                    \n\tUse jpg images for screen capture\n\
\n--Capture-TGA                    \n\tUse tga images for screen capture\n\
\n-numberOfCaptureThreads <integer>\n\tSet the maximum amount of threads\n\tthat should be used during framecapture (default 8)\n------------------------------------\n\n");
}

} // namespace sgct


/*
    For feedback: breaks a frame lock wait condition every time interval (FrameLockTimeout)
    in order to print waiting message.
*/
void updateFrameLockLoop(void* arg) {
    bool run = true;

    while (run) {
        sgct::SGCTMutexManager::instance()->lockMutex(
            sgct::SGCTMutexManager::FrameSyncMutex
        );
        run = sRunUpdateFrameLockLoop;
        sgct::SGCTMutexManager::instance()->unlockMutex(
            sgct::SGCTMutexManager::FrameSyncMutex
        );

        sgct_core::NetworkManager::gCond.notify_all();

        std::this_thread::sleep_for(FrameLockTimeout);
    }
}
