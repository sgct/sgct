/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/engine.h>

#include <sgct/clustermanager.h>
#include <sgct/font.h>
#include <sgct/fontmanager.h>
#include <sgct/freetype.h>
#include <sgct/messagehandler.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/readconfig.h>
#include <sgct/mutexmanager.h>
#include <sgct/settings.h>
#include <sgct/shadermanager.h>
#include <sgct/shareddata.h>
#include <sgct/statistics.h>
#include <sgct/texturemanager.h>
#include <sgct/user.h>
#include <sgct/version.h>
#include <sgct/viewport.h>
#include <sgct/touch.h>
#include <sgct/helpers/stringfunctions.h>
#include <sgct/shaders/internalshaders.h>
#include <sgct/shaders/internalshaders_modern.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>

// Callback wrappers for GLFW
std::function<void(int, int, int, int)> gKeyboardCallbackFnPtr = nullptr;
std::function<void(unsigned int, int)> gCharCallbackFnPtr = nullptr;
std::function<void(int, int, int)> gMouseButtonCallbackFnPtr = nullptr;
std::function<void(double, double)> gMousePosCallbackFnPtr = nullptr;
std::function<void(double, double)> gMouseScrollCallbackFnPtr = nullptr;
std::function<void(int, const char**)> gDropCallbackFnPtr = nullptr;
std::function<void(const sgct_core::Touch*)> gTouchCallbackFnPtr = nullptr;
sgct_core::Touch gCurrentTouchPoints;

bool sRunUpdateFrameLockLoop = true;

#ifdef GLEW_MX
GLEWContext* glewGetContext();
#endif // GLEW_MX

namespace {
    constexpr const bool UseSleepToWaitForNodes = false;
    constexpr const bool RunFrameLockCheckThread = true;
    constexpr const std::chrono::milliseconds FrameLockTimeout(100);

    // For feedback: breaks a frame lock wait condition every time interval
    // (FrameLockTimeout) in order to print waiting message.
    void updateFrameLockLoop(void*) {
        bool run = true;

        while (run) {
            sgct::MutexManager::instance()->mFrameSyncMutex.lock();
            run = sRunUpdateFrameLockLoop;
            sgct::MutexManager::instance()->mFrameSyncMutex.unlock();

            sgct_core::NetworkManager::gCond.notify_all();

            std::this_thread::sleep_for(FrameLockTimeout);
        }
    }

    void keyCallback(GLFWwindow*, int key, int scancode, int action, int mods) {
        if (gKeyboardCallbackFnPtr) {
            gKeyboardCallbackFnPtr(key, scancode, action, mods);
        }
    }

    void keyCharModsCallback(GLFWwindow*, unsigned int ch, int mod) {
        if (gCharCallbackFnPtr) {
            gCharCallbackFnPtr(ch, mod);
        }
    }

    void mouseButtonCallback(GLFWwindow*, int button, int action, int mods) {
        if (gMouseButtonCallbackFnPtr) {
            gMouseButtonCallbackFnPtr(button, action, mods);
        }
    }

    void mousePosCallback(GLFWwindow*, double xPos, double yPos) {
        if (gMousePosCallbackFnPtr) {
            gMousePosCallbackFnPtr(xPos, yPos);
        }
    }

    void mouseScrollCallback(GLFWwindow*, double xOffset, double yOffset) {
        if (gMouseScrollCallbackFnPtr) {
            gMouseScrollCallbackFnPtr(xOffset, yOffset);
        }
    }

    void glfwErrorCallback(int error, const char* description) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "GLFW error (%i): %s\n", error, description
        );
    }

    void dropCallback(GLFWwindow*, int count, const char** paths) {
        if (gDropCallbackFnPtr) {
            gDropCallbackFnPtr(count, paths);
        }
    }


    void touchCallback(GLFWwindow*, GLFWtouch* touchPoints, int count) {
        sgct::Engine& eng = *sgct::Engine::instance();
        glm::ivec4 coords = eng.getCurrentWindow().getCurrentViewportPixelCoords();

        gCurrentTouchPoints.processPoints(touchPoints, count, coords.z, coords.w);
        gCurrentTouchPoints.setLatestPointsHandled();
    }

    void outputHelpMessage() {
        fprintf(stderr, R"(
Parameters:
------------------------------------
-config <filename.xml>
    Set XML confiuration file
-logPath <filepath>
    Set log file path
--help
    Display help message and exit
-local <integer>
    Force node in configuration to localhost (index starts at 0)
--client
    Run the application as client\n\t(only available when running as local)
--slave
    Run the application as client\n\t(only available when running as local)
--debug
    Set the notify level of messagehandler to debug
--Firm-Sync
    Enable firm frame sync
--Loose-Sync
    Disable firm frame sync
--Ignore-Sync
    Disable frame sync
-MSAA <integer>
    Enable MSAA as default (argument must be a power of two)
--FXAA
    Enable FXAA as default
-notify <integer>
    Set the notify level used in the MessageHandler\n\t(0 = highest priority)
--gDebugger
    Force textures to be generated using glTexImage2D instead of glTexStorage2D
--No-FBO
    Disable frame buffer objects
    (some stereo modes, Multi-Window rendering,
    FXAA and fisheye rendering will be disabled)
--Capture-PNG
    Use png images for screen capture (default)
--Capture-JPG
    Use jpg images for screen capture
--Capture-TGA
    Use tga images for screen capture
-numberOfCaptureThreads <integer>
    Set the maximum amount of thread that should be used during framecapture (default 8)
------------------------------------
)");
    }

} // namespace

namespace sgct {

Engine* Engine::mInstance = nullptr;

Engine* Engine::instance() {
    return mInstance;
}

Engine::Engine(std::vector<std::string>& arg) {
    mInstance = this;

    setClearBufferFunction(clearBuffer);

    // parse needs to be before read config since the path to the XML is parsed here
    parseArguments(arg);

    if (!mHelpMode) {
        glfwSetErrorCallback(glfwErrorCallback);
        if (!glfwInit()) {
            mTerminate = true;
        }
    }
}

Engine::~Engine() {
    clean();
}

bool Engine::init(RunMode rm, std::string configurationFile) {
    mRunMode = rm;
    if (!configurationFile.empty()) {
        configFilename = std::move(configurationFile);
    }

    MessageHandler::instance()->print(
        MessageHandler::Level::VersionInfo,
        "%s\n", getVersion().c_str()
    );

    if (mHelpMode) {
        return false;
    }

    if (mTerminate) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Failed to init GLFW! Application will close in 5 seconds\n"
        );
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return false;
    }

    try {
        sgct_core::readconfig::readConfig(configFilename);
    }
    catch (const std::runtime_error& e) {
        // fatal error
        outputHelpMessage();
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Error in xml config file parsing. %s. Application will close in 5 seconds\n",
            e.what()
        );
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return false;
    }

    if (!initNetwork()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Network init error. Application will close in 5 seconds\n"
        );
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return false;
    }

    if (!initWindows()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Window init error. Application will close in 5 seconds\n"
        );
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return false;
    }

    // Window resolution may have been set when reading config. However, it only sets a
    // pending resolution, so it needs to apply it using the same routine as in the end of
    // a frame.
    for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        mThisNode->getWindow(i).updateResolutions();
    }

    // if a single node, skip syncing
    if (sgct_core::ClusterManager::instance()->getNumberOfNodes() == 1) {
        sgct_core::ClusterManager::instance()->setUseIgnoreSync(true);
    }

    for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        GLFWwindow* window = getWindow(i).getWindowHandle();
        if (gKeyboardCallbackFnPtr) {
            glfwSetKeyCallback(window, keyCallback);
        }
        if (gMouseButtonCallbackFnPtr) {
            glfwSetMouseButtonCallback(window, mouseButtonCallback);
        }
        if (gMousePosCallbackFnPtr) {
            glfwSetCursorPosCallback(window, mousePosCallback);
        }
        if (gCharCallbackFnPtr) {
            glfwSetCharModsCallback(window, keyCharModsCallback);
        }
        if (gMouseScrollCallbackFnPtr) {
            glfwSetScrollCallback(window, mouseScrollCallback);
        }
        if (gDropCallbackFnPtr) {
            glfwSetDropCallback(window, dropCallback);
        }
        if (gTouchCallbackFnPtr) {
            glfwSetTouchCallback(window, touchCallback);
        }
    }

    initOGL();

    // start sampling tracking data
    if (isMaster()) {
        getTrackingManager().startSampling();
    }

    mInitialized = true;
    return true;
}

void Engine::terminate() {
    mTerminate = true;
}

bool Engine::initNetwork() {
    try {
        mNetworkConnections = std::make_unique<sgct_core::NetworkManager>(
            sgct_core::ClusterManager::instance()->getNetworkMode()
        );
    }
    catch (const std::runtime_error& e) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Initiating network connections failed! Error: '%s'\n", e.what()
        );
        return false;
    }
    catch (const char* err) {
        // @TODO (abock, 2019-08-25) NetworkManager threw a lot of C arrays around, but I
        // changed them to std::runtime_error.  I might have missed one or two, though?
        // Anyway, this catch clause should be get rid of
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Initiating network connections failed. Error: '%s'\n", err
        );
        return false;
    }

    //check in cluster configuration which it is
    if (sgct_core::ClusterManager::instance()->getNetworkMode() ==
        sgct_core::NetworkManager::NetworkMode::Remote)
    {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "Matching ip address to find node in configuration\n"
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

    // If the user has provided the node id as an incorrect cmd argument then make the
    // mThisNode invalid
    if (sgct_core::ClusterManager::instance()->getThisNodeId() >=
        static_cast<int>(sgct_core::ClusterManager::instance()->getNumberOfNodes()) ||
        sgct_core::ClusterManager::instance()->getThisNodeId() < 0)
    {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "This computer is not a part of the cluster configuration\n"
        );
        mNetworkConnections->close();
        return false;
    }
    else {
        mThisNode = sgct_core::ClusterManager::instance()->getThisNode();
    }

    if (!mLogfilePath.empty()) {
        MessageHandler::instance()->setLogPath(
            mLogfilePath.c_str(),
            sgct_core::ClusterManager::instance()->getThisNodeId()
        );
        MessageHandler::instance()->setLogToFile(true);
    }

    if (!mNetworkConnections->init()) {
        return false;
    }

    return true;
}

bool Engine::initWindows() {
    if (mThisNode->getNumberOfWindows() == 0) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "No windows exist in configuration\n"
        );
        return false;
    }

    {
        int tmpGlfwVer[3];
        glfwGetVersion(&tmpGlfwVer[0], &tmpGlfwVer[1], &tmpGlfwVer[2]);
        MessageHandler::instance()->print(
            MessageHandler::Level::VersionInfo,
            "Using GLFW version %d.%d.%d\n", tmpGlfwVer[0], tmpGlfwVer[1], tmpGlfwVer[2]
        );
    }

    switch (mRunMode) {
        case RunMode::OpenGL_3_3_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 330 core";
            break;
        case RunMode::OpenGL_4_0_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 400 core";
            break;

        case RunMode::OpenGL_4_1_Debug_Core_Profile:
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
            [[ fallthrough ]];
        case RunMode::OpenGL_4_1_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 410 core";
            break;
        case RunMode::OpenGL_4_2_Debug_Core_Profile:
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
            [[fallthrough]];
        case RunMode::OpenGL_4_2_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 420 core";
            break;
        case RunMode::OpenGL_4_3_Debug_Core_Profile:
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
            [[fallthrough]];
        case RunMode::OpenGL_4_3_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 430 core";
            break;
        case RunMode::OpenGL_4_4_Debug_Core_Profile:
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
            [[fallthrough]];
        case RunMode::OpenGL_4_4_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 440 core";
            break;
        case RunMode::OpenGL_4_5_Debug_Core_Profile:
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
            [[fallthrough]];
        case RunMode::OpenGL_4_5_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 450 core";
            break;
        case RunMode::OpenGL_4_6_Debug_Core_Profile:
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
            [[fallthrough]];
        case RunMode::OpenGL_4_6_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glewExperimental = true; // Needed for core profile
            mGLSLVersion = "#version 460 core";
            break;
        default:
            mGLSLVersion = "#version 120";
            break;
    }

    if (mPreWindowFnPtr) {
        mPreWindowFnPtr();
    }

    mStatistics = std::make_unique<sgct_core::Statistics>();

    GLFWwindow* share = nullptr;
    int lastWindowIdx = mThisNode->getNumberOfWindows() - 1;
    for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        if (i > 0) {
            share = mThisNode->getWindow(0).getWindowHandle();
        }
        
        if (!mThisNode->getWindow(i).openWindow(share, lastWindowIdx)) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Failed to open window %d\n", i
            );
            return false;
        }
    }

    GLenum err = glewInit();
    if (GLEW_OK != err) {
      // Problem: glewInit failed, something is seriously wrong.
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
            MessageHandler::Level::Error,
            "GLEW init triggered an OpenGL error\n"
        );
    }

    // Window/Contexty creation callback
    if (mThisNode->getNumberOfWindows() > 0) {
        share = mThisNode->getWindow(0).getWindowHandle();

        if (mContextCreationFnPtr) {
            mContextCreationFnPtr(share);
        }
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "No windows created on this node!\n"
        );
        return false;
    }

    for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        Window& win = mThisNode->getWindow(i);
        win.init();
        updateAAInfo(win);
    }

    // init draw buffer resolution
    updateDrawBufferResolutions();
    waitForAllWindowsInSwapGroupToOpen();

    if (RunFrameLockCheckThread) {
        if (sgct_core::ClusterManager::instance()->getNumberOfNodes() > 1) {
            mThreadPtr = std::make_unique<std::thread>(updateFrameLockLoop, nullptr);
        }
    }

    // init swap group if enabled
    if (mThisNode->isUsingSwapGroups()) {
        Window::initNvidiaSwapGroups();
    }

    return true;
}

