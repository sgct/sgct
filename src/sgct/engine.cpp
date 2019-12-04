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
            if (window.stereoMode() != Window::StereoMode::Active) {
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
            const glm::vec4 color = sgct::Engine::instance().clearColor();
            const float alpha = window.hasAlpha() ? 0.f : color.a;
            glClearColor(color.r, color.g, color.b, alpha);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
    }


    void prepareBuffer(Window& window, Window::TextureIndex ti) {
        core::OffScreenBuffer* fbo = window.fbo();
        fbo->bind();
        if (fbo->isMultiSampled()) {
            return;
        }

        // update attachments
        fbo->attachColorTexture(window.frameBufferTexture(ti));

        if (Settings::instance().useDepthTexture()) {
            fbo->attachDepthTexture(
                window.frameBufferTexture(Window::TextureIndex::Depth)
            );
        }

        if (Settings::instance().useNormalTexture()) {
            fbo->attachColorTexture(
                window.frameBufferTexture(Window::TextureIndex::Normals),
                GL_COLOR_ATTACHMENT1
            );
        }

        if (Settings::instance().usePositionTexture()) {
            fbo->attachColorTexture(
                window.frameBufferTexture(Window::TextureIndex::Positions),
                GL_COLOR_ATTACHMENT2
            );
        }
    }

    void updateRenderingTargets(Window& window, Window::TextureIndex ti) {
        // copy AA-buffer to "regular" / non-AA buffer
        core::OffScreenBuffer* fbo = window.fbo();
        if (!fbo->isMultiSampled()) {
            return;
        }

        // bind separate read and draw buffers to prepare blit operation
        fbo->bindBlit();

        // update attachments
        fbo->attachColorTexture(window.frameBufferTexture(ti));

        if (Settings::instance().useDepthTexture()) {
            fbo->attachDepthTexture(
                window.frameBufferTexture(Window::TextureIndex::Depth)
            );
        }

        if (Settings::instance().useNormalTexture()) {
            fbo->attachColorTexture(
                window.frameBufferTexture(Window::TextureIndex::Normals),
                GL_COLOR_ATTACHMENT1
            );
        }

        if (Settings::instance().usePositionTexture()) {
            fbo->attachColorTexture(
                window.frameBufferTexture(Window::TextureIndex::Positions),
                GL_COLOR_ATTACHMENT2
            );
        }

        fbo->blit();
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
            std::cout << helpMessage() << '\n';
            throw;
        }
        catch (...) {
            std::cout << helpMessage() << '\n';
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

    core::NetworkManager::NetworkMode netMode = core::NetworkManager::NetworkMode::Remote;
    if (config.isServer) {
        netMode = *config.isServer ?
            core::NetworkManager::NetworkMode::LocalServer :
            core::NetworkManager::NetworkMode::LocalClient;
    }
    if (config.logPath) {
        Logger::instance().setLogPath(
            *config.logPath,
            core::ClusterManager::instance().thisNodeId()
        );
        Logger::instance().setLogToFile(true);
    }
    if (config.logLevel) {
        Logger::instance().setNotifyLevel(*config.logLevel);
    }
    if (config.showHelpText) {
        std::cout << helpMessage() << '\n';
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
        Settings::instance().setNumberOfCaptureThreads(*config.nCaptureThreads);
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

    Logger::Info("SGCT version: %s", version().c_str());

    Logger::Debug("Validating cluster configuration");
    config::validateCluster(cluster);

    core::ClusterManager::instance().applyCluster(cluster);
    for (const config::Tracker& tracker : cluster.trackers) {
        TrackingManager::instance().applyTracker(tracker);
    }

    core::NetworkManager::create(netMode);
    initNetwork(netMode);
}

void Engine::initialize(Profile profile) {
    initWindows(profile);

    // Window resolution may have been set by the config. However, it only sets a pending
    // resolution, so it needs to apply it using the same routine as in the end of a frame
    core::Node& thisNode = core::ClusterManager::instance().thisNode();
    for (int i = 0; i < thisNode.numberOfWindows(); ++i) {
        thisNode.window(i).updateResolutions();
    }
    // if a single node, skip syncing
    if (core::ClusterManager::instance().numberOfNodes() == 1) {
        core::ClusterManager::instance().setUseIgnoreSync(true);
    }

    for (int i = 0; i < thisNode.numberOfWindows(); ++i) {
        GLFWwindow* win = window(i).windowHandle();
        if (gKeyboardCallback) {
            glfwSetKeyCallback(
                win,
                [](GLFWwindow*, int key, int scancode, int a, int m) {
                    gKeyboardCallback(Key(key), Modifier(m), Action(a), scancode);
                }
            );
        }
        if (gMouseButtonCallback) {
            glfwSetMouseButtonCallback(
                win,
                [](GLFWwindow*, int b, int a, int m) {
                    gMouseButtonCallback(MouseButton(b), Modifier(m), Action(a));
                }
            );
        }
        if (gMousePosCallback) {
            glfwSetCursorPosCallback(
                win,
                [](GLFWwindow*, double xPos, double yPos) {
                    gMousePosCallback(xPos, yPos);
                }
            );
        }
        if (gCharCallback) {
            glfwSetCharModsCallback(
                win,
                [](GLFWwindow*, unsigned int ch, int mod) {
                    gCharCallback(ch, mod);
                }
            );
        }
        if (gMouseScrollCallback) {
            glfwSetScrollCallback(
                win,
                [](GLFWwindow*, double xOffset, double yOffset) {
                    gMouseScrollCallback(xOffset, yOffset);
                }
            );
        }
        if (gDropCallback) {
            glfwSetDropCallback(
                win,
                [](GLFWwindow*, int count, const char** paths) {
                    gDropCallback(count, paths);
                }
            );
        }
    }

    // Get OpenGL version from the first window as there has to be one
    GLFWwindow* winHandle = window(0).windowHandle();
    int v[] = {
        glfwGetWindowAttrib(winHandle, GLFW_CONTEXT_VERSION_MAJOR),
        glfwGetWindowAttrib(winHandle, GLFW_CONTEXT_VERSION_MINOR),
        glfwGetWindowAttrib(winHandle, GLFW_CONTEXT_REVISION)
    };
    Logger::Info("OpenGL version %d.%d.%d core profile", v[0], v[1], v[2]);

    Logger::Info("Vendor: %s", glGetString(GL_VENDOR));
    Logger::Info("Renderer: %s", glGetString(GL_RENDERER));

    if (core::ClusterManager::instance().numberOfNodes() > 1) {
        std::string path = Settings::instance().capturePath() + "_node";
        path += std::to_string(core::ClusterManager::instance().thisNodeId());

        Settings::instance().setCapturePath(path, Settings::CapturePath::Mono);
        Settings::instance().setCapturePath(path, Settings::CapturePath::LeftStereo);
        Settings::instance().setCapturePath(path, Settings::CapturePath::RightStereo);
    }

    Window::makeSharedContextCurrent();

    //
    // Load Shaders
    //
    bool needsFxaa = false;
    for (int i = 0; i < thisNode.numberOfWindows(); ++i) {
        needsFxaa |= thisNode.window(i).useFXAA();
    }

    if (needsFxaa) {
        _fxaa = FXAAShader();
        _fxaa->shader = ShaderProgram("FXAAShader");
        _fxaa->shader.addShaderSource(core::shaders::FXAAVert, core::shaders::FXAAFrag);
        _fxaa->shader.createAndLinkProgram();
        _fxaa->shader.bind();

        const int id = _fxaa->shader.id();
        _fxaa->sizeX = glGetUniformLocation(id, "rt_w");
        const glm::ivec2 framebufferSize = window(0).framebufferResolution();
        glUniform1f(_fxaa->sizeX, static_cast<float>(framebufferSize.x));

        _fxaa->sizeY = glGetUniformLocation(id, "rt_h");
        glUniform1f(_fxaa->sizeY, static_cast<float>(framebufferSize.y));

        _fxaa->subPixTrim = glGetUniformLocation(id, "FXAA_SUBPIX_TRIM");
        glUniform1f(_fxaa->subPixTrim, FxaaSubPixTrim);

        _fxaa->subPixOffset = glGetUniformLocation(id, "FXAA_SUBPIX_OFFSET");
        glUniform1f(_fxaa->subPixOffset, FxaaSubPixOffset);

        glUniform1i(glGetUniformLocation(id, "tex"), 0);
        ShaderProgram::unbind();
    }

    // Used for overlays & mono.
    _fboQuad = ShaderProgram("FBOQuadShader");
    _fboQuad.addShaderSource(core::shaders::BaseVert, core::shaders::BaseFrag);
    _fboQuad.createAndLinkProgram();
    _fboQuad.bind();
    glUniform1i(glGetUniformLocation(_fboQuad.id(), "tex"), 0);
    ShaderProgram::unbind();

    _overlay = ShaderProgram("OverlayShader");
    _overlay.addShaderSource(core::shaders::OverlayVert, core::shaders::OverlayFrag);
    _overlay.createAndLinkProgram();
    _overlay.bind();
    glUniform1i(glGetUniformLocation(_overlay.id(), "Tex"), 0);
    ShaderProgram::unbind();

    if (_initOpenGLFn) {
        Logger::Info("Calling initialization callback");
        _initOpenGLFn();
    }

    // link all users to their viewports
    for (int i = 0; i < thisNode.numberOfWindows(); i++) {
        Window& win = thisNode.window(i);
        win.initOGL();
        for (int j = 0; j < win.numberOfViewports(); ++j) {
            win.viewport(j).linkUserName();
        }
    }

    updateFrustums();

#ifdef SGCT_HAS_TEXT
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

    for (int i = 0; i < thisNode.numberOfWindows(); ++i) {
        thisNode.window(i).initContextSpecificOGL();
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
    const bool hasNode = cm.thisNodeId() > -1 && cm.thisNodeId() < cm.numberOfNodes();

    if (hasNode && cm.thisNode().numberOfWindows() > 0) {
        Window::makeSharedContextCurrent();
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
    // There might not be any thisNode as its creation might have failed
    if (hasNode && core::ClusterManager::instance().numberOfNodes() > 0) {
        for (int i = 0; i < cm.thisNode().numberOfWindows(); ++i) {
            cm.thisNode().window(i).close();
        }
    }

    // close TCP connections
    Logger::Debug("Destroying network manager");
    core::NetworkManager::destroy();

    // Shared contex
    if (hasNode && cm.thisNode().numberOfWindows() > 0) {
        Window::makeSharedContextCurrent();
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
    if (hasNode && cm.thisNode().numberOfWindows() > 0) {
        cm.thisNode().window(0).makeOpenGLContextCurrent();
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

void Engine::initNetwork(core::NetworkManager::NetworkMode netMode) {
    core::ClusterManager& cm = core::ClusterManager::instance();
    // check in cluster configuration which it is
    if (netMode == core::NetworkManager::NetworkMode::Remote) {
        Logger::Debug("Matching ip address to find node in configuration");

        for (int i = 0; i < core::ClusterManager::instance().numberOfNodes(); i++) {
            // check ip
            const std::string& addr = cm.node(i).address();
            if (core::NetworkManager::instance().matchesAddress(addr)) {
                cm.setThisNodeId(i);
                Logger::Debug("Running in cluster mode as node %d", i);
                break;
            }
        }
    }
    else {
        Logger::Debug("Running locally as node %d", cm.thisNodeId());
    }

    // If the user has provided the node _id as an incorrect cmd argument
    if (cm.thisNodeId() < 0) {
        core::NetworkManager::destroy();
        throw Err(3001, "Computer is not a part of the cluster configuration");
    }
    if (cm.thisNodeId() >= cm.numberOfNodes()) {
        core::NetworkManager::destroy();
        throw Err(3002, "Requested node id was not found in the cluster configuration");
    }

    core::NetworkManager::instance().init();
}

void Engine::initWindows(Profile profile) {
    {
        int ver[3];
        glfwGetVersion(&ver[0], &ver[1], &ver[2]);
        Logger::Info("Using GLFW version %d.%d.%d", ver[0], ver[1], ver[2]);
    }

    auto [major, minor] = [](Profile p) -> std::pair<int, int> {
        switch (p) {
            case Profile::OpenGL_3_3_Core: return { 3, 3 };
            case Profile::OpenGL_4_0_Core: return { 4, 0 };
            case Profile::OpenGL_4_1_Core: return { 4, 1 };
            case Profile::OpenGL_4_2_Core: return { 4, 2 };
            case Profile::OpenGL_4_3_Core: return { 4, 3 };
            case Profile::OpenGL_4_4_Core: return { 4, 4 };
            case Profile::OpenGL_4_5_Core: return { 4, 5 };
            case Profile::OpenGL_4_6_Core: return { 4, 6 };
            default: throw std::logic_error("Missing case label");
        }
    }(profile);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (_createDebugContext) {
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    }

    if (_preWindowFn) {
        _preWindowFn();
    }

    core::Node& thisNode = core::ClusterManager::instance().thisNode();
    for (int i = 0; i < thisNode.numberOfWindows(); ++i) {
        GLFWwindow* s = i == 0 ? nullptr : thisNode.window(0).windowHandle();
        const bool isLastWindow = i == thisNode.numberOfWindows() - 1;
        thisNode.window(i).openWindow(s, isLastWindow);
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
    if (thisNode.numberOfWindows() > 0) {
        if (_contextCreationFn) {
            GLFWwindow* share = thisNode.window(0).windowHandle();
            _contextCreationFn(share);
        }
    }

    for (int i = 0; i < thisNode.numberOfWindows(); ++i) {
        thisNode.window(i).init();
    }

    //init draw buffer resolution
    waitForAllWindowsInSwapGroupToOpen();

    if (RunFrameLockCheckThread) {
        if (core::ClusterManager::instance().numberOfNodes() > 1) {
            _thread = std::make_unique<std::thread>(updateFrameLockLoop, nullptr);
        }
    }

    // init swap group if enabled
    if (thisNode.isUsingSwapGroups()) {
        Window::initNvidiaSwapGroups();
    }
}

void Engine::terminate() {
    _shouldTerminate = true;
}

void Engine::frameLockPreStage() {
    core::NetworkManager& nm = core::NetworkManager::instance();

    const double ts = glfwGetTime();
    // from server to clients
    using P = std::pair<double, double>;
    std::optional<P> minMax = nm.sync(core::NetworkManager::SyncMode::SendDataToClients);
    if (minMax) {
        addValue(_statistics.loopTimeMin, minMax->first);
        addValue(_statistics.loopTimeMax, minMax->second);
    }
    if (nm.isComputerServer()) {
        addValue(_statistics.syncTimes, static_cast<float>(glfwGetTime() - ts));
    }

    // run only on clients
    if (nm.isComputerServer() && !core::ClusterManager::instance().ignoreSync())
    {
        return;
    }

    // not server
    const double t0 = glfwGetTime();
    while (nm.isRunning() && _isRunning && !nm.isSyncComplete()) {
        std::unique_lock lk(core::mutex::FrameSync);
        core::NetworkManager::cond.wait(lk);

        if (glfwGetTime() - t0 <= 1.0) {
            continue;
        }

        // more than a second
        core::Network* c = nm.syncConnection(0);
        if (_printSyncMessage && !c->isUpdated()) {
            Logger::Info(
                "Waiting for master. frame send %d != recv %d\n\tSwap groups: %s\n\t"
                "Swap barrier: %s\n\tUniversal frame number: %u\n\tSGCT frame number: %u",
                c->sendFrameCurrent(), c->recvFramePrevious(),
                Window::isUsingSwapGroups() ? "enabled" : "disabled",
                Window::isBarrierActive() ? "enabled" : "disabled",
                Window::swapGroupFrameNumber(), _frameCounter
            );
        }

        if (glfwGetTime() - t0 > _syncTimeout) {
            const std::string s = std::to_string(_syncTimeout);
            throw Err(3003, "No sync signal from master after " + s + " s");
        }
    }

    // A this point all data needed for rendering a frame is received.
    // Let's signal that back to the master/server.
    nm.sync(core::NetworkManager::SyncMode::Acknowledge);
    if (!nm.isComputerServer()) {
        addValue(_statistics.syncTimes, glfwGetTime() - t0);
    }
}

void Engine::frameLockPostStage() {
    core::NetworkManager& nm = core::NetworkManager::instance();
    // post stage
    if (core::ClusterManager::instance().ignoreSync() ||
        !nm.isComputerServer())
    {
        return;
    }

    const double t0 = glfwGetTime();
    while (nm.isRunning() && _isRunning && nm.activeConnectionsCount() > 0 &&
          !nm.isSyncComplete())
    {
        std::unique_lock lk(core::mutex::FrameSync);
        core::NetworkManager::cond.wait(lk);

        if (glfwGetTime() - t0 <= 1.0) {
            continue;
        }
        // more than a second
        for (int i = 0; i < nm.syncConnectionsCount(); ++i) {
            if (_printSyncMessage && !nm.connection(i).isUpdated()) {
                Logger::Info(
                    "Waiting for IG%d: send frame %d != recv frame %d\n\tSwap groups: %s"
                    "\n\tSwap barrier: %s\n\tUniversal frame number: %u\n\t"
                    "SGCT frame number: %u",
                    i, nm.connection(i).sendFrameCurrent(),
                    nm.connection(i).recvFrameCurrent(),
                    Window::isUsingSwapGroups() ? "enabled" : "disabled",
                    Window::isBarrierActive() ? "enabled" : "disabled",
                    Window::swapGroupFrameNumber(), _frameCounter
                );
            }
        }

        if (glfwGetTime() - t0 > _syncTimeout) {
            const std::string s = std::to_string(_syncTimeout);
            throw Err(3004, "No sync signal from clients after " + s + " s");
        }
    }

    addValue(_statistics.syncTimes, glfwGetTime() - t0);
}

void Engine::render() {
    _isRunning = true;

    Window::makeSharedContextCurrent();
    unsigned int timeQueryBegin = 0;
    glGenQueries(1, &timeQueryBegin);
    unsigned int timeQueryEnd = 0;
    glGenQueries(1, &timeQueryEnd);

    core::Node& thisNode = core::ClusterManager::instance().thisNode();
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

        for (int i = 0; i < thisNode.numberOfWindows(); ++i) {
            thisNode.window(i).update();
        }

        _renderingOffScreen = true;
        Window::makeSharedContextCurrent();

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
        for (int i = 0; i < thisNode.numberOfWindows(); ++i) {
            Window& win = thisNode.window(i);
            if (!(win.isVisible() || win.isRenderingWhileHidden())) {
                continue;
            }

            if (!_renderingOffScreen) {
                win.makeOpenGLContextCurrent();
            }

            Window::StereoMode sm = win.stereoMode();

            // Render Left/Mono non-linear projection viewports to cubemap
            for (int j = 0; j < win.numberOfViewports(); ++j) {
                const core::Viewport& vp = win.viewport(j);
                if (!vp.hasSubViewports()) {
                    continue;
                }

                core::NonLinearProjection* nonLinearProj = vp.nonLinearProjection();

                nonLinearProj->setAlpha(win.hasAlpha() ? 0.f : 1.f);
                if (sm == Window::StereoMode::NoStereo) {
                    // for mono viewports frustum mode can be selected by user or xml
                    nonLinearProj->renderCubemap(win, win.viewport(j).eye());
                }
                else {
                    nonLinearProj->renderCubemap(win, Frustum::Mode::StereoLeftEye);
                }
            }

            // Render left/mono regular viewports to FBO
            // if any stereo type (except passive) then set frustum mode to left eye
            if (sm == Window::StereoMode::NoStereo) {
                renderViewports(
                    win,
                    Frustum::Mode::MonoEye,
                    Window::TextureIndex::LeftEye
                );
            }
            else {
                renderViewports(
                    win,
                    Frustum::Mode::StereoLeftEye,
                    Window::TextureIndex::LeftEye
                );
            }

            // if we are not rendering in stereo, we are done
            if (sm == Window::StereoMode::NoStereo) {
                continue;
            }

            // Render right non-linear projection viewports to cubemap
            for (int j = 0; j < win.numberOfViewports(); ++j) {
                const core::Viewport& vp = win.viewport(j);

                if (!vp.hasSubViewports()) {
                    continue;
                }
                core::NonLinearProjection* p = vp.nonLinearProjection();

                p->setAlpha(win.hasAlpha() ? 0.f : 1.f);
                p->renderCubemap(win, Frustum::Mode::StereoRightEye);
            }

            // Render right regular viewports to FBO
            // use a single texture for side-by-side and top-bottom stereo modes
            if (sm >= Window::StereoMode::SideBySide) {
                renderViewports(
                    win,
                    Frustum::Mode::StereoRightEye,
                    Window::TextureIndex::LeftEye
                );
            }
            else {
                renderViewports(
                    win,
                    Frustum::Mode::StereoRightEye,
                    Window::TextureIndex::RightEye
                );
            }
        }

        // Render to screen
        for (int i = 0; i < thisNode.numberOfWindows(); ++i) {
            if (thisNode.window(i).isVisible()) {
                _renderingOffScreen = false;
                renderFBOTexture(window(i));
            }
        }
        Window::makeSharedContextCurrent();

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
        for (int i = 0; i < thisNode.numberOfWindows(); ++i) {
            thisNode.window(i).swap(_takeScreenshot);
        }

        glfwPollEvents();
        for (int i = 0; i < thisNode.numberOfWindows(); ++i) {
            thisNode.window(i).updateResolutions();
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

    Window::makeSharedContextCurrent();
    glDeleteQueries(1, &timeQueryBegin);
    glDeleteQueries(1, &timeQueryEnd);
}

void Engine::drawOverlays(const Window& window, Frustum::Mode frustum) {
    for (int i = 0; i < window.numberOfViewports(); ++i) {
        const core::Viewport& vp = window.viewport(i);
        
        // if viewport has overlay
        if (!vp.hasOverlayTexture() || !vp.isEnabled()) {
            continue;
        }

        setupViewport(window, vp, frustum);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, vp.overlayTextureIndex());
        _overlay.bind();
        window.renderScreenQuad();
    }
    ShaderProgram::unbind();
}

void Engine::renderFBOTexture(Window& window) {
    core::OffScreenBuffer::unbind();

    window.makeOpenGLContextCurrent();

    glDisable(GL_BLEND);
    // needed for shaders
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Frustum::Mode frustum = (window.stereoMode() == Window::StereoMode::Active) ?
        Frustum::Mode::StereoLeftEye :
        Frustum::Mode::MonoEye;

    const glm::ivec2 size = glm::ivec2(
        glm::ceil(window.scale() * glm::vec2(window.resolution()))
    );

    glViewport(0, 0, size.x, size.y);
    setAndClearBuffer(window, BufferMode::BackBufferBlack, frustum);
   
    Window::StereoMode sm = window.stereoMode();
    bool maskShaderSet = false;
    if (sm > Window::StereoMode::Active && sm < Window::StereoMode::SideBySide) {
        window.bindStereoShaderProgram(
            window.frameBufferTexture(Window::TextureIndex::LeftEye),
            window.frameBufferTexture(Window::TextureIndex::RightEye)
        );

        for (int i = 0; i < window.numberOfViewports(); i++) {
            window.viewport(i).renderWarpMesh();
            //window.getViewport(i).renderQuadMesh();
        }
    }
    else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(
            GL_TEXTURE_2D,
            window.frameBufferTexture(Window::TextureIndex::LeftEye)
        );

        _fboQuad.bind();
        maskShaderSet = true;

        for (int i = 0; i < window.numberOfViewports(); ++i) {
            window.viewport(i).renderWarpMesh();
            //win.getViewport(i).renderQuadMesh();
        }

        // render right eye in active stereo mode
        if (window.stereoMode() == Window::StereoMode::Active) {
            glViewport(0, 0, size.x, size.y);
            
            // clear buffers
            setAndClearBuffer(
                window,
                BufferMode::BackBufferBlack,
                Frustum::Mode::StereoRightEye
            );

            glBindTexture(
                GL_TEXTURE_2D,
                window.frameBufferTexture(Window::TextureIndex::RightEye)
            );
            for (int i = 0; i < window.numberOfViewports(); ++i) {
                window.viewport(i).renderWarpMesh();
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
        for (int i = 0; i < window.numberOfViewports(); ++i) {
            const core::Viewport& vp = window.viewport(i);
            if (vp.hasBlendMaskTexture() && vp.isEnabled()) {
                glBindTexture(GL_TEXTURE_2D, vp.blendMaskTextureIndex());
                vp.renderMaskMesh();
            }
        }

        // render black level masks
        for (int i = 0; i < window.numberOfViewports(); ++i) {
            const core::Viewport& vp = window.viewport(i);
            if (vp.hasBlackLevelMaskTexture() && vp.isEnabled()) {
                glBindTexture(GL_TEXTURE_2D, vp.blackLevelMaskTextureIndex());

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

void Engine::renderViewports(Window& win, Frustum::Mode frustum,
                             Window::TextureIndex ti)
{
    prepareBuffer(win, ti);

    Window::StereoMode sm = win.stereoMode();
    // render all viewports for selected eye
    for (int i = 0; i < win.numberOfViewports(); ++i) {
        core::Viewport& vp = win.viewport(i);

        if (!vp.isEnabled()) {
            continue;
        }

        // if passive stereo or mono
        if (sm == Window::StereoMode::NoStereo) {
            // @TODO (abock, 2019-12-04) Not sure about this one; the frustum is set in
            // the calling function based on the stereo mode already and we are 
            // overwriting it here
            frustum = vp.eye();
        }

        if (vp.hasSubViewports()) {
            if (vp.isTracked()) {
                vp.nonLinearProjection()->updateFrustums(
                    frustum,
                    _nearClipPlane,
                    _farClipPlane
                );
            }

            if (win.shouldCallDraw3DFunction()) {
                vp.nonLinearProjection()->render(win, vp, frustum);
            }
        }
        else {
            // no subviewports
            if (vp.isTracked()) {
                vp.calculateFrustum(frustum, _nearClipPlane, _farClipPlane);
            }

            // check if we want to blit the previous window before we do anything else
            if (win.shouldBlitPreviousWindow()) {
                if (win.id() == 0) {
                    Logger::Error("Cannot blit into first window");
                }
                else {
                    const int prevId = win.id() - 1;
                    Window& prevWindow = window(prevId);
                    blitPreviousWindowViewport(prevWindow, win, vp, frustum);
                }
            }

            if (win.shouldCallDraw3DFunction()) {
                // run scissor test to prevent clearing of entire buffer
                setupViewport(win, vp, frustum);
                glEnable(GL_SCISSOR_TEST);
                setAndClearBuffer(win, BufferMode::RenderToTexture, frustum);
                glDisable(GL_SCISSOR_TEST);

                if (_drawFn) {
                    RenderData renderData(
                        win,
                        vp,
                        frustum,
                        core::ClusterManager::instance().sceneTransform(),
                        vp.projection(frustum).viewMatrix(),
                        vp.projection(frustum).projectionMatrix(),
                        vp.projection(frustum).viewProjectionMatrix() *
                            core::ClusterManager::instance().sceneTransform()
                    );
                    _drawFn(renderData);
                }
            }
        }
    }

    // If we did not render anything, make sure we clear the screen at least
    if (!win.shouldCallDraw3DFunction() && !win.shouldBlitPreviousWindow()) {
        setAndClearBuffer(win, BufferMode::RenderToTexture, frustum);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    // for side-by-side or top-bottom mode, do postfx/blit only after rendering right eye
    const bool isSplitScreen = (sm >= Window::StereoMode::SideBySide);
    if (!(isSplitScreen && frustum == Frustum::Mode::StereoLeftEye)) {
        render2D(win, frustum);
        if (isSplitScreen) {
            // render left eye info and graph to render 2D items after post fx
            render2D(win, Frustum::Mode::StereoLeftEye);
        }

        updateRenderingTargets(win, ti); // only used if multisampled FBOs
    }

    glDisable(GL_BLEND);
}

void Engine::render2D(const Window& win, Frustum::Mode frustum) {
    // draw viewport overlays if any
    drawOverlays(win, frustum);

    // draw info & stats
    // the cubemap viewports are all the same so it makes no sense to render everything
    // several times therefore just loop one iteration in that case.
    if (!(_statisticsRenderer || _draw2DFn)) {
        return;
    }

    for (int i = 0; i < win.numberOfViewports(); ++i) {
        const core::Viewport& vp = win.viewport(i);
        if (!vp.isEnabled()) {
            continue;
        }
        setupViewport(win, vp, frustum);

        if (_statisticsRenderer) {
            _statisticsRenderer->render(win, vp);
        }

        // Check if we should call the use defined draw2D function
        if (_draw2DFn && win.shouldCallDraw2DFunction()) {
            RenderData renderData(
                win,
                vp,
                frustum,
                core::ClusterManager::instance().sceneTransform(),
                vp.projection(frustum).viewMatrix(),
                vp.projection(frustum).projectionMatrix(),
                vp.projection(frustum).viewProjectionMatrix() *
                    core::ClusterManager::instance().sceneTransform()
            );
            _draw2DFn(renderData);
        }
    }
}

void Engine::renderPostFX(Window& window, Window::TextureIndex targetIndex) {
    if (window.useFXAA()) {
        assert(_fxaa.has_value());

        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        // bind target FBO
        window.fbo()->attachColorTexture(window.frameBufferTexture(targetIndex));

        glm::ivec2 framebufferSize = window.framebufferResolution();
        glViewport(0, 0, framebufferSize.x, framebufferSize.y);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);

        glBindTexture(
            GL_TEXTURE_2D,
            window.frameBufferTexture(Window::TextureIndex::Intermediate)
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

bool Engine::isMaster() const {
    return core::NetworkManager::instance().isComputerServer();
}

unsigned int Engine::currentFrameNumber() const {
    return _frameCounter;
}

void Engine::waitForAllWindowsInSwapGroupToOpen() {
    core::ClusterManager& cm = core::ClusterManager::instance();
    core::Node& thisNode = cm.thisNode();

    // clear the buffers initially
    for (int i = 0; i < thisNode.numberOfWindows(); ++i) {
        thisNode.window(i).makeOpenGLContextCurrent();
        glDrawBuffer(thisNode.window(i).isDoubleBuffered() ? GL_BACK : GL_FRONT);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (thisNode.window(i).isDoubleBuffered()) {
            glfwSwapBuffers(thisNode.window(i).windowHandle());
        }
        else {
            glFinish();
        }
    }
    glfwPollEvents();
    
    // Must wait until all nodes are running if using swap barrier
    if (cm.ignoreSync() || cm.numberOfNodes() <= 1) {
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

    while (!core::NetworkManager::instance().areAllNodesConnected()) {
        // Swap front and back rendering buffers
        for (int i = 0; i < thisNode.numberOfWindows(); i++) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            if (thisNode.window(i).isDoubleBuffered()) {
                glfwSwapBuffers(thisNode.window(i).windowHandle());
            }
            else {
                glFinish();
            }
        }
        glfwPollEvents();

        if (_shouldTerminate || !core::NetworkManager::instance().isRunning() ||
            thisNode.closeAllWindows())
        {
            exit(0);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Engine::updateFrustums() {
    core::Node& thisNode = core::ClusterManager::instance().thisNode();
    for (int i = 0; i < thisNode.numberOfWindows(); i++) {
        Window& win = thisNode.window(i);
        for (int j = 0; j < win.numberOfViewports(); j++) {
            core::Viewport& vp = win.viewport(j);
            if (vp.isTracked()) {
                // if not tracked update, otherwise this is done on the fly
                continue;
            }

            using Mode = Frustum::Mode;
            if (vp.hasSubViewports()) {
                core::NonLinearProjection& p = *vp.nonLinearProjection();
                p.updateFrustums(Mode::MonoEye, _nearClipPlane, _farClipPlane);
                p.updateFrustums(Mode::StereoLeftEye, _nearClipPlane, _farClipPlane);
                p.updateFrustums(Mode::StereoRightEye, _nearClipPlane, _farClipPlane);
            }
            else {
                vp.calculateFrustum(Mode::MonoEye, _nearClipPlane, _farClipPlane);
                vp.calculateFrustum(Mode::StereoLeftEye, _nearClipPlane, _farClipPlane);
                vp.calculateFrustum(Mode::StereoRightEye, _nearClipPlane, _farClipPlane);
            }
        }
    }
}

void Engine::blitPreviousWindowViewport(Window& prevWindow, Window& window,
                                        const core::Viewport& viewport,
                                        Frustum::Mode mode)
{
    // run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);
    setupViewport(window, viewport, mode);
    setAndClearBuffer(window, BufferMode::RenderToTexture, mode);
    glDisable(GL_SCISSOR_TEST);

    _overlay.bind();

    glActiveTexture(GL_TEXTURE0);
    Window::TextureIndex m = [](Frustum::Mode v) {
        switch (v) {
            // @TODO (abock, 2019-09-27) Yep, I'm confused about this mapping, too. But I
            // just took the enumerations values as they were and I assume that it was an
            // undetected bug
            case Frustum::Mode::MonoEye: return Window::TextureIndex::LeftEye;
            case Frustum::Mode::StereoLeftEye: return Window::TextureIndex::RightEye;
            case Frustum::Mode::StereoRightEye: return Window::TextureIndex::Intermediate;
            default: throw std::logic_error("Unhandled case label");
        }
    }(mode);
    glBindTexture(GL_TEXTURE_2D, prevWindow.frameBufferTexture(m));

    window.renderScreenQuad();
    ShaderProgram::unbind();
}

void Engine::setupViewport(const Window& window, const core::BaseViewport& viewport,
                           Frustum::Mode frustum)
{
    const glm::vec2 res = glm::vec2(window.framebufferResolution());
    const glm::vec2 p = viewport.position() * res;
    const glm::vec2 s = viewport.size() * res;
    glm::ivec4 vpCoordinates = glm::ivec4(glm::ivec2(p), glm::ivec2(s));

    Window::StereoMode sm = window.stereoMode();
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

const Engine::Statistics& Engine::statistics() const {
    return _statistics;
}

double Engine::dt() const {
    return _statistics.frametimes[0];
}

double Engine::avgFPS() const {
    return 1.0 / avgDt();
}

double Engine::avgDt() const {
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

double Engine::minDt() const {
    return *std::min_element(
        _statistics.frametimes.begin(),
        _statistics.frametimes.end()
    );
}

double Engine::maxDt() const {
    return *std::max_element(
        _statistics.frametimes.begin(),
        _statistics.frametimes.end()
    );
}

glm::vec4 Engine::clearColor() const {
    return _clearColor;
}

float Engine::nearClipPlane() const {
    return _nearClipPlane;
}

float Engine::farClipPlane() const {
    return _farClipPlane;
}

void Engine::setNearAndFarClippingPlanes(float nearClip, float farClip) {
    _nearClipPlane = nearClip;
    _farClipPlane = farClip;
    updateFrustums();
}

void Engine::setEyeSeparation(float eyeSeparation) {
    core::Node& thisNode = core::ClusterManager::instance().thisNode();
    for (int w = 0; w < thisNode.numberOfWindows(); w++) {
        Window& window = thisNode.window(w);

        for (int i = 0; i < window.numberOfViewports(); i++) {
            window.viewport(i).user().setEyeSeparation(eyeSeparation);
        }
    }
    updateFrustums();
}

void Engine::setClearColor(glm::vec4 color) {
    _clearColor = std::move(color);
}

const Window* Engine::focusedWindow() const {
    const core::Node& thisNode = core::ClusterManager::instance().thisNode();
    for (int i = 0; i < thisNode.numberOfWindows(); i++) {
        if (thisNode.window(i).isFocused()) {
            return &thisNode.window(i);
        }
    }
    return nullptr; // no window has focus
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

const std::function<void(RenderData)>& Engine::drawFunction() const {
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
    if (nm.externalControlConnection()) {
        nm.externalControlConnection()->sendData(data, length);
    }
}

void Engine::transferDataBetweenNodes(const void* data, int length, int packageId) {
    core::NetworkManager::instance().transferData(data, length, packageId);
}

bool Engine::isExternalControlConnected() const {
    return (core::NetworkManager::instance().externalControlConnection() &&
        core::NetworkManager::instance().externalControlConnection()->isConnected());
}

const core::Node& Engine::thisNode() const {
    return core::ClusterManager::instance().thisNode();
}

Window& Engine::window(int index) const {
    return core::ClusterManager::instance().thisNode().window(index);
}

int Engine::numberOfWindows() const {
    return core::ClusterManager::instance().thisNode().numberOfWindows();
}

core::User& Engine::defaultUser() {
    return core::ClusterManager::instance().defaultUser();
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

unsigned int Engine::screenShotNumber() const {
    return _shotCounter;
}

} // namespace sgct
