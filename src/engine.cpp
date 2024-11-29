/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/engine.h>
#include <sgct/clustermanager.h>
#include <sgct/commandline.h>
#include <sgct/config.h>
#include <sgct/error.h>
#include <sgct/font.h>
#include <sgct/fontmanager.h>
#include <sgct/format.h>
#include <sgct/freetype.h>
#include <sgct/internalshaders.h>
#include <sgct/networkmanager.h>
#include <sgct/node.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/profiling.h>
#include <sgct/screencapture.h>
#include <sgct/shadermanager.h>
#include <sgct/shareddata.h>
#include <sgct/statisticsrenderer.h>
#include <sgct/texturemanager.h>
#ifdef SGCT_HAS_VRPN
#include <sgct/trackingmanager.h>
#endif // SGCT_HAS_VRPN
#include <sgct/user.h>
#include <sgct/version.h>
#include <sgct/projection/nonlinearprojection.h>
#include <cassert>
#include <iostream>
#include <numeric>
#include <cmath>

#ifdef WIN32
#include <glad/glad_wgl.h>
#else // ^^^^ WIN32 // !WIN32 vvvv
#include <glad/glad.h>
#endif // WIN32

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define Err(code, msg) Error(Error::Component::Engine, code, msg)

namespace sgct {

namespace {
    constexpr bool RunFrameLockCheckThread = true;
    constexpr std::chrono::milliseconds FrameLockTimeout(100);

    constexpr float FxaaSubPixTrim = 1.f / 4.f;
    constexpr float FxaaSubPixOffset = 1.f / 2.f;

    enum class BufferMode { BackBufferBlack, RenderToTexture };

    bool sRunUpdateFrameLockLoop = true;
    std::mutex FrameSync;

    // Callback wrappers for GLFW
    std::function<void(Key, Modifier, Action, int, Window*)> gKeyboardCallback = nullptr;
    std::function<void(unsigned int, int, Window*)> gCharCallback = nullptr;
    std::function<void(MouseButton, Modifier, Action, Window*)> gMouseButtonCallback = nullptr;
    std::function<void(double, double, Window*)> gMousePosCallback = nullptr;
    std::function<void(double, double, Window*)> gMouseScrollCallback = nullptr;
    std::function<void(std::vector<std::string_view>)> gDropCallback = nullptr;

    // For feedback: breaks a frame lock wait condition every time interval
    // (FrameLockTimeout) in order to print waiting message.
    void updateFrameLockLoop(void*) {
        bool run = true;

        while (run) {
            FrameSync.lock();
            run = sRunUpdateFrameLockLoop;
            FrameSync.unlock();
            NetworkManager::cond.notify_all();
            std::this_thread::sleep_for(FrameLockTimeout);
        }
    }

    void addValue(std::array<double, Engine::Statistics::HistoryLength>& a, double v) {
        std::rotate(std::rbegin(a), std::rbegin(a) + 1, std::rend(a));
        a[0] = v;
    }