void Engine::initOGL() {
    // Set up function pointers etc. depending on fixed or programmable pipeline
    if (mRunMode > RunMode::OpenGL_Compatibility_Profile) {
        mInternalDrawFn = [this]() { draw(); };
        mInternalRenderFBOFn = [this]() { renderFBOTexture(); };
        mInternalDrawOverlaysFn = [this]() { drawOverlays(); };
        mInternalRenderPostFXFn = [this](TextureIndexes idx) { renderPostFX(idx); };

        // force buffer objects since display lists are not supported in core opengl 3.3+
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

    // Get OpenGL version
    int version[3];
    GLFWwindow* winHandle = getCurrentWindow().getWindowHandle();
    version[0] = glfwGetWindowAttrib(winHandle, GLFW_CONTEXT_VERSION_MAJOR);
    version[1] = glfwGetWindowAttrib(winHandle, GLFW_CONTEXT_VERSION_MINOR);
    version[2] = glfwGetWindowAttrib(winHandle, GLFW_CONTEXT_REVISION);

    MessageHandler::instance()->print(
        MessageHandler::Level::VersionInfo,
        "OpenGL version %d.%d.%d %s\n",
        version[0], version[1], version[2],
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

    if (!glfwExtensionSupported("GL_EXT_framebuffer_object") && version[0] < 2) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Warning! Frame buffer objects are not supported! "
            "A lot of features in SGCT will not work!\n"
        );
        Settings::instance()->setUseFBO(false);
    }
    else if (!glfwExtensionSupported("GL_EXT_framebuffer_multisample") && version[0] < 2)
    {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Warning! FBO multisampling is not supported!\n"
        );
        Settings::instance()->setUseFBO(true);

        for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
            mThisNode->getWindow(i).setNumberOfAASamples(1);
        }
    }

    if (sgct_core::ClusterManager::instance()->getNumberOfNodes() > 1) {
        std::string path = Settings::instance()->getCapturePath();
        path += "_node";
        path += std::to_string(sgct_core::ClusterManager::instance()->getThisNodeId());

        using CapturePath = Settings::CapturePath;
        Settings::instance()->setCapturePath(path, CapturePath::Mono);
        Settings::instance()->setCapturePath(path, CapturePath::LeftStereo);
        Settings::instance()->setCapturePath(path, CapturePath::RightStereo);
    }

    // init window opengl data
    getCurrentWindow().makeOpenGLContextCurrent(Window::Context::Shared);

    loadShaders();
    mStatistics->initVBO(mFixedOGLPipeline);

    if (mInitOGLFnPtr) {
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

    // create all textures, etc
    for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        mThisNode->setCurrentWindowIndex(i);
        // set context to shared
        getCurrentWindow().initOGL();
        
        if (mScreenShotFnPtr) {
            // set callback
            auto callback = [this](sgct_core::Image* img, size_t size,
                                   sgct_core::ScreenCapture::EyeIndex idx,
                                   unsigned int type)
            {
                invokeScreenShotCallback(img, size, idx, type);
            };
            
            Window& win = getCurrentWindow();
            // left channel (Mono and Stereo_Left)
            sgct_core::ScreenCapture* monoCapture = win.getScreenCapturePointer(
                Window::Eye::MonoOrLeft
            );
            if (monoCapture) {
                monoCapture->setCaptureCallback(callback);
            }
            // right channel (Stereo_Right)
            sgct_core::ScreenCapture* rightCapture = win.getScreenCapturePointer(
                Window::Eye::Right
            );
            if (rightCapture) {
                rightCapture->setCaptureCallback(callback);
            }
        }
    }

    // link all users to their viewports
    for (int w = 0; w < mThisNode->getNumberOfWindows(); w++) {
        Window& winPtr = mThisNode->getWindow(w);
        for (int i = 0; i < winPtr.getNumberOfViewports(); i++) {
            winPtr.getViewport(i).linkUserName();
        }
    }

    updateFrustums();

    // Add fonts
#ifdef SGCT_HAS_TEXT
    if (Settings::instance()->getOSDTextFontPath().empty()) {
        const bool success = sgct_text::FontManager::instance()->addFont(
            "SGCTFont",
            Settings::instance()->getOSDTextFontName()
        );
        if (!success) {
            sgct_text::FontManager::instance()->getFont(
                "SGCTFont",
                Settings::instance()->getOSDTextFontSize()
            );
        }
    }
    else {
        std::string tmpPath = Settings::instance()->getOSDTextFontPath() +
                              Settings::instance()->getOSDTextFontName();
        const bool success = sgct_text::FontManager::instance()->addFont(
            "SGCTFont",
            tmpPath,
            sgct_text::FontManager::FontPath::Local
        );
        if (!success) {
            sgct_text::FontManager::instance()->getFont(
                "SGCTFont",
                Settings::instance()->getOSDTextFontSize()
            );
        }
    }
#endif // SGCT_HAS_TEXT

    // init swap barrier is swap groups are active
    Window::setBarrier(true);
    Window::resetSwapGroupFrameNumber();

    for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        // generate mesh (VAO and VBO)
        mThisNode->getWindow(i).initContextSpecificOGL();
    }

    // check for errors
    checkForOGLErrors();

    MessageHandler::instance()->print(
        MessageHandler::Level::Important,
        "\nReady to render\n"
    );
}

void Engine::clean() {
    MessageHandler::instance()->print(
        MessageHandler::Level::Important,
        "Cleaning up\n"
    );

    if (mCleanUpFnPtr) {
        if (mThisNode && mThisNode->getNumberOfWindows() > 0) {
            mThisNode->getWindow(0).makeOpenGLContextCurrent(Window::Context::Shared);
        }
        mCleanUpFnPtr();
    }

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Clearing all callbacks\n"
    );
    clearAllCallbacks();

    // kill thread
    if (mThreadPtr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "Waiting for frameLock thread to finish\n"
        );

        sgct::MutexManager::instance()->mFrameSyncMutex.lock();
        sRunUpdateFrameLockLoop = false;
        sgct::MutexManager::instance()->mFrameSyncMutex.unlock();

        mThreadPtr->join();
        mThreadPtr = nullptr;
        MessageHandler::instance()->print(MessageHandler::Level::Debug, "Done.\n");
    }

    // de-init window and unbind swapgroups
    if (mThisNode && sgct_core::ClusterManager::instance()->getNumberOfNodes() > 0) {
        for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
            mThisNode->getWindow(i).close();
        }
    }

    // close TCP connections
    mNetworkConnections = nullptr;

    // Shared contex
    if (mThisNode && mThisNode->getNumberOfWindows() > 0) {
        mThisNode->getWindow(0).makeOpenGLContextCurrent(Window::Context::Shared);
    }

    mStatistics = nullptr;

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Destroying shader manager and internal shaders\n"
    );
    ShaderManager::destroy();

    mShader.fboQuad.deleteProgram();
    mShader.fxaa.deleteProgram();
    mShader.overlay.deleteProgram();

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Destroying texture manager\n"
    );
    TextureManager::destroy();

#ifdef SGCT_HAS_TEXT
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Destroying font manager\n"
    );
    sgct_text::FontManager::destroy();
#endif // SGCT_HAS_TEXT

    // Window specific context
    if (mThisNode && mThisNode->getNumberOfWindows() > 0) {
        mThisNode->getWindow(0).makeOpenGLContextCurrent(Window::Context::Window);
    }
    
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Destroying shared data\n"
    );
    SharedData::destroy();
    
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Destroying cluster manager\n"
    );
    sgct_core::ClusterManager::destroy();
    
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Destroying settings\n"
    );
    Settings::destroy();

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Destroying message handler\n"
    );
    MessageHandler::destroy();

    std::cout << "Destroying mutexes\n" << std::endl;
    MutexManager::destroy();

    // Close window and terminate GLFW
    std::cout << std::endl << "Terminating glfw...";
    glfwTerminate();
    std::cout << " Done." << std::endl;
}

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
    mScreenShotFnPtr = nullptr;

    mInternalDrawFn = nullptr;
    mInternalRenderFBOFn = nullptr;
    mInternalDrawOverlaysFn = nullptr;
    mInternalRenderPostFXFn = nullptr;

    gKeyboardCallbackFnPtr = nullptr;
    gMouseButtonCallbackFnPtr = nullptr;
    gMousePosCallbackFnPtr = nullptr;
    gMouseScrollCallbackFnPtr = nullptr;
    gDropCallbackFnPtr = nullptr;

    for (TimerInformation& ti : mTimers) {
        ti.mCallback = nullptr;
    }
}

bool Engine::frameLockPreStage() {
    using namespace sgct_core;

    double t0 = glfwGetTime();
    // from server to clients
    mNetworkConnections->sync(NetworkManager::SyncMode::SendDataToClients, *mStatistics);
    mStatistics->setSyncTime(static_cast<float>(glfwGetTime() - t0));

    // run only on clients/slaves
    if (ClusterManager::instance()->getIgnoreSync() ||
        mNetworkConnections->isComputerServer())
    {
        return true;
    }


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
            std::unique_lock lk(MutexManager::instance()->mFrameSyncMutex);
            sgct_core::NetworkManager::gCond.wait(lk);
        }

        // for debugging
        sgct_core::Network* conn;
        if (glfwGetTime() - t0 > 1.0) {
            // more than a second
            conn = mNetworkConnections->getSyncConnectionByIndex(0);
            if (mPrintSyncMessage && !conn->isUpdated()) {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Info,
                    "Slave: waiting for master... send frame %d != previous recv "
                    "frame %d\n\tNvidia swap groups: %s\n\tNvidia swap barrier: "
                    "%s\n\tNvidia universal frame number: %u\n\tSGCT frame "
                    "number: %u\n",
                    conn->getSendFrameCurrent(),
                    conn->getRecvFramePrevious(),
                    getCurrentWindow().isUsingSwapGroups() ? "enabled" : "disabled",
                    getCurrentWindow().isBarrierActive() ? "enabled" : "disabled",
                    getCurrentWindow().getSwapGroupFrameNumber(),
                    mFrameCounter
                );
            }

            if (glfwGetTime() - t0 > mSyncTimeout) {
                // more than a minute
                MessageHandler::instance()->print(
                    MessageHandler::Level::Error,
                    "Slave: no sync signal from master after %.1f seconds! "
                    "Exiting...", mSyncTimeout
                );
                return false;
            }
        }
    }

    // A this point all data needed for rendering a frame is received.
    // Let's signal that back to the master/server.
    mNetworkConnections->sync(NetworkManager::SyncMode::AcknowledgeData, *mStatistics);
    mStatistics->addSyncTime(static_cast<float>(glfwGetTime() - t0));

    return true;
}

