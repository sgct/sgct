/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/engine.h>

#include <sgct/clustermanager.h>
#include <sgct/commandline.h>
#include <sgct/error.h>
#include <sgct/font.h>
#include <sgct/fontmanager.h>
#include <sgct/freetype.h>
#include <sgct/nonlinearprojection.h>
#include <sgct/readconfig.h>
#include <sgct/shareddata.h>
#include <sgct/statisticsrenderer.h>
#include <sgct/texturemanager.h>
#include <sgct/trackingmanager.h>
#include <sgct/user.h>
#include <sgct/version.h>
#include <sgct/shaders/internalshaders.h>
#include <iostream>
#include <numeric>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define Err(code, msg) Error(Error::Component::Engine, code, msg)

namespace sgct {

namespace {
    constexpr const bool RunFrameLockCheckThread = true;
    constexpr const std::chrono::milliseconds FrameLockTimeout(100);

    constexpr const float FxaaSubPixTrim = 1.f / 4.f;
    constexpr const float FxaaSubPixOffset = 1.f / 2.f;

    enum class BufferMode { BackBufferBlack, RenderToTexture };

    bool sRunUpdateFrameLockLoop = true;

    // Callback wrappers for GLFW
    std::function<void(Key, Modifier, Action, int)> gKeyboardCallback = nullptr;
    std::function<void(unsigned int, int)> gCharCallback = nullptr;
    std::function<void(MouseButton, Modifier, Action)> gMouseButtonCallback = nullptr;
    std::function<void(double, double)> gMousePosCallback = nullptr;
    std::function<void(double, double)> gMouseScrollCallback = nullptr;
    std::function<void(int, const char**)> gDropCallback = nullptr;

    // For feedback: breaks a frame lock wait condition every time interval
    // (FrameLockTimeout) in order to print waiting message.
    void updateFrameLockLoop(void*) {
        bool run = true;

        while (run) {
            core::mutex::FrameSync.lock();
            run = sRunUpdateFrameLockLoop;
            core::mutex::FrameSync.unlock();
            core::NetworkManager::cond.notify_all();
            std::this_thread::sleep_for(FrameLockTimeout);
        }
    }

    std::string getStereoString(Window::StereoMode stereoMode) {
        switch (stereoMode) {
            case Window::StereoMode::Active: return "active";
            case Window::StereoMode::AnaglyphRedCyan: return "anaglyph_red_cyan";
            case Window::StereoMode::AnaglyphAmberBlue: return "anaglyph_amber_blue";
            case Window::StereoMode::AnaglyphRedCyanWimmer: return "anaglyph_wimmer";
            case Window::StereoMode::Checkerboard: return "checkerboard";
            case Window::StereoMode::CheckerboardInverted: return "checkerboard_inverted";
            case Window::StereoMode::VerticalInterlaced: return "vertical_interlaced";
            case Window::StereoMode::VerticalInterlacedInverted:
                return "vertical_interlaced_inverted";
            case Window::StereoMode::Dummy: return "dummy";
            case Window::StereoMode::SideBySide: return "side_by_side";
            case Window::StereoMode::SideBySideInverted: return "side_by_side_inverted";
            case Window::StereoMode::TopBottom: return "top_bottom";
            case Window::StereoMode::TopBottomInverted: return "top_bottom_inverted";
            default:
                return "none";
        }
    }

    void addValue(std::array<double, Engine::Statistics::HistoryLength>& a, double v) {
        std::rotate(std::rbegin(a), std::rbegin(a) + 1, std::rend(a));
        a[0] = v;
    }