    void setAndClearBuffer(const Window& window, BufferMode buffer, Frustum::Mode frustum)
    {
        ZoneScoped;

        if (buffer == BufferMode::BackBufferBlack) {
            // Set buffer
            if (window.stereoMode() != Window::StereoMode::Active) {
                glDrawBuffer(GL_BACK);
                glReadBuffer(GL_BACK);
            }
            else if (frustum == Frustum::Mode::StereoLeftEye) {
                // if active left
                glDrawBuffer(GL_BACK_LEFT);
                glReadBuffer(GL_BACK_LEFT);
            }
            else if (frustum == Frustum::Mode::StereoRightEye) {
                // if active right
                glDrawBuffer(GL_BACK_RIGHT);
                glReadBuffer(GL_BACK_RIGHT);
            }

            // when rendering textures to backbuffer (using fbo)
            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        else {
            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
    }

    void updateRenderingTargets(const Window& win, Window::TextureIndex ti) {
        ZoneScoped;

        // copy AA-buffer to "regular" / non-AA buffer
        OffScreenBuffer* fbo = win.fbo();
        if (!fbo->isMultiSampled()) {
            return;
        }

        // bind separate read and draw buffers to prepare blit operation
        fbo->bindBlit();

        // update attachments
        fbo->attachColorTexture(win.frameBufferTexture(ti), GL_COLOR_ATTACHMENT0);

        if (Engine::instance().useDepthTexture()) {
            fbo->attachDepthTexture(win.frameBufferTexture(Window::TextureIndex::Depth));
        }

        if (Engine::instance().useNormalTexture()) {
            fbo->attachColorTexture(
                win.frameBufferTexture(Window::TextureIndex::Normals),
                GL_COLOR_ATTACHMENT1
            );
        }

        if (Engine::instance().usePositionTexture()) {
            fbo->attachColorTexture(
                win.frameBufferTexture(Window::TextureIndex::Positions),
                GL_COLOR_ATTACHMENT2
            );
        }

        fbo->blit();
    }
} // namespace

double Engine::Statistics::dt() const {
    return frametimes.front();
}

double Engine::Statistics::avgDt() const {
    const double accFT = std::accumulate(frametimes.begin(), frametimes.end(), 0.0);
    const int nValues = static_cast<int>(std::count_if(
        frametimes.cbegin(),
        frametimes.cend(),
        [](double d) { return d != 0.0; }
    ));
    // We must take the frame counter into account as the history might not be filled yet
    const unsigned int frameCounter = Engine::instance().currentFrameNumber();
    const unsigned f = std::clamp<unsigned int>(frameCounter, 1, nValues);
    return accFT / f;
}

double Engine::Statistics::minDt() const {
    return *std::min_element(frametimes.begin(), frametimes.end());
}

double Engine::Statistics::maxDt() const {
    return *std::max_element(frametimes.begin(), frametimes.end());
}

Engine* Engine::_instance = nullptr;

Engine& Engine::instance() {
    if (_instance == nullptr) {
        throw std::logic_error("Using the instance before it was created or set");
    }
    return *_instance;
}

void Engine::create(config::Cluster cluster, Callbacks callbacks,
                    const Configuration& arg)
{
    ZoneScoped;

    if (_instance) {
        throw std::logic_error("Creating the instance when one already existed");
    }

    // (2019-12-02, abock) Unfortunately I couldn't find a better why than using this two
    // phase initialization approach. There are a few callbacks in the second phase that
    // are calling out to client code that (rightly) assumes that the Engine has been
    // created and are calling Engine::instance from they registered callbacks. If this
    // client code is executed from the constructor, the _instance variable has not yet
    // been set and will therefore cause the logic_error in the instance() function.
    _instance = new Engine(std::move(cluster), std::move(callbacks), arg);
    _instance->initialize();
}

void Engine::destroy() {
    ZoneScoped;

    delete _instance;
    _instance = nullptr;
}

config::Cluster loadCluster(std::optional<std::filesystem::path> path) {
    ZoneScoped;

    if (path) {
        assert(std::filesystem::exists(*path) && std::filesystem::is_regular_file(*path));
        try {
            Log::Debug(std::format("Parsing config '{}'", *path));
            config::Cluster cluster = readConfig(*path);

            Log::Debug("Config file read successfully");
            Log::Info(std::format("Number of nodes: {}", cluster.nodes.size()));

            for (size_t i = 0; i < cluster.nodes.size(); i++) {
                const config::Node& node = cluster.nodes[i];
                Log::Info(std::format(
                    "\tNode ({}) address: {} [{}]", i, node.address, node.port
                ));
            }
            return cluster;
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
        cluster.success = true;

        // Create a default configuration
        constexpr float hFov = 90.f;
        constexpr float vFov = hFov / (16.f / 9.f);

        sgct::config::PlanarProjection proj;
        sgct::config::PlanarProjection::FOV fov;
        fov.down = -vFov / 2.f;
        fov.up = vFov / 2.f;
        fov.left = -hFov / 2.f;
        fov.right = hFov / 2.f;
        proj.fov = fov;

        sgct::config::Viewport viewport;
        viewport.projection = proj;

        sgct::config::Window window;
        window.id = 0;
        window.isFullScreen = false;
        window.size = ivec2{ 1280, 720 };
        window.viewports.push_back(viewport);

        sgct::config::Node node;
        node.address = "localhost";
        node.port = 20401;
        node.windows.push_back(window);

        sgct::config::User user;
        user.eyeSeparation = 0.06f;
        user.position = vec3{ 0.f, 0.f, 0.f };
        cluster.users.push_back(user);

        cluster.masterAddress = "localhost";
        cluster.nodes.push_back(node);
        return cluster;
    }
}

double time() {
    return glfwGetTime();
}

Engine::Engine(config::Cluster cluster, Callbacks callbacks, const Configuration& config)
    : _preWindowFn(std::move(callbacks.preWindow))
    , _initOpenGLFn(std::move(callbacks.initOpenGL))
    , _preSyncFn(std::move(callbacks.preSync))
    , _postSyncPreDrawFn(std::move(callbacks.postSyncPreDraw))
    , _drawFn(std::move(callbacks.draw))
    , _draw2DFn(std::move(callbacks.draw2D))
    , _postDrawFn(std::move(callbacks.postDraw))
    , _cleanupFn(std::move(callbacks.cleanup))
{
    ZoneScoped;

    SharedData::instance().setEncodeFunction(std::move(callbacks.encode));
    SharedData::instance().setDecodeFunction(std::move(callbacks.decode));

    gKeyboardCallback = std::move(callbacks.keyboard);
    gCharCallback = std::move(callbacks.character);
    gMouseButtonCallback = std::move(callbacks.mouseButton);
    gMousePosCallback = std::move(callbacks.mousePos);
    gMouseScrollCallback = std::move(callbacks.mouseScroll);
    gDropCallback = std::move(callbacks.drop);

    NetworkManager::NetworkMode netMode = NetworkManager::NetworkMode::Remote;
    if (config.isServer) {
        netMode = *config.isServer ?
            NetworkManager::NetworkMode::LocalServer :
            NetworkManager::NetworkMode::LocalClient;
    }
    if (config.logLevel) {
        Log::instance().setNotifyLevel(*config.logLevel);
    }
    if (config.showHelpText) {
        std::cout << helpMessage() << '\n';
        std::exit(0);
    }

    if (config.firmSync) {
        cluster.firmSync = config.firmSync;
    }
    _settings.capture.nCaptureThreads =
        config.nCaptureThreads.value_or(_settings.capture.nCaptureThreads);
    _settings.createDebugContext =
        config.useOpenGLDebugContext.value_or(_settings.createDebugContext);
    _settings.capture.capturePath =
        config.screenshotPath.value_or(_settings.capture.capturePath);
    _settings.capture.prefix = config.screenshotPrefix.value_or(_settings.capture.prefix);
    _settings.capture.addNodeName =
        config.addNodeNameInScreenshot.value_or(_settings.capture.addNodeName);
    if (config.omitWindowNameInScreenshot) {
        _settings.capture.addWindowName = !(*config.omitWindowNameInScreenshot);
    }
    if (cluster.settings) {
        if (cluster.settings->display) {
            _settings.swapInterval = cluster.settings->display->swapInterval.value_or(
                _settings.swapInterval
            );
        }
        if (cluster.settings->useDepthTexture) {
            _settings.textures.useDepthTexture = *cluster.settings->useDepthTexture;
        }
        if (cluster.settings->useNormalTexture) {
            _settings.textures.useNormalTexture = *cluster.settings->useNormalTexture;
        }
        if (cluster.settings->usePositionTexture) {
            _settings.textures.usePositionTexture = *cluster.settings->usePositionTexture;
        }
    }
    if (cluster.capture) {
        if (cluster.capture->path) {
            _settings.capture.capturePath = cluster.capture->path.value_or(
                _settings.capture.capturePath
            );
            setScreenshotNumber(0);
        }

        if (cluster.capture->range) {
            _settings.capture.limits = {
                static_cast<uint64_t>(cluster.capture->range->first),
                static_cast<uint64_t>(cluster.capture->range->last)
            };
        }
    }
    if (cluster.setThreadAffinity) {
#ifdef WIN32
        SetThreadAffinityMask(GetCurrentThread(), *cluster.setThreadAffinity);
#else
        Log::Error("Using thread affinity on an operating system that is not supported");
#endif // WIN32
    }
    {
        ZoneScopedN("GLFW initialization");
        glfwSetErrorCallback(
            [](int error, const char* desc) {
                throw Err(3010, std::format("GLFW error ({}): {}", error, desc));
            }
        );
        const int res = glfwInit();
        if (res == GLFW_FALSE) {
            throw Err(3000, "Failed to initialize GLFW");
        }
    }

    Log::Info(std::format("SGCT version: {}", Version));

    Log::Debug("Validating cluster configuration");
    config::validateCluster(cluster);

    NetworkManager::create(
        netMode,
        std::move(callbacks.dataTransferDecode),
        std::move(callbacks.dataTransferStatus),
        std::move(callbacks.dataTransferAcknowledge)
    );
#ifdef SGCT_HAS_VRPN
    for (const config::Tracker& tracker : cluster.trackers) {
        TrackingManager::instance().applyTracker(tracker);
    }
#endif // SGCT_HAS_VRPN
    int clusterId = -1;
    // check in cluster configuration which it is
    if (netMode == NetworkManager::NetworkMode::Remote) {
        Log::Debug("Matching ip address to find node in configuration");

        for (size_t i = 0; i < cluster.nodes.size(); i++) {
            if (NetworkManager::instance().matchesAddress(cluster.nodes[i].address)) {
                clusterId = static_cast<int>(i);
                Log::Debug(std::format("Running in cluster mode as node {}", i));
                break;
            }
        }
    }
    else {
        if (config.nodeId) {
            if (*config.nodeId >= static_cast<int>(cluster.nodes.size())) {
                NetworkManager::destroy();
                throw Err(
                    3001, "Requested node id was not found in the cluster configuration"
                );
            }
            clusterId = *config.nodeId;
            Log::Debug(std::format("Running locally as node {}", clusterId));
        }
        else {
            throw Err(3002, "When running locally, a node ID needs to be specified");
        }
    }

    if (clusterId < 0) {
        NetworkManager::destroy();
        throw Err(3003, "Computer is not a part of the cluster configuration");
    }

    ClusterManager::create(cluster, clusterId);
    if (config.ignoreSync) {
        ClusterManager::instance().setUseIgnoreSync(*config.ignoreSync);
    }

    NetworkManager::instance().initialize();
}

void Engine::initialize() {
    ZoneScoped;

    int major = 0;
    int minor = 0;
    {
        ZoneScopedN("OpenGL Version");

        // Detect the available OpenGL version
#ifdef __APPLE__
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif // __APPLE__
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
        GLFWwindow* offscreen = glfwCreateWindow(128, 128, "", nullptr, nullptr);
        glfwMakeContextCurrent(offscreen);
        gladLoadGL();

        // Get the OpenGL version
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);

        // And get rid of the window again
        glfwDestroyWindow(offscreen);
        glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
    }
    Log::Info(std::format("Detected OpenGL version: {}.{}", major, minor));

    initWindows(major, minor);

    // Window resolution may have been set by the config. However, it only sets a pending
    // resolution, so it needs to apply it using the same routine as in the end of a frame
    const Node& thisNode = ClusterManager::instance().thisNode();
    const std::vector<std::unique_ptr<Window>>& wins = thisNode.windows();
    std::for_each(wins.cbegin(), wins.cend(), std::mem_fn(&Window::updateResolutions));

    // if a single node, skip syncing
    if (ClusterManager::instance().numberOfNodes() == 1) {
        ClusterManager::instance().setUseIgnoreSync(true);
    }

    for (const std::unique_ptr<Window>& window : wins) {
        GLFWwindow* win = window->windowHandle();
        if (gKeyboardCallback) {
            glfwSetKeyCallback(
                win,
                [](GLFWwindow* w, int key, int scancode, int a, int m) {
                    void* sgctWindow = glfwGetWindowUserPointer(w);
                    gKeyboardCallback(
                        Key(key),
                        Modifier(m),
                        Action(a),
                        scancode,
                        reinterpret_cast<Window*>(sgctWindow)
                    );
                }
            );
        }
        if (gMouseButtonCallback) {
            glfwSetMouseButtonCallback(
                win,
                [](GLFWwindow* w, int b, int a, int m) {
                    void* sgctWindow = glfwGetWindowUserPointer(w);
                    gMouseButtonCallback(
                        MouseButton(b),
                        Modifier(m),
                        Action(a),
                        reinterpret_cast<Window*>(sgctWindow)
                    );
                }
            );
        }
        if (gMousePosCallback) {
            glfwSetCursorPosCallback(
                win,
                [](GLFWwindow* w, double xPos, double yPos) {
                    void* sgctWindow = glfwGetWindowUserPointer(w);
                    gMousePosCallback(xPos, yPos, reinterpret_cast<Window*>(sgctWindow));
                }
            );
        }
        if (gCharCallback) {
            glfwSetCharModsCallback(
                win,
                [](GLFWwindow* w, unsigned int ch, int mod) {
                    void* sgctWindow = glfwGetWindowUserPointer(w);
                    gCharCallback(ch, mod, reinterpret_cast<Window*>(sgctWindow));
                }
            );
        }
        if (gMouseScrollCallback) {
            glfwSetScrollCallback(
                win,
                [](GLFWwindow* w, double xOffset, double yOffset) {
                    void* sgctWindow = glfwGetWindowUserPointer(w);
                    gMouseScrollCallback(
                        xOffset,
                        yOffset,
                        reinterpret_cast<Window*>(sgctWindow)
                    );
                }
            );
        }
        if (gDropCallback) {
            glfwSetDropCallback(
                win,
                [](GLFWwindow*, int count, const char** paths) {
                    std::vector<std::string_view> p;
                    p.reserve(count);
                    for (int i = 0; i < count; i++) {
                        p.emplace_back(paths[i]);
                    }
                    gDropCallback(std::move(p));
                }
            );
        }
    }

    // Get OpenGL version from the first window as there has to be one
    GLFWwindow* winHandle = wins[0]->windowHandle();
    const std::array<int, 3> v = {
        glfwGetWindowAttrib(winHandle, GLFW_CONTEXT_VERSION_MAJOR),
        glfwGetWindowAttrib(winHandle, GLFW_CONTEXT_VERSION_MINOR),
        glfwGetWindowAttrib(winHandle, GLFW_CONTEXT_REVISION)
    };
    Log::Info(std::format("OpenGL version {}.{}.{} core profile", v[0], v[1], v[2]));

    std::string vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    Log::Info(std::format("Vendor: {}", vendor));
    std::string renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    Log::Info(std::format("Renderer: {}", renderer));

    Window::makeSharedContextCurrent();

    //
    // Load Shaders
    const bool needsFxaa = std::any_of(
        wins.begin(),
        wins.end(),
        std::mem_fn(&Window::useFXAA)
    );
    if (needsFxaa) {
        ZoneScopedN("FXAA Shader");

        _fxaa = FXAAShader();
        _fxaa->shader = ShaderProgram("FXAAShader");
        _fxaa->shader.addShaderSource(shaders::FXAAVert, GL_VERTEX_SHADER);
        _fxaa->shader.addShaderSource(shaders::FXAAFrag, GL_FRAGMENT_SHADER);
        _fxaa->shader.createAndLinkProgram();
        _fxaa->shader.bind();

        const int id = _fxaa->shader.id();
        _fxaa->sizeX = glGetUniformLocation(id, "rt_w");
        const ivec2 framebufferSize = wins[0]->framebufferResolution();
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

    {
        ZoneScopedN("FBO Quad Shader");

        // Used for overlays & mono.
        _fboQuad = ShaderProgram("FBOQuadShader");
        _fboQuad.addShaderSource(shaders::BaseVert, GL_VERTEX_SHADER);
        _fboQuad.addShaderSource(shaders::BaseFrag, GL_FRAGMENT_SHADER);
        _fboQuad.createAndLinkProgram();
        _fboQuad.bind();
        glUniform1i(glGetUniformLocation(_fboQuad.id(), "tex"), 0);
        ShaderProgram::unbind();

        _overlay = ShaderProgram("OverlayShader");
        _overlay.addShaderSource(shaders::BaseVert, GL_VERTEX_SHADER);
        _overlay.addShaderSource(shaders::OverlayFrag, GL_FRAGMENT_SHADER);
        _overlay.createAndLinkProgram();
        _overlay.bind();
        glUniform1i(glGetUniformLocation(_overlay.id(), "tex"), 0);
        ShaderProgram::unbind();
    }

    if (_initOpenGLFn) {
        Log::Info("Calling initialization callback");
        ZoneScopedN("[SGCT] OpenGL Initialization");
        GLFWwindow* share = thisNode.windows().front()->windowHandle();
        _initOpenGLFn(share);
    }

    for (const std::unique_ptr<Window>& win : wins) {
        win->initOGL();
        const std::vector<std::unique_ptr<Viewport>>& vps = win->viewports();
        std::for_each(vps.cbegin(), vps.cend(), std::mem_fn(&Viewport::linkUserName));
    }

    updateFrustums();

#ifdef SGCT_HAS_TEXT
#ifdef WIN32
    constexpr std::string_view FontName = "verdanab.ttf";
#elif defined(__APPLE__)
    constexpr std::string_view FontName = "HelveticaNeue.ttc";
#else // !WIN32 && !__APPLE__
    constexpr std::string_view FontName = "FreeSansBold.ttf";
#endif // WIN32
    text::FontManager::instance().addFont("SGCTFont", std::string(FontName));
#endif // SGCT_HAS_TEXT

    // init draw buffer resolution
    waitForAllWindowsInSwapGroupToOpen();
    // init swap group if enabled
    if (thisNode.isUsingSwapGroups()) {
        Window::initNvidiaSwapGroups();
    }

    // init swap barrier is swap groups are active
    Window::setBarrier(true);
    Window::resetSwapGroupFrameNumber();

    std::for_each(wins.begin(), wins.end(), std::mem_fn(&Window::initContextSpecificOGL));

#ifdef SGCT_HAS_VRPN
    // start sampling tracking data
    if (isMaster()) {
        TrackingManager::instance().startSampling();
    }
#endif // SGCT_HAS_VRPN
}

Engine::~Engine() {
    Log::Info("Cleaning up");

    // First check whether we ever created a node for ourselves.  This might have failed
    // if the configuration was illformed
    const ClusterManager& cm = ClusterManager::instance();
    const bool hasNode = cm.thisNodeId() > -1 && cm.thisNodeId() < cm.numberOfNodes();
    if (hasNode) {
        Window::makeSharedContextCurrent();
        if (_cleanupFn) {
            _cleanupFn();
        }
    }

    // We are only clearing the callbacks that might be called asynchronously
    Log::Debug("Clearing callbacks");
    NetworkManager::instance().clearCallbacks();
    gKeyboardCallback = nullptr;
    gMouseButtonCallback = nullptr;
    gMousePosCallback = nullptr;
    gMouseScrollCallback = nullptr;
    gDropCallback = nullptr;

    // kill thread
    if (_thread) {
        Log::Debug("Waiting for frameLock thread to finish");

        FrameSync.lock();
        sRunUpdateFrameLockLoop = false;
        FrameSync.unlock();

        _thread->join();
        _thread = nullptr;
        Log::Debug("Done");
    }

    // de-init window and unbind swapgroups
    // There might not be any thisNode as its creation might have failed
    if (hasNode) {
        const std::vector<std::unique_ptr<Window>>& windows = cm.thisNode().windows();
        std::for_each(windows.cbegin(), windows.cend(), std::mem_fn(&Window::close));
    }

    // close TCP connections
    Log::Debug("Destroying network manager");
    NetworkManager::destroy();

    // Shared contex
    if (hasNode && !cm.thisNode().windows().empty()) {
        Window::makeSharedContextCurrent();
    }

    Log::Debug("Destroying shader manager and internal shaders");
    ShaderManager::destroy();

    if (hasNode) {
        _fboQuad.deleteProgram();
        if (_fxaa) {
            _fxaa->shader.deleteProgram();
        }
        _overlay.deleteProgram();
    }

    _statisticsRenderer = nullptr;

    Log::Debug("Destroying texture manager");
    TextureManager::destroy();

#ifdef SGCT_HAS_TEXT
    Log::Debug("Destroying font manager");
    text::FontManager::destroy();
#endif // SGCT_HAS_TEXT

    // Window specific context
    if (hasNode && !cm.thisNode().windows().empty()) {
        cm.thisNode().windows().front()->makeOpenGLContextCurrent();
    }

    Log::Debug("Destroying shared data");
    SharedData::destroy();

    Log::Debug("Destroying cluster manager");
    ClusterManager::destroy();

    Log::Debug("Destroying message handler");
    Log::destroy();

    Log::Debug("Terminating glfw");
    glfwTerminate();

    Log::Debug("Finished cleaning");
}

void Engine::initWindows(int majorVersion, int minorVersion) {
    ZoneScoped;

    assert(majorVersion > 0);
    assert(minorVersion > 0);

    {
        int major = 0;
        int minor = 0;
        int release = 0;
        glfwGetVersion(&major, &minor, &release);
        Log::Info(std::format("Using GLFW version {}.{}.{}", major, minor, release));
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, majorVersion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minorVersion);

    const Node& thisNode = ClusterManager::instance().thisNode();
    const std::vector<std::unique_ptr<Window>>& windows = thisNode.windows();

    bool needsCompatProfile = std::any_of(
        windows.cbegin(),
        windows.cend(),
        std::mem_fn(&Window::needsCompatibilityProfile)
    );
    
    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        needsCompatProfile ? GLFW_OPENGL_COMPAT_PROFILE : GLFW_OPENGL_CORE_PROFILE
    );

    if (_settings.createDebugContext) {
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    }

    if (_preWindowFn) {
        ZoneScopedN("[SGCT] Pre-window creation");
        _preWindowFn();
    }

    for (size_t i = 0; i < windows.size(); i++) {
        ZoneScopedN("Creating Window");

        GLFWwindow* s = (i == 0) ? nullptr : windows[0]->windowHandle();
        const bool isLastWindow = i == windows.size() - 1;
        windows[i]->openWindow(s, isLastWindow);
        gladLoadGL();
#ifdef WIN32
        gladLoadWGL(wglGetCurrentDC());
#endif // WIN32
        TracyGpuContext;
    }

    // clear directly otherwise junk will be displayed on some OSs (OS X Yosemite)
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (RunFrameLockCheckThread) {
        if (ClusterManager::instance().numberOfNodes() > 1) {
            _thread = std::make_unique<std::thread>(updateFrameLockLoop, nullptr);
        }
    }
}

void Engine::terminate() {
    _shouldTerminate = true;
}

void Engine::frameLockPreStage() {
    ZoneScoped;

    NetworkManager& nm = NetworkManager::instance();

    const double ts = glfwGetTime();
    // from server to clients
    using P = std::pair<double, double>;
    std::optional<P> minMax = nm.sync(NetworkManager::SyncMode::SendDataToClients);
    if (minMax) {
        addValue(_statistics.loopTimeMin, minMax->first);
        addValue(_statistics.loopTimeMax, minMax->second);
    }
    if (nm.isComputerServer()) {
        addValue(_statistics.syncTimes, static_cast<float>(glfwGetTime() - ts));
    }

    // run only on clients
    if (nm.isComputerServer() && !ClusterManager::instance().ignoreSync()) {
        return;
    }

    // not server
    const double t0 = glfwGetTime();
    while (nm.isRunning() && !nm.isSyncComplete()) {
        std::unique_lock lk(FrameSync);
        NetworkManager::cond.wait(lk);

        if (glfwGetTime() - t0 <= 1.0) {
            continue;
        }

        // more than a second
        const Network& c = nm.syncConnection(0);
        if (_printSyncMessage && !c.isUpdated()) {
            Log::Info(std::format(
                "Waiting for master. frame send {} != recv {}\n\tSwap groups: {}\n\t"
                "Swap barrier: {}\n\tUniversal frame number: {}\n\tSGCT frame number: {}",
                c.sendFrameCurrent(), c.recvFramePrevious(),
                Window::isUsingSwapGroups() ? "enabled" : "disabled",
                Window::isBarrierActive() ? "enabled" : "disabled",
                Window::swapGroupFrameNumber(), _frameCounter
            ));
        }

        if (glfwGetTime() - t0 > _syncTimeout) {
            const std::string s = std::to_string(_syncTimeout);
            throw Err(3004, std::format("No sync signal from master after {} s", s));
        }
    }

    // A this point all data needed for rendering a frame is received.
    // Let's signal that back to the master/server.
    nm.sync(NetworkManager::SyncMode::Acknowledge);
    if (!nm.isComputerServer()) {
        addValue(_statistics.syncTimes, glfwGetTime() - t0);
    }
}

void Engine::frameLockPostStage() {
    ZoneScoped;

    const NetworkManager& nm = NetworkManager::instance();
    // post stage
    if (ClusterManager::instance().ignoreSync() || !nm.isComputerServer()) {
        return;
    }

    const double t0 = glfwGetTime();
    while (nm.isRunning() && nm.activeConnectionsCount() > 0 && !nm.isSyncComplete()) {
        std::unique_lock lk(FrameSync);
        NetworkManager::cond.wait(lk);

        if (glfwGetTime() - t0 <= 1.0) {
            continue;
        }
        // more than a second
        for (int i = 0; i < nm.syncConnectionsCount(); i++) {
            if (_printSyncMessage && !nm.connection(i).isUpdated()) {
                Log::Info(std::format(
                    "Waiting for IG{}: send frame {} != recv frame {}\n\tSwap groups: {}"
                    "\n\tSwap barrier: {}\n\tUniversal frame number: {}\n\t"
                    "SGCT frame number: {}", i, nm.connection(i).sendFrameCurrent(),
                    nm.connection(i).recvFrameCurrent(),
                    Window::isUsingSwapGroups() ? "enabled" : "disabled",
                    Window::isBarrierActive() ? "enabled" : "disabled",
                    Window::swapGroupFrameNumber(), _frameCounter
                ));
            }
        }

        if (glfwGetTime() - t0 > _syncTimeout) {
            const std::string s = std::to_string(_syncTimeout);
            throw Err(3005, std::format("No sync signal from clients after {} s", s));
        }
    }

    addValue(_statistics.syncTimes, glfwGetTime() - t0);
}

void Engine::exec() {
    Window::makeSharedContextCurrent();

    unsigned int timeQueryBegin = 0;
    glGenQueries(1, &timeQueryBegin);
    unsigned int timeQueryEnd = 0;
    glGenQueries(1, &timeQueryEnd);

    Node& thisNode = ClusterManager::instance().thisNode();
    const std::vector<std::unique_ptr<Window>>& windows = thisNode.windows();
    while (!_shouldTerminate && !thisNode.closeAllWindows() &&
           NetworkManager::instance().isRunning()) [[unlikely]]
    {
#ifdef SGCT_HAS_VRPN
        if (isMaster()) {
            TrackingManager::instance().updateTrackingDevices();
        }
#endif // SGCT_HAS_VRPN

        {
            ZoneScopedN("GLFW Poll Events");
            glfwPollEvents();
        }

        Window::makeSharedContextCurrent();

        if (_preSyncFn) [[likely]] {
            ZoneScopedN("[SGCT] PreSync");
            _preSyncFn();
        }

        if (NetworkManager::instance().isComputerServer()) {
            SharedData::instance().encode();
        }
        else if (!NetworkManager::instance().isRunning()) {
            // exit if not running
            Log::Error("Network disconnected. Exiting");
            break;
        }

        frameLockPreStage();
        std::for_each(windows.cbegin(), windows.cend(), std::mem_fn(&Window::update));
        Window::makeSharedContextCurrent();

        if (_postSyncPreDrawFn) [[likely]] {
            ZoneScopedN("[SGCT] PostSyncPreDraw");
            _postSyncPreDrawFn();
        }

        {
            ZoneScopedN("Statistics update");
            const double startFrameTime = glfwGetTime();
            const double ft = static_cast<float>(startFrameTime - _statsPrevTimestamp);
            addValue(_statistics.frametimes, ft);
            _statsPrevTimestamp = startFrameTime;

            if (_statisticsRenderer) [[unlikely]] {
                glQueryCounter(timeQueryBegin, GL_TIMESTAMP);
            }
        }

        // Render Viewports / Draw
        for (const std::unique_ptr<Window>& win : windows) {
            ZoneScopedN("Render window");

            if (!(win->isVisible() || win->isRenderingWhileHidden())) [[unlikely]] {
                continue;
            }

            const Window::StereoMode sm = win->stereoMode();

            // Render Left/Mono non-linear projection viewports to cubemap
            for (const std::unique_ptr<Viewport>& vp : win->viewports()) {
                ZoneScopedN("Render viewport");

                if (!vp->hasSubViewports()) {
                    continue;
                }

                NonLinearProjection* nonLinearProj = vp->nonLinearProjection();
                if (sm == Window::StereoMode::NoStereo) {
                    // for mono viewports frustum mode can be selected by user or config
                    nonLinearProj->renderCubemap(vp->eye());
                }
                else {
                    nonLinearProj->renderCubemap(Frustum::Mode::StereoLeftEye);
                }
            }

            // Render left/mono regular viewports to FBO
            // if any stereo type (except passive) then set frustum mode to left eye
            if (sm == Window::StereoMode::NoStereo) {
                renderViewports(
                    *win,
                    Frustum::Mode::MonoEye,
                    Window::TextureIndex::LeftEye
                );

                // if we are not rendering in stereo, we are done
                continue;
            }
            else {
                renderViewports(
                    *win,
                    Frustum::Mode::StereoLeftEye,
                    Window::TextureIndex::LeftEye
                );
            }

            // Render right non-linear projection viewports to cubemap
            for (const std::unique_ptr<Viewport>& vp : win->viewports()) {
                ZoneScopedN("Render Cubemap");
                if (!vp->hasSubViewports()) {
                    continue;
                }
                NonLinearProjection* p = vp->nonLinearProjection();
                p->renderCubemap(Frustum::Mode::StereoRightEye);
            }

            // Render right regular viewports to FBO
            // use a single texture for side-by-side and top-bottom stereo modes
            if (sm >= Window::StereoMode::SideBySide) {
                renderViewports(
                    *win,
                    Frustum::Mode::StereoRightEye,
                    Window::TextureIndex::LeftEye
                );
            }
            else {
                renderViewports(
                    *win,
                    Frustum::Mode::StereoRightEye,
                    Window::TextureIndex::RightEye
                );
            }
        }

        // Render to screen
        for (const std::unique_ptr<Window>& window : windows) {
            if (window->isVisible()) [[likely]] {
                renderFBOTexture(*window);
            }
        }
        Window::makeSharedContextCurrent();

        if (_statisticsRenderer) [[unlikely]] {
            ZoneScopedN("glQueryCounter");
            glQueryCounter(timeQueryEnd, GL_TIMESTAMP);
        }

        if (_postDrawFn) [[likely]] {
            ZoneScopedN("[SGCT] PostDraw");
            _postDrawFn();
        }

        if (_statisticsRenderer) [[unlikely]] {
            ZoneScopedN("Statistics Update");
            // wait until the query results are available
            GLint done = GL_FALSE;
            while (!done) {
                glGetQueryObjectiv(timeQueryEnd, GL_QUERY_RESULT_AVAILABLE, &done);
            }

            // get the query results
            GLuint64 timerStart = 0;
            glGetQueryObjectui64v(timeQueryBegin, GL_QUERY_RESULT, &timerStart);
            GLuint64 timerEnd = 0;
            glGetQueryObjectui64v(timeQueryEnd, GL_QUERY_RESULT, &timerEnd);

            const double t = static_cast<double>(timerEnd - timerStart) / 1000000000.0;
            addValue(_statistics.drawTimes, t);

            _statisticsRenderer->update();
        }

        // master will wait for nodes render before swapping
        frameLockPostStage();
        // Swap front and back rendering buffers
        for (const std::unique_ptr<Window>& window : windows) {
            bool shouldTakeScreenshot = _shouldTakeScreenshot;

            // The window might want to opt out of taking screenshots
            shouldTakeScreenshot &= window->shouldTakeScreenshot();

            // If we don't want to take any screenshots anyway, there is no need for any
            // extra work. Same thing if we want to take a screenshot of all windows,
            // meaning that the _takeScreenshotIds list is empty
            if (shouldTakeScreenshot && !_shouldTakeScreenshotIds.empty()) {
                auto it = std::find(
                    _shouldTakeScreenshotIds.cbegin(),
                    _shouldTakeScreenshotIds.cend(),
                    window->id()
                );
                // If the window id is in the list of ids, then we want to take a
                // screenshot. We already checked that `shouldTakeScreenshot` is true in
                // the if statement above
                shouldTakeScreenshot = (it != _shouldTakeScreenshotIds.cend());
            }
            window->swapBuffers(shouldTakeScreenshot);
        }

        TracyGpuCollect;
        FrameMark;

        std::for_each(
            windows.cbegin(), windows.cend(),
            std::mem_fn(&Window::updateResolutions)
        );

        // for all windows
        _frameCounter++;
        if (_shouldTakeScreenshot) {
            _shotCounter++;
        }
        _shouldTakeScreenshot = false;
    }

    Window::makeSharedContextCurrent();
    glDeleteQueries(1, &timeQueryBegin);
    glDeleteQueries(1, &timeQueryEnd);
}

void Engine::drawOverlays(const Window& window, Frustum::Mode frustum) const {
    ZoneScoped;

    for (const std::unique_ptr<Viewport>& vp : window.viewports()) {
        // if viewport has overlay
        if (!vp->hasOverlayTexture() || !vp->isEnabled()) {
            continue;
        }

        vp->setupViewport(frustum);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, vp->overlayTextureIndex());
        _overlay.bind();
        window.renderScreenQuad();
    }
    ShaderProgram::unbind();
}

void Engine::renderFBOTexture(Window& window) const {
    ZoneScoped;

    OffScreenBuffer::unbind();

    window.makeOpenGLContextCurrent();

    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const Frustum::Mode frustum =
        (window.stereoMode() == Window::StereoMode::Active) ?
        Frustum::Mode::StereoLeftEye :
        Frustum::Mode::MonoEye;

    const ivec2 size = ivec2 {
        static_cast<int>(std::ceil(window.scale().x * window.resolution().x)),
        static_cast<int>(std::ceil(window.scale().y * window.resolution().y))
    };

    glViewport(0, 0, size.x, size.y);
    setAndClearBuffer(window, BufferMode::BackBufferBlack, frustum);

    const Window::StereoMode sm = window.stereoMode();
    bool maskShaderSet = false;
    const std::vector<std::unique_ptr<Viewport>>& vps = window.viewports();
    if (sm > Window::StereoMode::Active && sm < Window::StereoMode::SideBySide) {
        window.bindStereoShaderProgram(
            window.frameBufferTexture(Window::TextureIndex::LeftEye),
            window.frameBufferTexture(Window::TextureIndex::RightEye)
        );

        std::for_each(vps.begin(), vps.end(), std::mem_fn(&Viewport::renderWarpMesh));
    }
    else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(
            GL_TEXTURE_2D,
            window.frameBufferTexture(Window::TextureIndex::LeftEye)
        );

        _fboQuad.bind();
        maskShaderSet = true;

        glUniform1i(glGetUniformLocation(_fboQuad.id(), "flipX"), window.flipX() ? 1 : 0);
        glUniform1i(glGetUniformLocation(_fboQuad.id(), "flipY"), window.flipY() ? 1 : 0);

        std::for_each(vps.begin(), vps.end(), std::mem_fn(&Viewport::renderWarpMesh));

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
            std::for_each(vps.begin(), vps.end(), std::mem_fn(&Viewport::renderWarpMesh));
        }
    }

