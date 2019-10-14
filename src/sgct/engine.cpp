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
#include <sgct/font.h>
#include <sgct/fontmanager.h>
#include <sgct/freetype.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/readconfig.h>
#include <sgct/mpcdi.h>
#include <sgct/mutexmanager.h>
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
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <glm/gtc/type_ptr.hpp>
#include <sgct/ogl_headers.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// Callback wrappers for GLFW
std::function<void(int, int, int, int)> gKeyboardCallbackFnPtr = nullptr;
std::function<void(unsigned int, int)> gCharCallbackFnPtr = nullptr;
std::function<void(int, int, int)> gMouseButtonCallbackFnPtr = nullptr;
std::function<void(double, double)> gMousePosCallbackFnPtr = nullptr;
std::function<void(double, double)> gMouseScrollCallbackFnPtr = nullptr;
std::function<void(int, const char**)> gDropCallbackFnPtr = nullptr;
std::function<void(const sgct::core::Touch*)> gTouchCallbackFnPtr = nullptr;
sgct::core::Touch gCurrentTouchPoints;

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
            sgct::MutexManager::instance()->frameSyncMutex.lock();
            run = sRunUpdateFrameLockLoop;
            sgct::MutexManager::instance()->frameSyncMutex.unlock();

            sgct::core::NetworkManager::cond.notify_all();

            std::this_thread::sleep_for(FrameLockTimeout);
        }
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

    void applyScene(const sgct::config::Scene& scene) {
        if (scene.offset) {
            sgct::core::ClusterManager::instance()->setSceneOffset(*scene.offset);
        }
        if (scene.orientation) {
            sgct::core::ClusterManager::instance()->setSceneRotation(
                glm::mat4_cast(*scene.orientation)
            );
        }
        if (scene.scale) {
            sgct::core::ClusterManager::instance()->setSceneScale(*scene.scale);
        }
    }

    void applyUser(const sgct::config::User& user) {
        using namespace sgct::core;

        User* usrPtr;
        if (user.name) {
            std::unique_ptr<User> usr = std::make_unique<User>(*user.name);
            usrPtr = usr.get();
            ClusterManager::instance()->addUser(std::move(usr));
            sgct::MessageHandler::instance()->printInfo(
                "ReadConfig: Adding user '%s'", user.name->c_str()
            );
        }
        else {
            usrPtr = &ClusterManager::instance()->getDefaultUser();
        }

        if (user.eyeSeparation) {
            usrPtr->setEyeSeparation(*user.eyeSeparation);
        }
        if (user.position) {
            usrPtr->setPos(*user.position);
        }
        if (user.transformation) {
            usrPtr->setTransform(*user.transformation);
        }
        if (user.tracking) {
            usrPtr->setHeadTracker(user.tracking->tracker, user.tracking->device);
        }
    }

    void applyCapture(const sgct::config::Capture& capture) {
        if (capture.monoPath) {
            sgct::Settings::instance()->setCapturePath(
                *capture.monoPath,
                sgct::Settings::CapturePath::Mono
            );
        }
        if (capture.leftPath) {
            sgct::Settings::instance()->setCapturePath(
                *capture.leftPath,
                sgct::Settings::CapturePath::LeftStereo
            );
        }
        if (capture.rightPath) {
            sgct::Settings::instance()->setCapturePath(
                *capture.rightPath,
                sgct::Settings::CapturePath::RightStereo
            );
        }
        if (capture.format) {
            sgct::Settings::CaptureFormat f = [](sgct::config::Capture::Format format) {
                switch (format) {
                    default:
                    case sgct::config::Capture::Format::PNG:
                        return sgct::Settings::CaptureFormat::PNG;
                    case sgct::config::Capture::Format::JPG:
                        return sgct::Settings::CaptureFormat::JPG;
                    case sgct::config::Capture::Format::TGA:
                        return sgct::Settings::CaptureFormat::TGA;
                }
            }(*capture.format);
            sgct::Settings::instance()->setCaptureFormat(f);
        }
    }

    void applyDevice(const sgct::config::Device& device) {
        sgct::core::ClusterManager& cm = *sgct::core::ClusterManager::instance();
        cm.getTrackingManager().addDeviceToCurrentTracker(device.name);

        for (const sgct::config::Device::Sensors& s : device.sensors) {
            cm.getTrackingManager().addSensorToCurrentDevice(s.vrpnAddress, s.identifier);
        }
        for (const sgct::config::Device::Buttons& b : device.buttons) {
            cm.getTrackingManager().addButtonsToCurrentDevice(b.vrpnAddress, b.count);
        }
        for (const sgct::config::Device::Axes& a : device.axes) {
            cm.getTrackingManager().addAnalogsToCurrentDevice(a.vrpnAddress, a.count);
        }
        if (device.offset) {
            sgct::Tracker& tr = *cm.getTrackingManager().getLastTracker();
            tr.getLastDevice()->setOffset(*device.offset);
        }
        if (device.transformation) {
            sgct::Tracker& tr = *cm.getTrackingManager().getLastTracker();
            tr.getLastDevice()->setTransform(*device.transformation);
        }
    }

    void applyTracker(const sgct::config::Tracker& tracker) {
        sgct::core::ClusterManager& cm = *sgct::core::ClusterManager::instance();
        cm.getTrackingManager().addTracker(tracker.name);

        for (const sgct::config::Device& device : tracker.devices) {
            applyDevice(device);
        }
        if (tracker.offset) {
            sgct::Tracker& tr = *cm.getTrackingManager().getLastTracker();
            tr.setOffset(*tracker.offset);
        }
        if (tracker.scale) {
            sgct::Tracker& tr = *cm.getTrackingManager().getLastTracker();
            tr.setScale(*tracker.scale);
        }
        if (tracker.transformation) {
            sgct::Tracker& tr = *cm.getTrackingManager().getLastTracker();
            tr.setTransform(*tracker.transformation);
        }
    }

    void applyWindow(const sgct::config::Window& window, sgct::core::Node& node) {
        sgct::Window win = sgct::Window(node.getNumberOfWindows());

        if (window.name) {
            win.setName(*window.name);
        }
        if (!window.tags.empty()) {
            win.setTags(window.tags);
        }
        if (window.bufferBitDepth) {
            sgct::Window::ColorBitDepth bd = [](sgct::config::Window::ColorBitDepth bd) {
                switch (bd) {
                    default:
                    case sgct::config::Window::ColorBitDepth::Depth8:
                        return sgct::Window::ColorBitDepth::Depth8;
                    case sgct::config::Window::ColorBitDepth::Depth16:
                        return sgct::Window::ColorBitDepth::Depth16;
                    case sgct::config::Window::ColorBitDepth::Depth16Float:
                        return sgct::Window::ColorBitDepth::Depth16Float;
                    case sgct::config::Window::ColorBitDepth::Depth32Float:
                        return sgct::Window::ColorBitDepth::Depth32Float;
                    case sgct::config::Window::ColorBitDepth::Depth16Int:
                        return sgct::Window::ColorBitDepth::Depth16Int;
                    case sgct::config::Window::ColorBitDepth::Depth32Int:
                        return sgct::Window::ColorBitDepth::Depth32Int;
                    case sgct::config::Window::ColorBitDepth::Depth16UInt:
                        return sgct::Window::ColorBitDepth::Depth16UInt;
                    case sgct::config::Window::ColorBitDepth::Depth32UInt:
                        return sgct::Window::ColorBitDepth::Depth32UInt;
                }
            }(*window.bufferBitDepth);
            win.setColorBitDepth(bd);
        }

        if (window.isFullScreen) {
            win.setWindowMode(*window.isFullScreen);
        }

        if (window.isFloating) {
            win.setFloating(*window.isFloating);
        }

        if (window.alwaysRender) {
            win.setRenderWhileHidden(*window.alwaysRender);
        }

        if (window.isHidden) {
            win.setVisibility(*window.isHidden);
        }

        if (window.doubleBuffered) {
            win.setDoubleBuffered(*window.doubleBuffered);
        }

        if (window.gamma) {
            win.setGamma(*window.gamma);
        }

        if (window.contrast) {
            win.setContrast(*window.contrast);
        }

        if (window.brightness) {
            win.setBrightness(*window.brightness);
        }

        if (window.msaa) {
            win.setNumberOfAASamples(*window.msaa);
        }

        if (window.hasAlpha) {
            win.setAlpha(*window.hasAlpha);
        }

        if (window.useFxaa) {
            win.setUseFXAA(*window.useFxaa);
        }

        if (window.isDecorated) {
            win.setWindowDecoration(*window.isDecorated);
        }

        if (window.hasBorder) {
            win.setWindowDecoration(*window.hasBorder);
        }

        if (window.draw2D) {
            win.setCallDraw2DFunction(*window.draw2D);
        }

        if (window.draw3D) {
            win.setCallDraw2DFunction(*window.draw3D);
        }

        if (window.copyPreviousWindowToCurrentWindow) {
            win.setCopyPreviousWindowToCurrentWindow(
                *window.copyPreviousWindowToCurrentWindow
            );
        }

        if (window.monitor) {
            win.setFullScreenMonitorIndex(*window.monitor);
        }

        if (window.mpcdi) {
            sgct::core::Mpcdi().parseConfiguration(*window.mpcdi, node, win);
            return;
        }

        if (window.stereo) {
            sgct::Window::StereoMode sm = [](sgct::config::Window::StereoMode sm) {
                switch (sm) {
                    default:
                    case sgct::config::Window::StereoMode::NoStereo:
                        return sgct::Window::StereoMode::NoStereo;
                    case sgct::config::Window::StereoMode::Active:
                        return sgct::Window::StereoMode::Active;
                    case sgct::config::Window::StereoMode::AnaglyphRedCyan:
                        return sgct::Window::StereoMode::AnaglyphRedCyan;
                    case sgct::config::Window::StereoMode::AnaglyphAmberBlue:
                        return sgct::Window::StereoMode::AnaglyphAmberBlue;
                    case sgct::config::Window::StereoMode::AnaglyphRedCyanWimmer:
                        return sgct::Window::StereoMode::AnaglyphRedCyanWimmer;
                    case sgct::config::Window::StereoMode::Checkerboard:
                        return sgct::Window::StereoMode::Checkerboard;
                    case sgct::config::Window::StereoMode::CheckerboardInverted:
                        return sgct::Window::StereoMode::CheckerboardInverted;
                    case sgct::config::Window::StereoMode::VerticalInterlaced:
                        return sgct::Window::StereoMode::VerticalInterlaced;
                    case sgct::config::Window::StereoMode::VerticalInterlacedInverted:
                        return sgct::Window::StereoMode::VerticalInterlacedInverted;
                    case sgct::config::Window::StereoMode::Dummy:
                        return sgct::Window::StereoMode::Dummy;
                    case sgct::config::Window::StereoMode::SideBySide:
                        return sgct::Window::StereoMode::SideBySide;
                    case sgct::config::Window::StereoMode::SideBySideInverted:
                        return sgct::Window::StereoMode::SideBySideInverted;
                    case sgct::config::Window::StereoMode::TopBottom:
                        return sgct::Window::StereoMode::TopBottom;
                    case sgct::config::Window::StereoMode::TopBottomInverted:
                        return sgct::Window::StereoMode::TopBottomInverted;
                }
            }(*window.stereo);
            win.setStereoMode(sm);
        }

        if (window.pos) {
            win.setWindowPosition(*window.pos);
        }

        win.initWindowResolution(window.size);

        if (window.resolution) {
            win.setFramebufferResolution(*window.resolution);
            win.setFixResolution(true);
        }

        for (const sgct::config::Viewport& viewport : window.viewports) {
            std::unique_ptr<sgct::core::Viewport> vp =
                std::make_unique<sgct::core::Viewport>();
            vp->applySettings(viewport);
            win.addViewport(std::move(vp));
        }
        node.addWindow(std::move(win));
    }

    void applyNode(const sgct::config::Node& node) {
        std::unique_ptr<sgct::core::Node> n = std::make_unique<sgct::core::Node>();

        n->setAddress(node.address);
        if (node.name) {
            n->setName(*node.name);
        }
        n->setSyncPort(node.port);
        if (node.dataTransferPort) {
            n->setDataTransferPort(*node.dataTransferPort);
        }
        if (node.swapLock) {
            n->setUseSwapGroups(*node.swapLock);
        }

        for (const sgct::config::Window& window : node.windows) {
            applyWindow(window, *n);
        }

        sgct::core::ClusterManager::instance()->addNode(std::move(n));
    }

    void applySettings(const sgct::config::Settings& settings) {
        sgct::Settings& s = *sgct::Settings::instance();

        if (settings.useDepthTexture) {
            s.setUseDepthTexture(*settings.useDepthTexture);
        }
        if (settings.useNormalTexture) {
            s.setUseNormalTexture(*settings.useNormalTexture);
        }
        if (settings.usePositionTexture) {
            s.setUsePositionTexture(*settings.usePositionTexture);
        }
        if (settings.bufferFloatPrecision) {
            sgct::Settings::BufferFloatPrecision p =
            [](sgct::config::Settings::BufferFloatPrecision p) {
                switch (p) {
                    default:
                    case sgct::config::Settings::BufferFloatPrecision::Float16Bit:
                        return sgct::Settings::BufferFloatPrecision::Float16Bit;
                    case sgct::config::Settings::BufferFloatPrecision::Float32Bit:
                        return sgct::Settings::BufferFloatPrecision::Float32Bit;
                }
            }(*settings.bufferFloatPrecision);
            s.setBufferFloatPrecision(p);
        }
        if (settings.display) {
            if (settings.display->swapInterval) {
                s.setSwapInterval(*settings.display->swapInterval);
            }
            if (settings.display->refreshRate) {
                s.setRefreshRateHint(*settings.display->refreshRate);
            }
            if (settings.display->maintainAspectRatio) {
                s.setTryMaintainAspectRatio(*settings.display->maintainAspectRatio);
            }
            if (settings.display->exportWarpingMeshes) {
                s.setExportWarpingMeshes(*settings.display->exportWarpingMeshes);
            }
        }
        if (settings.osdText) {
            if (settings.osdText->name) {
                s.setOSDTextFontName(*settings.osdText->name);
            }
            if (settings.osdText->path) {
                s.setOSDTextFontPath(*settings.osdText->path);
            }
            if (settings.osdText->size) {
                s.setOSDTextFontSize(*settings.osdText->size);
            }
            if (settings.osdText->xOffset) {
                const glm::vec2& curr = s.getOSDTextOffset();
                s.setOSDTextOffset(glm::vec2(*settings.osdText->xOffset, curr.y));
            }
            if (settings.osdText->yOffset) {
                const glm::vec2& curr = s.getOSDTextOffset();
                s.setOSDTextOffset(glm::vec2(curr.x, *settings.osdText->yOffset));
            }
        }
        if (settings.fxaa) {
            if (settings.fxaa->offset) {
                s.setFXAASubPixOffset(*settings.fxaa->offset);
            }
            if (settings.fxaa->trim) {
                s.setFXAASubPixTrim(1.f / *settings.fxaa->trim);
            }
        }
    }

    void applyCluster(const sgct::config::Cluster& cluster) {
        sgct::core::ClusterManager::instance()->setMasterAddress(cluster.masterAddress);
        if (cluster.debug) {
            sgct::MessageHandler::instance()->setNotifyLevel(
                *cluster.debug ?
                sgct::MessageHandler::Level::Debug :
                sgct::MessageHandler::Level::Warning
            );
        }
        if (cluster.externalControlPort) {
            sgct::core::ClusterManager::instance()->setExternalControlPort(
                *cluster.externalControlPort
            );
        }
        if (cluster.firmSync) {
            sgct::core::ClusterManager::instance()->setFirmFrameLockSyncStatus(
                *cluster.firmSync
            );
        }
        if (cluster.scene) {
            applyScene(*cluster.scene);
        }
        for (const sgct::config::Node& node : cluster.nodes) {
            applyNode(node);
        }
        if (cluster.settings) {
            applySettings(*cluster.settings);
        }
        if (cluster.user) {
            applyUser(*cluster.user);
        }
        if (cluster.capture) {
            applyCapture(*cluster.capture);
        }
        if (cluster.tracker) {
            applyTracker(*cluster.tracker);
        }
    }
} // namespace