    void setAndClearBuffer(Window& window, BufferMode buffer, Frustum::Mode frustum) {
        if (buffer == BufferMode::BackBufferBlack) {
            const bool doubleBuffered = window.isDoubleBuffered();
            // Set buffer
            if (window.getStereoMode() != Window::StereoMode::Active) {
                glDrawBuffer(doubleBuffered ? GL_BACK : GL_FRONT);
                glReadBuffer(doubleBuffered ? GL_BACK : GL_FRONT);
            }
            else if (frustum == Frustum::Mode::StereoLeftEye) {
                // if active left
                glDrawBuffer(doubleBuffered ? GL_BACK_LEFT : GL_FRONT_LEFT);
                glReadBuffer(doubleBuffered ? GL_BACK_LEFT : GL_FRONT_LEFT);
            }
            else if (frustum == Frustum::Mode::StereoRightEye) {
                // if active right
                glDrawBuffer(doubleBuffered ? GL_BACK_RIGHT : GL_FRONT_RIGHT);
                glReadBuffer(doubleBuffered ? GL_BACK_RIGHT : GL_FRONT_RIGHT);
            }

            // when rendering textures to backbuffer (using fbo)
            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        else {
            const glm::vec4 color = sgct::Engine::instance().getClearColor();
            const float alpha = window.hasAlpha() ? 0.f : color.a;
            glClearColor(color.r, color.g, color.b, alpha);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
    }

} // namespace

Engine* Engine::_instance = nullptr;

Engine& Engine::instance() {
    if (_instance == nullptr) {
        throw std::logic_error("Using the instance before it was created or set");
    }
    return *_instance;
}

void Engine::create(config::Cluster cluster, Callbacks callbacks,
                    const Configuration& arg, Profile profile)
{
    // abock (2019-12-02) Unfortunately I couldn't find a better why than using this two
    // phase initialization approach. There are a few callbacks in the second phase that
    // are calling out to client code that (rightly) assumes that the Engine has been
    // created and are calling Engine::instance from they registered callbacks. If this
    // client code is executed from the constructor, the _instance variable has not yet
    // been set and will therefore cause the logic_error in the instance() function.
    _instance = new Engine(cluster, callbacks, arg);
    _instance->initialize(profile);
}

void Engine::destroy() {
    delete _instance;
    _instance = nullptr;
}

config::Cluster loadCluster(std::optional<std::string> path) {
    if (path) {
        try {
            return core::readConfig(*path);
        }
        catch (const std::runtime_error& e) {
            std::cout << e.what() << '\n';
            std::cout << getHelpMessage() << '\n';
            throw;
        }
        catch (...) {
            std::cout << getHelpMessage() << '\n';
            throw;
        }
    }
    else {
        config::Cluster cluster;
        // Create a default configuration
        sgct::config::ProjectionPlane proj;
        proj.lowerLeft = glm::vec3(-16.f/9.f, -1.f, 0.f);
        proj.upperLeft = glm::vec3(-16.f/9.f, 1.f, 0.f);
        proj.upperRight = glm::vec3(16.f/9.f, 1.f, 0.f);

        sgct::config::Viewport viewport;
        viewport.projection = proj;

        sgct::config::Window window;
        window.isFullScreen = false;
        window.size = glm::ivec2(1280, 720);
        window.viewports.push_back(viewport);

        sgct::config::Node node;
        node.address = "localhost";
        node.port = 20401;
        node.windows.push_back(window);

        sgct::config::User user;
        user.eyeSeparation = 0.06f;
        user.position = glm::vec3(0.f, 0.f, 4.f);
        cluster.users.push_back(user);

        cluster.masterAddress = "localhost";
        cluster.nodes.push_back(node);
        return cluster;
    }
}

Engine::Engine(config::Cluster cluster, Callbacks callbacks, const Configuration& config)
    : _drawFn(std::move(callbacks.draw))
    , _preSyncFn(std::move(callbacks.preSync))
    , _postSyncPreDrawFn(std::move(callbacks.postSyncPreDraw))
    , _postDrawFn(std::move(callbacks.postDraw))
    , _preWindowFn(std::move(callbacks.preWindow))
    , _initOpenGLFn(std::move(callbacks.initOpenGL))
    , _cleanUpFn(std::move(callbacks.cleanUp))
    , _draw2DFn(std::move(callbacks.draw2D))
    , _externalDecodeFn(std::move(callbacks.externalDecode))
    , _externalStatusFn(std::move(callbacks.externalStatus))
    , _dataTransferDecodeFn(std::move(callbacks.dataTransferDecode))
    , _dataTransferStatusFn(std::move(callbacks.dataTransferStatus))
    , _dataTransferAcknowledgeFn(std::move(callbacks.dataTransferAcknowledge))
    , _contextCreationFn(std::move(callbacks.contextCreation))
{
    SharedData::instance().setEncodeFunction(std::move(callbacks.encode));
    SharedData::instance().setDecodeFunction(std::move(callbacks.decode));

    gKeyboardCallback = std::move(callbacks.keyboard);
    gCharCallback = std::move(callbacks.character);
    gMouseButtonCallback = std::move(callbacks.mouseButton);
    gMousePosCallback = std::move(callbacks.mousePos);
    gMouseScrollCallback = std::move(callbacks.mouseScroll);
    gDropCallback = std::move(callbacks.drop);

    if (config.isServer) {
        _networkMode = *config.isServer ?
            core::NetworkManager::NetworkMode::LocalServer :
            core::NetworkManager::NetworkMode::LocalClient;
    }
    if (config.logPath) {
        Logger::instance().setLogPath(
            config.logPath->c_str(),
            core::ClusterManager::instance().getThisNodeId()
        );
        Logger::instance().setLogToFile(true);
    }
    if (config.logLevel) {
        Logger::instance().setNotifyLevel(*config.logLevel);
    }
    if (config.showHelpText) {
        std::cout << getHelpMessage() << '\n';
        std::exit(0);
    }
    if (config.nodeId) {
        // This nodeId might not be a valid node, but we have no way of knowing this yet
        // as the configuration hasn't been loaded yet
        core::ClusterManager::instance().setThisNodeId(*config.nodeId);
    }
    if (config.firmSync) {
        core::ClusterManager::instance().setFirmFrameLockSyncStatus(*config.firmSync);
    }
    if (config.ignoreSync) {
        core::ClusterManager::instance().setUseIgnoreSync(*config.ignoreSync);
    }
    if (config.captureFormat) {
        Settings::instance().setCaptureFormat(*config.captureFormat);
    }
    if (config.nCaptureThreads) {
        if (*config.nCaptureThreads > 0) {
            Settings::instance().setNumberOfCaptureThreads(*config.nCaptureThreads);
        }
        else {
            Logger::Error("Only positive number of capture threads allowed");
        }
    }
    if (cluster.checkOpenGL) {
        _checkOpenGLCalls = *cluster.checkOpenGL;
    }
    if (config.checkOpenGL) {
        _checkOpenGLCalls = *config.checkOpenGL;
    }
    if (cluster.checkFBOs) {
        _checkFBOs = *cluster.checkFBOs;
    }
    if (config.checkFBOs) {
        _checkFBOs = *config.checkFBOs;
    }
    if (config.useOpenGLDebugContext) {
        _createDebugContext = *config.useOpenGLDebugContext;
    }

    glfwSetErrorCallback([](int error, const char* desc) {
        throw Err(3010, "GLFW error (" + std::to_string(error) + "): " + desc);
    });
    const int res = glfwInit();
    if (res == GLFW_FALSE) {
        throw Err(3000, "Failed to initialize GLFW");
    }

    Logger::Info("SGCT version: %s", getVersion().c_str());

    Logger::Debug("Validating cluster configuration");
    config::validateCluster(cluster);

    core::ClusterManager::instance().applyCluster(cluster);
    for (const config::Tracker& tracker : cluster.trackers) {
        TrackingManager::instance().applyTracker(tracker);
    }
}

void Engine::initialize(Profile profile) {
    initNetwork();
    initWindows(profile);

    // Window resolution may have been set by the config. However, it only sets a pending
    // resolution, so it needs to apply it using the same routine as in the end of a frame
    core::Node& thisNode = core::ClusterManager::instance().getThisNode();
    for (int i = 0; i < thisNode.getNumberOfWindows(); ++i) {
        thisNode.getWindow(i).updateResolutions();
    }
    // if a single node, skip syncing
    if (core::ClusterManager::instance().getNumberOfNodes() == 1) {
        core::ClusterManager::instance().setUseIgnoreSync(true);
    }

    for (int i = 0; i < thisNode.getNumberOfWindows(); ++i) {
        GLFWwindow* window = getWindow(i).getWindowHandle();
        if (gKeyboardCallback) {
            glfwSetKeyCallback(
                window,
                [](GLFWwindow*, int key, int scancode, int a, int m) {
                if (gKeyboardCallback) {
                    gKeyboardCallback(Key(key), Modifier(m), Action(a), scancode);
                }
            }
            );
        }
        if (gMouseButtonCallback) {
            glfwSetMouseButtonCallback(
                window,
                [](GLFWwindow*, int b, int a, int m) {
                if (gMouseButtonCallback) {
                    gMouseButtonCallback(MouseButton(b), Modifier(m), Action(a));
                }
            }
            );
        }
        if (gMousePosCallback) {
            glfwSetCursorPosCallback(
                window,
                [](GLFWwindow*, double xPos, double yPos) {
                if (gMousePosCallback) {
                    gMousePosCallback(xPos, yPos);
                }
            }
            );
        }
        if (gCharCallback) {
            glfwSetCharModsCallback(
                window,
                [](GLFWwindow*, unsigned int ch, int mod) {
                if (gCharCallback) {
                    gCharCallback(ch, mod);
                }
            }
            );
        }
        if (gMouseScrollCallback) {
            glfwSetScrollCallback(
                window,
                [](GLFWwindow*, double xOffset, double yOffset) {
                if (gMouseScrollCallback) {
                    gMouseScrollCallback(xOffset, yOffset);
                }
            }
            );
        }
        if (gDropCallback) {
            glfwSetDropCallback(
                window,
                [](GLFWwindow*, int count, const char** paths) {
                if (gDropCallback) {
                    gDropCallback(count, paths);
                }
            }
            );
        }
    }

    // Get OpenGL version from the first window as there has to be one
    int v[3];
    GLFWwindow* winHandle = getWindow(0).getWindowHandle();
    v[0] = glfwGetWindowAttrib(winHandle, GLFW_CONTEXT_VERSION_MAJOR);
    v[1] = glfwGetWindowAttrib(winHandle, GLFW_CONTEXT_VERSION_MINOR);
    v[2] = glfwGetWindowAttrib(winHandle, GLFW_CONTEXT_REVISION);
    Logger::Info("OpenGL version %d.%d.%d core profile", v[0], v[1], v[2]);

    Logger::Info("Vendor: %s", glGetString(GL_VENDOR));
    Logger::Info("Renderer: %s", glGetString(GL_RENDERER));

    if (core::ClusterManager::instance().getNumberOfNodes() > 1) {
        std::string path = Settings::instance().getCapturePath() + "_node";
        path += std::to_string(core::ClusterManager::instance().getThisNodeId());

        Settings::instance().setCapturePath(path, Settings::CapturePath::Mono);
        Settings::instance().setCapturePath(path, Settings::CapturePath::LeftStereo);
        Settings::instance().setCapturePath(path, Settings::CapturePath::RightStereo);
    }

    // init window opengl data
    getWindow(0).makeOpenGLContextCurrent(Window::Context::Shared);

    //
    // Load Shaders
    //
    bool needsFxaa = false;
    for (int i = 0; i < getThisNode().getNumberOfWindows(); ++i) {
        needsFxaa |= getThisNode().getWindow(i).useFXAA();
    }

    if (needsFxaa) {
        FXAAShader shdr;
        shdr.shader = ShaderProgram("FXAAShader");
        shdr.shader.addShaderSource(core::shaders::FXAAVert, core::shaders::FXAAFrag);
        shdr.shader.createAndLinkProgram();
        shdr.shader.bind();

        const int id = shdr.shader.getId();
        shdr.sizeX = glGetUniformLocation(id, "rt_w");
        const glm::ivec2 framebufferSize = getWindow(0).getFramebufferResolution();
        glUniform1f(shdr.sizeX, static_cast<float>(framebufferSize.x));

        shdr.sizeY = glGetUniformLocation(id, "rt_h");
        glUniform1f(shdr.sizeY, static_cast<float>(framebufferSize.y));

        shdr.subPixTrim = glGetUniformLocation(id, "FXAA_SUBPIX_TRIM");
        glUniform1f(shdr.subPixTrim, FxaaSubPixTrim);

        shdr.subPixOffset = glGetUniformLocation(id, "FXAA_SUBPIX_OFFSET");
        glUniform1f(shdr.subPixOffset, FxaaSubPixOffset);

        glUniform1i(glGetUniformLocation(id, "tex"), 0);
        ShaderProgram::unbind();

        _fxaa = std::move(shdr);
    }

    // Used for overlays & mono.
    _fboQuad = ShaderProgram("FBOQuadShader");
    _fboQuad.addShaderSource(core::shaders::BaseVert, core::shaders::BaseFrag);
    _fboQuad.createAndLinkProgram();
    _fboQuad.bind();
    glUniform1i(glGetUniformLocation(_fboQuad.getId(), "tex"), 0);
    ShaderProgram::unbind();

    _overlay = ShaderProgram("OverlayShader");
    _overlay.addShaderSource(core::shaders::OverlayVert, core::shaders::OverlayFrag);
    _overlay.createAndLinkProgram();
    _overlay.bind();
    glUniform1i(glGetUniformLocation(_overlay.getId(), "Tex"), 0);
    ShaderProgram::unbind();

    if (_initOpenGLFn) {
        Logger::Info("Calling initialization callback");
        _initOpenGLFn();
    }

    // link all users to their viewports
    for (int i = 0; i < thisNode.getNumberOfWindows(); i++) {
        Window& win = thisNode.getWindow(i);
        win.initOGL();
        for (int j = 0; j < win.getNumberOfViewports(); ++j) {
            win.getViewport(j).linkUserName();
        }
    }

    updateFrustums();

#ifdef SGCT_HAS_TEXT
    // Add font
#ifdef WIN32
    constexpr const char* FontName = "verdanab.ttf";
#elif defined(__APPLE__)
    constexpr const char* FontName = "Tahoma Bold.ttf";
#else
    constexpr const char* FontName = "FreeSansBold.ttf";
#endif
    text::FontManager::instance().addFont("SGCTFont", FontName);
#endif // SGCT_HAS_TEXT

    // init swap barrier is swap groups are active
    Window::setBarrier(true);
    Window::resetSwapGroupFrameNumber();

    for (int i = 0; i < thisNode.getNumberOfWindows(); ++i) {
        thisNode.getWindow(i).initContextSpecificOGL();
    }

    // start sampling tracking data
    if (isMaster()) {
        TrackingManager::instance().startSampling();
    }
}

Engine::~Engine() {
    Logger::Info("Cleaning up");

    // First check whether we ever created a node for ourselves.  This might have failed
    // if the configuration was illformed
    core::ClusterManager& cm = core::ClusterManager::instance();
    const bool hasNode =
        cm.getThisNodeId() > -1 && cm.getThisNodeId() < cm.getNumberOfNodes();

    if (hasNode && cm.getThisNode().getNumberOfWindows() > 0) {
        cm.getThisNode().getWindow(0).makeOpenGLContextCurrent(Window::Context::Shared);
        if (_cleanUpFn) {
            _cleanUpFn();
        }
    }

    // We are only clearing the callbacks that might be called asynchronously
    Logger::Debug("Clearing callbacks");
    _externalDecodeFn = nullptr;
    _externalStatusFn = nullptr;
    _dataTransferDecodeFn = nullptr;
    _dataTransferStatusFn = nullptr;
    _dataTransferAcknowledgeFn = nullptr;
    gKeyboardCallback = nullptr;
    gMouseButtonCallback = nullptr;
    gMousePosCallback = nullptr;
    gMouseScrollCallback = nullptr;
    gDropCallback = nullptr;

    // kill thread
    if (_thread) {
        Logger::Debug("Waiting for frameLock thread to finish");

        core::mutex::FrameSync.lock();
        sRunUpdateFrameLockLoop = false;
        core::mutex::FrameSync.unlock();

        _thread->join();
        _thread = nullptr;
        Logger::Debug("Done");
    }

    // de-init window and unbind swapgroups
    if (hasNode && core::ClusterManager::instance().getNumberOfNodes() > 0) {
        for (int i = 0; i < cm.getThisNode().getNumberOfWindows(); ++i) {
            cm.getThisNode().getWindow(i).close();
        }
    }

    // close TCP connections
    Logger::Debug("Destroying network manager");
    core::NetworkManager::destroy();

    // Shared contex
    if (hasNode && cm.getThisNode().getNumberOfWindows() > 0) {
        cm.getThisNode().getWindow(0).makeOpenGLContextCurrent(Window::Context::Shared);
    }

    Logger::Debug("Destroying shader manager and internal shaders");
    ShaderManager::destroy();

    if (hasNode) {
        _fboQuad.deleteProgram();
        if (_fxaa) {
            _fxaa->shader.deleteProgram();
        }
        _overlay.deleteProgram();
    }

    _statisticsRenderer = nullptr;

    Logger::Debug("Destroying texture manager");
    TextureManager::destroy();

#ifdef SGCT_HAS_TEXT
    Logger::Debug("Destroying font manager");
    text::FontManager::destroy();
#endif // SGCT_HAS_TEXT

    // Window specific context
    if (hasNode && cm.getThisNode().getNumberOfWindows() > 0) {
        cm.getThisNode().getWindow(0).makeOpenGLContextCurrent(Window::Context::Window);
    }
    
    Logger::Debug("Destroying shared data");
    SharedData::destroy();
    
    Logger::Debug("Destroying cluster manager");
    core::ClusterManager::destroy();
    
    Logger::Debug("Destroying settings");
    Settings::destroy();

    Logger::Debug("Destroying message handler");
    Logger::destroy();

    Logger::Debug("Terminating glfw");
    glfwTerminate();
 
    Logger::Debug("Finished cleaning");
}

void Engine::terminate() {
    _shouldTerminate = true;
}

void Engine::initNetwork() {
    core::ClusterManager& cm = core::ClusterManager::instance();
    core::NetworkManager::create(_networkMode);

    // check in cluster configuration which it is
    if (_networkMode == core::NetworkManager::NetworkMode::Remote) {
        Logger::Debug("Matching ip address to find node in configuration");

        for (int i = 0; i < core::ClusterManager::instance().getNumberOfNodes(); i++) {
            // check ip
            const std::string& addr = cm.getNode(i).getAddress();
            if (core::NetworkManager::instance().matchesAddress(addr)) {
                cm.setThisNodeId(i);
                Logger::Debug("Running in cluster mode as node %d", i);
                break;
            }
        }
    }
    else {
        Logger::Debug("Running locally as node %d", cm.getThisNodeId());
    }

    // If the user has provided the node _id as an incorrect cmd argument then make the
    // _thisNode invalid
    if (cm.getThisNodeId() < 0) {
        core::NetworkManager::destroy();
        throw Err(3001, "Computer is not a part of the cluster configuration");
    }
    if (cm.getThisNodeId() >= cm.getNumberOfNodes()) {
        core::NetworkManager::destroy();
        throw Err(3002, "Requested node id was not found in the cluster configuration");
    }

    core::NetworkManager::instance().init();
}

void Engine::initWindows(Profile profile) {
    core::Node& thisNode = core::ClusterManager::instance().getThisNode();
    {
        int ver[3];
        glfwGetVersion(&ver[0], &ver[1], &ver[2]);
        Logger::Info("Using GLFW version %d.%d.%d", ver[0], ver[1], ver[2]);
    }

    switch (profile) {
        case Profile::OpenGL_3_3_Core:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            break;
        case Profile::OpenGL_4_0_Core:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
            break;
        case Profile::OpenGL_4_1_Core:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
            break;
        case Profile::OpenGL_4_2_Core:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
            break;
        case Profile::OpenGL_4_3_Core:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            break;
        case Profile::OpenGL_4_4_Core:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
            break;
        case Profile::OpenGL_4_5_Core:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
            break;
        case Profile::OpenGL_4_6_Core:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
            break;
        default:
            throw std::logic_error("Missing case label");
    }

    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (_createDebugContext) {
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    }

    if (_preWindowFn) {
        _preWindowFn();
    }

    for (int i = 0; i < thisNode.getNumberOfWindows(); ++i) {
        GLFWwindow* s = i == 0 ? nullptr : thisNode.getWindow(0).getWindowHandle();
        const bool isLastWindow = i == thisNode.getNumberOfWindows() - 1;
        thisNode.getWindow(i).openWindow(s, isLastWindow);
    }

    glbinding::Binding::initialize(glfwGetProcAddress);

    if (_checkOpenGLCalls || _checkFBOs) {
        using namespace glbinding;

        // The callback mask needs to be set in order to prevent an infinite loop when
        // calling these functions in the error checking callback
        Binding::setCallbackMaskExcept(
            CallbackMask::After,
            { "glGetError", "glCheckFramebufferStatus" }
        );
        Binding::setAfterCallback([this](const FunctionCall& f) {
            const GLenum error = _checkOpenGLCalls ? glGetError() : GL_NO_ERROR;
            const GLenum fboStatus = _checkFBOs ?
                glCheckFramebufferStatus(GL_FRAMEBUFFER) :
                GL_FRAMEBUFFER_COMPLETE;
            if (error == GL_NO_ERROR && fboStatus == GL_FRAMEBUFFER_COMPLETE) {
                return;
            }

            const char* n = f.function->name();
            switch (error) {
                case GL_NO_ERROR:
                    break;
                case GL_INVALID_ENUM:
                    Logger::Error("OpenGL error. %s: GL_INVALID_ENUM", n);
                    break;
                case GL_INVALID_VALUE:
                    Logger::Error("OpenGL error. %s: GL_INVALID_VALUE", n);
                    break;
                case GL_INVALID_OPERATION:
                    Logger::Error("OpenGL error. %s: GL_INVALID_OPERATION", n);
                    break;
                case GL_INVALID_FRAMEBUFFER_OPERATION:
                    Logger::Error(
                        "OpenGL error. %s: GL_INVALID_FRAMEBUFFER_OPERATION", n
                    );
                    break;
                case GL_STACK_OVERFLOW:
                    Logger::Error("OpenGL error. %s: GL_STACK_OVERFLOW", n);
                    break;
                case GL_STACK_UNDERFLOW:
                    Logger::Error("OpenGL error. %s: GL_STACK_UNDERFLOW", n);
                    break;
                case GL_OUT_OF_MEMORY:
                    Logger::Error("OpenGL error. %s: GL_OUT_OF_MEMORY", n);
                    break;
                default:
                    Logger::Error("OpenGL error. %s: %i", n, error);
            }

            switch (fboStatus) {
                case GL_FRAMEBUFFER_COMPLETE:
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                    Logger::Error(
                        "FBO error. %s: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT", n
                    );
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                    Logger::Error(
                        "FBO error. %s: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT", n
                    );
                    break;
                case GL_FRAMEBUFFER_UNSUPPORTED:
                    Logger::Error("FBO error. %s: GL_FRAMEBUFFER_UNSUPPORTED", n);
                    break;
                case GL_FRAMEBUFFER_UNDEFINED:
                    Logger::Error("FBO error. %s: GL_FRAMEBUFFER_UNDEFINED", n);
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                    Logger::Error(
                        "FBO error. %s: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER", n
                    );
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                    Logger::Error(
                        "FBO error. %s: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER", n
                    );
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                    Logger::Error(
                        "FBO error. %s: GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE", n
                    );
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
                    Logger::Error(
                        "FBO error. %s: GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS", n
                    );
                    break;
                default:
                    Logger::Error("FBO error. %s: %i", n, static_cast<int>(fboStatus));
            }
        });
    }

    // clear directly otherwise junk will be displayed on some OSs (OS X Yosemite)
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Window/Context creation callback
    if (thisNode.getNumberOfWindows() > 0) {
        if (_contextCreationFn) {
            GLFWwindow* share = thisNode.getWindow(0).getWindowHandle();
            _contextCreationFn(share);
        }
    }

    for (int i = 0; i < thisNode.getNumberOfWindows(); ++i) {
        thisNode.getWindow(i).init();
    }

    // init draw buffer resolution
    //updateDrawBufferResolutions();
    waitForAllWindowsInSwapGroupToOpen();

    if (RunFrameLockCheckThread) {
        if (core::ClusterManager::instance().getNumberOfNodes() > 1) {
            _thread = std::make_unique<std::thread>(updateFrameLockLoop, nullptr);
        }
    }

    // init swap group if enabled
    if (thisNode.isUsingSwapGroups()) {
        Window::initNvidiaSwapGroups();
    }
}

void Engine::frameLockPreStage() {
    const double ts = glfwGetTime();
    // from server to clients
    using P = std::pair<double, double>;
    std::optional<P> minMax = core::NetworkManager::instance().sync(
        core::NetworkManager::SyncMode::SendDataToClients
    );
    if (minMax) {
        addValue(_statistics.loopTimeMin, minMax->first);
        addValue(_statistics.loopTimeMax, minMax->second);
    }
    if (core::NetworkManager::instance().isComputerServer()) {
        addValue(_statistics.syncTimes, static_cast<float>(glfwGetTime() - ts));
    }

    // run only on clients
    if (core::NetworkManager::instance().isComputerServer() &&
        !core::ClusterManager::instance().getIgnoreSync())
    {
        return;
    }

    // not server
    const double t0 = glfwGetTime();
    while (core::NetworkManager::instance().isRunning() && _isRunning &&
           !core::NetworkManager::instance().isSyncComplete())
    {
        std::unique_lock lk(core::mutex::FrameSync);
        core::NetworkManager::cond.wait(lk);

        if (glfwGetTime() - t0 <= 1.0) {
            continue;
        }

        // more than a second
        core::Network* c = core::NetworkManager::instance().getSyncConnectionByIndex(0);
        if (_printSyncMessage && !c->isUpdated()) {
            Logger::Info(
                "Waiting for master. frame send %d != recv %d\n\tSwap groups: %s\n\t"
                "Swap barrier: %s\n\tUniversal frame number: %u\n\tSGCT frame number: %u",
                c->getSendFrameCurrent(), c->getRecvFramePrevious(),
                Window::isUsingSwapGroups() ? "enabled" : "disabled",
                Window::isBarrierActive() ? "enabled" : "disabled",
                Window::getSwapGroupFrameNumber(), _frameCounter
            );
        }

        if (glfwGetTime() - t0 > _syncTimeout) {
            throw Err(
                3003,
                "No sync signal from master after " + std::to_string(_syncTimeout) + " s"
            );
        }
    }

    // A this point all data needed for rendering a frame is received.
    // Let's signal that back to the master/server.
    core::NetworkManager::instance().sync(core::NetworkManager::SyncMode::Acknowledge);
    if (!core::NetworkManager::instance().isComputerServer()) {
        addValue(_statistics.syncTimes, glfwGetTime() - t0);
    }
}

void Engine::frameLockPostStage() {
    // post stage
    if (core::ClusterManager::instance().getIgnoreSync() ||
        !core::NetworkManager::instance().isComputerServer())
    {
        return;
    }

    const double t0 = glfwGetTime();
    while (core::NetworkManager::instance().isRunning() && _isRunning &&
        core::NetworkManager::instance().getActiveConnectionsCount() > 0 &&
        !core::NetworkManager::instance().isSyncComplete())
    {
        std::unique_lock lk(core::mutex::FrameSync);
        core::NetworkManager::cond.wait(lk);

        if (glfwGetTime() - t0 <= 1.0) {
            continue;
        }
        // more than a second
        core::NetworkManager& nm = core::NetworkManager::instance();
        for (int i = 0; i < nm.getSyncConnectionsCount(); ++i) {
            if (_printSyncMessage && !nm.getConnectionByIndex(i).isUpdated()) {
                Logger::Info(
                    "Waiting for IG%d: send frame %d != recv frame %d\n\tSwap groups: %s"
                    "\n\tSwap barrier: %s\n\tUniversal frame number: %u\n\t"
                    "SGCT frame number: %u",
                    i, nm.getConnectionByIndex(i).getSendFrameCurrent(),
                    nm.getConnectionByIndex(i).getRecvFrameCurrent(),
                    Window::isUsingSwapGroups() ? "enabled" : "disabled",
                    Window::isBarrierActive() ? "enabled" : "disabled",
                    Window::getSwapGroupFrameNumber(), _frameCounter
                );
            }
        }

        if (glfwGetTime() - t0 > _syncTimeout) {
            // more than a minute
            throw Err(
                3004,
                "No sync signal from clients after " + std::to_string(_syncTimeout) + " s"
            );
        }
    }

    addValue(_statistics.syncTimes, glfwGetTime() - t0);
}

void Engine::render() {
    _isRunning = true;

    // @TODO (abock, 2019-12-03) This can probably be replaced with a static call instead
    getWindow(0).makeOpenGLContextCurrent(Window::Context::Shared);
    unsigned int timeQueryBegin = 0;
    glGenQueries(1, &timeQueryBegin);
    unsigned int timeQueryEnd = 0;
    glGenQueries(1, &timeQueryEnd);

    core::Node& thisNode = core::ClusterManager::instance().getThisNode();
    while (_isRunning) {
        _renderingOffScreen = false;

        // update tracking data
        if (isMaster()) {
            TrackingManager::instance().updateTrackingDevices();
        }

        if (_preSyncFn) {
            _preSyncFn();
        }

        if (core::NetworkManager::instance().isComputerServer()) {
            SharedData::instance().encode();
        }
        else if (!core::NetworkManager::instance().isRunning()) {
            // exit if not running
            Logger::Error("Network disconnected. Exiting");
            break;
        }

        frameLockPreStage();

        bool buffersNeedUpdate = false;
        for (int i = 0; i < thisNode.getNumberOfWindows(); ++i) {
            const bool bufUpdate = thisNode.getWindow(i).update();
            buffersNeedUpdate |= bufUpdate;
        }

        if (buffersNeedUpdate) {
            //updateDrawBufferResolutions();
        }

        _renderingOffScreen = true;
        getWindow(0).makeOpenGLContextCurrent(Window::Context::Shared);

        // Make sure correct context is current
        if (_postSyncPreDrawFn) {
            _postSyncPreDrawFn();
        }

        const double startFrameTime = glfwGetTime();
        const double ft = static_cast<float>(startFrameTime - _statsPrevTimestamp);
        addValue(_statistics.frametimes, ft);
        _statsPrevTimestamp = startFrameTime;

        if (_statisticsRenderer) {
            glQueryCounter(timeQueryBegin, GL_TIMESTAMP);
        }

        // Render Viewports / Draw
        for (int i = 0; i < thisNode.getNumberOfWindows(); ++i) {
            Window& win = thisNode.getWindow(i);
            if (!(win.isVisible() || win.isRenderingWhileHidden())) {
                continue;
            }

            if (!_renderingOffScreen) {
                win.makeOpenGLContextCurrent(Window::Context::Window);
            }

            Window::StereoMode sm = win.getStereoMode();

            // Render Left/Mono non-linear projection viewports to cubemap
            for (int j = 0; j < win.getNumberOfViewports(); ++j) {
                core::Viewport& vp = win.getViewport(j);
                if (!vp.hasSubViewports()) {
                    continue;
                }

                core::NonLinearProjection* nonLinearProj = vp.getNonLinearProjection();

                nonLinearProj->setAlpha(win.hasAlpha() ? 0.f : 1.f);
                if (sm == Window::StereoMode::NoStereo) {
                    // for mono viewports frustum mode can be selected by user or xml
                    _currentFrustumMode = win.getViewport(j).getEye();
                    nonLinearProj->renderCubemap(win, _currentFrustumMode);
                }
                else {
                    _currentFrustumMode = Frustum::Mode::StereoLeftEye;
                    nonLinearProj->renderCubemap(win, _currentFrustumMode);
                }
            }

            // Render left/mono regular viewports to FBO
            // if any stereo type (except passive) then set frustum mode to left eye
            if (sm == Window::StereoMode::NoStereo) {
                _currentFrustumMode = Frustum::Mode::MonoEye;
                renderViewports(win, Window::TextureIndex::LeftEye);
            }
            else {
                _currentFrustumMode = Frustum::Mode::StereoLeftEye;
                renderViewports(win, Window::TextureIndex::LeftEye);
            }

            // if we are not rendering in stereo, we are done
            if (sm == Window::StereoMode::NoStereo) {
                continue;
            }

            // Render right non-linear projection viewports to cubemap
            for (int j = 0; j < win.getNumberOfViewports(); ++j) {
                core::Viewport& vp = win.getViewport(j);

                if (!vp.hasSubViewports()) {
                    continue;
                }
                core::NonLinearProjection* p = vp.getNonLinearProjection();

                p->setAlpha(win.hasAlpha() ? 0.f : 1.f);
                _currentFrustumMode = Frustum::Mode::StereoRightEye;
                p->renderCubemap(win, _currentFrustumMode);
            }

            // Render right regular viewports to FBO
            _currentFrustumMode = Frustum::Mode::StereoRightEye;
            // use a single texture for side-by-side and top-bottom stereo modes
            if (sm >= Window::StereoMode::SideBySide) {
                renderViewports(win, Window::TextureIndex::LeftEye);
            }
            else {
                renderViewports(win, Window::TextureIndex::RightEye);
            }
        }

        // Render to screen
        for (int i = 0; i < thisNode.getNumberOfWindows(); ++i) {
            if (thisNode.getWindow(i).isVisible()) {
                _renderingOffScreen = false;
                renderFBOTexture(getWindow(i));
            }
        }
        getWindow(0).makeOpenGLContextCurrent(Window::Context::Shared);

        if (_statisticsRenderer) {
            glQueryCounter(timeQueryEnd, GL_TIMESTAMP);
        }

        if (_postDrawFn) {
            _postDrawFn();
        }

        if (_statisticsRenderer) {
            // wait until the query results are available
            GLboolean done = GL_FALSE;
            while (!done) {
                glGetQueryObjectiv(timeQueryEnd, GL_QUERY_RESULT_AVAILABLE, &done);
            }

            // get the query results
            GLuint64 timerStart;
            glGetQueryObjectui64v(timeQueryBegin, GL_QUERY_RESULT, &timerStart);
            GLuint64 timerEnd;
            glGetQueryObjectui64v(timeQueryEnd, GL_QUERY_RESULT, &timerEnd);

            const double t = static_cast<double>(timerEnd - timerStart) / 1000000000.0;
            addValue(_statistics.drawTimes, t);

            _statisticsRenderer->update();
        }
        
        // master will wait for nodes render before swapping
        frameLockPostStage();

        // Swap front and back rendering buffers
        for (int i = 0; i < thisNode.getNumberOfWindows(); ++i) {
            thisNode.getWindow(i).swap(_takeScreenshot);
        }

        glfwPollEvents();
        for (int i = 0; i < thisNode.getNumberOfWindows(); ++i) {
            thisNode.getWindow(i).updateResolutions();
        }

        // Check if exit key was pressed or window was closed
        _isRunning = !(_shouldTerminate || thisNode.closeAllWindows() ||
                     !core::NetworkManager::instance().isRunning());

        // for all windows
        _frameCounter++;
        if (_takeScreenshot) {
            _shotCounter++;
        }
        _takeScreenshot = false;
    }

    getWindow(0).makeOpenGLContextCurrent(Window::Context::Shared);
    glDeleteQueries(1, &timeQueryBegin);
    glDeleteQueries(1, &timeQueryEnd);
}


void Engine::drawOverlays(Window& window) {
    for (int i = 0; i < window.getNumberOfViewports(); ++i) {
        window.setCurrentViewport(i);
        const core::Viewport& vp = window.getViewport(i);
        
        // if viewport has overlay
        if (!vp.hasOverlayTexture() || !vp.isEnabled()) {
            continue;
        }

        enterCurrentViewport(window, _currentFrustumMode);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, vp.getOverlayTextureIndex());
        _overlay.bind();
        window.renderScreenQuad();
        ShaderProgram::unbind();
    }
}

void Engine::prepareBuffer(Window& window, Window::TextureIndex ti) {
    core::OffScreenBuffer* fbo = window.getFBO();
    fbo->bind();
    if (fbo->isMultiSampled()) {
        return;
    }

    // update attachments
    fbo->attachColorTexture(window.getFrameBufferTexture(ti));

    if (Settings::instance().useDepthTexture()) {
        fbo->attachDepthTexture(
            window.getFrameBufferTexture(Window::TextureIndex::Depth)
        );
    }

    if (Settings::instance().useNormalTexture()) {
        fbo->attachColorTexture(
            window.getFrameBufferTexture(Window::TextureIndex::Normals),
            GL_COLOR_ATTACHMENT1
        );
    }

    if (Settings::instance().usePositionTexture()) {
        fbo->attachColorTexture(
            window.getFrameBufferTexture(Window::TextureIndex::Positions),
            GL_COLOR_ATTACHMENT2
        );
    }
}

void Engine::renderFBOTexture(Window& window) {
    core::OffScreenBuffer::unbind();

    window.makeOpenGLContextCurrent(Window::Context::Window);

    glDisable(GL_BLEND);
    // needed for shaders
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    _currentFrustumMode = (window.getStereoMode() == Window::StereoMode::Active) ?
        Frustum::Mode::StereoLeftEye :
        Frustum::Mode::MonoEye;

    const glm::ivec2 size = glm::ivec2(
        glm::ceil(window.getScale() * glm::vec2(window.getResolution()))
    );

    glViewport(0, 0, size.x, size.y);
    setAndClearBuffer(window, BufferMode::BackBufferBlack, _currentFrustumMode);
   
    Window::StereoMode sm = window.getStereoMode();
    bool maskShaderSet = false;
    if (sm > Window::StereoMode::Active && sm < Window::StereoMode::SideBySide) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(
            GL_TEXTURE_2D,
            window.getFrameBufferTexture(Window::TextureIndex::LeftEye)
        );

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(
            GL_TEXTURE_2D,
            window.getFrameBufferTexture(Window::TextureIndex::RightEye)
        );

        window.bindStereoShaderProgram();

        glUniform1i(window.getStereoShaderLeftTexLoc(), 0);
        glUniform1i(window.getStereoShaderRightTexLoc(), 1);

        for (int i = 0; i < window.getNumberOfViewports(); i++) {
            window.getViewport(i).renderWarpMesh();
            //window.getViewport(i).renderQuadMesh();
        }
    }
    else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(
            GL_TEXTURE_2D,
            window.getFrameBufferTexture(Window::TextureIndex::LeftEye)
        );