    // render mask (mono)
    if (window.hasAnyMasks()) {
        if (!maskShaderSet) {
            _fboQuad.bind();

            glUniform1i(
                glGetUniformLocation(_fboQuad.id(), "flipX"),
                window.flipX() ? 1 : 0
            );
            glUniform1i(
                glGetUniformLocation(_fboQuad.id(), "flipY"),
                window.flipY() ? 1 : 0
            );
        }

        glDrawBuffer(GL_BACK);
        glReadBuffer(GL_BACK);
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_BLEND);

        // Result = (Color * BlendMask) * (1-BlackLevel) + BlackLevel
        // render blend masks
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        for (const std::unique_ptr<Viewport>& vp : window.viewports()) {
            ZoneScopedN("Render Viewport");

            if (vp->hasBlendMaskTexture() && vp->isEnabled()) {
                glBindTexture(GL_TEXTURE_2D, vp->blendMaskTextureIndex());
                vp->renderMaskMesh();
            }
            if (vp->hasBlackLevelMaskTexture() && vp->isEnabled()) {
                glBindTexture(GL_TEXTURE_2D, vp->blackLevelMaskTextureIndex());
                vp->renderMaskMesh();
            }
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    ShaderProgram::unbind();
    glDisable(GL_BLEND);
}

void Engine::renderViewports(const Window& window, Frustum::Mode frustum,
                             Window::TextureIndex ti) const
{
    ZoneScoped;

    OffScreenBuffer* fbo = window.fbo();
    fbo->bind();
    if (fbo->isMultiSampled()) {
        return;
    }

    // update attachments
    fbo->attachColorTexture(window.frameBufferTexture(ti), GL_COLOR_ATTACHMENT0);

    if (Engine::instance().useDepthTexture()) {
        fbo->attachDepthTexture(window.frameBufferTexture(Window::TextureIndex::Depth));
    }

    if (Engine::instance().useNormalTexture()) {
        fbo->attachColorTexture(
            window.frameBufferTexture(Window::TextureIndex::Normals),
            GL_COLOR_ATTACHMENT1
        );
    }

    if (Engine::instance().usePositionTexture()) {
        fbo->attachColorTexture(
            window.frameBufferTexture(Window::TextureIndex::Positions),
            GL_COLOR_ATTACHMENT2
        );
    }

    const Window::StereoMode sm = window.stereoMode();
    // render all viewports for selected eye
    for (const std::unique_ptr<Viewport>& vp : window.viewports()) {
        if (!vp->isEnabled()) {
            continue;
        }

        // if passive stereo or mono
        if (sm == Window::StereoMode::NoStereo) {
            // @TODO (abock, 2019-12-04) Not sure about this one; the frustum is set in
            // the calling function based on the stereo mode already and we are
            // overwriting it here
            frustum = vp->eye();
        }

        if (vp->hasSubViewports()) {
            if (vp->isTracked()) {
                vp->nonLinearProjection()->updateFrustums(
                    frustum,
                    _nearClipPlane,
                    _farClipPlane
                );
            }

            if (window.shouldCallDraw3DFunction()) {
                vp->nonLinearProjection()->render(window, *vp, frustum);
            }
        }
        else {
            // no subviewports
            if (vp->isTracked()) {
                vp->calculateFrustum(frustum, _nearClipPlane, _farClipPlane);
            }

            // check if we want to blit the previous window before we do anything else
            if (window.blitWindowId() >= 0) {
                const std::vector<std::unique_ptr<Window>>& wins = windows();
                auto it = std::find_if(
                    wins.cbegin(),
                    wins.cend(),
                    [id = window.blitWindowId()](const std::unique_ptr<Window>& w) {
                        return w->id() == id;
                    }
                );
                assert(it != wins.cend());
                blitWindowViewport(**it, window, *vp, frustum);
            }

            if (window.shouldCallDraw3DFunction()) {
                // run scissor test to prevent clearing of entire buffer
                vp->setupViewport(frustum);
                glEnable(GL_SCISSOR_TEST);
                setAndClearBuffer(window, BufferMode::RenderToTexture, frustum);
                glDisable(GL_SCISSOR_TEST);

                if (_drawFn) {
                    ZoneScopedN("[SGCT] Draw");
                    const RenderData renderData = {
                        window,
                        *vp,
                        frustum,
                        ClusterManager::instance().sceneTransform(),
                        vp->projection(frustum).viewMatrix(),
                        vp->projection(frustum).projectionMatrix(),
                        vp->projection(frustum).viewProjectionMatrix() *
                            ClusterManager::instance().sceneTransform(),
                        window.finalFBODimensions()
                    };
                    _drawFn(renderData);
                }
            }
        }
    }

    // If we did not render anything, make sure we clear the screen at least
    const int blitId = window.blitWindowId();
    if (!window.shouldCallDraw3DFunction() && blitId == -1) {
        setAndClearBuffer(window, BufferMode::RenderToTexture, frustum);
    }
    else if (blitId != -1) {
        const std::vector<std::unique_ptr<Window>>& wins = windows();
        auto it = std::find_if(
            wins.cbegin(),
            wins.cend(),
            [id = window.blitWindowId()](const std::unique_ptr<Window>& w) {
                return w->id() == id;
            }
        );
        assert(it != wins.cend());
        const Window& srcWin = **it;

        if (!srcWin.isVisible() && !srcWin.isRenderingWhileHidden()) {
            setAndClearBuffer(window, BufferMode::RenderToTexture, frustum);
        }
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    // for side-by-side or top-bottom mode, do postfx/blit only after rendering right eye
    const bool isSplitScreen = (sm >= Window::StereoMode::SideBySide);
    if (!isSplitScreen || frustum != Frustum::Mode::StereoLeftEye) {
        ZoneScopedN("PostFX/Blit");

        updateRenderingTargets(window, ti);
        if (window.useFXAA()) {
            renderFXAA(window, ti);
        }

        render2D(window, frustum);
        if (isSplitScreen) {
            // render left eye info and graph to render 2D items after post fx
            render2D(window, Frustum::Mode::StereoLeftEye);
        }
    }

    glDisable(GL_BLEND);
}

void Engine::render2D(const Window& window, Frustum::Mode frustum) const {
    ZoneScoped;

    // draw viewport overlays if any
    drawOverlays(window, frustum);

    // draw info & stats
    // the cubemap viewports are all the same so it makes no sense to render everything
    // several times therefore just loop one iteration in that case.
    if (!(_statisticsRenderer || _draw2DFn)) {
        return;
    }

    for (const std::unique_ptr<Viewport>& vp : window.viewports()) {
        if (!vp->isEnabled()) {
            continue;
        }
        vp->setupViewport(frustum);

        if (_statisticsRenderer) {
            _statisticsRenderer->render(window, *vp);
        }

        // Check if we should call the use defined draw2D function
        if (_draw2DFn && window.shouldCallDraw2DFunction()) {
            ZoneScopedN("[SGCT] Draw 2D");
            const RenderData renderData = {
                window,
                *vp,
                frustum,
                ClusterManager::instance().sceneTransform(),
                vp->projection(frustum).viewMatrix(),
                vp->projection(frustum).projectionMatrix(),
                vp->projection(frustum).viewProjectionMatrix() *
                    ClusterManager::instance().sceneTransform(),
                window.finalFBODimensions()
            };
            _draw2DFn(renderData);
        }
    }
}

void Engine::renderFXAA(const Window& window, Window::TextureIndex targetIndex) const {
    ZoneScoped;

    assert(_fxaa.has_value());

    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    // bind target FBO
    window.fbo()->attachColorTexture(
        window.frameBufferTexture(targetIndex),
        GL_COLOR_ATTACHMENT0
    );

    const ivec2 framebufferSize = window.framebufferResolution();
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

bool Engine::isMaster() const {
    return NetworkManager::instance().isComputerServer();
}

unsigned int Engine::currentFrameNumber() const {
    return _frameCounter;
}

void Engine::waitForAllWindowsInSwapGroupToOpen() {
    ZoneScoped;

    ClusterManager& cm = ClusterManager::instance();
    Node& thisNode = cm.thisNode();

    // clear the buffers initially
    for (const std::unique_ptr<Window>& window : thisNode.windows()) {
        ZoneScopedN("Clear Windows");
        window->makeOpenGLContextCurrent();
        glDrawBuffer(GL_BACK);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        {
            ZoneScopedN("glfwSwapBuffers");
            glfwSwapBuffers(window->windowHandle());
        }
    }

    {
        ZoneScopedN("GLFW Poll Events");
        glfwPollEvents();
    }

    // Must wait until all nodes are running if using swap barrier
    if (cm.ignoreSync() || cm.numberOfNodes() == 1) {
        return;
    }

    // check if swapgroups are supported
#ifdef WIN32
    const bool hasSwapGroup = glfwExtensionSupported("WGL_NV_swap_group");
    if (hasSwapGroup) {
        Log::Info("Swap groups are supported by hardware");
}
    else {
        Log::Info("Swap groups are not supported by hardware");
    }
#else // ^^^^ WIN32 // !WIN32 vvvv
    Log::Info("Swap groups are not supported by hardware");
#endif // WIN32

    Log::Info("Waiting for all nodes to connect");

    while (!NetworkManager::instance().areAllNodesConnected()) {
        // Swap front and back rendering buffers
        for (const std::unique_ptr<Window>& window : thisNode.windows()) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glfwSwapBuffers(window->windowHandle());
        }
        {
            ZoneScopedN("GLFW Poll Events");
            glfwPollEvents();
        }

        if (_shouldTerminate || !NetworkManager::instance().isRunning() ||
            thisNode.closeAllWindows())
        {
            // We can't just exit as the client application might need the OpenGL state
            // for some cleanup.  Instead, we are calling the terminate function which
            // will cause the first `render` call to be bypassed and the cleanup should
            // work as expected
            terminate();
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Engine::updateFrustums() const {
    ZoneScoped;

    const Node& thisNode = ClusterManager::instance().thisNode();
    for (const std::unique_ptr<Window>& win : thisNode.windows()) {
        for (const std::unique_ptr<Viewport>& vp : win->viewports()) {
            if (vp->isTracked()) {
                // if not tracked update, otherwise this is done on the fly
                continue;
            }

            using Mode = Frustum::Mode;
            vp->calculateFrustum(Mode::MonoEye, _nearClipPlane, _farClipPlane);
            vp->calculateFrustum(Mode::StereoLeftEye, _nearClipPlane, _farClipPlane);
            vp->calculateFrustum(Mode::StereoRightEye, _nearClipPlane, _farClipPlane);
        }
    }
}

void Engine::blitWindowViewport(const Window& prevWindow, const Window& window,
                                const Viewport& viewport, Frustum::Mode mode) const
{
    ZoneScoped;

    assert(prevWindow.id() != window.id());

    // run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);
    viewport.setupViewport(mode);
    setAndClearBuffer(window, BufferMode::RenderToTexture, mode);
    glDisable(GL_SCISSOR_TEST);

    _overlay.bind();

    glActiveTexture(GL_TEXTURE0);
    const Window::TextureIndex m = [](Frustum::Mode v) {
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

const Engine::Statistics& Engine::statistics() const {
    return _statistics;
}

float Engine::nearClipPlane() const {
    return _nearClipPlane;
}

float Engine::farClipPlane() const {
    return _farClipPlane;
}

void Engine::setNearAndFarClippingPlanes(float nearClippingPlane, float farClippingPlane)
{
    _nearClipPlane = nearClippingPlane;
    _farClipPlane = farClippingPlane;
    updateFrustums();
}

const Window* Engine::focusedWindow() const {
    ZoneScoped;

    const Node& thisNode = ClusterManager::instance().thisNode();
    const std::vector<std::unique_ptr<Window>>& ws = thisNode.windows();
    const auto it = std::find_if(ws.begin(), ws.end(), std::mem_fn(&Window::isFocused));
    return it != ws.end() ? it->get() : nullptr;
}

void Engine::setStatsGraphVisibility(bool value) {
    if (value && _statisticsRenderer == nullptr) {
        _statisticsRenderer = std::make_unique<StatisticsRenderer>(_statistics);
    }
    if (!value && _statisticsRenderer) {
        _statisticsRenderer = nullptr;
    }
}

float Engine::statsGraphScale() const {
    return _statisticsRenderer ? _statisticsRenderer->scale() : -1.f;
}

void Engine::setStatsGraphScale(float scale) {
    if (_statisticsRenderer) {
        _statisticsRenderer->setScale(scale);
    }
}

void Engine::takeScreenshot(std::vector<int> windowIds) {
    _shouldTakeScreenshot = true;
    _shouldTakeScreenshotIds = std::move(windowIds);
}

void Engine::resetScreenshotNumber() {
    _shotCounter = 0;
}

const std::function<void(const RenderData&)>& Engine::drawFunction() const {
    return _drawFn;
}

const Node& Engine::thisNode() const {
    return ClusterManager::instance().thisNode();
}

const std::vector<std::unique_ptr<Window>>& Engine::windows() const {
    return ClusterManager::instance().thisNode().windows();
}

User& Engine::defaultUser() {
    return ClusterManager::instance().defaultUser();
}

void Engine::setSyncParameters(bool printMessage, float timeout) {
    _printSyncMessage = printMessage;
    _syncTimeout = timeout;
}

void Engine::setScreenshotNumber(unsigned int number) {
    _shotCounter = number;
}

unsigned int Engine::screenShotNumber() const {
    return _shotCounter;
}

int Engine::swapInterval() const{
    return _settings.swapInterval;
}

void Engine::setCapturePath(std::filesystem::path path) {
    _settings.capture.capturePath = std::move(path);
    setScreenshotNumber(0);
}

const std::filesystem::path& Engine::capturePath() const {
    return _settings.capture.capturePath;
}

std::optional<std::pair<uint64_t, uint64_t>> Engine::screenshotLimit() const {
    return _settings.capture.limits;
}

const std::string& Engine::prefixScreenshot() const {
    return _settings.capture.prefix;
}

bool Engine::addNodeNameToScreenshot() const {
    return _settings.capture.addNodeName;
}

bool Engine::addWindowNameToScreenshot() const {
    return _settings.capture.addWindowName;
}

int Engine::numberCaptureThreads() const {
    return _settings.capture.nCaptureThreads;
}

bool Engine::useDepthTexture() const {
    return _settings.textures.useDepthTexture;
}

bool Engine::useNormalTexture() const {
    return _settings.textures.useNormalTexture;
}

bool Engine::usePositionTexture() const {
    return _settings.textures.usePositionTexture;
}

Engine::DrawBufferType Engine::drawBufferType() const {
    if (_settings.textures.usePositionTexture) {
        if (_settings.textures.useNormalTexture) {
            return DrawBufferType::DiffuseNormalPosition;
        }
        else {
            return DrawBufferType::DiffusePosition;
        }
    }
    else {
        if (_settings.textures.useNormalTexture) {
            return DrawBufferType::DiffuseNormal;
        }
        else {
            return DrawBufferType::Diffuse;
        }
    }
}

void Engine::setCaptureFromBackBuffer(bool state) {
    _settings.captureBackBuffer = state;
}

bool Engine::captureFromBackBuffer() const {
    return _settings.captureBackBuffer;
}

} // namespace sgct