namespace sgct {

Engine* Engine::_instance = nullptr;

Engine* Engine::instance() {
    return _instance;
}

config::Cluster loadCluster(std::optional<std::string> path) {
    if (path) {
        try {
            return core::readconfig::readConfig(*path);
        }
        catch (const std::runtime_error& e) {
            // fatal error
            // (abock, 2019-10-11) This conversion from string_view to string is necessary
            // to keep this code compiling with VS 2017 15.9.16 (and probably before)
            std::cout << std::string(getHelpMessage()) << '\n';
            MessageHandler::instance()->printError("Configuration error. %s", e.what());
            throw;
        }
    }
    else {
        config::Cluster cluster;
        // Create a default configuration
        sgct::config::ProjectionPlane proj;
        proj.lowerLeft = glm::vec3(-1.778f, -1.f, 0.f);
        proj.upperLeft = glm::vec3(-1.778f, 1.f, 0.f);
        proj.upperRight = glm::vec3(1.778f, 1.f, 0.f);

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

        cluster.masterAddress = "localhost";
        cluster.nodes.push_back(node);
        cluster.user = user;
        return cluster;
    }
}

Engine::Engine(const Configuration& config) {
    _instance = this;

    if (config.isServer) {
        core::ClusterManager::instance()->setNetworkMode(
            *config.isServer ?
                core::NetworkManager::NetworkMode::LocalServer :
                core::NetworkManager::NetworkMode::LocalClient
        );
    }
    if (config.logPath) {
        MessageHandler::instance()->setLogPath(
            config.logPath->c_str(),
            core::ClusterManager::instance()->getThisNodeId()
        );
        MessageHandler::instance()->setLogToFile(true);
    }
    if (config.logLevel) {
        MessageHandler::instance()->setNotifyLevel(*config.logLevel);
    }
    if (config.showHelpText) {
        _helpMode = true;
        // (abock, 2019-10-11) This conversion from string_view to string is necessary to
        // keep this code compiling with VS 2017 15.9.16 (and probably before)
        std::cout << std::string(getHelpMessage()) << '\n';
    }
    if (config.nodeId) {
        core::ClusterManager::instance()->setThisNodeId(*config.nodeId);
    }
    if (config.firmSync) {
        core::ClusterManager::instance()->setFirmFrameLockSyncStatus(*config.firmSync);
    }
    if (config.ignoreSync) {
        core::ClusterManager::instance()->setUseIgnoreSync(*config.ignoreSync);
    }
    if (config.fxaa) {
        Settings::instance()->setDefaultFXAAState(*config.fxaa);
    }
    if (config.msaaSamples) {
        if (*config.msaaSamples > 0) {
            Settings::instance()->setDefaultNumberOfAASamples(*config.msaaSamples);
        }
        else {
            MessageHandler::instance()->printError(
                "Number of MSAA samples must be positive"
            );
        }
    }
    if (config.captureFormat) {
        Settings::instance()->setCaptureFormat(*config.captureFormat);
    }
    if (config.nCaptureThreads) {
        if (*config.nCaptureThreads > 0) {
            Settings::instance()->setNumberOfCaptureThreads(*config.nCaptureThreads);
        }
        else {
            MessageHandler::instance()->printError(
                "Only positive number of capture threads allowed"
            );
        }
    }

    if (_helpMode) {
        return;
    }

    glfwSetErrorCallback([](int error, const char* desc) {
        MessageHandler::instance()->printError("GLFW error (%i): %s", error, desc);
    });
    const int res = glfwInit();
    if (res == GLFW_FALSE) {
        _shouldTerminate = true;
    }
}

Engine::~Engine() {
    clean();
}

bool Engine::init(RunMode rm, config::Cluster cluster) {
    _runMode = rm;
    MessageHandler::instance()->printInfo("%s", getVersion().c_str());

    if (_helpMode) {
        return false;
    }

    if (_shouldTerminate) {
        MessageHandler::instance()->printError(
            "Failed to init GLFW! Application will close in 5 seconds"
        );
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return false;
    }

    const bool validation = sgct::config::validateCluster(cluster);
    if (!validation) {
        throw std::runtime_error("Validation of configuration failes");
    }
         
    applyCluster(cluster);

    bool networkSuccess = initNetwork();
    if (!networkSuccess) { 
        MessageHandler::instance()->printError(
            "Network init error. Application will close in 5 seconds"
        );
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return false;
    }

    if (!initWindows()) {
        MessageHandler::instance()->printError(
            "Window init error. Application will close in 5 seconds"
        );
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return false;
    }

    // Window resolution may have been set when reading config. However, it only sets a
    // pending resolution, so it needs to apply it using the same routine as in the end of
    // a frame.
    core::Node* thisNode = core::ClusterManager::instance()->getThisNode();
    for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
        thisNode->getWindow(i).updateResolutions();
    }

    // if a single node, skip syncing
    if (core::ClusterManager::instance()->getNumberOfNodes() == 1) {
        core::ClusterManager::instance()->setUseIgnoreSync(true);
    }