        _fboQuad.bind();
        maskShaderSet = true;

        for (int i = 0; i < window.getNumberOfViewports(); ++i) {
            window.getViewport(i).renderWarpMesh();
            //win.getViewport(i).renderQuadMesh();
        }

        // render right eye in active stereo mode
        if (window.getStereoMode() == Window::StereoMode::Active) {
            glViewport(0, 0, size.x, size.y);
            
            //clear buffers
            _currentFrustumMode = Frustum::Mode::StereoRightEye;
            setAndClearBuffer(window, BufferMode::BackBufferBlack, _currentFrustumMode);

            glBindTexture(
                GL_TEXTURE_2D,
                window.getFrameBufferTexture(Window::TextureIndex::RightEye)
            );
            for (int i = 0; i < window.getNumberOfViewports(); ++i) {
                window.getViewport(i).renderWarpMesh();
                //window.getViewport(i).renderQuadMesh();
            }
        }
    }

    // render mask (mono)
    if (window.hasAnyMasks()) {
        if (!maskShaderSet) {
            _fboQuad.bind();
        }
        
        glDrawBuffer(window.isDoubleBuffered() ? GL_BACK : GL_FRONT);
        glReadBuffer(window.isDoubleBuffered() ? GL_BACK : GL_FRONT);
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_BLEND);