bool Engine::frameLockPostStage() {
    // post stage
    if (sgct_core::ClusterManager::instance()->getIgnoreSync() ||
        !mNetworkConnections->isComputerServer())
    {
        return true;
    }

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
            std::unique_lock lk(MutexManager::instance()->mFrameSyncMutex);
            sgct_core::NetworkManager::gCond.wait(lk);
        }

        // for debugging

        if (glfwGetTime() - t0 <= 1.0) {
            continue;
        }
        // more than a second

        for (int i = 0; i < mNetworkConnections->getSyncConnectionsCount(); i++) {
            const sgct_core::Network& conn = mNetworkConnections->getConnectionByIndex(i);

            if (mPrintSyncMessage && !conn.isUpdated()) {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Info,
                    "Waiting for slave%d: send frame %d != recv frame %d\n\t"
                    "Nvidia swap groups: %s\n\tNvidia swap barrier: %s\n\t"
                    "Nvidia universal frame number: %u\n\t"
                    "SGCT frame number: %u\n",
                    i,
                    mNetworkConnections->getConnectionByIndex(i).getSendFrameCurrent(),
                    mNetworkConnections->getConnectionByIndex(i).getRecvFrameCurrent(),
                    getCurrentWindow().isUsingSwapGroups() ? "enabled" : "disabled",
                    getCurrentWindow().isBarrierActive() ? "enabled" : "disabled",
                    getCurrentWindow().getSwapGroupFrameNumber(),
                    mFrameCounter
                );
            }
        }

        if (glfwGetTime() - t0 > mSyncTimeout) {
            // more than a minute
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Master: no sync signal from all slaves after %.1f seconds. "
                "Exiting\n", mSyncTimeout
            );

            return false;
        }
    }
    mStatistics->addSyncTime(static_cast<float>(glfwGetTime() - t0));

    return true;
}

void Engine::render() {
    using namespace sgct_core;

    if (!mInitialized) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Render function called before initialization\n"
        );
        return;
    }
    
    mRunning = true;

    // create OpenGL query objects for Opengl 3.3+
    GLuint timeQueryBegin = 0;
    GLuint timeQueryEnd = 0;
    if (!mFixedOGLPipeline) {
        getCurrentWindow().makeOpenGLContextCurrent(Window::Context::Shared);
        glGenQueries(1, &timeQueryBegin);
        glGenQueries(1, &timeQueryEnd);
    }

    while (mRunning) {
        mRenderingOffScreen = false;

        // update tracking data
        if (isMaster()) {
            ClusterManager::instance()->getTrackingManager().updateTrackingDevices();
        }

        if (mPreSyncFnPtr) {
            mPreSyncFnPtr();
        }

        if (mNetworkConnections->isComputerServer()) {
            SharedData::instance()->encode();
        }
        else if (!mNetworkConnections->isRunning())  {
            // exit if not running
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Network disconnected! Exiting\n"
            );
            break;
        }

        if (!frameLockPreStage()) {
            break;
        }

        // check if re-size needed of VBO and PBO
        // context switching may occur if multiple windows are used
        bool buffersNeedUpdate = false;
        for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
            const bool bufUpdate = mThisNode->getWindow(i).update();
            buffersNeedUpdate |= bufUpdate;
        }

        if (buffersNeedUpdate) {
            updateDrawBufferResolutions();
        }
    
        mRenderingOffScreen = Settings::instance()->useFBO();
        if (mRenderingOffScreen) {
            getCurrentWindow().makeOpenGLContextCurrent(Window::Context::Shared);
        }

        // Make sure correct context is current
        if (mPostSyncPreDrawFnPtr) {
            mPostSyncPreDrawFnPtr();
        }

        double startFrameTime = glfwGetTime();
        calculateFPS(startFrameTime); // measures time between calls

        if (!mFixedOGLPipeline && mShowGraph) {
            glQueryCounter(timeQueryBegin, GL_TIMESTAMP);
        }

        // Render Viewports / Draw
        mCurrentDrawBufferIndex = 0;
        size_t firstDrawBufferIndexInWindow = 0;

        for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
            if (!(mThisNode->getWindow(i).isVisible() ||
                  mThisNode->getWindow(i).isRenderingWhileHidden()))
            {
                continue;
            }


            // store the first buffer index for each window
            firstDrawBufferIndexInWindow = mCurrentDrawBufferIndex;

            // @TODO (abock, 2019-09-02): This is kinda weird; I tried commenting this
            // part out and only use mThisNode->getWindow(i) directly, but then it failed
            // to have two separate rendering windows. So some hidden state somewhere, I
            // guess?!
            mThisNode->setCurrentWindowIndex(i);
            Window& win = getCurrentWindow();

            if (!mRenderingOffScreen) {
                win.makeOpenGLContextCurrent(Window::Context::Window);
            }

            Window::StereoMode sm = win.getStereoMode();

            // Render Left/Mono non-linear projection viewports to cubemap
            mCurrentRenderTarget = RenderTarget::NonLinearBuffer;

            for (int j = 0; j < win.getNumberOfViewports(); j++) {
                Viewport& vp = win.getViewport(j);
                mCurrentViewportIndex.main = j;
                if (!vp.hasSubViewports()) {
                    continue;
                }

                NonLinearProjection* nonLinearProj = vp.getNonLinearProjection();
                mCurrentOffScreenBuffer = nonLinearProj->getOffScreenBuffer();

                nonLinearProj->setAlpha(getCurrentWindow().getAlpha() ? 0.f : 1.f);
                if (sm == Window::StereoMode::NoStereo) {
                    //for mono viewports frustum mode can be selected by user or xml
                    mCurrentFrustumMode = win.getViewport(j).getEye();
                    nonLinearProj->renderCubemap(&mCurrentViewportIndex.sub);
                }
                else {
                    mCurrentFrustumMode = Frustum::StereoLeftEye;
                    nonLinearProj->renderCubemap(&mCurrentViewportIndex.sub);
                }

                // FBO index, every window and every nonlinear projection has it's own FBO
                mCurrentDrawBufferIndex++;
            }

            // Render left/mono regular viewports to fbo
            mCurrentRenderTarget = RenderTarget::WindowBuffer;
            mCurrentOffScreenBuffer = win.getFBO();

            // if any stereo type (except passive) then set frustum mode to left eye
            if (sm == Window::StereoMode::NoStereo) {
                mCurrentFrustumMode = Frustum::MonoEye;
                renderViewports(LeftEye);
            }
            else {
                mCurrentFrustumMode = Frustum::StereoLeftEye;
                renderViewports(LeftEye);
            }

            // FBO index, every window and every non-linear projection has it's own FBO
            mCurrentDrawBufferIndex++;

            // if we are not rendering in stereo, we are done
            if (sm == Window::StereoMode::NoStereo) {
                continue;
            }

            // jump back counter to the first buffer index for current window
            mCurrentDrawBufferIndex = firstDrawBufferIndexInWindow;

            // Render right non-linear projection viewports to cubemap
            mCurrentRenderTarget = RenderTarget::NonLinearBuffer;
            for (int j = 0; j < win.getNumberOfViewports(); j++) {
                mCurrentViewportIndex.main = j;

                if (!win.getViewport(j).hasSubViewports()) {
                    continue;
                }
                NonLinearProjection* p = win.getViewport(j).getNonLinearProjection();
                mCurrentOffScreenBuffer = p->getOffScreenBuffer();

                p->setAlpha(getCurrentWindow().getAlpha() ? 0.f : 1.f);
                mCurrentFrustumMode = Frustum::StereoRightEye;
                p->renderCubemap(&mCurrentViewportIndex.sub);

                // FBO index, every window and every nonlinear projection has it's own
                mCurrentDrawBufferIndex++;
            }

            // Render right regular viewports to FBO
            mCurrentRenderTarget = RenderTarget::WindowBuffer;
            mCurrentOffScreenBuffer = win.getFBO();

            mCurrentFrustumMode = Frustum::StereoRightEye;
            // use a single texture for side-by-side and top-bottom stereo modes
            if (sm >= Window::StereoMode::SideBySide) {
                renderViewports(LeftEye);
            }
            else {
                renderViewports(RightEye);
            }

            // FBO index, every window and every non-linear projection has their own
            mCurrentDrawBufferIndex++;
        }

        // Render to screen
        for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
            if (mThisNode->getWindow(i).isVisible()) {
                mThisNode->setCurrentWindowIndex(i);

                mRenderingOffScreen = false;
                if (Settings::instance()->useFBO()) {
                    mInternalRenderFBOFn();
                }
            }
        }
        getCurrentWindow().makeOpenGLContextCurrent(Window::Context::Shared);

#ifdef __SGCT_DEBUG__
        checkForOGLErrors();
#endif

        if (!mFixedOGLPipeline && mShowGraph) {
            glQueryCounter(timeQueryEnd, GL_TIMESTAMP);
        }

        double endFrameTime = glfwGetTime();
        updateTimers(endFrameTime);

        if (mPostDrawFnPtr) {
            mPostDrawFnPtr();
        }

        if (mFixedOGLPipeline) {
            mStatistics->setDrawTime(static_cast<float>(endFrameTime - startFrameTime));
        }
        else if (mShowGraph) {
            // wait until the query results are available
            GLint done = GL_FALSE;
            while (!done) {
                glGetQueryObjectiv(timeQueryEnd, GL_QUERY_RESULT_AVAILABLE, &done);
            }

            GLuint64 timerStart;
            GLuint64 timerEnd;
            // get the query results
            glGetQueryObjectui64v(timeQueryBegin, GL_QUERY_RESULT, &timerStart);
            glGetQueryObjectui64v(timeQueryEnd, GL_QUERY_RESULT, &timerEnd);

            const double t = static_cast<double>(timerEnd - timerStart) / 1000000000.0;
            mStatistics->setDrawTime(static_cast<float>(t));
        }

        if (mShowGraph) {
            mStatistics->update();
        }
        
        // master will wait for nodes render before swapping
        if (!frameLockPostStage()) {
            break;
        }

        // Swap front and back rendering buffers
        for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
            mThisNode->getWindow(i).swap(mTakeScreenshot);
        }

        glfwPollEvents();
        for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
            mThisNode->getWindow(i).updateResolutions();
        }

        // Check if ESC key was pressed or window was closed
        mRunning = !(mThisNode->getKeyPressed(mExitKey) ||
                   mThisNode->shouldAllWindowsClose() || mTerminate ||
                   !mNetworkConnections->isRunning());

        // for all windows
        mFrameCounter++;
        if (mTakeScreenshot) {
            mShotCounter++;
        }
        mTakeScreenshot = false;
    }

    if (!mFixedOGLPipeline) {
        getCurrentWindow().makeOpenGLContextCurrent(Window::Context::Shared);
        glDeleteQueries(1, &timeQueryBegin);
        glDeleteQueries(1, &timeQueryEnd);
    }
}