    for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
        GLFWwindow* window = getWindow(i).getWindowHandle();
        if (gKeyboardCallbackFnPtr) {
            glfwSetKeyCallback(
                window,
                [](GLFWwindow*, int key, int scancode, int action, int mods) {
                    if (gKeyboardCallbackFnPtr) {
                        gKeyboardCallbackFnPtr(key, scancode, action, mods);
                    }
                }
            );
        }
        if (gMouseButtonCallbackFnPtr) {
            glfwSetMouseButtonCallback(
                window,
                [](GLFWwindow*, int button, int action, int mods) {
                    if (gMouseButtonCallbackFnPtr) {
                        gMouseButtonCallbackFnPtr(button, action, mods);
                    }
                }
            );
        }
        if (gMousePosCallbackFnPtr) {
            glfwSetCursorPosCallback(
                window,
                [](GLFWwindow*, double xPos, double yPos) {
                    if (gMousePosCallbackFnPtr) {
                        gMousePosCallbackFnPtr(xPos, yPos);
                    }
                }
            );
        }
        if (gCharCallbackFnPtr) {
            glfwSetCharModsCallback(
                window,
                [](GLFWwindow*, unsigned int ch, int mod) {
                    if (gCharCallbackFnPtr) {
                        gCharCallbackFnPtr(ch, mod);
                    }
                }
            );
        }
        if (gMouseScrollCallbackFnPtr) {
            glfwSetScrollCallback(
                window,
                [](GLFWwindow*, double xOffset, double yOffset) {
                    if (gMouseScrollCallbackFnPtr) {
                        gMouseScrollCallbackFnPtr(xOffset, yOffset);
                    }
                }
            );
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

    _isInitialized = true;
    return true;
}

void Engine::terminate() {
    _shouldTerminate = true;
}

bool Engine::initNetwork() {
    try {
        _networkConnections = std::make_unique<core::NetworkManager>(
            core::ClusterManager::instance()->getNetworkMode()
        );
    }
    catch (const std::runtime_error& e) {
        MessageHandler::instance()->printError(
            "Initiating network connections failed! Error: '%s'", e.what()
        );
        return false;
    }

    // check in cluster configuration which it is
    if (core::ClusterManager::instance()->getNetworkMode() ==
        core::NetworkManager::NetworkMode::Remote)
    {
        MessageHandler::instance()->printDebug(
            "Matching ip address to find node in configuration"
        );
        _networkConnections->retrieveNodeId();
    }
    else {
        MessageHandler::instance()->printDebug(
            "Running locally as node %d",
            core::ClusterManager::instance()->getThisNodeId()
        );
    }

    // If the user has provided the node _id as an incorrect cmd argument then make the
    // _thisNode invalid
    if (core::ClusterManager::instance()->getThisNodeId() >=
        static_cast<int>(core::ClusterManager::instance()->getNumberOfNodes()) ||
        core::ClusterManager::instance()->getThisNodeId() < 0)
    {
        MessageHandler::instance()->printError(
            "This computer is not a part of the cluster configuration"
        );
        _networkConnections->close();
        return false;
    }

    if (!_networkConnections->init()) {
        return false;
    }

    return true;
}

bool Engine::initWindows() {
    core::Node* thisNode = core::ClusterManager::instance()->getThisNode();
    if (thisNode->getNumberOfWindows() == 0) {
        MessageHandler::instance()->printError("No windows exist in configuration");
        return false;
    }

    {
        int tmpGlfwVer[3];
        glfwGetVersion(&tmpGlfwVer[0], &tmpGlfwVer[1], &tmpGlfwVer[2]);
        MessageHandler::instance()->printInfo(
            "Using GLFW version %d.%d.%d", tmpGlfwVer[0], tmpGlfwVer[1], tmpGlfwVer[2]
        );
    }

    switch (_runMode) {
        case RunMode::OpenGL_3_3_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            _glslVersion = "#version 330 core";
            break;
        case RunMode::OpenGL_4_0_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            _glslVersion = "#version 400 core";
            break;

        case RunMode::OpenGL_4_1_Debug_Core_Profile:
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
            [[ fallthrough ]];
        case RunMode::OpenGL_4_1_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            _glslVersion = "#version 410 core";
            break;
        case RunMode::OpenGL_4_2_Debug_Core_Profile:
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
            [[fallthrough]];
        case RunMode::OpenGL_4_2_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            _glslVersion = "#version 420 core";
            break;
        case RunMode::OpenGL_4_3_Debug_Core_Profile:
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
            [[fallthrough]];
        case RunMode::OpenGL_4_3_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            _glslVersion = "#version 430 core";
            break;
        case RunMode::OpenGL_4_4_Debug_Core_Profile:
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
            [[fallthrough]];
        case RunMode::OpenGL_4_4_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            _glslVersion = "#version 440 core";
            break;
        case RunMode::OpenGL_4_5_Debug_Core_Profile:
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
            [[fallthrough]];
        case RunMode::OpenGL_4_5_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            _glslVersion = "#version 450 core";
            break;
        case RunMode::OpenGL_4_6_Debug_Core_Profile:
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
            [[fallthrough]];
        case RunMode::OpenGL_4_6_Core_Profile:
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
#ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            _glslVersion = "#version 460 core";
            break;
        default:
            _glslVersion = "#version 120";
            break;
    }

    if (_preWindowFn) {
        _preWindowFn();
    }

    _statistics = std::make_unique<core::Statistics>();

    GLFWwindow* share = nullptr;
    int lastWindowIdx = thisNode->getNumberOfWindows() - 1;
    for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
        if (i > 0) {
            share = thisNode->getWindow(0).getWindowHandle();
        }
        
        if (!thisNode->getWindow(i).openWindow(share, lastWindowIdx)) {
            MessageHandler::instance()->printError("Failed to open window %d", i);
            return false;
        }
    }

    glbinding::Binding::initialize(glfwGetProcAddress);

    // clear directly otherwise junk will be displayed on some OSs (OS X Yosemite)
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    if (!checkForOGLErrors()) {
        MessageHandler::instance()->printError("GLEW init triggered an OpenGL error");
    }

    // Window/Contexty creation callback
    if (thisNode->getNumberOfWindows() > 0) {
        share = thisNode->getWindow(0).getWindowHandle();

        if (_contextCreationFn) {
            _contextCreationFn(share);
        }
    }
    else {
        MessageHandler::instance()->printError("No windows created on this node");
        return false;
    }

    for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
        Window& win = thisNode->getWindow(i);
        win.init();
    }

    // init draw buffer resolution
    updateDrawBufferResolutions();
    waitForAllWindowsInSwapGroupToOpen();

    if (RunFrameLockCheckThread) {
        if (core::ClusterManager::instance()->getNumberOfNodes() > 1) {
            _thread = std::make_unique<std::thread>(updateFrameLockLoop, nullptr);
        }
    }

    // init swap group if enabled
    if (thisNode->isUsingSwapGroups()) {
        Window::initNvidiaSwapGroups();
    }

    return true;
}

void Engine::initOGL() {
    // Get OpenGL version
    int version[3];
    GLFWwindow* winHandle = getCurrentWindow().getWindowHandle();
    version[0] = glfwGetWindowAttrib(winHandle, GLFW_CONTEXT_VERSION_MAJOR);
    version[1] = glfwGetWindowAttrib(winHandle, GLFW_CONTEXT_VERSION_MINOR);
    version[2] = glfwGetWindowAttrib(winHandle, GLFW_CONTEXT_REVISION);

    MessageHandler::instance()->printInfo(
        "OpenGL version %d.%d.%d %s", version[0], version[1], version[2], "core profile"
    );

    MessageHandler::instance()->printInfo("Vendor: %s", glGetString(GL_VENDOR));
    MessageHandler::instance()->printInfo("Renderer: %s", glGetString(GL_RENDERER));

    if (core::ClusterManager::instance()->getNumberOfNodes() > 1) {
        std::string path = Settings::instance()->getCapturePath();
        path += "_node";
        path += std::to_string(core::ClusterManager::instance()->getThisNodeId());

        using CapturePath = Settings::CapturePath;
        Settings::instance()->setCapturePath(path, CapturePath::Mono);
        Settings::instance()->setCapturePath(path, CapturePath::LeftStereo);
        Settings::instance()->setCapturePath(path, CapturePath::RightStereo);
    }

    // init window opengl data
    getCurrentWindow().makeOpenGLContextCurrent(Window::Context::Shared);

    loadShaders();
    _statistics->initVBO();

    if (_initOpenGLFn) {
        MessageHandler::instance()->printInfo("Calling init callback");
        _initOpenGLFn();
        MessageHandler::instance()->printInfo("-------------------------------");
    }

    // create all textures, etc
    core::Node* thisNode = core::ClusterManager::instance()->getThisNode();
    for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
        thisNode->setCurrentWindowIndex(i);
        // set context to shared
        getCurrentWindow().initOGL();
        
        if (_screenShotFn) {
            // set callback
            auto callback = [this](core::Image* img, size_t size,
                                   core::ScreenCapture::EyeIndex idx, GLenum type)
            {
                invokeScreenShotCallback(img, size, idx, type);
            };
            
            Window& win = getCurrentWindow();
            // left channel (Mono and Stereo_Left)
            core::ScreenCapture* monoCapture = win.getScreenCapturePointer(
                Window::Eye::MonoOrLeft
            );
            if (monoCapture) {
                monoCapture->setCaptureCallback(callback);
            }
            // right channel (Stereo_Right)
            core::ScreenCapture* rightCapture = win.getScreenCapturePointer(
                Window::Eye::Right
            );
            if (rightCapture) {
                rightCapture->setCaptureCallback(callback);
            }
        }
    }

    // link all users to their viewports
    for (int w = 0; w < thisNode->getNumberOfWindows(); w++) {
        Window& winPtr = thisNode->getWindow(w);
        for (int i = 0; i < winPtr.getNumberOfViewports(); i++) {
            winPtr.getViewport(i).linkUserName();
        }
    }

    updateFrustums();

    // Add fonts
#ifdef SGCT_HAS_TEXT
    if (Settings::instance()->getOSDTextFontPath().empty()) {
        const bool success = text::FontManager::instance()->addFont(
            "SGCTFont",
            Settings::instance()->getOSDTextFontName()
        );
        if (!success) {
            text::FontManager::instance()->getFont(
                "SGCTFont",
                Settings::instance()->getOSDTextFontSize()
            );
        }
    }
    else {
        std::string tmpPath = Settings::instance()->getOSDTextFontPath() +
                              Settings::instance()->getOSDTextFontName();
        const bool success = text::FontManager::instance()->addFont(
            "SGCTFont",
            tmpPath,
            text::FontManager::FontPath::Local
        );
        if (!success) {
            text::FontManager::instance()->getFont(
                "SGCTFont",
                Settings::instance()->getOSDTextFontSize()
            );
        }
    }
#endif // SGCT_HAS_TEXT

    // init swap barrier is swap groups are active
    Window::setBarrier(true);
    Window::resetSwapGroupFrameNumber();

    for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
        // generate mesh (VAO and VBO)
        thisNode->getWindow(i).initContextSpecificOGL();
    }

    // check for errors
    checkForOGLErrors();

    MessageHandler::instance()->printInfo("Ready to render");
}

void Engine::clean() {
    MessageHandler::instance()->printInfo("Cleaning up");

    core::Node* thisNode = core::ClusterManager::instance()->getThisNode();
    if (_cleanUpFn) {
        if (thisNode && thisNode->getNumberOfWindows() > 0) {
            thisNode->getWindow(0).makeOpenGLContextCurrent(Window::Context::Shared);
        }
        _cleanUpFn();
    }

    MessageHandler::instance()->printInfo("Clearing all callbacks");
    clearAllCallbacks();

    // kill thread
    if (_thread) {
        MessageHandler::instance()->printDebug("Waiting for frameLock thread to finish");

        MutexManager::instance()->frameSyncMutex.lock();
        sRunUpdateFrameLockLoop = false;
        MutexManager::instance()->frameSyncMutex.unlock();

        _thread->join();
        _thread = nullptr;
        MessageHandler::instance()->printDebug("Done");
    }

    // de-init window and unbind swapgroups
    if (thisNode && core::ClusterManager::instance()->getNumberOfNodes() > 0) {
        for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
            thisNode->getWindow(i).close();
        }
    }

    // close TCP connections
    _networkConnections = nullptr;

    // Shared contex
    if (thisNode && thisNode->getNumberOfWindows() > 0) {
        thisNode->getWindow(0).makeOpenGLContextCurrent(Window::Context::Shared);
    }

    _statistics = nullptr;

    MessageHandler::instance()->printInfo(
        "Destroying shader manager and internal shaders"
    );
    ShaderManager::destroy();

    _shader.fboQuad.deleteProgram();
    _shader.fxaa.deleteProgram();
    _shader.overlay.deleteProgram();

    MessageHandler::instance()->printInfo("Destroying texture manager");
    TextureManager::destroy();