        // Result = (Color * BlendMask) * (1-BlackLevel) + BlackLevel
        // render blend masks
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        for (int i = 0; i < window.getNumberOfViewports(); ++i) {
            const core::Viewport& vp = window.getViewport(i);
            if (vp.hasBlendMaskTexture() && vp.isEnabled()) {
                glBindTexture(GL_TEXTURE_2D, vp.getBlendMaskTextureIndex());
                vp.renderMaskMesh();
            }
        }

        // render black level masks
        for (int i = 0; i < window.getNumberOfViewports(); ++i) {
            const core::Viewport& vp = window.getViewport(i);
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

void Engine::renderViewports(Window& window, Window::TextureIndex ti) {
    prepareBuffer(window, ti);

    Window::StereoMode sm = window.getStereoMode();
    // render all viewports for selected eye
    for (int i = 0; i < window.getNumberOfViewports(); ++i) {
        window.setCurrentViewport(i);
        core::Viewport& vp = window.getViewport(i);

        if (!vp.isEnabled()) {
            continue;
        }

        // if passive stereo or mono
        if (sm == Window::StereoMode::NoStereo) {
            _currentFrustumMode = vp.getEye();
        }

        if (vp.hasSubViewports()) {
            if (vp.isTracked()) {
                vp.getNonLinearProjection()->updateFrustums(
                    _currentFrustumMode,
                    _nearClipPlane,
                    _farClipPlane
                );
            }

            if (window.shouldCallDraw3DFunction()) {
                vp.getNonLinearProjection()->render(window, _currentFrustumMode);
            }
        }
        else {
            // no subviewports
            if (vp.isTracked()) {
                vp.calculateFrustum(_currentFrustumMode, _nearClipPlane, _farClipPlane);
            }

            // check if we want to blit the previous window before we do anything else
            if (window.shouldBlitPreviousWindow()) {
                if (window.getId() == 0) {
                    Logger::Error("Cannot blit into first window");
                }
                else {
                    const int prevId = window.getId() - 1;
                    Window& prevWindow = getWindow(prevId);
                    blitPreviousWindowViewport(prevWindow, window, _currentFrustumMode);
                }
            }

            if (window.shouldCallDraw3DFunction()) {
                // run scissor test to prevent clearing of entire buffer
                glEnable(GL_SCISSOR_TEST);

                enterCurrentViewport(window, _currentFrustumMode);
                setAndClearBuffer(
                    window,
                    BufferMode::RenderToTexture,
                    _currentFrustumMode
                );
                glDisable(GL_SCISSOR_TEST);

                if (_drawFn) {
                    RenderData renderData(
                        window,
                        _currentFrustumMode,
                        core::ClusterManager::instance().getSceneTransform(),
                        vp.getProjection(_currentFrustumMode).getViewMatrix(),
                        vp.getProjection(_currentFrustumMode).getProjectionMatrix(),
                        vp.getProjection(_currentFrustumMode).getViewProjectionMatrix() *
                            core::ClusterManager::instance().getSceneTransform()
                    );
                    _drawFn(renderData);
                }
            }
        }
    }

    // If we did not render anything, make sure we clear the screen at least
    if (!window.shouldCallDraw3DFunction() && !window.shouldBlitPreviousWindow()) {
        setAndClearBuffer(window, BufferMode::RenderToTexture, _currentFrustumMode);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    // for side-by-side or top-bottom mode, do postfx/blit only after rendering right eye
    const bool isSplitScreen = (sm >= Window::StereoMode::SideBySide);
    if (!(isSplitScreen && _currentFrustumMode == Frustum::Mode::StereoLeftEye)) {
        render2D(window);
        if (isSplitScreen) {
            // render left eye info and graph to render 2D items after post fx
            _currentFrustumMode = Frustum::Mode::StereoLeftEye;
            render2D(window);
        }

        updateRenderingTargets(window, ti); // only used if multisampled FBOs
    }

    glDisable(GL_BLEND);
}

void Engine::render2D(Window& window) {
    // draw viewport overlays if any
    drawOverlays(window);

    // draw info & stats
    // the cubemap viewports are all the same so it makes no sense to render everything
    // several times therefore just loop one iteration in that case.
    if (!(_statisticsRenderer || _draw2DFn)) {
        return;
    }

    for (int i = 0; i < window.getNumberOfViewports(); ++i) {
        window.setCurrentViewport(i);
        if (!window.getCurrentViewport()->isEnabled()) {
            continue;
        }
        enterCurrentViewport(window, _currentFrustumMode);

        if (_statisticsRenderer) {
            _statisticsRenderer->render(window);
        }

        // Check if we should call the use defined draw2D function
        if (_draw2DFn && window.shouldCallDraw2DFunction()) {
            const core::BaseViewport& vp = *window.getCurrentViewport();

            RenderData renderData(
                window,
                _currentFrustumMode,
                core::ClusterManager::instance().getSceneTransform(),
                vp.getProjection(_currentFrustumMode).getViewMatrix(),
                vp.getProjection(_currentFrustumMode).getProjectionMatrix(),
                vp.getProjection(_currentFrustumMode).getViewProjectionMatrix() *
                    core::ClusterManager::instance().getSceneTransform()
            );
            _draw2DFn(renderData);
        }
    }
}

void Engine::renderPostFX(Window& window, Window::TextureIndex targetIndex) {
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    if (window.useFXAA()) {
        assert(_fxaa.has_value());

        // bind target FBO
        window.getFBO()->attachColorTexture(window.getFrameBufferTexture(targetIndex));

        glm::ivec2 framebufferSize = window.getFramebufferResolution();
        glViewport(0, 0, framebufferSize.x, framebufferSize.y);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);

        glBindTexture(
            GL_TEXTURE_2D,
            window.getFrameBufferTexture(Window::TextureIndex::Intermediate)
        );

        _fxaa->shader.bind();
        glUniform1f(_fxaa->sizeX, static_cast<float>(framebufferSize.x));
        glUniform1f(_fxaa->sizeY, static_cast<float>(framebufferSize.y));
        glUniform1f(_fxaa->subPixTrim, FxaaSubPixTrim);
        glUniform1f(_fxaa->subPixOffset, FxaaSubPixOffset);

        window.renderScreenQuad();
        ShaderProgram::unbind();
    }
}

void Engine::updateRenderingTargets(Window& window, Window::TextureIndex ti) {
    // copy AA-buffer to "regular" / non-AA buffer
    core::OffScreenBuffer* fbo = window.getFBO();
    if (!fbo->isMultiSampled()) {
        return;
    }

    // bind separate read and draw buffers to prepare blit operation
    fbo->bindBlit();

    // update attachments
    fbo->attachColorTexture(window.getFrameBufferTexture(ti));

    if (Settings::instance().useDepthTexture()) {
        fbo->attachDepthTexture(
            window.getFrameBufferTexture(Window::TextureIndex::Depth)
        );
    }

    if (Settings::instance().useNormalTexture()) {
        fbo->attachColorTexture(
            window.getFrameBufferTexture(Window::TextureIndex::Normals),
            GL_COLOR_ATTACHMENT1
        );
    }

    if (Settings::instance().usePositionTexture()) {
        fbo->attachColorTexture(
            window.getFrameBufferTexture(Window::TextureIndex::Positions),
            GL_COLOR_ATTACHMENT2
        );
    }

    fbo->blit();
}

bool Engine::isMaster() const {
    return core::NetworkManager::instance().isComputerServer();
}

unsigned int Engine::getCurrentFrameNumber() const {
    return _frameCounter;
}

void Engine::waitForAllWindowsInSwapGroupToOpen() {
    core::Node& thisNode = core::ClusterManager::instance().getThisNode();

    // clear the buffers initially
    for (int i = 0; i < thisNode.getNumberOfWindows(); ++i) {
        thisNode.getWindow(i).makeOpenGLContextCurrent(Window::Context::Window);
        glDrawBuffer(thisNode.getWindow(i).isDoubleBuffered() ? GL_BACK : GL_FRONT);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (thisNode.getWindow(i).isDoubleBuffered()) {
            glfwSwapBuffers(thisNode.getWindow(i).getWindowHandle());
        }
        else {
            glFinish();
        }
    }
    glfwPollEvents();
    
    // Must wait until all nodes are running if using swap barrier
    if (core::ClusterManager::instance().getIgnoreSync() ||
        core::ClusterManager::instance().getNumberOfNodes() <= 1)
    {
        return;
    }

    // check if swapgroups are supported
#ifdef WIN32
    const bool hasSwapGroup = glfwExtensionSupported("WGL_NV_swap_group");
#else
    const bool hasSwapGroup = false;
#endif
    if (hasSwapGroup) {
        Logger::Info("Swap groups are supported by hardware");
    }
    else {
        Logger::Info("Swap groups are not supported by hardware");
    }
    Logger::Info("Waiting for all nodes to connect");

    while (!_shouldTerminate && core::NetworkManager::instance().isRunning() &&
           !thisNode.closeAllWindows())
    {
        // Swap front and back rendering buffers
        for (int i = 0; i < thisNode.getNumberOfWindows(); i++) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            if (thisNode.getWindow(i).isDoubleBuffered()) {
                glfwSwapBuffers(thisNode.getWindow(i).getWindowHandle());
            }
            else {
                glFinish();
            }
        }
        glfwPollEvents();

        if (core::NetworkManager::instance().areAllNodesConnected()) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Engine::updateFrustums() {
    core::Node& thisNode = core::ClusterManager::instance().getThisNode();
    for (int w = 0; w < thisNode.getNumberOfWindows(); w++) {
        Window& win = thisNode.getWindow(w);
        for (int i = 0; i < win.getNumberOfViewports(); i++) {
            core::Viewport& vp = win.getViewport(i);
            if (vp.isTracked()) {
                // if not tracked update, otherwise this is done on the fly
                continue;
            }

            if (vp.hasSubViewports()) {
                core::NonLinearProjection& proj = *vp.getNonLinearProjection();
                proj.updateFrustums(
                    Frustum::Mode::MonoEye,
                    _nearClipPlane,
                    _farClipPlane
                );

                proj.updateFrustums(
                    Frustum::Mode::StereoLeftEye,
                    _nearClipPlane,
                    _farClipPlane
                );

                proj.updateFrustums(
                    Frustum::Mode::StereoRightEye,
                    _nearClipPlane,
                    _farClipPlane
                );
            }
            else {
                vp.calculateFrustum(
                    Frustum::Mode::MonoEye,
                    _nearClipPlane,
                    _farClipPlane
                );

                vp.calculateFrustum(
                    Frustum::Mode::StereoLeftEye,
                    _nearClipPlane,
                    _farClipPlane
                );

                vp.calculateFrustum(
                    Frustum::Mode::StereoRightEye,
                    _nearClipPlane,
                    _farClipPlane
                );
            }
        }
    }
}

void Engine::blitPreviousWindowViewport(Window& prevWindow, Window& window,
                                        Frustum::Mode mode)
{
    // run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);
    enterCurrentViewport(window, _currentFrustumMode);
    setAndClearBuffer(window, BufferMode::RenderToTexture, _currentFrustumMode);
    glDisable(GL_SCISSOR_TEST);

    _overlay.bind();

    glActiveTexture(GL_TEXTURE0);
    Window::TextureIndex m = [](Frustum::Mode v) {
        switch (v) {
            // @TODO abock (2019-09-27) Yep, I'm confused about this mapping, too. But I
            // just took the enumerations values as they were and I assume that it was an
            // undetected bug
            case Frustum::Mode::MonoEye: return Window::TextureIndex::LeftEye;
            case Frustum::Mode::StereoLeftEye: return Window::TextureIndex::RightEye;
            case Frustum::Mode::StereoRightEye: return Window::TextureIndex::Intermediate;
            default: throw std::logic_error("Unhandled case label");
        }
    }(mode);
    glBindTexture(GL_TEXTURE_2D, prevWindow.getFrameBufferTexture(m));

    window.renderScreenQuad();
    ShaderProgram::unbind();
}

void Engine::enterCurrentViewport(const Window& window, Frustum::Mode frustum) {
    core::BaseViewport* vp = window.getCurrentViewport();
    
    const glm::vec2 res = glm::vec2(window.getFramebufferResolution());
    const glm::vec2 p = vp->getPosition() * res;
    const glm::vec2 s = vp->getSize() * res;
    glm::ivec4 vpCoordinates = glm::ivec4(glm::ivec2(p), glm::ivec2(s));

    Window::StereoMode sm = window.getStereoMode();
    if (frustum == Frustum::Mode::StereoLeftEye) {
        switch (sm) {
            case Window::StereoMode::SideBySide:
                vpCoordinates.x /= 2;
                vpCoordinates.z /= 2;
                break;
            case Window::StereoMode::SideBySideInverted:
                vpCoordinates.x = (vpCoordinates.x / 2) + (vpCoordinates.z / 2);
                vpCoordinates.z = vpCoordinates.z / 2;
                break;
            case Window::StereoMode::TopBottom:
                vpCoordinates.y = (vpCoordinates.y / 2) + (vpCoordinates.w / 2);
                vpCoordinates.w /= 2;
                break;
            case Window::StereoMode::TopBottomInverted:
                vpCoordinates.y /= 2;
                vpCoordinates.w /= 2;
                break;
            default:
                break;
        }
    }
    else {
        switch (sm) {
            case Window::StereoMode::SideBySide:
                vpCoordinates.x = (vpCoordinates.x / 2) + (vpCoordinates.z / 2);
                vpCoordinates.z /= 2;
                break;
            case Window::StereoMode::SideBySideInverted:
                vpCoordinates.x /= 2;
                vpCoordinates.z /= 2;
                break;
            case Window::StereoMode::TopBottom:
                vpCoordinates.y /= 2;
                vpCoordinates.w /= 2;
                break;
            case Window::StereoMode::TopBottomInverted:
                vpCoordinates.y = (vpCoordinates.y / 2) + (vpCoordinates.w / 2);
                vpCoordinates.w /= 2;
                break;
            default:
                break;
        }
    }

    glViewport(vpCoordinates.x, vpCoordinates.y, vpCoordinates.z, vpCoordinates.w);
    glScissor(vpCoordinates.x, vpCoordinates.y, vpCoordinates.z, vpCoordinates.w);
}

const Engine::Statistics& Engine::getStatistics() const {
    return _statistics;
}

double Engine::getDt() const {
    return _statistics.frametimes[0];
}

double Engine::getAvgFPS() const {
    return 1.0 / getAvgDt();
}

double Engine::getAvgDt() const {
    const double accFrameTime = std::accumulate(
        _statistics.frametimes.begin(),
        _statistics.frametimes.end(),
        0.0
    );
    const int nValues = static_cast<int>(std::count_if(
        _statistics.frametimes.begin(),
        _statistics.frametimes.end(),
        [](double d) { return d != 0.0; }
    ));
    // We must take the frame counter into account as the history might not be filled yet
    unsigned f = std::clamp<unsigned int>(_frameCounter, 1, nValues);
    return accFrameTime / f;
}

double Engine::getMinDt() const {
    return *std::min_element(
        _statistics.frametimes.begin(),
        _statistics.frametimes.end()
    );
}

double Engine::getMaxDt() const {
    return *std::max_element(
        _statistics.frametimes.begin(),
        _statistics.frametimes.end()
    );
}

double Engine::getDtStandardDeviation() const {
    const double avg = getAvgDt();
    const double sumSquare = std::accumulate(
        _statistics.frametimes.begin(),
        _statistics.frametimes.end(),
        0.0,
        [avg](double cur, double rhs) { return cur + pow(rhs - avg, 2.0); }
    );
    const int nValues = static_cast<int>(std::count_if(
        _statistics.frametimes.begin(),
        _statistics.frametimes.end(),
        [](double d) { return d != 0.0; }
    ));
    // We must take the frame counter into account as the history might not be filled yet
    unsigned f = std::clamp<unsigned int>(_frameCounter, 1, nValues);
    return sumSquare / f;
}

glm::vec4 Engine::getClearColor() const {
    return _clearColor;
}

float Engine::getNearClipPlane() const {
    return _nearClipPlane;
}

float Engine::getFarClipPlane() const {
    return _farClipPlane;
}

void Engine::setNearAndFarClippingPlanes(float nearClip, float farClip) {
    _nearClipPlane = nearClip;
    _farClipPlane = farClip;
    updateFrustums();
}

void Engine::setEyeSeparation(float eyeSeparation) {
    core::Node& thisNode = core::ClusterManager::instance().getThisNode();
    for (int w = 0; w < thisNode.getNumberOfWindows(); w++) {
        Window& window = thisNode.getWindow(w);

        for (int i = 0; i < window.getNumberOfViewports(); i++) {
            window.getViewport(i).getUser().setEyeSeparation(eyeSeparation);
        }
    }
    updateFrustums();
}

void Engine::setClearColor(glm::vec4 color) {
    _clearColor = std::move(color);
}

unsigned int Engine::getDrawTexture(Window& window) const {
    return window.getFrameBufferTexture(
        (_currentFrustumMode == Frustum::Mode::StereoRightEye) ?
        Window::TextureIndex::RightEye : Window::TextureIndex::LeftEye
    );
}

int Engine::getFocusedWindowIndex() const {
    const core::Node& thisNode = core::ClusterManager::instance().getThisNode();
    for (int i = 0; i < thisNode.getNumberOfWindows(); i++) {
        if (thisNode.getWindow(i).isFocused()) {
            return i;
        }
    }
    return -1; // no window has focus
}

void Engine::setStatsGraphVisibility(bool state) {
    if (state && _statisticsRenderer == nullptr) {
        _statisticsRenderer = std::make_unique<core::StatisticsRenderer>(_statistics);
    }
    if (!state && _statisticsRenderer) {
        _statisticsRenderer = nullptr;
    }
}

void Engine::takeScreenshot() {
    _takeScreenshot = true;
}

const std::function<void(RenderData)>& Engine::getDrawFunction() const {
    return _drawFn;
}

void Engine::invokeDecodeCallbackForExternalControl(const char* data, int length, int) {
    if (_externalDecodeFn && length > 0) {
        _externalDecodeFn(data, length);
    }
}

void Engine::invokeUpdateCallbackForExternalControl(bool connected) {
    if (_externalStatusFn) {
        _externalStatusFn(connected);
    }
}

void Engine::invokeDecodeCallbackForDataTransfer(void* d, int len, int package, int id) {
    if (_dataTransferDecodeFn && len > 0) {
        _dataTransferDecodeFn(d, len, package, id);
    }
}

void Engine::invokeUpdateCallbackForDataTransfer(bool connected, int clientId) {
    if (_dataTransferStatusFn) {
        _dataTransferStatusFn(connected, clientId);
    }
}

void Engine::invokeAcknowledgeCallbackForDataTransfer(int packageId, int clientId) {
    if (_dataTransferAcknowledgeFn) {
        _dataTransferAcknowledgeFn(packageId, clientId);
    }
}

void Engine::sendMessageToExternalControl(const void* data, int length) {
    core::NetworkManager& nm = core::NetworkManager::instance();
    if (nm.getExternalControlConnection()) {
        nm.getExternalControlConnection()->sendData(data, length);
    }
}

void Engine::transferDataBetweenNodes(const void* data, int length, int packageId) {
    core::NetworkManager::instance().transferData(data, length, packageId);
}

bool Engine::isExternalControlConnected() const {
    return (core::NetworkManager::instance().getExternalControlConnection() &&
        core::NetworkManager::instance().getExternalControlConnection()->isConnected());
}

int Engine::getKey(int winIndex, int key) {
    return glfwGetKey(_instance->getWindow(winIndex).getWindowHandle(), key);
}

int Engine::getMouseButton(int winIndex, int button) {
    return glfwGetMouseButton(_instance->getWindow(winIndex).getWindowHandle(), button);
}

void Engine::getMousePos(int winIndex, double* xPos, double* yPos) {
    glfwGetCursorPos(_instance->getWindow(winIndex).getWindowHandle(), xPos, yPos);
}

const char* Engine::getJoystickName(Joystick joystick) {
    return glfwGetJoystickName(static_cast<int>(joystick));
}

const float* Engine::getJoystickAxes(Joystick joystick, int* numOfValues) {
    return glfwGetJoystickAxes(static_cast<int>(joystick), numOfValues);
}

const unsigned char* Engine::getJoystickButtons(Joystick joystick, int* numOfValues) {
    return glfwGetJoystickButtons(static_cast<int>(joystick), numOfValues);
}

const core::Node& Engine::getThisNode() const {
    return core::ClusterManager::instance().getThisNode();
}

Window& Engine::getWindow(int index) const {
    return core::ClusterManager::instance().getThisNode().getWindow(index);
}

int Engine::getNumberOfWindows() const {
    return core::ClusterManager::instance().getThisNode().getNumberOfWindows();
}

core::User& Engine::getDefaultUser() {
    return core::ClusterManager::instance().getDefaultUser();
}

double Engine::getTime() {
    return glfwGetTime();
}

void Engine::setSyncParameters(bool printMessage, float timeout) {
    _printSyncMessage = printMessage;
    _syncTimeout = timeout;
}

void Engine::setScreenShotNumber(unsigned int number) {
    _shotCounter = number;
}

unsigned int Engine::getScreenShotNumber() const {
    return _shotCounter;
}

} // namespace sgct