void Engine::renderDisplayInfo() {
#ifdef SGCT_HAS_TEXT
    unsigned int lFrameNumber = getCurrentWindow().getSwapGroupFrameNumber();

    glm::vec4 strokeColor = sgct_text::FontManager::instance()->getStrokeColor();
    sgct_text::FontManager::instance()->setStrokeColor(glm::vec4(0.f, 0.f, 0.f, 0.8f));

    unsigned int font_size = Settings::instance()->getOSDTextFontSize();
    font_size = static_cast<unsigned int>(
        static_cast<float>(font_size) * getCurrentWindow().getScale().x
    );
    
    sgct_text::Font* font = sgct_text::FontManager::instance()->getFont(
        "SGCTFont",
        font_size
    );

    if (font) {
        float lineHeight = font->getHeight() * 1.59f;
        glm::vec2 pos = glm::vec2(getCurrentWindow().getResolution()) *
                        Settings::instance()->getOSDTextOffset();
        
        sgct_text::print(
            *font,
            sgct_text::TextAlignMode::TopLeft,
            pos.x,
            lineHeight * 6.f + pos.y,
            glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
            "Node ip: %s (%s)",
            mThisNode->getAddress().c_str(),
            mNetworkConnections->isComputerServer() ? "master" : "slave"
        );

        sgct_text::print(
            *font,
            sgct_text::TextAlignMode::TopLeft,
            pos.x,
            lineHeight * 5.f + pos.y,
            glm::vec4(0.8f,0.8f,0.f,1.f),
            "Frame rate: %.2f Hz, frame: %u",
            mStatistics->getAvgFPS(),
            mFrameCounter
        );

        sgct_text::print(
            *font,
            sgct_text::TextAlignMode::TopLeft,
            pos.x,
            lineHeight * 4.f + pos.y,
            glm::vec4(0.8f, 0.f, 0.8f, 1.f),
            "Avg. draw time: %.2f ms",
            mStatistics->getAvgDrawTime() * 1000.f
        );

        if (isMaster()) {
            sgct_text::print(
                *font,
                sgct_text::TextAlignMode::TopLeft,
                pos.x,
                lineHeight * 3.f + pos.y,
                glm::vec4(0.f,0.8f,0.8f,1.f),
                "Avg. sync time: %.2f ms (%d bytes, comp: %.3f)",
                mStatistics->getAvgSyncTime() * 1000.0,
                SharedData::instance()->getUserDataSize(),
                SharedData::instance()->getCompressionRatio()
            );
        }
        else {
            sgct_text::print(
                *font,
                sgct_text::TextAlignMode::TopLeft,
                pos.x,
                lineHeight * 3.f + pos.y,
                glm::vec4(0.f, 0.8f, 0.8f, 1.f),
                "Avg. sync time: %.2f ms",
                mStatistics->getAvgSyncTime() * 1000.0
            );
        }

        bool usingSwapGroups = getCurrentWindow().isUsingSwapGroups();
        if (usingSwapGroups) {
            sgct_text::print(
                *font,
                sgct_text::TextAlignMode::TopLeft,
                pos.x,
                lineHeight * 2.f + pos.y,
                glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
                "Swap groups: %s and barrier is %s (%s) | Frame: %d",
                getCurrentWindow().isUsingSwapGroups() ? "Enabled" : "Disabled",
                getCurrentWindow().isBarrierActive() ? "active" : "inactive",
                getCurrentWindow().isSwapGroupMaster() ? "master" : "slave",
                lFrameNumber
            );
        }
        else {
            sgct_text::print(
                *font,
                sgct_text::TextAlignMode::TopLeft,
                pos.x,
                lineHeight * 2.f + pos.y,
                glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
                "Swap groups: Disabled"
            );
        }

        sgct_text::print(
            *font,
            sgct_text::TextAlignMode::TopLeft,
            pos.x,
            lineHeight * 1.f + pos.y,
            glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
            "Frame buffer resolution: %d x %d",
            getCurrentWindow().getFramebufferResolution().x,
            getCurrentWindow().getFramebufferResolution().y
        );

        sgct_text::print(
            *font,
            sgct_text::TextAlignMode::TopLeft,
            pos.x,
            lineHeight * 0.f + pos.y,
            glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
            "Anti-Aliasing: %s",
            mAAInfo.c_str()
        );

        // if active stereoscopic rendering
        if (mCurrentFrustumMode == sgct_core::Frustum::StereoLeftEye) {
            sgct_text::print(
                *font,
                sgct_text::TextAlignMode::TopLeft,
                pos.x,
                lineHeight * 8.f + pos.y,
                glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
                "Stereo type: %s\nCurrent eye: Left",
                getCurrentWindow().getStereoModeStr().c_str()
            );
        }
        else if (mCurrentFrustumMode == sgct_core::Frustum::StereoRightEye) {
            sgct_text::print(
                *font,
                sgct_text::TextAlignMode::TopLeft,
                pos.x,
                lineHeight * 8.f + pos.y,
                glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
                "Stereo type: %s\nCurrent eye:          Right",
                getCurrentWindow().getStereoModeStr().c_str()
            );
        }
    }

    // reset
    sgct_text::FontManager::instance()->setStrokeColor(strokeColor);
#endif // SGCT_HAS_TEXT
}

void Engine::draw() {
    // run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);
    
    enterCurrentViewport();
    
    if (Settings::instance()->useFBO()) {
        setAndClearBuffer(BufferMode::RenderToTexture);
    }
    else {
        setAndClearBuffer(BufferMode::BackBuffer);
    }
    
    glDisable(GL_SCISSOR_TEST);

    if (mDrawFnPtr) {
        glLineWidth(1.0);
        if (mShowWireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        mDrawFnPtr();

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void Engine::drawFixedPipeline() {
    // run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);
    
    // set glViewport & glScissor
    enterCurrentViewport();
    
    if (Settings::instance()->useFBO()) {
        setAndClearBuffer(BufferMode::RenderToTexture);
    }
    else {
        setAndClearBuffer(BufferMode::BackBuffer);
    }
    
    glDisable(GL_SCISSOR_TEST);

    glMatrixMode(GL_PROJECTION);

    sgct_core::Projection& proj =
        getCurrentWindow().getCurrentViewport()->getProjection(mCurrentFrustumMode);

    glLoadMatrixf(glm::value_ptr(proj.getProjectionMatrix()));

    glMatrixMode(GL_MODELVIEW);

    glLoadMatrixf(glm::value_ptr(proj.getViewMatrix() * getModelMatrix()));

    if (mDrawFnPtr) {
        glLineWidth(1.0);

        if (mShowWireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        mDrawFnPtr();

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void Engine::drawOverlays() {
    for (int i = 0; i < getCurrentWindow().getNumberOfViewports(); i++) {
        getCurrentWindow().setCurrentViewport(i);
        const sgct_core::Viewport& vp = getCurrentWindow().getViewport(i);
        
        // if viewport has overlay
        if (!vp.hasOverlayTexture() || !vp.isEnabled()) {
            continue;
        }

        enterCurrentViewport();
            
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, vp.getOverlayTextureIndex());
        mShader.overlay.bind();
        glUniform1i(mShaderLoc.overlayTex, 0);
        getCurrentWindow().bindVAO();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        getCurrentWindow().unbindVAO();
        ShaderProgram::unbind();
    }
}

void Engine::drawOverlaysFixedPipeline() {
    for (int i = 0; i < getCurrentWindow().getNumberOfViewports(); i++) {
        getCurrentWindow().setCurrentViewport(i);
        const sgct_core::Viewport& vp = getCurrentWindow().getViewport(i);

        if (!vp.hasOverlayTexture() || !vp.isEnabled()) {
            return;
        }

        // enter ortho mode
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glPushMatrix();
            
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

        glActiveTexture(GL_TEXTURE0);
            
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, vp.getOverlayTextureIndex());

        glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
            
        getCurrentWindow().bindVBO();

        glClientActiveTexture(GL_TEXTURE0);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(0));

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 5 * sizeof(float), reinterpret_cast<void*>(8));

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        getCurrentWindow().unbindVBO();

        glPopClientAttrib();
        glPopAttrib();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
    }
}

void Engine::prepareBuffer(TextureIndexes ti) {
    if (!Settings::instance()->useFBO()) {
        return;
    }
    if (getCurrentWindow().usePostFX()) {
        ti = Intermediate;
    }

    sgct_core::OffScreenBuffer* fbo = getCurrentWindow().getFBO();

    fbo->bind();
    if (fbo->isMultiSampled()) {
        return;
    }

    // update attachments
    fbo->attachColorTexture(getCurrentWindow().getFrameBufferTexture(ti));

    if (Settings::instance()->useDepthTexture()) {
        fbo->attachDepthTexture(getCurrentWindow().getFrameBufferTexture(Depth));
    }

    if (Settings::instance()->useNormalTexture()) {
        fbo->attachColorTexture(
            getCurrentWindow().getFrameBufferTexture(Normals),
            GL_COLOR_ATTACHMENT1
        );
    }

    if (Settings::instance()->usePositionTexture()) {
        fbo->attachColorTexture(
            getCurrentWindow().getFrameBufferTexture(Positions),
            GL_COLOR_ATTACHMENT2
        );
    }
}

void Engine::renderFBOTexture() {
    sgct_core::OffScreenBuffer::unBind();

    bool maskShaderSet = false;

    Window& win = getCurrentWindow();
    win.makeOpenGLContextCurrent(Window::Context::Window);

    glDisable(GL_BLEND);
    // needed for shaders
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    mCurrentFrustumMode = win.getStereoMode() == Window::StereoMode::Active ?
        sgct_core::Frustum::StereoLeftEye :
        sgct_core::Frustum::MonoEye;

    glm::ivec2 size = glm::ivec2(
        glm::ceil(win.getScale() * glm::vec2(win.getResolution()))
    );
        
    glViewport(0, 0, size.x, size.y);
    setAndClearBuffer(BufferMode::BackBufferBlack);
   
    const bool useWarping = Settings::instance()->getUseWarping();
    Window::StereoMode sm = win.getStereoMode();
    if (sm > Window::StereoMode::Active && sm < Window::StereoMode::SideBySide) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, win.getFrameBufferTexture(LeftEye));

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, win.getFrameBufferTexture(RightEye));

        win.bindStereoShaderProgram();

        glUniform1i(win.getStereoShaderLeftTexLoc(), 0);
        glUniform1i(win.getStereoShaderRightTexLoc(), 1);

        for (int i = 0; i < win.getNumberOfViewports(); i++) {
            if (useWarping) {
               win.getViewport(i).renderWarpMesh();
            }
            else {
                win.getViewport(i).renderQuadMesh();
            }
        }
    }
    else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, win.getFrameBufferTexture(LeftEye));

        mShader.fboQuad.bind();
        glUniform1i(mShaderLoc.monoTex, 0);
        maskShaderSet = true;

        for (int i = 0; i < win.getNumberOfViewports(); i++) {
            if (useWarping) {
                win.getViewport(i).renderWarpMesh();
            }
            else {
                win.getViewport(i).renderQuadMesh();
            }
        }

        // render right eye in active stereo mode
        if (win.getStereoMode() == Window::StereoMode::Active) {
            glViewport(0, 0, size.x, size.y);
            
            //clear buffers
            mCurrentFrustumMode = sgct_core::Frustum::StereoRightEye;
            setAndClearBuffer(BufferMode::BackBufferBlack);

            glBindTexture(GL_TEXTURE_2D, win.getFrameBufferTexture(RightEye));
            for (int i = 0; i < win.getNumberOfViewports(); i++) {
                if (useWarping) {
                    win.getViewport(i).renderWarpMesh();
                }
                else {
                    win.getViewport(i).renderQuadMesh();
                }
            }
        }
    }

    // render mask (mono)
    if (win.hasAnyMasks()) {
        if (!maskShaderSet) {
            mShader.fboQuad.bind();
            glUniform1i(mShaderLoc.monoTex, 0);
        }
        
        glDrawBuffer(win.isDoubleBuffered() ? GL_BACK : GL_FRONT);
        glReadBuffer(win.isDoubleBuffered() ? GL_BACK : GL_FRONT);
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_BLEND);

        // Result = (Color * BlendMask) * (1-BlackLevel) + BlackLevel

        // render blend masks
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        for (int i = 0; i < win.getNumberOfViewports(); i++) {
            const sgct_core::Viewport& vp = win.getViewport(i);
            if (vp.hasBlendMaskTexture() && vp.isEnabled()) {
                glBindTexture(GL_TEXTURE_2D, vp.getBlendMaskTextureIndex());
                vp.renderMaskMesh();
            }
        }

        // render black level masks
        for (int i = 0; i < win.getNumberOfViewports(); i++) {
            const sgct_core::Viewport& vp = win.getViewport(i);
            if (vp.hasBlackLevelMaskTexture() && vp.isEnabled()) {
                glBindTexture(GL_TEXTURE_2D, vp.getBlackLevelMaskTextureIndex());

                // inverse multiply
                glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
                vp.renderMaskMesh();

                // add
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                vp.renderMaskMesh();
            }
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    ShaderProgram::unbind();
    glDisable(GL_BLEND);
}