#ifdef SGCT_HAS_TEXT
    MessageHandler::instance()->printInfo("Destroying font manager");
    sgct::text::FontManager::destroy();
#endif // SGCT_HAS_TEXT

    // Window specific context
    if (thisNode && thisNode->getNumberOfWindows() > 0) {
        thisNode->getWindow(0).makeOpenGLContextCurrent(Window::Context::Window);
    }
    
    MessageHandler::instance()->printInfo("Destroying shared data");
    SharedData::destroy();
    
    MessageHandler::instance()->printInfo("Destroying cluster manager");
    core::ClusterManager::destroy();
    
    MessageHandler::instance()->printInfo("Destroying settings");
    Settings::destroy();

    MessageHandler::instance()->printInfo("Destroying message handler");
    MessageHandler::destroy();


    MessageHandler::instance()->printInfo("Destroying mutexes");
    MutexManager::destroy();

    // Close window and terminate GLFW
    MessageHandler::instance()->printInfo("Terminating glfw");
    glfwTerminate();
 
    MessageHandler::instance()->printDebug("Finished cleaning");
}

void Engine::clearAllCallbacks() {
    _drawFn = nullptr;
    _draw2DFn = nullptr;
    _preSyncFn = nullptr;
    _postSyncPreDrawFn = nullptr;
    _postDrawFn = nullptr;
    _initOpenGLFn = nullptr;
    _cleanUpFn = nullptr;
    _externalDecodeCallbackFn = nullptr;
    _externalStatusCallbackFn = nullptr;
    _dataTransferDecodeCallbackFn = nullptr;
    _dataTransferStatusCallbackFn = nullptr;
    _dataTransferAcknowledgeCallbackFn = nullptr;
    _contextCreationFn = nullptr;
    _screenShotFn = nullptr;

    gKeyboardCallbackFnPtr = nullptr;
    gMouseButtonCallbackFnPtr = nullptr;
    gMousePosCallbackFnPtr = nullptr;
    gMouseScrollCallbackFnPtr = nullptr;
    gDropCallbackFnPtr = nullptr;
}

bool Engine::frameLockPreStage() {
    using namespace core;

    double t0 = glfwGetTime();
    // from server to clients
    _networkConnections->sync(NetworkManager::SyncMode::SendDataToClients, *_statistics);
    _statistics->setSyncTime(static_cast<float>(glfwGetTime() - t0));

    // run only on clients/slaves
    if (ClusterManager::instance()->getIgnoreSync() ||
        _networkConnections->isComputerServer())
    {
        return true;
    }


    // not server
    t0 = glfwGetTime();
    while (_networkConnections->isRunning() && _isRunning) {
        if (_networkConnections->isSyncComplete()) {
            break;
        }

        if (UseSleepToWaitForNodes) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        else {
            std::unique_lock lk(MutexManager::instance()->frameSyncMutex);
            core::NetworkManager::cond.wait(lk);
        }

        // for debugging
        core::Network* conn;
        if (glfwGetTime() - t0 > 1.0) {
            // more than a second
            conn = _networkConnections->getSyncConnectionByIndex(0);
            if (_printSyncMessage && !conn->isUpdated()) {
                MessageHandler::instance()->printInfo(
                    "Slave: waiting for master... send frame %d != previous recv "
                    "frame %d\n\tNvidia swap groups: %s\n\tNvidia swap barrier: "
                    "%s\n\tNvidia universal frame number: %u\n\tSGCT frame "
                    "number: %u",
                    conn->getSendFrameCurrent(),
                    conn->getRecvFramePrevious(),
                    getCurrentWindow().isUsingSwapGroups() ? "enabled" : "disabled",
                    getCurrentWindow().isBarrierActive() ? "enabled" : "disabled",
                    getCurrentWindow().getSwapGroupFrameNumber(), _frameCounter
                );
            }

            if (glfwGetTime() - t0 > _syncTimeout) {
                // more than a minute
                MessageHandler::instance()->printError(
                    "Slave: no sync signal from master after %.1f seconds! Exiting...",
                    _syncTimeout
                );
                return false;
            }
        }
    }

    // A this point all data needed for rendering a frame is received.
    // Let's signal that back to the master/server.
    _networkConnections->sync(NetworkManager::SyncMode::AcknowledgeData, *_statistics);
    _statistics->addSyncTime(static_cast<float>(glfwGetTime() - t0));

    return true;
}

bool Engine::frameLockPostStage() {
    // post stage
    if (core::ClusterManager::instance()->getIgnoreSync() ||
        !_networkConnections->isComputerServer())
    {
        return true;
    }

    double t0 = glfwGetTime();
    while (_networkConnections->isRunning() && _isRunning &&
           _networkConnections->getActiveConnectionsCount() > 0)
    {
        if (_networkConnections->isSyncComplete()) {
            break;
        }

        if (UseSleepToWaitForNodes) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        else {
            std::unique_lock lk(MutexManager::instance()->frameSyncMutex);
            core::NetworkManager::cond.wait(lk);
        }

        // for debugging
        if (glfwGetTime() - t0 <= 1.0) {
            continue;
        }
        // more than a second

        for (int i = 0; i < _networkConnections->getSyncConnectionsCount(); i++) {
            const core::Network& conn = _networkConnections->getConnectionByIndex(i);

            if (_printSyncMessage && !conn.isUpdated()) {
                MessageHandler::instance()->printInfo(
                    "Waiting for slave%d: send frame %d != recv frame %d\n\t"
                    "Nvidia swap groups: %s\n\tNvidia swap barrier: %s\n\t"
                    "Nvidia universal frame number: %u\n\t"
                    "SGCT frame number: %u",
                    i,
                    _networkConnections->getConnectionByIndex(i).getSendFrameCurrent(),
                    _networkConnections->getConnectionByIndex(i).getRecvFrameCurrent(),
                    getCurrentWindow().isUsingSwapGroups() ? "enabled" : "disabled",
                    getCurrentWindow().isBarrierActive() ? "enabled" : "disabled",
                    getCurrentWindow().getSwapGroupFrameNumber(), _frameCounter
                );
            }
        }

        if (glfwGetTime() - t0 > _syncTimeout) {
            // more than a minute
            MessageHandler::instance()->printError(
                "Master: no sync signal from all slaves after %.1f seconds. Exiting",
                _syncTimeout
            );

            return false;
        }
    }
    _statistics->addSyncTime(static_cast<float>(glfwGetTime() - t0));

    return true;
}

void Engine::render() {
    if (!_isInitialized) {
        MessageHandler::instance()->printError(
            "Render function called before initialization"
        );
        return;
    }
    
    _isRunning = true;

    // create OpenGL query objects for Opengl 3.3+
    GLuint timeQueryBegin = 0;
    GLuint timeQueryEnd = 0;
    getCurrentWindow().makeOpenGLContextCurrent(Window::Context::Shared);
    glGenQueries(1, &timeQueryBegin);
    glGenQueries(1, &timeQueryEnd);

    core::Node* thisNode = core::ClusterManager::instance()->getThisNode();
    while (_isRunning) {
        _renderingOffScreen = false;

        // update tracking data
        if (isMaster()) {
            using namespace core;
            ClusterManager::instance()->getTrackingManager().updateTrackingDevices();
        }

        if (_preSyncFn) {
            _preSyncFn();
        }

        if (_networkConnections->isComputerServer()) {
            SharedData::instance()->encode();
        }
        else if (!_networkConnections->isRunning())  {
            // exit if not running
            MessageHandler::instance()->printError("Network disconnected! Exiting");
            break;
        }

        if (!frameLockPreStage()) {
            break;
        }

        // check if re-size needed of VBO and PBO
        // context switching may occur if multiple windows are used
        bool buffersNeedUpdate = false;
        for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
            const bool bufUpdate = thisNode->getWindow(i).update();
            buffersNeedUpdate |= bufUpdate;
        }

        if (buffersNeedUpdate) {
            updateDrawBufferResolutions();
        }
    
        _renderingOffScreen = true;
        getCurrentWindow().makeOpenGLContextCurrent(Window::Context::Shared);

        // Make sure correct context is current
        if (_postSyncPreDrawFn) {
            _postSyncPreDrawFn();
        }

        double startFrameTime = glfwGetTime();
        calculateFPS(startFrameTime); // measures time between calls

        if (_showGraph) {
            glQueryCounter(timeQueryBegin, GL_TIMESTAMP);
        }

        // Render Viewports / Draw
        _currentDrawBufferIndex = 0;
        size_t firstDrawBufferIndexInWindow = 0;

        for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
            if (!(thisNode->getWindow(i).isVisible() ||
                  thisNode->getWindow(i).isRenderingWhileHidden()))
            {
                continue;
            }

            // store the first buffer index for each window
            firstDrawBufferIndexInWindow = _currentDrawBufferIndex;

            // @TODO (abock, 2019-09-02): This is kinda weird; I tried commenting this
            // part out and only use _thisNode->getWindow(i) directly, but then it failed
            // to have two separate rendering windows. So some hidden state somewhere, I
            // guess?!
            thisNode->setCurrentWindowIndex(i);
            Window& win = getCurrentWindow();

            if (!_renderingOffScreen) {
                win.makeOpenGLContextCurrent(Window::Context::Window);
            }

            Window::StereoMode sm = win.getStereoMode();

            // Render Left/Mono non-linear projection viewports to cubemap
            _currentRenderTarget = RenderTarget::NonLinearBuffer;

            for (int j = 0; j < win.getNumberOfViewports(); j++) {
                core::Viewport& vp = win.getViewport(j);
                _currentViewportIndex.main = j;
                if (!vp.hasSubViewports()) {
                    continue;
                }

                core::NonLinearProjection* nonLinearProj = vp.getNonLinearProjection();

                nonLinearProj->setAlpha(getCurrentWindow().getAlpha() ? 0.f : 1.f);
                if (sm == Window::StereoMode::NoStereo) {
                    //for mono viewports frustum mode can be selected by user or xml
                    _currentFrustumMode = win.getViewport(j).getEye();
                    nonLinearProj->renderCubemap(&_currentViewportIndex.sub);
                }
                else {
                    _currentFrustumMode = core::Frustum::Mode::StereoLeftEye;
                    nonLinearProj->renderCubemap(&_currentViewportIndex.sub);
                }

                // FBO index, every window and every nonlinear projection has it's own FBO
                _currentDrawBufferIndex++;
            }

            // Render left/mono regular viewports to fbo
            _currentRenderTarget = RenderTarget::WindowBuffer;

            // if any stereo type (except passive) then set frustum mode to left eye
            if (sm == Window::StereoMode::NoStereo) {
                _currentFrustumMode = core::Frustum::Mode::MonoEye;
                renderViewports(LeftEye);
            }
            else {
                _currentFrustumMode = core::Frustum::Mode::StereoLeftEye;
                renderViewports(LeftEye);
            }

            // FBO index, every window and every non-linear projection has it's own FBO
            _currentDrawBufferIndex++;

            // if we are not rendering in stereo, we are done
            if (sm == Window::StereoMode::NoStereo) {
                continue;
            }

            // jump back counter to the first buffer index for current window
            _currentDrawBufferIndex = firstDrawBufferIndexInWindow;

            // Render right non-linear projection viewports to cubemap
            _currentRenderTarget = RenderTarget::NonLinearBuffer;
            for (int j = 0; j < win.getNumberOfViewports(); j++) {
                _currentViewportIndex.main = j;

                if (!win.getViewport(j).hasSubViewports()) {
                    continue;
                }
                core::NonLinearProjection* p = win.getViewport(j).getNonLinearProjection();

                p->setAlpha(getCurrentWindow().getAlpha() ? 0.f : 1.f);
                _currentFrustumMode = core::Frustum::Mode::StereoRightEye;
                p->renderCubemap(&_currentViewportIndex.sub);

                // FBO index, every window and every nonlinear projection has it's own
                _currentDrawBufferIndex++;
            }

            // Render right regular viewports to FBO
            _currentRenderTarget = RenderTarget::WindowBuffer;

            _currentFrustumMode = core::Frustum::Mode::StereoRightEye;
            // use a single texture for side-by-side and top-bottom stereo modes
            if (sm >= Window::StereoMode::SideBySide) {
                renderViewports(LeftEye);
            }
            else {
                renderViewports(RightEye);
            }

            // FBO index, every window and every non-linear projection has their own
            _currentDrawBufferIndex++;
        }

        // Render to screen
        for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
            if (thisNode->getWindow(i).isVisible()) {
                thisNode->setCurrentWindowIndex(i);

                _renderingOffScreen = false;
                renderFBOTexture();
            }
        }
        getCurrentWindow().makeOpenGLContextCurrent(Window::Context::Shared);