void Engine::renderFBOTextureFixedPipeline() {
    sgct_core::OffScreenBuffer::unBind();
    
    Window& win = getCurrentWindow();
    win.makeOpenGLContextCurrent(Window::Context::Window);
    
    mCurrentFrustumMode = win.getStereoMode() == Window::StereoMode::Active ?
        sgct_core::Frustum::StereoLeftEye :
        sgct_core::Frustum::MonoEye;
    
    glm::ivec2 s = glm::ivec2(glm::ceil(win.getScale() * glm::vec2(win.getResolution())));
    glViewport(0, 0, s.x, s.y);
    setAndClearBuffer(BufferMode::BackBufferBlack);

    // enter ortho mode
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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glLoadIdentity();
    
    Window::StereoMode sm = win.getStereoMode();

    const bool useWarping = Settings::instance()->getUseWarping();
    if (sm > Window::StereoMode::Active && sm < Window::StereoMode::SideBySide) {
        win.bindStereoShaderProgram();

        glUniform1i(win.getStereoShaderLeftTexLoc(), 0);
        glUniform1i(win.getStereoShaderRightTexLoc(), 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, win.getFrameBufferTexture(LeftEye));
        glEnable(GL_TEXTURE_2D);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, win.getFrameBufferTexture(RightEye));
        glEnable(GL_TEXTURE_2D);

        for (int i = 0; i < win.getNumberOfViewports(); i++) {
            if (useWarping) {
                win.getViewport(i).renderWarpMesh();
            }
            else {
                win.getViewport(i).renderQuadMesh();
            }
        }
        ShaderProgram::unbind();
    }
    else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, win.getFrameBufferTexture(LeftEye));
        glEnable(GL_TEXTURE_2D);

        for (int i = 0; i < win.getNumberOfViewports(); i++) {
            if (useWarping) {
                win.getViewport(i).renderWarpMesh();
            }
            else {
                win.getViewport(i).renderQuadMesh();
            }
        }

        // render right eye in active stereo mode
        if (win.getStereoMode() == Window::StereoMode::Active) {
            glm::ivec2 res = win.getResolution();
            glViewport(0, 0, res.x, res.y);
            
            mCurrentFrustumMode = sgct_core::Frustum::StereoRightEye;
            setAndClearBuffer(BufferMode::BackBufferBlack);

            glBindTexture(GL_TEXTURE_2D, win.getFrameBufferTexture(RightEye));

            for (int i = 0; i < win.getNumberOfViewports(); i++) {
                if (useWarping) {
                    win.getViewport(i).renderWarpMesh();
                }
                else {
                    win.getViewport(i).renderQuadMesh();
                }
            }
        }
    }

    // render mask (mono)
    if (win.hasAnyMasks()) {
        glDrawBuffer(win.isDoubleBuffered() ? GL_BACK : GL_FRONT);
        glReadBuffer(win.isDoubleBuffered() ? GL_BACK : GL_FRONT);

        // if stereo != active stereo
        glActiveTexture(GL_TEXTURE1);
        glDisable(GL_TEXTURE_2D);

        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_BLEND);

        // Result = (Color * BlendMask) * (1-BlackLevel) + BlackLevel

        // render blend masks
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        for (int i = 0; i < win.getNumberOfViewports(); i++) {
            const sgct_core::Viewport& vp = win.getViewport(i);
            if (vp.hasBlendMaskTexture() && vp.isEnabled()) {
                glBindTexture(GL_TEXTURE_2D, vp.getBlendMaskTextureIndex());
                vp.renderMaskMesh();
            }
        }

        // render black level masks
        for (int i = 0; i < win.getNumberOfViewports(); i++) {
            const sgct_core::Viewport& vp = win.getViewport(i);
            if (vp.hasBlackLevelMaskTexture() && vp.isEnabled()) {
                glBindTexture(GL_TEXTURE_2D, vp.getBlackLevelMaskTextureIndex());

                // inverse multiply
                glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
                vp.renderMaskMesh();

                // add
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                vp.renderMaskMesh();
            }
        }
    }
    
    glPopAttrib();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

void Engine::renderViewports(TextureIndexes ti) {
    prepareBuffer(ti);

    Window::StereoMode sm = getCurrentWindow().getStereoMode();
    
    // render all viewports for selected eye
    for (int i = 0; i < getCurrentWindow().getNumberOfViewports(); i++) {
        getCurrentWindow().setCurrentViewport(i);
        mCurrentViewportIndex.main = i;
        sgct_core::Viewport& vp = getCurrentWindow().getViewport(i);

        if (!vp.isEnabled()) {
            continue;
        }

        // if passive stereo or mono
        if (sm == Window::StereoMode::NoStereo) {
            mCurrentFrustumMode = vp.getEye();
        }

        if (vp.hasSubViewports()) {
            if (vp.isTracked()) {
                vp.getNonLinearProjection()->updateFrustums(
                    mCurrentFrustumMode,
                    mNearClippingPlaneDist,
                    mFarClippingPlaneDist
                );
            }

            if (getCurrentWindow().getCallDraw3DFunction()) {
                vp.getNonLinearProjection()->render();
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

            // check if we want to copy the previous window into this one before we go
            // ahead with anyting else
            if (getCurrentWindow().getCopyPreviousWindowToCurrentWindow()) {
                copyPreviousWindowViewportToCurrentWindowViewport(mCurrentFrustumMode);
            }

            if (getCurrentWindow().getCallDraw3DFunction()) {
                mInternalDrawFn();
            }
        }
    }

    // If we did not render anything, make sure we clear the screen at least
    if (!getCurrentWindow().getCallDraw3DFunction() &&
        !getCurrentWindow().getCopyPreviousWindowToCurrentWindow())
    {
        if (Settings::instance()->useFBO()) {
            setAndClearBuffer(BufferMode::RenderToTexture);
        }
        else {
            setAndClearBuffer(BufferMode::BackBuffer);
        }
    }

    if (mFixedOGLPipeline) {
        glPushAttrib(GL_ALL_ATTRIB_BITS);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    // if side-by-side and top-bottom mode only do post fx and blit only after rendered
    // right eye
    bool splitScreenStereo = (sm >= Window::StereoMode::SideBySide);
    if (!(splitScreenStereo && mCurrentFrustumMode == sgct_core::Frustum::StereoLeftEye))
    {
        if (getCurrentWindow().usePostFX()) {
            // blit buffers
            updateRenderingTargets(ti); // only used if multisampled FBOs

            mInternalRenderPostFXFn(ti);

            render2D();
            if (splitScreenStereo) {
                // render left eye info and graph so that all 2D items are rendered after
                // post fx
                mCurrentFrustumMode = sgct_core::Frustum::StereoLeftEye;
                render2D();
            }
        }
        else {
            render2D();
            if (splitScreenStereo) {
                // render left eye info and graph so that all 2D items are rendered after
                // post fx
                mCurrentFrustumMode = sgct_core::Frustum::StereoLeftEye;
                render2D();
            }

            updateRenderingTargets(ti); // only used if multisampled FBOs
        }
    }

    glDisable(GL_BLEND);
    if (mFixedOGLPipeline) {
        glPopAttrib();
    }
}

void Engine::render2D() {
    // draw viewport overlays if any
    mInternalDrawOverlaysFn();

    // draw info & stats
    // the cubemap viewports are all the same so it makes no sense to render everything
    // several times therefore just loop one iteration in that case.
    if (!(mShowGraph || mShowInfo || mDraw2DFnPtr)) {
        return;
    }

    for (int i = 0; i < getCurrentWindow().getNumberOfViewports(); i++) {
        getCurrentWindow().setCurrentViewport(i);
        mCurrentViewportIndex.main = i;
            
        if (!getCurrentWindow().getCurrentViewport()->isEnabled()) {
            continue;
        }
        enterCurrentViewport();

        if (mShowGraph) {
            mStatistics->draw(
                static_cast<float>(getCurrentWindow().getFramebufferResolution().y)
                / static_cast<float>(getCurrentWindow().getResolution().y)
            );
        }
        // The text renderer enters automatically the correct viewport
        if (mShowInfo) {
            // choose specified eye from config
            if (getCurrentWindow().getStereoMode() == Window::StereoMode::NoStereo) {
                mCurrentFrustumMode = getCurrentWindow().getCurrentViewport()->getEye();
            }
            renderDisplayInfo();
        }

        // Check if we should call the use defined draw2D function
        if (mDraw2DFnPtr && getCurrentWindow().getCallDraw2DFunction()) {
            mDraw2DFnPtr();
        }
    }
}

void Engine::renderPostFX(TextureIndexes finalTargetIndex) {
    glDrawBuffer(GL_COLOR_ATTACHMENT0);


    size_t numberOfPasses = getCurrentWindow().getNumberOfPostFXs();
    for (size_t i = 0; i < numberOfPasses; i++) {
        PostFX& fx = getCurrentWindow().getPostFX(i);

        // set output
        if (i == (numberOfPasses - 1) && !getCurrentWindow().useFXAA()) {
            // if last
            fx.setOutputTexture(
                getCurrentWindow().getFrameBufferTexture(finalTargetIndex)
            );
        }
        else {
            // ping pong between the two FX buffers
            fx.setOutputTexture(
                getCurrentWindow().getFrameBufferTexture((i % 2 == 0) ? FX1 : FX2)
            ); 
        }

        // set input (dependent on output)
        if (i == 0) {
            fx.setInputTexture(getCurrentWindow().getFrameBufferTexture(Intermediate));
        }
        else {
            PostFX& fxPrevious = getCurrentWindow().getPostFX(i - 1);
            fx.setInputTexture(fxPrevious.getOutputTexture());
        }

        fx.render();
    }

    if (getCurrentWindow().useFXAA()) {
        PostFX* lastFx = numberOfPasses > 0 ?
            &getCurrentWindow().getPostFX(numberOfPasses - 1) :
            nullptr;

        // bind target FBO
        getCurrentWindow().getFBO()->attachColorTexture(
            getCurrentWindow().getFrameBufferTexture(finalTargetIndex)
        );

        glm::ivec2 framebufferSize = getCurrentWindow().getFramebufferResolution();
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
                getCurrentWindow().getFrameBufferTexture(Intermediate)
            );
        }

        mShader.fxaa.bind();
        glUniform1f(mShaderLoc.sizeX, static_cast<float>(framebufferSize.x));
        glUniform1f(mShaderLoc.sizeY, static_cast<float>(framebufferSize.y));
        glUniform1i(mShaderLoc.fxaaTexture, 0);
        glUniform1f(
            mShaderLoc.fxaaSubPixTrim,
            Settings::instance()->getFXAASubPixTrim()
        );
        glUniform1f(
            mShaderLoc.fxaaSubPixOffset,
            Settings::instance()->getFXAASubPixOffset()
        );

        getCurrentWindow().bindVAO();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        getCurrentWindow().unbindVAO();

        //unbind FXAA
        ShaderProgram::unbind();
    }
}

void Engine::renderPostFXFixedPipeline(TextureIndexes finalTargetIndex) {
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    size_t numberOfPasses = getCurrentWindow().getNumberOfPostFXs();
    if (numberOfPasses > 0) {
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_CULL_FACE);
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
    }

    for (size_t i = 0; i < numberOfPasses; i++) {
        PostFX& fx = getCurrentWindow().getPostFX(i);

        // set output
        if (i == (numberOfPasses - 1) && !getCurrentWindow().useFXAA()) {
            // if last
            fx.setOutputTexture(
                getCurrentWindow().getFrameBufferTexture(finalTargetIndex)
            );
        }
        else {
            // ping pong between the two FX buffers
            fx.setOutputTexture(
                getCurrentWindow().getFrameBufferTexture((i % 2 == 0) ? FX1 : FX2)
            );
        }

        // set input (dependent on output)
        if (i == 0) {
            fx.setInputTexture(getCurrentWindow().getFrameBufferTexture(Intermediate));
        }
        else {
            PostFX& fxPrevious = getCurrentWindow().getPostFX(i - 1);
            fx.setInputTexture(fxPrevious.getOutputTexture());
        }

        fx.render();
    }

    if (numberOfPasses > 0) {
        glPopAttrib();
    }

    if (getCurrentWindow().useFXAA()) {
        PostFX* lastFx = numberOfPasses > 0 ?
            &getCurrentWindow().getPostFX(numberOfPasses - 1) :
            nullptr;

        // bind target FBO
        getCurrentWindow().getFBO()->attachColorTexture(
            getCurrentWindow().getFrameBufferTexture(finalTargetIndex)
        );

        // if for some reson the active texture has been reset
        glActiveTexture(GL_TEXTURE0);
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();

        glMatrixMode(GL_MODELVIEW);
        glm::ivec2 framebufferSize = getCurrentWindow().getFramebufferResolution();
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
                getCurrentWindow().getFrameBufferTexture(Intermediate)
            );
        }

        glDisable(GL_CULL_FACE);
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);

        mShader.fxaa.bind();
        glUniform1f(mShaderLoc.sizeX, static_cast<float>(framebufferSize.x));
        glUniform1f(mShaderLoc.sizeY, static_cast<float>(framebufferSize.y));
        glUniform1i(mShaderLoc.fxaaTexture, 0);
        glUniform1f(
            mShaderLoc.fxaaSubPixTrim,
            Settings::instance()->getFXAASubPixTrim()
        );
        glUniform1f(
            mShaderLoc.fxaaSubPixOffset,
            Settings::instance()->getFXAASubPixOffset()
        );

        glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

        getCurrentWindow().bindVBO();
        glClientActiveTexture(GL_TEXTURE0);

        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(0));

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(8));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        getCurrentWindow().unbindVBO();

        ShaderProgram::unbind();

        glPopClientAttrib();
        glPopAttrib();
    }
}

void Engine::updateRenderingTargets(TextureIndexes ti) {
    // copy AA-buffer to "regular"/non-AA buffer
    sgct_core::OffScreenBuffer* fbo = getCurrentWindow().getFBO();
    if (!fbo->isMultiSampled()) {
        return;
    }

    if (getCurrentWindow().usePostFX()) {
        ti = Intermediate;
    }

    // bind separate read and draw buffers to prepare blit operation
    fbo->bindBlit();

    // update attachments
    fbo->attachColorTexture(getCurrentWindow().getFrameBufferTexture(ti));

    if (Settings::instance()->useDepthTexture()) {
        fbo->attachDepthTexture(getCurrentWindow().getFrameBufferTexture(Depth));
    }

    if (Settings::instance()->useNormalTexture()) {
        fbo->attachColorTexture(
            getCurrentWindow().getFrameBufferTexture(Normals),
            GL_COLOR_ATTACHMENT1
        );
    }

    if (Settings::instance()->usePositionTexture()) {
        fbo->attachColorTexture(
            getCurrentWindow().getFrameBufferTexture(Positions),
            GL_COLOR_ATTACHMENT2
        );
    }

    fbo->blit();
}

void Engine::updateTimers(double timeStamp) {
    // check all timers if one of them has expired
    if (isMaster()) {
        for (TimerInformation& timer : mTimers) {
            const double timeSinceLastFiring = timeStamp - timer.mLastFired;
            if (timeSinceLastFiring > timer.mInterval) {
                timer.mLastFired = timeStamp;
                timer.mCallback(timer.mId);
            }
        }
    }
}

void Engine::loadShaders() {
    mShader.fxaa.setName("FXAAShader");
    std::string fxaaVertShader;
    std::string fxaaFragShader;
    
    if (mFixedOGLPipeline) {
        fxaaVertShader = sgct_core::shaders::FXAAVert;
        fxaaFragShader = sgct_core::shaders::FXAAFrag;
    }
    else {
        fxaaVertShader = sgct_core::shaders_modern::FXAAVert;
        fxaaFragShader = sgct_core::shaders_modern::FXAAFrag;
    }

    sgct_helpers::findAndReplace(fxaaVertShader, "**glsl_version**", getGLSLVersion());
    sgct_helpers::findAndReplace(fxaaFragShader, "**glsl_version**", getGLSLVersion());

    bool fxaaVertSuccess = mShader.fxaa.addShaderSrc(
        fxaaVertShader,
        GL_VERTEX_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!fxaaVertSuccess) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Failed to load FXAA vertex shader\n"
        );
    }

    bool fxaaFragSuccess = mShader.fxaa.addShaderSrc(
        fxaaFragShader,
        GL_FRAGMENT_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!fxaaFragSuccess) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Failed to load FXAA fragment shader\n"
        );
    }
    mShader.fxaa.createAndLinkProgram();
    mShader.fxaa.bind();

    mShaderLoc.sizeX = mShader.fxaa.getUniformLocation("rt_w");
    glm::ivec2 framebufferSize = getCurrentWindow().getFramebufferResolution();
    glUniform1f(mShaderLoc.sizeX, static_cast<float>(framebufferSize.x));

    mShaderLoc.sizeY = mShader.fxaa.getUniformLocation("rt_h");
    glUniform1f(mShaderLoc.sizeY, static_cast<float>(framebufferSize.y));

    mShaderLoc.fxaaSubPixTrim = mShader.fxaa.getUniformLocation("FXAA_SUBPIX_TRIM");
    glUniform1f(mShaderLoc.fxaaSubPixTrim, Settings::instance()->getFXAASubPixTrim());

    mShaderLoc.fxaaSubPixOffset = mShader.fxaa.getUniformLocation("FXAA_SUBPIX_OFFSET");
    glUniform1f(mShaderLoc.fxaaSubPixOffset, Settings::instance()->getFXAASubPixOffset());

    mShaderLoc.fxaaTexture = mShader.fxaa.getUniformLocation("tex");
    glUniform1i(mShaderLoc.fxaaTexture, 0);

    ShaderProgram::unbind();

    // Used for overlays & mono.
    if (!mFixedOGLPipeline) {
        std::string fboQuadVertShader;
        std::string fboQuadFragShader;
        fboQuadVertShader = sgct_core::shaders_modern::BaseVert;
        fboQuadFragShader = sgct_core::shaders_modern::BaseFrag;
        
        const std::string glslVersion = getGLSLVersion();

        sgct_helpers::findAndReplace(fboQuadVertShader, "**glsl_version**", glslVersion);
        sgct_helpers::findAndReplace(fboQuadFragShader, "**glsl_version**", glslVersion);
        
        mShader.fboQuad.setName("FBOQuadShader");
        bool quadVertSuccess = mShader.fboQuad.addShaderSrc(
            fboQuadVertShader,
            GL_VERTEX_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!quadVertSuccess) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Failed to load FBO quad vertex shader\n"
            );
        }
        bool quadFragSuccess = mShader.fboQuad.addShaderSrc(
            fboQuadFragShader,
            GL_FRAGMENT_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!quadFragSuccess) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Failed to load FBO quad fragment shader\n"
            );
        }
        mShader.fboQuad.createAndLinkProgram();
        mShader.fboQuad.bind();
        mShaderLoc.monoTex = mShader.fboQuad.getUniformLocation("Tex");
        glUniform1i(mShaderLoc.monoTex, 0);
        ShaderProgram::unbind();
        
        std::string overlayVertShader;
        std::string overlayFragShader;
        overlayVertShader = sgct_core::shaders_modern::OverlayVert;
        overlayFragShader = sgct_core::shaders_modern::OverlayFrag;
        
        //replace glsl version
        sgct_helpers::findAndReplace(
            overlayVertShader,
            "**glsl_version**",
            getGLSLVersion()
        );
        sgct_helpers::findAndReplace(
            overlayFragShader,
            "**glsl_version**",
            getGLSLVersion()
        );
        
        mShader.overlay.setName("OverlayShader");
        bool overlayVertSuccess = mShader.overlay.addShaderSrc(
            overlayVertShader,
            GL_VERTEX_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!overlayVertSuccess) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Failed to load overlay vertex shader\n"
            );
        }
        bool overlayFragSuccess = mShader.overlay.addShaderSrc(
            overlayFragShader,
            GL_FRAGMENT_SHADER,
            ShaderProgram::ShaderSourceType::String
        );
        if (!overlayFragSuccess) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Failed to load overlay fragment shader\n"
            );
        }
        mShader.overlay.createAndLinkProgram();
        mShader.overlay.bind();
        mShaderLoc.overlayTex = mShader.overlay.getUniformLocation("Tex");
        glUniform1i(mShaderLoc.overlayTex, 0);
        ShaderProgram::unbind();
    }
}

void Engine::setAndClearBuffer(BufferMode mode) {
    if (mode < BufferMode::RenderToTexture) {
        const bool doubleBuffered = getCurrentWindow().isDoubleBuffered();
        // Set buffer
        if (getCurrentWindow().getStereoMode() != Window::StereoMode::Active) {
            glDrawBuffer(doubleBuffered ? GL_BACK : GL_FRONT);
            glReadBuffer(doubleBuffered ? GL_BACK : GL_FRONT);
        }
        else if (mCurrentFrustumMode == sgct_core::Frustum::StereoLeftEye) {
            // if active left
            glDrawBuffer(doubleBuffered ? GL_BACK_LEFT : GL_FRONT_LEFT);
            glReadBuffer(doubleBuffered ? GL_BACK_LEFT : GL_FRONT_LEFT);
        }
        else if (mCurrentFrustumMode == sgct_core::Frustum::StereoRightEye) {
            // if active right
            glDrawBuffer(doubleBuffered ? GL_BACK_RIGHT : GL_FRONT_RIGHT);
            glReadBuffer(doubleBuffered ? GL_BACK_RIGHT : GL_FRONT_RIGHT);
        }
    }

    // clear
    if (mode != BufferMode::BackBufferBlack && mClearBufferFnPtr) {
        mClearBufferFnPtr();
    }
    else {
        //when rendering textures to backbuffer (using fbo)
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
}

bool Engine::checkForOGLErrors() {
    using Level = MessageHandler::Level;
    MessageHandler& mh = *MessageHandler::instance();

    const GLenum oglError = glGetError();
    switch (oglError) {
        case GL_INVALID_ENUM:
            mh.print(Level::Error, "OpenGL error: GL_INVALID_ENUM\n");
            break;
        case GL_INVALID_VALUE:
            mh.print(Level::Error, "OpenGL error: GL_INVALID_VALUE\n");
            break;
        case GL_INVALID_OPERATION:
            mh.print(Level::Error, "OpenGL error: GL_INVALID_OPERATION\n");
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            mh.print(Level::Error, "OpenGL error: GL_INVALID_FRAMEBUFFER_OPERATION\n");
            break;
        case GL_STACK_OVERFLOW:
            mh.print(Level::Error, "OpenGL error: GL_STACK_OVERFLOW\n");
            break;
        case GL_STACK_UNDERFLOW:
            mh.print(Level::Error, "OpenGL error: GL_STACK_UNDERFLOW\n");
            break;
        case GL_OUT_OF_MEMORY:
            mh.print(Level::Error, "OpenGL error: GL_OUT_OF_MEMORY\n");
            break;
        case GL_TABLE_TOO_LARGE:
            mh.print(Level::Error, "OpenGL error: GL_TABLE_TOO_LARGE\n");
            break;
    }

    return oglError == GL_NO_ERROR;
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

sgct_core::Frustum::Mode Engine::getCurrentFrustumMode() const {
    return mCurrentFrustumMode;
}

const glm::mat4& Engine::getCurrentProjectionMatrix() const {
    const sgct_core::BaseViewport& vp = *getCurrentWindow().getCurrentViewport();
    return vp.getProjection(mCurrentFrustumMode).getProjectionMatrix();
}

const glm::mat4& Engine::getCurrentViewMatrix() const {
    const sgct_core::BaseViewport& vp = *getCurrentWindow().getCurrentViewport();
    return vp.getProjection(mCurrentFrustumMode).getViewMatrix();
}

const glm::mat4& Engine::getModelMatrix() const {
    return sgct_core::ClusterManager::instance()->getSceneTransform();
}

const glm::mat4& Engine::getCurrentViewProjectionMatrix() const {
    const sgct_core::BaseViewport& vp = *getCurrentWindow().getCurrentViewport();
    return vp.getProjection(mCurrentFrustumMode).getViewProjectionMatrix();
}

glm::mat4 Engine::getCurrentModelViewProjectionMatrix() const {
    const sgct_core::BaseViewport& vp = *getCurrentWindow().getCurrentViewport();
    return vp.getProjection(mCurrentFrustumMode).getViewProjectionMatrix() * 
           sgct_core::ClusterManager::instance()->getSceneTransform();
}

glm::mat4 Engine::getCurrentModelViewMatrix() const {
    const sgct_core::BaseViewport& vp = *getCurrentWindow().getCurrentViewport();
    return vp.getProjection(mCurrentFrustumMode).getViewMatrix() *
           sgct_core::ClusterManager::instance()->getSceneTransform();
}

unsigned int Engine::getCurrentFrameNumber() const {
    return mFrameCounter;
}

bool Engine::isOGLPipelineFixed() const {
    return mFixedOGLPipeline;
}

bool Engine::isOpenGLCompatibilityMode() const {
    return mRunMode <= RunMode::OpenGL_Compatibility_Profile;
}

const std::string& Engine::getGLSLVersion() const {
    return mGLSLVersion;
}

void Engine::waitForAllWindowsInSwapGroupToOpen() {
    // clear the buffers initially
    for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        mThisNode->getWindow(i).makeOpenGLContextCurrent(Window::Context::Window);
        glDrawBuffer(getCurrentWindow().isDoubleBuffered() ? GL_BACK : GL_FRONT);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (mThisNode->getWindow(i).isDoubleBuffered()) {
            glfwSwapBuffers(mThisNode->getWindow(i).getWindowHandle());
        }
        else {
            glFinish();
        }
    }
    glfwPollEvents();
    
    // Must wait until all nodes are running if using swap barrier
    if (!sgct_core::ClusterManager::instance()->getIgnoreSync() &&
        sgct_core::ClusterManager::instance()->getNumberOfNodes() > 1)
    {
        // check if swapgroups are supported
#ifdef __WIN32__
        if (glfwExtensionSupported("WGL_NV_swap_group")) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Swap groups are supported by hardware\n"
            );
        }
        else {
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Swap groups are not supported by hardware\n"
            );
        }
#else
        if (glfwExtensionSupported("GLX_NV_swap_group")) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Swap groups are supported by hardware\n"
            );
        }
        else {
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Swap groups are not supported by hardware\n"
            );
        }