#ifdef __SGCT_DEBUG__
        checkForOGLErrors();
#endif

        if (_showGraph) {
            glQueryCounter(timeQueryEnd, GL_TIMESTAMP);
        }

        if (_postDrawFn) {
            _postDrawFn();
        }

        if (_showGraph) {
            // wait until the query results are available
            GLboolean done = GL_FALSE;
            while (!done) {
                glGetQueryObjectiv(timeQueryEnd, GL_QUERY_RESULT_AVAILABLE, &done);
            }

            GLuint64 timerStart;
            GLuint64 timerEnd;
            // get the query results
            glGetQueryObjectui64v(timeQueryBegin, GL_QUERY_RESULT, &timerStart);
            glGetQueryObjectui64v(timeQueryEnd, GL_QUERY_RESULT, &timerEnd);

            const double t = static_cast<double>(timerEnd - timerStart) / 1000000000.0;
            _statistics->setDrawTime(static_cast<float>(t));
        }

        if (_showGraph) {
            _statistics->update();
        }
        
        // master will wait for nodes render before swapping
        if (!frameLockPostStage()) {
            break;
        }

        // Swap front and back rendering buffers
        for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
            thisNode->getWindow(i).swap(_takeScreenshot);
        }

        glfwPollEvents();
        for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
            thisNode->getWindow(i).updateResolutions();
        }

        // Check if ESC key was pressed or window was closed
        _isRunning = !(thisNode->getKeyPressed(_exitKey) ||
                   thisNode->shouldAllWindowsClose() || _shouldTerminate ||
                   !_networkConnections->isRunning());

        // for all windows
        _frameCounter++;
        if (_takeScreenshot) {
            _shotCounter++;
        }
        _takeScreenshot = false;
    }

    getCurrentWindow().makeOpenGLContextCurrent(Window::Context::Shared);
    glDeleteQueries(1, &timeQueryBegin);
    glDeleteQueries(1, &timeQueryEnd);
}