#endif

        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Waiting for all nodes to connect\n"
        );
        MessageHandler::instance()->setShowTime(false);
        
        while (mNetworkConnections->isRunning() && !mThisNode->getKeyPressed(mExitKey) &&
               !mThisNode->shouldAllWindowsClose() && !mTerminate)
        {
            MessageHandler::instance()->print(MessageHandler::Level::Info, ".");

            // Swap front and back rendering buffers
            for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                if (mThisNode->getWindow(i).isDoubleBuffered()) {
                    glfwSwapBuffers(mThisNode->getWindow(i).getWindowHandle());
                }
                else {
                    glFinish();
                }
            }
            glfwPollEvents();

            if (mNetworkConnections->areAllNodesConnected()) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        MessageHandler::instance()->print(MessageHandler::Level::Info, "\n");

        // wait for user to release exit key
        while (mThisNode->getKeyPressed(mExitKey)) {
            // Swap front and back rendering buffers
            // key buffers also swapped
            for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                if (mThisNode->getWindow(i).isDoubleBuffered()) {
                    glfwSwapBuffers(mThisNode->getWindow(i).getWindowHandle());
                }
                else {
                    glFinish();
                }
            }
            glfwPollEvents();
            
            MessageHandler::instance()->print(MessageHandler::Level::Info, ".");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        MessageHandler::instance()->print(MessageHandler::Level::Info, "\n");
        MessageHandler::instance()->setShowTime(true);
    }
}

void Engine::updateFrustums() {
    if (mThisNode == nullptr) {
        return;
    }

    for (int w = 0; w < mThisNode->getNumberOfWindows(); w++) {
        Window& win = mThisNode->getWindow(w);
        for (int i = 0; i < win.getNumberOfViewports(); i++) {
            sgct_core::Viewport& vp = win.getViewport(i);
            if (vp.isTracked()) {
                // if not tracked update, otherwise this is done on the fly
                continue;
            }

            if (vp.hasSubViewports()) {
                sgct_core::NonLinearProjection& proj = *vp.getNonLinearProjection();
                proj.updateFrustums(
                    sgct_core::Frustum::MonoEye,
                    mNearClippingPlaneDist,
                    mFarClippingPlaneDist
                );

                proj.updateFrustums(
                    sgct_core::Frustum::StereoLeftEye,
                    mNearClippingPlaneDist,
                    mFarClippingPlaneDist
                );

                proj.updateFrustums(
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

void Engine::copyPreviousWindowViewportToCurrentWindowViewport(
                                                            sgct_core::Frustum::Mode mode)
{
    // Check that we have a previous window
    if (getCurrentWindowIndex() < 1) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Warning,
            "Could not copy from previous window, as this window is the first one\n"
        );
        return;
    }

    Window& previousWindow = getWindow(getCurrentWindowIndex() - 1);

    // run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);

    enterCurrentViewport();

    if (Settings::instance()->useFBO()) {
        setAndClearBuffer(BufferMode::RenderToTexture);
    }
    else {
        setAndClearBuffer(BufferMode::BackBuffer);
    }

    glDisable(GL_SCISSOR_TEST);
    mShader.overlay.bind();

    glUniform1i(mShaderLoc.overlayTex, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, previousWindow.getFrameBufferTexture(mode));

    getCurrentWindow().bindVAO();

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    getCurrentWindow().unbindVAO();
    ShaderProgram::unbind();
}

void Engine::parseArguments(std::vector<std::string>& arg) {
    MessageHandler::instance()->print(MessageHandler::Level::Info, "Parsing arguments\n");
    size_t i = 0;
    while (i < arg.size()) {
        if (arg[i] == "-config" && arg.size() > (i + 1)) {
            configFilename = arg[i + 1];
            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--client") {
            sgct_core::ClusterManager::instance()->setNetworkMode(
                sgct_core::NetworkManager::NetworkMode::LocalClient
            );
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--slave") {
            sgct_core::ClusterManager::instance()->setNetworkMode(
                sgct_core::NetworkManager::NetworkMode::LocalClient
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
                sgct_core::NetworkManager::NetworkMode::LocalServer
            );
            
            int id = std::stoi(arg[i + 1]);
            sgct_core::ClusterManager::instance()->setThisNodeId(id);
            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-logPath") {
            // Remove unwanted chars
            std::string tmpStr = arg[i+1];
            tmpStr.erase(remove(tmpStr.begin(), tmpStr.end(), '\"'), tmpStr.end());
            size_t lastPos = tmpStr.length() - 1;
            
            const char last = tmpStr.at(lastPos);
            if (last == '\\' || last == '/') {
                tmpStr.erase(lastPos);
            }

            mLogfilePath = tmpStr;

            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-notify" && arg.size() > (i + 1)) {
            int level = std::stoi(arg[i + 1]);
            MessageHandler::instance()->setNotifyLevel(
                static_cast<MessageHandler::Level>(level)
            );
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
            Settings::instance()->setForceGlTexImage2D(true);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--FXAA") {
            Settings::instance()->setDefaultFXAAState(true);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-MSAA" && arg.size() > (i + 1)) {
            int msaa = std::stoi(arg[i + 1]);
            Settings::instance()->setDefaultNumberOfAASamples(msaa);
            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--No-FBO") {
            Settings::instance()->setUseFBO(false);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--Capture-TGA") {
            Settings::instance()->setCaptureFormat(Settings::CaptureFormat::TGA);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--Capture-PNG") {
            Settings::instance()->setCaptureFormat(Settings::CaptureFormat::PNG);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "--Capture-JPG") {
            Settings::instance()->setCaptureFormat(Settings::CaptureFormat::JPG);
            arg.erase(arg.begin() + i);
        }
        else if (arg[i] == "-numberOfCaptureThreads" && arg.size() > (i + 1)) {
            int nThreads = std::stoi(arg[i + 1]);
            if (nThreads > 0) {
                Settings::instance()->setNumberOfCaptureThreads(nThreads);
            }

            arg.erase(arg.begin() + i);
            arg.erase(arg.begin() + i);
        }
        else {
            i++;
        }
    }

    MessageHandler::instance()->print(MessageHandler::Level::Info, "Done\n");
}

void Engine::setDrawFunction(std::function<void()> fn) {
    mDrawFnPtr = std::move(fn);
}

void Engine::setDraw2DFunction(std::function<void()> fn) {
    mDraw2DFnPtr = std::move(fn);
}

void Engine::setPreSyncFunction(std::function<void()> fn) {
    mPreSyncFnPtr = std::move(fn);
}

void Engine::setPostSyncPreDrawFunction(std::function<void()> fn) {
    mPostSyncPreDrawFnPtr = std::move(fn);
}

void Engine::setPostDrawFunction(std::function<void()> fn) {
    mPostDrawFnPtr = std::move(fn);
}

void Engine::setInitOGLFunction(std::function<void()> fn) {
    mInitOGLFnPtr = std::move(fn);
}

void Engine::setPreWindowFunction(std::function<void()> fn) {
    mPreWindowFnPtr = std::move(fn);
}

void Engine::setClearBufferFunction(std::function<void()> fn) {
    mClearBufferFnPtr = std::move(fn);
}

void Engine::setCleanUpFunction(std::function<void()> fn) {
    mCleanUpFnPtr = std::move(fn);
}

void Engine::setExternalControlCallback(std::function<void(const char*, int)> fn) {
    mExternalDecodeCallbackFnPtr = std::move(fn);
}

void Engine::setExternalControlStatusCallback(std::function<void(bool)> fn) {
    mExternalStatusCallbackFnPtr = std::move(fn);
}

void Engine::setDataTransferCallback(std::function<void(void*, int, int, int)> fn) {
    mDataTransferDecodeCallbackFnPtr = std::move(fn);
}

void Engine::setDataTransferStatusCallback(std::function<void(bool, int)> fn) {
    mDataTransferStatusCallbackFnPtr = std::move(fn);
}

void Engine::setDataAcknowledgeCallback(std::function<void(int, int)> fn) {
    mDataTransferAcknowledgeCallbackFnPtr = std::move(fn);
}

void Engine::setContextCreationCallback(std::function<void(GLFWwindow*)> fn) {
    mContextCreationFnPtr = std::move(fn);
}

void Engine::setScreenShotCallback(std::function<void(sgct_core::Image*, size_t,
                               sgct_core::ScreenCapture::EyeIndex, unsigned int type)> fn)
{
    mScreenShotFnPtr = std::move(fn);
}

void Engine::setKeyboardCallbackFunction(std::function<void(int, int, int, int)> fn) {
    gKeyboardCallbackFnPtr = std::move(fn);
}

void Engine::setCharCallbackFunction(std::function<void(unsigned int, int)> fn) {
    gCharCallbackFnPtr = std::move(fn);
}

void Engine::setMouseButtonCallbackFunction(std::function<void(int, int, int)> fn) {
    gMouseButtonCallbackFnPtr = std::move(fn);
}

void Engine::setMousePosCallbackFunction(std::function<void(double, double)> fn) {
    gMousePosCallbackFnPtr = std::move(fn);
}

void Engine::setMouseScrollCallbackFunction(std::function<void(double, double)> fn) {
    gMouseScrollCallbackFnPtr = std::move(fn);
}

void Engine::setDropCallbackFunction(std::function<void(int, const char**)> fn) {
    gDropCallbackFnPtr = std::move(fn);
}

 void Engine::setTouchCallbackFunction(std::function<void(const sgct_core::Touch*)> fn) {
     gTouchCallbackFnPtr = std::move(fn);
 }

void sgct::Engine::clearBuffer() {
    glm::vec4 color = Engine::instance()->getClearColor();

    const float alpha = instance()->getCurrentWindow().getAlpha() ? 0.f : color.a;
    glClearColor(color.r, color.g, color.b, alpha);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Engine::printNodeInfo(unsigned int nodeId) {
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "This node has index %d.\n", nodeId
    );
}

void Engine::enterCurrentViewport() {
    sgct_core::BaseViewport* vp = getCurrentWindow().getCurrentViewport();
    
    glm::vec2 res = glm::vec2(getCurrentWindow().getFramebufferResolution());
        
    glm::vec2 p = vp->getPosition() * res;
    glm::vec2 s = vp->getSize() * res;
    mCurrentViewportCoords = glm::ivec4(glm::ivec2(p), glm::ivec2(s));

    Window::StereoMode sm = getCurrentWindow().getStereoMode();
    if (sm >= Window::StereoMode::SideBySide) {
        if (mCurrentFrustumMode == sgct_core::Frustum::StereoLeftEye) {
            switch (sm) {
                case Window::StereoMode::SideBySide:
                    mCurrentViewportCoords.x /= 2;
                    mCurrentViewportCoords.z /= 2;
                    break;
                case Window::StereoMode::SideBySideInverted:
                    mCurrentViewportCoords.x =
                        (mCurrentViewportCoords.x / 2) + (mCurrentViewportCoords.z / 2);
                    mCurrentViewportCoords.z = mCurrentViewportCoords.z / 2;
                    break;
                case Window::StereoMode::TopBottom:
                    mCurrentViewportCoords.y =
                        (mCurrentViewportCoords.y / 2) + (mCurrentViewportCoords.w / 2);
                    mCurrentViewportCoords.w /= 2;
                    break;
                case Window::StereoMode::TopBottomInverted:
                    mCurrentViewportCoords.y /= 2;
                    mCurrentViewportCoords.w /= 2;
                    break;
                default:
                    break;
            }
        }
        else {
            switch (sm) {
                case Window::StereoMode::SideBySide:
                    mCurrentViewportCoords.x =
                        (mCurrentViewportCoords.x / 2) + (mCurrentViewportCoords.z / 2);
                    mCurrentViewportCoords.z /= 2;
                    break;
                case Window::StereoMode::SideBySideInverted:
                    mCurrentViewportCoords.x /= 2;
                    mCurrentViewportCoords.z /= 2;
                    break;
                case Window::StereoMode::TopBottom:
                    mCurrentViewportCoords.y /= 2;
                    mCurrentViewportCoords.w /= 2;
                    break;
                case Window::StereoMode::TopBottomInverted:
                    mCurrentViewportCoords.y =
                        (mCurrentViewportCoords.y / 2) + (mCurrentViewportCoords.w / 2);
                    mCurrentViewportCoords.w /= 2;
                    break;
                default:
                    break;
            }
        }
    }

    glViewport(
        mCurrentViewportCoords.x,
        mCurrentViewportCoords.y,
        mCurrentViewportCoords.z,
        mCurrentViewportCoords.w
    );
    
    glScissor(
        mCurrentViewportCoords.x,
        mCurrentViewportCoords.y,
        mCurrentViewportCoords.z,
        mCurrentViewportCoords.w
    );
}

void Engine::calculateFPS(double timestamp) {
    // @TODO (abock, 2019-08-29): I don't know why these variables are specified as static
    // but I guess they don't need to be

    // @TODO (abock, 2019-08-29): Also; i don't know why updateAAInfo should be called
    // once per second.  It would be better to listen to something that can signal when it
    // needs to be called instead.
    static double lastTimestamp = glfwGetTime();
    mStatistics->setFrameTime(static_cast<float>(timestamp - lastTimestamp));
    lastTimestamp = timestamp;
    static float renderedFrames = 0.f;
    static float tmpTime = 0.f;
    renderedFrames += 1.f;
    tmpTime += mStatistics->getFrameTime();
    if (tmpTime >= 1.f) {
        mStatistics->setAvgFPS(renderedFrames / tmpTime);
        renderedFrames = 0.f;
        tmpTime = 0.f;

        for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
            if (mThisNode->getWindow(i).isVisible()) {
                updateAAInfo(mThisNode->getWindow(i));
            }
        }
    }
}

double Engine::getDt() const {
    return mStatistics->getFrameTime();
}

double Engine::getAvgFPS() const {
    return mStatistics->getAvgFPS();
}

double Engine::getAvgDt() const {
    return mStatistics->getAvgFrameTime();
}

double Engine::getMinDt() const {
    return mStatistics->getMinFrameTime();
}

double Engine::getMaxDt() const {
    return mStatistics->getMaxFrameTime();
}

double Engine::getDtStandardDeviation() const {
    return mStatistics->getFrameTimeStandardDeviation();
}

double Engine::getDrawTime() const {
    return mStatistics->getDrawTime();
}

double Engine::getSyncTime() const {
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

void Engine::setNearAndFarClippingPlanes(float nearClip, float farClip) {
    mNearClippingPlaneDist = nearClip;
    mFarClippingPlaneDist = farClip;
    updateFrustums();
}

void Engine::setEyeSeparation(float eyeSeparation) {
    for (int w = 0; w < mThisNode->getNumberOfWindows(); w++) {
        Window& window = mThisNode->getWindow(w);

        for (int i = 0; i < window.getNumberOfViewports(); i++) {
            window.getViewport(i).getUser().setEyeSeparation(eyeSeparation);
        }
    }
    updateFrustums();
}

void Engine::setClearColor(glm::vec4 color) {
    mClearColor = std::move(color);
}

void Engine::setExitKey(int key) {
    mExitKey = key;
}

void Engine::addPostFX(PostFX fx) {
    for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        mThisNode->getWindow(i).setUsePostFX(true);
        mThisNode->getWindow(i).addPostFX(std::move(fx));
    }
}

unsigned int Engine::getCurrentDrawTexture() const {
    if (getCurrentWindow().usePostFX()) {
        return getCurrentWindow().getFrameBufferTexture(Intermediate);
    }
    else {
        return getCurrentWindow().getFrameBufferTexture(
            mCurrentFrustumMode == sgct_core::Frustum::StereoRightEye ?
                RightEye :
                LeftEye
        );
    }
}

unsigned int Engine::getCurrentDepthTexture() const {
    return getCurrentWindow().getFrameBufferTexture(Depth);
}

unsigned int Engine::getCurrentNormalTexture() const {
    return getCurrentWindow().getFrameBufferTexture(Normals);
}

unsigned int Engine::getCurrentPositionTexture() const {
    return getCurrentWindow().getFrameBufferTexture(Positions);
}

glm::ivec2 Engine::getCurrentResolution() const {
    return getCurrentWindow().getFramebufferResolution();
}

size_t Engine::getFocusedWindowIndex() const {
    for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        if (mThisNode->getWindow(i).isFocused()) {
            return i;
        }
    }
    return 0; // no window has focus
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

void Engine::invokeDecodeCallbackForExternalControl(const char* receivedData,
                                                    int receivedLength, int)
{
    if (mExternalDecodeCallbackFnPtr && receivedLength > 0) {
        mExternalDecodeCallbackFnPtr(receivedData, receivedLength);
    }
}

void Engine::invokeUpdateCallbackForExternalControl(bool connected) {
    if (mExternalStatusCallbackFnPtr) {
        mExternalStatusCallbackFnPtr(connected);
    }
}

void Engine::invokeDecodeCallbackForDataTransfer(void* receivedData, int length,
                                                 int packageId, int clientId)
{
    if (mDataTransferDecodeCallbackFnPtr && length > 0) {
        mDataTransferDecodeCallbackFnPtr(receivedData, length, packageId, clientId);
    }
}

void Engine::invokeUpdateCallbackForDataTransfer(bool connected, int clientId) {
    if (mDataTransferStatusCallbackFnPtr) {
        mDataTransferStatusCallbackFnPtr(connected, clientId);
    }
}

void Engine::invokeAcknowledgeCallbackForDataTransfer(int packageId, int clientId) {
    if (mDataTransferAcknowledgeCallbackFnPtr) {
        mDataTransferAcknowledgeCallbackFnPtr(packageId, clientId);
    }
}

void Engine::invokeScreenShotCallback(sgct_core::Image* imPtr, size_t winIndex,
                                       sgct_core::ScreenCapture::EyeIndex ei,
                                       unsigned int type)
{
    if (mScreenShotFnPtr) {
        mScreenShotFnPtr(imPtr, winIndex, ei, type);
    }
}

void Engine::sendMessageToExternalControl(const void* data, int length) {
    if (mNetworkConnections->getExternalControlPtr()) {
        mNetworkConnections->getExternalControlPtr()->sendData(data, length);
    }
}

void Engine::setDataTransferCompression(bool state, int level) {
    mNetworkConnections->setDataTransferCompression(state, level);
}

void Engine::transferDataBetweenNodes(const void* data, int length, int packageId) {
    mNetworkConnections->transferData(data, length, packageId);
}

void Engine::transferDataToNode(const void* data, int length, int packageId,
                                size_t nodeIndex)
{
    mNetworkConnections->transferData(data, length, packageId, nodeIndex);
}

void Engine::sendMessageToExternalControl(const std::string& msg) {
    if (mNetworkConnections->getExternalControlPtr()) {
        const int size = static_cast<int>(msg.size());
        mNetworkConnections->getExternalControlPtr()->sendData(msg.c_str(), size);
    }
}

bool Engine::isExternalControlConnected() const {
    return (mNetworkConnections->getExternalControlPtr() &&
            mNetworkConnections->getExternalControlPtr()->isConnected());
}

void Engine::setExternalControlBufferSize(unsigned int newSize) {
    if (mNetworkConnections->getExternalControlPtr()) {
        mNetworkConnections->getExternalControlPtr()->setBufferSize(newSize);
    }
}

void Engine::updateAAInfo(const Window& window) {
    if (window.useFXAA()) {
        if (window.getNumberOfAASamples() > 1) {
            mAAInfo = "FXAA+MSAAx" + std::to_string(window.getNumberOfAASamples());
        }
        else {
            mAAInfo = "FXAA";
        }
    }
    else {
        // no FXAA
        if (window.getNumberOfAASamples() > 1) {
            mAAInfo = "MSAAx" + std::to_string(window.getNumberOfAASamples());
        }
        else {
            mAAInfo = "none";
        }
    }
}

void Engine::updateDrawBufferResolutions() {
    mDrawBufferResolutions.clear();

    for (int i = 0; i < mThisNode->getNumberOfWindows(); i++) {
        Window& win = getWindow(i);
        
        // first add cubemap resolutions if any
        for (int j = 0; j < win.getNumberOfViewports(); j++) {
            const sgct_core::Viewport& vp = win.getViewport(j);
            if (vp.hasSubViewports()) {
                int cubeRes = vp.getNonLinearProjection()->getCubemapResolution();
                mDrawBufferResolutions.push_back(glm::ivec2(cubeRes, cubeRes));
            }
        }

        // second add window resolution
        glm::ivec2 size = win.getFinalFBODimensions();
        mDrawBufferResolutions.push_back(size);
    }
}

int Engine::getKey(int winIndex, int key) {
    return glfwGetKey(mInstance->getWindow(winIndex).getWindowHandle(), key);
}

int Engine::getMouseButton(int winIndex, int button) {
    return glfwGetMouseButton(
        mInstance->getWindow(winIndex).getWindowHandle(),
        button
    );
}

void Engine::getMousePos(int winIndex, double* xPos, double* yPos) {
    glfwGetCursorPos(mInstance->getWindow(winIndex).getWindowHandle(), xPos, yPos);
}

void Engine::setMousePos(int winIndex, double xPos, double yPos) {
    glfwSetCursorPos(mInstance->getWindow(winIndex).getWindowHandle(), xPos, yPos);
}

void Engine::setMouseCursorVisibility(int winIndex, bool state) {
    GLFWwindow* win = mInstance->getWindow(winIndex).getWindowHandle();
    glfwSetInputMode(win, GLFW_CURSOR, state ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
}

const char* Engine::getJoystickName(int joystick) {
    return glfwGetJoystickName(joystick);
}

const float* Engine::getJoystickAxes(int joystick, int* numOfValues) {
    return glfwGetJoystickAxes(joystick, numOfValues);
}

const unsigned char* Engine::getJoystickButtons(int joystick, int* numOfValues) {
    return glfwGetJoystickButtons(joystick, numOfValues);
}

const sgct_core::Node* Engine::getThisNode() const {
    return mThisNode;
}

Window& Engine::getWindow(int index) const {
    return mThisNode->getWindow(index);
}

size_t Engine::getNumberOfWindows() const {
    return mThisNode->getNumberOfWindows();
}

Window& Engine::getCurrentWindow() const {
    return mThisNode->getCurrentWindow();
}

int Engine::getCurrentWindowIndex() const {
    return mThisNode->getCurrentWindowIndex();
}

sgct_core::User& Engine::getDefaultUser() {
    return sgct_core::ClusterManager::instance()->getDefaultUser();
}

TrackingManager& Engine::getTrackingManager() {
    return sgct_core::ClusterManager::instance()->getTrackingManager();
}

int Engine::createTimer(double millisec, std::function<void(int)> fn) {
    if (isMaster()) {
        // construct the timer object
        TimerInformation timer;
        timer.mCallback = std::move(fn);
        // we want to present timers in millisec, but glfwGetTime uses seconds
        timer.mInterval = millisec / 1000.0; 
        timer.mId = mTimerID++;  // use and post-increase
        timer.mLastFired = getTime();
        mTimers.push_back(timer);
        return timer.mId;
    }
    else {
        return std::numeric_limits<int>::max();
    }
}

void Engine::stopTimer(int id) {
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

double Engine::getTime() {
    return glfwGetTime();
}

glm::ivec2 Engine::getCurrentViewportSize() const {
    return { mCurrentViewportCoords.z, mCurrentViewportCoords.w };
}

glm::ivec2 Engine::getCurrentDrawBufferSize() const {
    return mDrawBufferResolutions[mCurrentDrawBufferIndex];
}

glm::ivec2 Engine::getDrawBufferSize(size_t index) const {
    if (index < mDrawBufferResolutions.size()) {
        return mDrawBufferResolutions[index];
    }
    else {
        return glm::ivec2(0, 0);
    }
}

size_t Engine::getNumberOfDrawBuffers() const {
    return mDrawBufferResolutions.size();
}

size_t Engine::getCurrentDrawBufferIndex() const {
    return mCurrentDrawBufferIndex;
}

Engine::RenderTarget Engine::getCurrentRenderTarget() const {
    return mCurrentRenderTarget;
}

sgct_core::OffScreenBuffer* Engine::getCurrentFBO() const {
    // @TODO (abock, 2019-09-18); Potentially unused; also affects 'getOffScreenBuffer' in
    // NonLinearProjection
    return mCurrentOffScreenBuffer;
}

glm::ivec4 Engine::getCurrentViewportPixelCoords() const {
    const sgct_core::Viewport& vp = getCurrentWindow().getViewport(
        mCurrentViewportIndex.main
    );
    if (vp.hasSubViewports()) {
        return vp.getNonLinearProjection()->getViewportCoords();
    }
    else {
        return mCurrentViewportCoords;
    }
}

bool Engine::getWireframe() const {
    return mShowWireframe;
}

void Engine::setSyncParameters(bool printMessage, float timeout) {
    mPrintSyncMessage = printMessage;
    mSyncTimeout = timeout;
}

void Engine::setScreenShotNumber(unsigned int number) {
    mShotCounter = number;
}

unsigned int Engine::getScreenShotNumber() const {
    return mShotCounter;
}

} // namespace sgct