void Engine::renderDisplayInfo() {
#ifdef SGCT_HAS_TEXT
    unsigned int lFrameNumber = getCurrentWindow().getSwapGroupFrameNumber();

    glm::vec4 strokeColor = sgct::text::FontManager::instance()->getStrokeColor();
    sgct::text::FontManager::instance()->setStrokeColor(glm::vec4(0.f, 0.f, 0.f, 0.8f));

    unsigned int font_size = Settings::instance()->getOSDTextFontSize();
    font_size = static_cast<unsigned int>(
        static_cast<float>(font_size) * getCurrentWindow().getScale().x
    );
    
    sgct::text::Font* font = sgct::text::FontManager::instance()->getFont(
        "SGCTFont",
        font_size
    );

    if (font) {
        float lineHeight = font->getHeight() * 1.59f;
        glm::vec2 pos = glm::vec2(getCurrentWindow().getResolution()) *
                        Settings::instance()->getOSDTextOffset();
        
        core::Node* thisNode = core::ClusterManager::instance()->getThisNode();
        sgct::text::print(
            *font,
            sgct::text::TextAlignMode::TopLeft,
            pos.x,
            lineHeight * 6.f + pos.y,
            glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
            "Node ip: %s (%s)",
            thisNode->getAddress().c_str(),
            _networkConnections->isComputerServer() ? "master" : "slave"
        );

        sgct::text::print(
            *font,
            sgct::text::TextAlignMode::TopLeft,
            pos.x,
            lineHeight * 5.f + pos.y,
            glm::vec4(0.8f,0.8f,0.f,1.f),
            "Frame rate: %.2f Hz, frame: %u",
            _statistics->getAvgFPS(),
            _frameCounter
        );

        sgct::text::print(
            *font,
            sgct::text::TextAlignMode::TopLeft,
            pos.x,
            lineHeight * 4.f + pos.y,
            glm::vec4(0.8f, 0.f, 0.8f, 1.f),
            "Avg. draw time: %.2f ms",
            _statistics->getAvgDrawTime() * 1000.f
        );

        if (isMaster()) {
            sgct::text::print(
                *font,
                sgct::text::TextAlignMode::TopLeft,
                pos.x,
                lineHeight * 3.f + pos.y,
                glm::vec4(0.f, 0.8f, 0.8f, 1.f),
                "Avg. sync time: %.2f ms (%d bytes, comp: %.3f)",
                _statistics->getAvgSyncTime() * 1000.0,
                SharedData::instance()->getUserDataSize(),
                SharedData::instance()->getCompressionRatio()
            );
        }
        else {
            sgct::text::print(
                *font,
                sgct::text::TextAlignMode::TopLeft,
                pos.x,
                lineHeight * 3.f + pos.y,
                glm::vec4(0.f, 0.8f, 0.8f, 1.f),
                "Avg. sync time: %.2f ms",
                _statistics->getAvgSyncTime() * 1000.0
            );
        }

        bool usingSwapGroups = getCurrentWindow().isUsingSwapGroups();
        if (usingSwapGroups) {
            sgct::text::print(
                *font,
                sgct::text::TextAlignMode::TopLeft,
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
            sgct::text::print(
                *font,
                sgct::text::TextAlignMode::TopLeft,
                pos.x,
                lineHeight * 2.f + pos.y,
                glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
                "Swap groups: Disabled"
            );
        }

        sgct::text::print(
            *font,
            sgct::text::TextAlignMode::TopLeft,
            pos.x,
            lineHeight * 1.f + pos.y,
            glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
            "Frame buffer resolution: %d x %d",
            getCurrentWindow().getFramebufferResolution().x,
            getCurrentWindow().getFramebufferResolution().y
        );

        // if active stereoscopic rendering
        if (_currentFrustumMode == core::Frustum::Mode::StereoLeftEye) {
            sgct::text::print(
                *font,
                sgct::text::TextAlignMode::TopLeft,
                pos.x,
                lineHeight * 8.f + pos.y,
                glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
                "Stereo type: %s\nCurrent eye: Left",
                getCurrentWindow().getStereoModeStr().c_str()
            );
        }
        else if (_currentFrustumMode == core::Frustum::Mode::StereoRightEye) {
            sgct::text::print(
                *font,
                sgct::text::TextAlignMode::TopLeft,
                pos.x,
                lineHeight * 8.f + pos.y,
                glm::vec4(0.8f, 0.8f, 0.8f, 1.f),
                "Stereo type: %s\nCurrent eye:          Right",
                getCurrentWindow().getStereoModeStr().c_str()
            );
        }
    }

    // reset
    sgct::text::FontManager::instance()->setStrokeColor(strokeColor);
#endif // SGCT_HAS_TEXT
}

void Engine::draw() {
    // run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);
    
    enterCurrentViewport();
    setAndClearBuffer(BufferMode::RenderToTexture);
    glDisable(GL_SCISSOR_TEST);

    if (_drawFn) {
        glLineWidth(1.0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        _drawFn();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void Engine::drawOverlays() {
    for (int i = 0; i < getCurrentWindow().getNumberOfViewports(); i++) {
        getCurrentWindow().setCurrentViewport(i);
        const core::Viewport& vp = getCurrentWindow().getViewport(i);
        
        // if viewport has overlay
        if (!vp.hasOverlayTexture() || !vp.isEnabled()) {
            continue;
        }

        enterCurrentViewport();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, vp.getOverlayTextureIndex());
        _shader.overlay.bind();
        glUniform1i(_shaderLoc.overlayTex, 0);
        getCurrentWindow().bindVAO();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        getCurrentWindow().unbindVAO();
        ShaderProgram::unbind();
    }
}

void Engine::prepareBuffer(TextureIndexes ti) {
    if (getCurrentWindow().usePostFX()) {
        ti = Intermediate;
    }

    core::OffScreenBuffer* fbo = getCurrentWindow().getFBO();

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
    core::OffScreenBuffer::unBind();

    bool maskShaderSet = false;

    Window& win = getCurrentWindow();
    win.makeOpenGLContextCurrent(Window::Context::Window);

    glDisable(GL_BLEND);
    // needed for shaders
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    _currentFrustumMode = (win.getStereoMode() == Window::StereoMode::Active) ?
        core::Frustum::Mode::StereoLeftEye :
        core::Frustum::Mode::MonoEye;

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

        _shader.fboQuad.bind();
        glUniform1i(_shaderLoc.monoTex, 0);
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
            _currentFrustumMode = core::Frustum::Mode::StereoRightEye;
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
            _shader.fboQuad.bind();
            glUniform1i(_shaderLoc.monoTex, 0);
        }
        
        glDrawBuffer(win.isDoubleBuffered() ? GL_BACK : GL_FRONT);
        glReadBuffer(win.isDoubleBuffered() ? GL_BACK : GL_FRONT);
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_BLEND);

        // Result = (Color * BlendMask) * (1-BlackLevel) + BlackLevel

        // render blend masks
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        for (int i = 0; i < win.getNumberOfViewports(); i++) {
            const core::Viewport& vp = win.getViewport(i);
            if (vp.hasBlendMaskTexture() && vp.isEnabled()) {
                glBindTexture(GL_TEXTURE_2D, vp.getBlendMaskTextureIndex());
                vp.renderMaskMesh();
            }
        }

        // render black level masks
        for (int i = 0; i < win.getNumberOfViewports(); i++) {
            const core::Viewport& vp = win.getViewport(i);
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

void Engine::renderViewports(TextureIndexes ti) {
    prepareBuffer(ti);

    Window::StereoMode sm = getCurrentWindow().getStereoMode();
    
    // render all viewports for selected eye
    for (int i = 0; i < getCurrentWindow().getNumberOfViewports(); i++) {
        getCurrentWindow().setCurrentViewport(i);
        _currentViewportIndex.main = i;
        core::Viewport& vp = getCurrentWindow().getViewport(i);

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
                    _nearClippingPlaneDist,
                    _farClippingPlaneDist
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
                    _currentFrustumMode,
                    _nearClippingPlaneDist,
                    _farClippingPlaneDist
                );
            }

            // check if we want to copy the previous window into this one before we go
            // ahead with anyting else
            if (getCurrentWindow().getCopyPreviousWindowToCurrentWindow()) {
                copyPreviousWindowViewportToCurrentWindowViewport(_currentFrustumMode);
            }

            if (getCurrentWindow().getCallDraw3DFunction()) {
                draw();
            }
        }
    }

    // If we did not render anything, make sure we clear the screen at least
    if (!getCurrentWindow().getCallDraw3DFunction() &&
        !getCurrentWindow().getCopyPreviousWindowToCurrentWindow())
    {
        setAndClearBuffer(BufferMode::RenderToTexture);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    // if side-by-side and top-bottom mode only do post fx and blit only after rendered
    // right eye
    bool splitScreenStereo = (sm >= Window::StereoMode::SideBySide);
    if (!(splitScreenStereo &&
        _currentFrustumMode == core::Frustum::Mode::StereoLeftEye))
    {
        if (getCurrentWindow().usePostFX()) {
            // blit buffers
            updateRenderingTargets(ti); // only used if multisampled FBOs

            renderPostFX(ti);

            render2D();
            if (splitScreenStereo) {
                // render left eye info and graph so that all 2D items are rendered after
                // post fx
                _currentFrustumMode = core::Frustum::Mode::StereoLeftEye;
                render2D();
            }
        }
        else {
            render2D();
            if (splitScreenStereo) {
                // render left eye info and graph so that all 2D items are rendered after
                // post fx
                _currentFrustumMode = core::Frustum::Mode::StereoLeftEye;
                render2D();
            }

            updateRenderingTargets(ti); // only used if multisampled FBOs
        }
    }

    glDisable(GL_BLEND);
}

void Engine::render2D() {
    // draw viewport overlays if any
    drawOverlays();

    // draw info & stats
    // the cubemap viewports are all the same so it makes no sense to render everything
    // several times therefore just loop one iteration in that case.
    if (!(_showGraph || _showInfo || _draw2DFn)) {
        return;
    }

    for (int i = 0; i < getCurrentWindow().getNumberOfViewports(); i++) {
        getCurrentWindow().setCurrentViewport(i);
        _currentViewportIndex.main = i;
            
        if (!getCurrentWindow().getCurrentViewport()->isEnabled()) {
            continue;
        }
        enterCurrentViewport();

        if (_showGraph) {
            _statistics->draw(
                static_cast<float>(getCurrentWindow().getFramebufferResolution().y) /
                static_cast<float>(getCurrentWindow().getResolution().y)
            );
        }
        // The text renderer enters automatically the correct viewport
        if (_showInfo) {
            // choose specified eye from config
            if (getCurrentWindow().getStereoMode() == Window::StereoMode::NoStereo) {
                _currentFrustumMode = getCurrentWindow().getCurrentViewport()->getEye();
            }
            renderDisplayInfo();
        }

        // Check if we should call the use defined draw2D function
        if (_draw2DFn && getCurrentWindow().getCallDraw2DFunction()) {
            _draw2DFn();
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
        PostFX* lastFx = (numberOfPasses > 0) ?
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

        _shader.fxaa.bind();
        glUniform1f(_shaderLoc.sizeX, static_cast<float>(framebufferSize.x));
        glUniform1f(_shaderLoc.sizeY, static_cast<float>(framebufferSize.y));
        glUniform1i(_shaderLoc.fxaaTexture, 0);
        glUniform1f(
            _shaderLoc.fxaaSubPixTrim,
            Settings::instance()->getFXAASubPixTrim()
        );
        glUniform1f(
            _shaderLoc.fxaaSubPixOffset,
            Settings::instance()->getFXAASubPixOffset()
        );

        getCurrentWindow().bindVAO();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        getCurrentWindow().unbindVAO();

        ShaderProgram::unbind();
    }
}

void Engine::updateRenderingTargets(TextureIndexes ti) {
    // copy AA-buffer to "regular"/non-AA buffer
    core::OffScreenBuffer* fbo = getCurrentWindow().getFBO();
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

void Engine::loadShaders() {
    _shader.fxaa.setName("FXAAShader");
    std::string fxaaVertShader = core::shaders::FXAAVert;
    std::string fxaaFragShader = core::shaders::FXAAFrag;
    
    helpers::findAndReplace(fxaaVertShader, "**glsl_version**", getGLSLVersion());
    helpers::findAndReplace(fxaaFragShader, "**glsl_version**", getGLSLVersion());

    const bool fxaaVertSuccess = _shader.fxaa.addShaderSrc(
        fxaaVertShader,
        GL_VERTEX_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!fxaaVertSuccess) {
        MessageHandler::instance()->printError("Failed to load FXAA vertex shader");
    }

    const bool fxaaFragSuccess = _shader.fxaa.addShaderSrc(
        fxaaFragShader,
        GL_FRAGMENT_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!fxaaFragSuccess) {
        MessageHandler::instance()->printError("Failed to load FXAA fragment shader");
    }
    _shader.fxaa.createAndLinkProgram();
    _shader.fxaa.bind();

    _shaderLoc.sizeX = _shader.fxaa.getUniformLocation("rt_w");
    const glm::ivec2 framebufferSize = getCurrentWindow().getFramebufferResolution();
    glUniform1f(_shaderLoc.sizeX, static_cast<float>(framebufferSize.x));

    _shaderLoc.sizeY = _shader.fxaa.getUniformLocation("rt_h");
    glUniform1f(_shaderLoc.sizeY, static_cast<float>(framebufferSize.y));

    _shaderLoc.fxaaSubPixTrim = _shader.fxaa.getUniformLocation("FXAA_SUBPIX_TRIM");
    glUniform1f(_shaderLoc.fxaaSubPixTrim, Settings::instance()->getFXAASubPixTrim());

    _shaderLoc.fxaaSubPixOffset = _shader.fxaa.getUniformLocation("FXAA_SUBPIX_OFFSET");
    glUniform1f(_shaderLoc.fxaaSubPixOffset, Settings::instance()->getFXAASubPixOffset());

    _shaderLoc.fxaaTexture = _shader.fxaa.getUniformLocation("tex");
    glUniform1i(_shaderLoc.fxaaTexture, 0);

    ShaderProgram::unbind();

    // Used for overlays & mono.
    std::string fboQuadVertShader;
    std::string fboQuadFragShader;
    fboQuadVertShader = core::shaders::BaseVert;
    fboQuadFragShader = core::shaders::BaseFrag;
        
    const std::string glslVersion = getGLSLVersion();

    helpers::findAndReplace(fboQuadVertShader, "**glsl_version**", glslVersion);
    helpers::findAndReplace(fboQuadFragShader, "**glsl_version**", glslVersion);
        
    _shader.fboQuad.setName("FBOQuadShader");
    const bool quadVertSuccess = _shader.fboQuad.addShaderSrc(
        fboQuadVertShader,
        GL_VERTEX_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!quadVertSuccess) {
        MessageHandler::instance()->printError("Failed to load FBO quad vertex shader");
    }
    const bool quadFragSuccess = _shader.fboQuad.addShaderSrc(
        fboQuadFragShader,
        GL_FRAGMENT_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!quadFragSuccess) {
        MessageHandler::instance()->printError("Failed to load FBO quad fragment shader");
    }
    _shader.fboQuad.createAndLinkProgram();
    _shader.fboQuad.bind();
    _shaderLoc.monoTex = _shader.fboQuad.getUniformLocation("Tex");
    glUniform1i(_shaderLoc.monoTex, 0);
    ShaderProgram::unbind();
        
    std::string overlayVertShader;
    std::string overlayFragShader;
    overlayVertShader = core::shaders::OverlayVert;
    overlayFragShader = core::shaders::OverlayFrag;
        
    //replace glsl version
    helpers::findAndReplace(overlayVertShader, "**glsl_version**", getGLSLVersion());
    helpers::findAndReplace(overlayFragShader, "**glsl_version**", getGLSLVersion());
        
    _shader.overlay.setName("OverlayShader");
    const bool overlayVertSuccess = _shader.overlay.addShaderSrc(
        overlayVertShader,
        GL_VERTEX_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!overlayVertSuccess) {
        MessageHandler::instance()->printError("Failed to load overlay vertex shader");
    }
    const bool overlayFragSuccess = _shader.overlay.addShaderSrc(
        overlayFragShader,
        GL_FRAGMENT_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!overlayFragSuccess) {
        MessageHandler::instance()->printError("Failed to load overlay fragment shader");
    }
    _shader.overlay.createAndLinkProgram();
    _shader.overlay.bind();
    _shaderLoc.overlayTex = _shader.overlay.getUniformLocation("Tex");
    glUniform1i(_shaderLoc.overlayTex, 0);
    ShaderProgram::unbind();
}

void Engine::setAndClearBuffer(BufferMode mode) {
    if (mode < BufferMode::RenderToTexture) {
        const bool doubleBuffered = getCurrentWindow().isDoubleBuffered();
        // Set buffer
        if (getCurrentWindow().getStereoMode() != Window::StereoMode::Active) {
            glDrawBuffer(doubleBuffered ? GL_BACK : GL_FRONT);
            glReadBuffer(doubleBuffered ? GL_BACK : GL_FRONT);
        }
        else if (_currentFrustumMode == core::Frustum::Mode::StereoLeftEye) {
            // if active left
            glDrawBuffer(doubleBuffered ? GL_BACK_LEFT : GL_FRONT_LEFT);
            glReadBuffer(doubleBuffered ? GL_BACK_LEFT : GL_FRONT_LEFT);
        }
        else if (_currentFrustumMode == core::Frustum::Mode::StereoRightEye) {
            // if active right
            glDrawBuffer(doubleBuffered ? GL_BACK_RIGHT : GL_FRONT_RIGHT);
            glReadBuffer(doubleBuffered ? GL_BACK_RIGHT : GL_FRONT_RIGHT);
        }
    }

    // clear
    if (mode != BufferMode::BackBufferBlack) {
        clearBuffer();
    }
    else {
        // when rendering textures to backbuffer (using fbo)
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
}

bool Engine::checkForOGLErrors() {
    MessageHandler& mh = *MessageHandler::instance();

    const GLenum oglError = glGetError();
    switch (oglError) {
        case GL_NO_ERROR:
            break;
        case GL_INVALID_ENUM:
            mh.printError("OpenGL error: GL_INVALID_ENUM");
            break;
        case GL_INVALID_VALUE:
            mh.printError("OpenGL error: GL_INVALID_VALUE");
            break;
        case GL_INVALID_OPERATION:
            mh.printError("OpenGL error: GL_INVALID_OPERATION");
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            mh.printError("OpenGL error: GL_INVALID_FRAMEBUFFER_OPERATION");
            break;
        case GL_STACK_OVERFLOW:
            mh.printError("OpenGL error: GL_STACK_OVERFLOW");
            break;
        case GL_STACK_UNDERFLOW:
            mh.printError("OpenGL error: GL_STACK_UNDERFLOW");
            break;
        case GL_OUT_OF_MEMORY:
            mh.printError("OpenGL error: GL_OUT_OF_MEMORY");
            break;
        case GL_TABLE_TOO_LARGE:
            mh.printError("OpenGL error: GL_TABLE_TOO_LARGE");
            break;
        default:
            mh.printError("OpenGL error: Unknown error %i", static_cast<int>(oglError));
            break;
    }

    return oglError == GL_NO_ERROR;
}

bool Engine::isMaster() const {
    return _networkConnections->isComputerServer();
}

bool Engine::isDisplayInfoRendered() const {
    return _showInfo;
}

core::Frustum::Mode Engine::getCurrentFrustumMode() const {
    return _currentFrustumMode;
}

const glm::mat4& Engine::getCurrentProjectionMatrix() const {
    const core::BaseViewport& vp = *getCurrentWindow().getCurrentViewport();
    return vp.getProjection(_currentFrustumMode).getProjectionMatrix();
}

const glm::mat4& Engine::getCurrentViewMatrix() const {
    const core::BaseViewport& vp = *getCurrentWindow().getCurrentViewport();
    return vp.getProjection(_currentFrustumMode).getViewMatrix();
}

const glm::mat4& Engine::getModelMatrix() const {
    return core::ClusterManager::instance()->getSceneTransform();
}

const glm::mat4& Engine::getCurrentViewProjectionMatrix() const {
    const core::BaseViewport& vp = *getCurrentWindow().getCurrentViewport();
    return vp.getProjection(_currentFrustumMode).getViewProjectionMatrix();
}

glm::mat4 Engine::getCurrentModelViewProjectionMatrix() const {
    const core::BaseViewport& vp = *getCurrentWindow().getCurrentViewport();
    return vp.getProjection(_currentFrustumMode).getViewProjectionMatrix() * 
           core::ClusterManager::instance()->getSceneTransform();
}

glm::mat4 Engine::getCurrentModelViewMatrix() const {
    const core::BaseViewport& vp = *getCurrentWindow().getCurrentViewport();
    return vp.getProjection(_currentFrustumMode).getViewMatrix() *
           core::ClusterManager::instance()->getSceneTransform();
}

unsigned int Engine::getCurrentFrameNumber() const {
    return _frameCounter;
}

const std::string& Engine::getGLSLVersion() const {
    return _glslVersion;
}

void Engine::waitForAllWindowsInSwapGroupToOpen() {
    core::Node* thisNode = core::ClusterManager::instance()->getThisNode();

    // clear the buffers initially
    for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
        thisNode->getWindow(i).makeOpenGLContextCurrent(Window::Context::Window);
        glDrawBuffer(getCurrentWindow().isDoubleBuffered() ? GL_BACK : GL_FRONT);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (thisNode->getWindow(i).isDoubleBuffered()) {
            glfwSwapBuffers(thisNode->getWindow(i).getWindowHandle());
        }
        else {
            glFinish();
        }
    }
    glfwPollEvents();
    
    // Must wait until all nodes are running if using swap barrier
    if (!core::ClusterManager::instance()->getIgnoreSync() &&
        core::ClusterManager::instance()->getNumberOfNodes() > 1)
    {
        // check if swapgroups are supported
#ifdef WIN32
        if (glfwExtensionSupported("WGL_NV_swap_group")) {
            MessageHandler::instance()->printInfo(
                "Swap groups are supported by hardware"
            );
        }
        else {
            MessageHandler::instance()->printInfo(
                "Swap groups are not supported by hardware"
            );
        }
#else
        if (glfwExtensionSupported("GLX_NV_swap_group")) {
            MessageHandler::instance()->printInfo(
                "Swap groups are supported by hardware"
            );
        }
        else {
            MessageHandler::instance()->printInfo(
                "Swap groups are not supported by hardware"
            );
        }
#endif

        MessageHandler::instance()->printInfo("Waiting for all nodes to connect");
        MessageHandler::instance()->setShowTime(false);
        
        while (_networkConnections->isRunning() && !thisNode->getKeyPressed(_exitKey) &&
               !thisNode->shouldAllWindowsClose() && !_shouldTerminate)
        {
            // Swap front and back rendering buffers
            for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                if (thisNode->getWindow(i).isDoubleBuffered()) {
                    glfwSwapBuffers(thisNode->getWindow(i).getWindowHandle());
                }
                else {
                    glFinish();
                }
            }
            glfwPollEvents();

            if (_networkConnections->areAllNodesConnected()) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // wait for user to release exit key
        while (thisNode->getKeyPressed(_exitKey)) {
            // Swap front and back rendering buffers
            // key buffers also swapped
            for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                if (thisNode->getWindow(i).isDoubleBuffered()) {
                    glfwSwapBuffers(thisNode->getWindow(i).getWindowHandle());
                }
                else {
                    glFinish();
                }
            }
            glfwPollEvents();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        MessageHandler::instance()->setShowTime(true);
    }
}

void Engine::updateFrustums() {
    core::Node* thisNode = core::ClusterManager::instance()->getThisNode();

    // @TODO (abock, 2019-10-11) Can thisNode actually be nullptr?  I think we are
    // checking somewhere that a node has to be created
    if (thisNode == nullptr) {
        return;
    }

    for (int w = 0; w < thisNode->getNumberOfWindows(); w++) {
        Window& win = thisNode->getWindow(w);
        for (int i = 0; i < win.getNumberOfViewports(); i++) {
            core::Viewport& vp = win.getViewport(i);
            if (vp.isTracked()) {
                // if not tracked update, otherwise this is done on the fly
                continue;
            }

            if (vp.hasSubViewports()) {
                core::NonLinearProjection& proj = *vp.getNonLinearProjection();
                proj.updateFrustums(
                    core::Frustum::Mode::MonoEye,
                    _nearClippingPlaneDist,
                    _farClippingPlaneDist
                );

                proj.updateFrustums(
                    core::Frustum::Mode::StereoLeftEye,
                    _nearClippingPlaneDist,
                    _farClippingPlaneDist
                );

                proj.updateFrustums(
                    core::Frustum::Mode::StereoRightEye,
                    _nearClippingPlaneDist,
                    _farClippingPlaneDist
                );
            }
            else {
                vp.calculateFrustum(
                    core::Frustum::Mode::MonoEye,
                    _nearClippingPlaneDist,
                    _farClippingPlaneDist
                );

                vp.calculateFrustum(
                    core::Frustum::Mode::StereoLeftEye,
                    _nearClippingPlaneDist,
                    _farClippingPlaneDist
                );

                vp.calculateFrustum(
                    core::Frustum::Mode::StereoRightEye,
                    _nearClippingPlaneDist,
                    _farClippingPlaneDist
                );
            }
        }
    }
}

void Engine::copyPreviousWindowViewportToCurrentWindowViewport(core::Frustum::Mode mode) {
    // Check that we have a previous window
    if (getCurrentWindowIndex() < 1) {
        MessageHandler::instance()->printWarning(
            "Could not copy from previous window, as this window is the first one"
        );
        return;
    }

    Window& previousWindow = getWindow(getCurrentWindowIndex() - 1);

    // run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);
    enterCurrentViewport();
    setAndClearBuffer(BufferMode::RenderToTexture);
    glDisable(GL_SCISSOR_TEST);

    _shader.overlay.bind();

    glUniform1i(_shaderLoc.overlayTex, 0);

    glActiveTexture(GL_TEXTURE0);
    TextureIndexes m = [](core::Frustum::Mode m) {
        switch (m) {
            // abock (2019-09-27) Yep, I'm confused about this mapping, too. But I just
            // took the enumerations values as they were and I assume that it was an
            // undetected bug
            default:
            case core::Frustum::Mode::MonoEye: return TextureIndexes::LeftEye;
            case core::Frustum::Mode::StereoLeftEye: return TextureIndexes::RightEye;
            case core::Frustum::Mode::StereoRightEye: return TextureIndexes::Intermediate;
        }
    }(mode);
    glBindTexture(GL_TEXTURE_2D, previousWindow.getFrameBufferTexture(m));

    getCurrentWindow().bindVAO();

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    getCurrentWindow().unbindVAO();
    ShaderProgram::unbind();
}

void Engine::setDrawFunction(std::function<void()> fn) {
    _drawFn = std::move(fn);
}

void Engine::setDraw2DFunction(std::function<void()> fn) {
    _draw2DFn = std::move(fn);
}

void Engine::setPreSyncFunction(std::function<void()> fn) {
    _preSyncFn = std::move(fn);
}

void Engine::setPostSyncPreDrawFunction(std::function<void()> fn) {
    _postSyncPreDrawFn = std::move(fn);
}

void Engine::setPostDrawFunction(std::function<void()> fn) {
    _postDrawFn = std::move(fn);
}

void Engine::setInitOGLFunction(std::function<void()> fn) {
    _initOpenGLFn = std::move(fn);
}

void Engine::setPreWindowFunction(std::function<void()> fn) {
    _preWindowFn = std::move(fn);
}

void Engine::setCleanUpFunction(std::function<void()> fn) {
    _cleanUpFn = std::move(fn);
}

void Engine::setExternalControlCallback(std::function<void(const char*, int)> fn) {
    _externalDecodeCallbackFn = std::move(fn);
}

void Engine::setExternalControlStatusCallback(std::function<void(bool)> fn) {
    _externalStatusCallbackFn = std::move(fn);
}

void Engine::setDataTransferCallback(std::function<void(void*, int, int, int)> fn) {
    _dataTransferDecodeCallbackFn = std::move(fn);
}

void Engine::setDataTransferStatusCallback(std::function<void(bool, int)> fn) {
    _dataTransferStatusCallbackFn = std::move(fn);
}

void Engine::setDataAcknowledgeCallback(std::function<void(int, int)> fn) {
    _dataTransferAcknowledgeCallbackFn = std::move(fn);
}

void Engine::setContextCreationCallback(std::function<void(GLFWwindow*)> fn) {
    _contextCreationFn = std::move(fn);
}

void Engine::setScreenShotCallback(std::function<void(core::Image*, size_t,
                                   core::ScreenCapture::EyeIndex, GLenum type)> fn)
{
    _screenShotFn = std::move(fn);
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

 void Engine::setTouchCallbackFunction(std::function<void(const core::Touch*)> fn) {
     gTouchCallbackFnPtr = std::move(fn);
 }

void Engine::clearBuffer() {
    glm::vec4 color = Engine::instance()->getClearColor();

    const float alpha = instance()->getCurrentWindow().getAlpha() ? 0.f : color.a;
    glClearColor(color.r, color.g, color.b, alpha);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Engine::enterCurrentViewport() {
    core::BaseViewport* vp = getCurrentWindow().getCurrentViewport();
    
    const glm::vec2 res = glm::vec2(getCurrentWindow().getFramebufferResolution());
        
    const glm::vec2 p = vp->getPosition() * res;
    const glm::vec2 s = vp->getSize() * res;
    _currentViewportCoords = glm::ivec4(glm::ivec2(p), glm::ivec2(s));

    Window::StereoMode sm = getCurrentWindow().getStereoMode();
    if (sm >= Window::StereoMode::SideBySide) {
        if (_currentFrustumMode == core::Frustum::Mode::StereoLeftEye) {
            switch (sm) {
                case Window::StereoMode::SideBySide:
                    _currentViewportCoords.x /= 2;
                    _currentViewportCoords.z /= 2;
                    break;
                case Window::StereoMode::SideBySideInverted:
                    _currentViewportCoords.x =
                        (_currentViewportCoords.x / 2) + (_currentViewportCoords.z / 2);
                    _currentViewportCoords.z = _currentViewportCoords.z / 2;
                    break;
                case Window::StereoMode::TopBottom:
                    _currentViewportCoords.y =
                        (_currentViewportCoords.y / 2) + (_currentViewportCoords.w / 2);
                    _currentViewportCoords.w /= 2;
                    break;
                case Window::StereoMode::TopBottomInverted:
                    _currentViewportCoords.y /= 2;
                    _currentViewportCoords.w /= 2;
                    break;
                default:
                    break;
            }
        }
        else {
            switch (sm) {
                case Window::StereoMode::SideBySide:
                    _currentViewportCoords.x =
                        (_currentViewportCoords.x / 2) + (_currentViewportCoords.z / 2);
                    _currentViewportCoords.z /= 2;
                    break;
                case Window::StereoMode::SideBySideInverted:
                    _currentViewportCoords.x /= 2;
                    _currentViewportCoords.z /= 2;
                    break;
                case Window::StereoMode::TopBottom:
                    _currentViewportCoords.y /= 2;
                    _currentViewportCoords.w /= 2;
                    break;
                case Window::StereoMode::TopBottomInverted:
                    _currentViewportCoords.y =
                        (_currentViewportCoords.y / 2) + (_currentViewportCoords.w / 2);
                    _currentViewportCoords.w /= 2;
                    break;
                default:
                    break;
            }
        }
    }

    glViewport(
        _currentViewportCoords.x,
        _currentViewportCoords.y,
        _currentViewportCoords.z,
        _currentViewportCoords.w
    );
    
    glScissor(
        _currentViewportCoords.x,
        _currentViewportCoords.y,
        _currentViewportCoords.z,
        _currentViewportCoords.w
    );
}

void Engine::calculateFPS(double timestamp) {
    // @TODO (abock, 2019-08-29): I don't know why these variables are specified as static
    // but I guess they don't need to be

    // @TODO (abock, 2019-08-29): Also; i don't know why updateAAInfo should be called
    // once per second.  It would be better to listen to something that can signal when it
    // needs to be called instead.
    static double lastTimestamp = glfwGetTime();
    _statistics->setFrameTime(static_cast<float>(timestamp - lastTimestamp));
    lastTimestamp = timestamp;
    static float renderedFrames = 0.f;
    static float tmpTime = 0.f;
    renderedFrames += 1.f;
    tmpTime += _statistics->getFrameTime();
    if (tmpTime >= 1.f) {
        _statistics->setAvgFPS(renderedFrames / tmpTime);
        renderedFrames = 0.f;
        tmpTime = 0.f;
    }
}

double Engine::getDt() const {
    return _statistics->getFrameTime();
}

double Engine::getAvgFPS() const {
    return _statistics->getAvgFPS();
}

double Engine::getAvgDt() const {
    return _statistics->getAvgFrameTime();
}

double Engine::getMinDt() const {
    return _statistics->getMinFrameTime();
}

double Engine::getMaxDt() const {
    return _statistics->getMaxFrameTime();
}

double Engine::getDtStandardDeviation() const {
    return _statistics->getFrameTimeStandardDeviation();
}

double Engine::getDrawTime() const {
    return _statistics->getDrawTime();
}

double Engine::getSyncTime() const {
    return _statistics->getSyncTime();
}

glm::vec4 Engine::getClearColor() const {
    return _clearColor;
}

float Engine::getNearClippingPlane() const {
    return _nearClippingPlaneDist;
}

float Engine::getFarClippingPlane() const {
    return _farClippingPlaneDist;
}

void Engine::setNearAndFarClippingPlanes(float nearClip, float farClip) {
    _nearClippingPlaneDist = nearClip;
    _farClippingPlaneDist = farClip;
    updateFrustums();
}

void Engine::setEyeSeparation(float eyeSeparation) {
    core::Node* thisNode = core::ClusterManager::instance()->getThisNode();
    for (int w = 0; w < thisNode->getNumberOfWindows(); w++) {
        Window& window = thisNode->getWindow(w);

        for (int i = 0; i < window.getNumberOfViewports(); i++) {
            window.getViewport(i).getUser().setEyeSeparation(eyeSeparation);
        }
    }
    updateFrustums();
}

void Engine::setClearColor(glm::vec4 color) {
    _clearColor = std::move(color);
}

void Engine::setExitKey(int key) {
    _exitKey = key;
}

void Engine::addPostFX(PostFX fx) {
    core::Node* thisNode = core::ClusterManager::instance()->getThisNode();
    for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
        thisNode->getWindow(i).setUsePostFX(true);
        thisNode->getWindow(i).addPostFX(fx);
    }
}

unsigned int Engine::getCurrentDrawTexture() const {
    if (getCurrentWindow().usePostFX()) {
        return getCurrentWindow().getFrameBufferTexture(Intermediate);
    }
    else {
        return getCurrentWindow().getFrameBufferTexture(
            (_currentFrustumMode == core::Frustum::Mode::StereoRightEye) ?
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

int Engine::getFocusedWindowIndex() const {
    core::Node* thisNode = core::ClusterManager::instance()->getThisNode();
    for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
        if (thisNode->getWindow(i).isFocused()) {
            return i;
        }
    }
    return 0; // no window has focus
}

void Engine::setDisplayInfoVisibility(bool state) {
    _showInfo = state;
}

void Engine::setStatsGraphVisibility(bool state) {
    _showGraph = state;
}

void Engine::takeScreenshot() {
    _takeScreenshot = true;
}

void Engine::invokeDecodeCallbackForExternalControl(const char* receivedData,
                                                    int receivedLength, int)
{
    if (_externalDecodeCallbackFn && receivedLength > 0) {
        _externalDecodeCallbackFn(receivedData, receivedLength);
    }
}

void Engine::invokeUpdateCallbackForExternalControl(bool connected) {
    if (_externalStatusCallbackFn) {
        _externalStatusCallbackFn(connected);
    }
}

void Engine::invokeDecodeCallbackForDataTransfer(void* receivedData, int length,
                                                 int packageId, int clientId)
{
    if (_dataTransferDecodeCallbackFn && length > 0) {
        _dataTransferDecodeCallbackFn(receivedData, length, packageId, clientId);
    }
}

void Engine::invokeUpdateCallbackForDataTransfer(bool connected, int clientId) {
    if (_dataTransferStatusCallbackFn) {
        _dataTransferStatusCallbackFn(connected, clientId);
    }
}

void Engine::invokeAcknowledgeCallbackForDataTransfer(int packageId, int clientId) {
    if (_dataTransferAcknowledgeCallbackFn) {
        _dataTransferAcknowledgeCallbackFn(packageId, clientId);
    }
}

void Engine::invokeScreenShotCallback(core::Image* imPtr, size_t winIndex,
                                      core::ScreenCapture::EyeIndex ei, GLenum type)
{
    if (_screenShotFn) {
        _screenShotFn(imPtr, winIndex, ei, type);
    }
}

void Engine::sendMessageToExternalControl(const void* data, int length) {
    if (_networkConnections->getExternalControlConnection()) {
        _networkConnections->getExternalControlConnection()->sendData(data, length);
    }
}

void Engine::setDataTransferCompression(bool state, int level) {
    _networkConnections->setDataTransferCompression(state, level);
}

void Engine::transferDataBetweenNodes(const void* data, int length, int packageId) {
    _networkConnections->transferData(data, length, packageId);
}

void Engine::transferDataToNode(const void* data, int length, int packageId,
                                size_t nodeIndex)
{
    _networkConnections->transferData(data, length, packageId, nodeIndex);
}

void Engine::sendMessageToExternalControl(const std::string& msg) {
    if (_networkConnections->getExternalControlConnection()) {
        const int size = static_cast<int>(msg.size());
        _networkConnections->getExternalControlConnection()->sendData(msg.c_str(), size);
    }
}

bool Engine::isExternalControlConnected() const {
    return (_networkConnections->getExternalControlConnection() &&
            _networkConnections->getExternalControlConnection()->isConnected());
}

void Engine::setExternalControlBufferSize(unsigned int newSize) {
    if (_networkConnections->getExternalControlConnection()) {
        _networkConnections->getExternalControlConnection()->setBufferSize(newSize);
    }
}

void Engine::updateDrawBufferResolutions() {
    core::Node* thisNode = core::ClusterManager::instance()->getThisNode();
    _drawBufferResolutions.clear();

    for (int i = 0; i < thisNode->getNumberOfWindows(); i++) {
        Window& win = getWindow(i);
        
        // first add cubemap resolutions if any
        for (int j = 0; j < win.getNumberOfViewports(); j++) {
            const core::Viewport& vp = win.getViewport(j);
            if (vp.hasSubViewports()) {
                int cubeRes = vp.getNonLinearProjection()->getCubemapResolution();
                _drawBufferResolutions.push_back(glm::ivec2(cubeRes, cubeRes));
            }
        }

        // second add window resolution
        const glm::ivec2 size = win.getFinalFBODimensions();
        _drawBufferResolutions.push_back(size);
    }
}

int Engine::getKey(int winIndex, int key) {
    return glfwGetKey(_instance->getWindow(winIndex).getWindowHandle(), key);
}

int Engine::getMouseButton(int winIndex, int button) {
    return glfwGetMouseButton(
        _instance->getWindow(winIndex).getWindowHandle(),
        button
    );
}

void Engine::getMousePos(int winIndex, double* xPos, double* yPos) {
    glfwGetCursorPos(_instance->getWindow(winIndex).getWindowHandle(), xPos, yPos);
}

void Engine::setMousePos(int winIndex, double xPos, double yPos) {
    glfwSetCursorPos(_instance->getWindow(winIndex).getWindowHandle(), xPos, yPos);
}

void Engine::setMouseCursorVisibility(int winIndex, bool state) {
    GLFWwindow* win = _instance->getWindow(winIndex).getWindowHandle();
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

const core::Node* Engine::getThisNode() const {
    return core::ClusterManager::instance()->getThisNode();
}

Window& Engine::getWindow(int index) const {
    return core::ClusterManager::instance()->getThisNode()->getWindow(index);
}

int Engine::getNumberOfWindows() const {
    return core::ClusterManager::instance()->getThisNode()->getNumberOfWindows();
}

Window& Engine::getCurrentWindow() const {
    return core::ClusterManager::instance()->getThisNode()->getCurrentWindow();
}

int Engine::getCurrentWindowIndex() const {
    return core::ClusterManager::instance()->getThisNode()->getCurrentWindowIndex();
}

core::User& Engine::getDefaultUser() {
    return core::ClusterManager::instance()->getDefaultUser();
}

TrackingManager& Engine::getTrackingManager() {
    return core::ClusterManager::instance()->getTrackingManager();
}

double Engine::getTime() {
    return glfwGetTime();
}

glm::ivec2 Engine::getCurrentViewportSize() const {
    return { _currentViewportCoords.z, _currentViewportCoords.w };
}

glm::ivec2 Engine::getCurrentDrawBufferSize() const {
    return _drawBufferResolutions[_currentDrawBufferIndex];
}

const std::vector<glm::ivec2>& Engine::getDrawBufferResolutions() const {
    return _drawBufferResolutions;
}

size_t Engine::getCurrentDrawBufferIndex() const {
    return _currentDrawBufferIndex;
}

Engine::RenderTarget Engine::getCurrentRenderTarget() const {
    return _currentRenderTarget;
}

glm::ivec4 Engine::getCurrentViewportPixelCoords() const {
    const core::Viewport& vp = getCurrentWindow().getViewport(_currentViewportIndex.main);
    if (vp.hasSubViewports()) {
        return vp.getNonLinearProjection()->getViewportCoords();
    }
    else {
        return _currentViewportCoords;
    }
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
