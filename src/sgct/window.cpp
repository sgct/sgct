/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/window.h>

#include <sgct/clustermanager.h>
#include <sgct/config.h>
#include <sgct/engine.h>
#include <sgct/error.h>
#include <sgct/log.h>
#include <sgct/mpcdi.h>
#include <sgct/networkmanager.h>
#include <sgct/node.h>
#include <sgct/nonlinearprojection.h>
#include <sgct/offscreenbuffer.h>
#include <sgct/ogl_headers.h>
#include <sgct/profiling.h>
#include <sgct/screencapture.h>
#include <sgct/settings.h>
#include <sgct/texturemanager.h>
#include <sgct/shaders/internalshaders.h>
#include <sgct/helpers/stringfunctions.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

#ifdef WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
HDC hDC;
#endif // WIN32

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define Err(code, msg) Error(Error::Component::Window, code, msg)

namespace {
    // abock(2019-10-02); glbinding doesn't come with default bindings against the wgl.xml
    // file, so we have to resolve these manually

#ifdef WIN32
namespace bind {
    bool AreFunctionsResolved = false;
    using BindSwapBarrier = GLboolean(*)(GLuint group, GLuint barrier);
    BindSwapBarrier wglBindSwapBarrierNV = nullptr;
    using JoinSwapGroup = GLboolean(*)(HDC hDC, GLuint group);
    JoinSwapGroup wglJoinSwapGroupNV = nullptr;
    using QueryMaxSwapGroups = GLboolean(*)(HDC hDC, GLuint* mGroups, GLuint* mBarriers);
    QueryMaxSwapGroups wglQueryMaxSwapGroupsNV = nullptr;
    using QueryFrameCount = GLboolean(*)(HDC hDC, GLuint* count);
    QueryFrameCount wglQueryFrameCountNV = nullptr;
    using ResetFrameCount = GLboolean(*)(HDC hDC);
    ResetFrameCount wglResetFrameCountNV = nullptr;
} // namespace bind
#endif // WIN32

    void windowResizeCallback(GLFWwindow* window, int width, int height) {
        width = std::max(width, 1);
        height = std::max(height, 1);

        const sgct::Node& node = sgct::ClusterManager::instance().thisNode();
        for (const std::unique_ptr<sgct::Window>& win : node.windows()) {
            if (win->windowHandle() == window) {
                win->setWindowResolution(glm::ivec2(width, height));
            }
        }
    }

    void frameBufferResizeCallback(GLFWwindow* window, int width, int height) {
        width = std::max(width, 1);
        height = std::max(height, 1);

        const sgct::Node& node = sgct::ClusterManager::instance().thisNode();
        for (const std::unique_ptr<sgct::Window>& win : node.windows()) {
            if (win->windowHandle() == window) {
                win->setFramebufferResolution(glm::ivec2(width, height));
            }
        }
    }

    void windowFocusCallback(GLFWwindow* window, int state) {
        const sgct::Node& node = sgct::ClusterManager::instance().thisNode();
        for (const std::unique_ptr<sgct::Window>& win : node.windows()) {
            if (win->windowHandle() == window) {
                win->setFocused(state == GLFW_TRUE);
            }
        }
    }
} // namespace

namespace sgct {

GLFWwindow* _activeContext = nullptr;

bool Window::_useSwapGroups = false;
bool Window::_isBarrierActive = false;
bool Window::_isSwapGroupMaster = false;
GLFWwindow* Window::_sharedHandle = nullptr;

Window::Window(int id) : _id(id) {}

Window::~Window() {}

void Window::applyWindow(const config::Window& window) {
    ZoneScoped

    if (window.name) {
        setName(*window.name);
    }
    if (!window.tags.empty()) {
        setTags(window.tags);
    }
    if (window.bufferBitDepth) {
        ColorBitDepth bd = [](config::Window::ColorBitDepth cbd) {
            using CBD = config::Window::ColorBitDepth;
            switch (cbd) {
                case CBD::Depth8: return ColorBitDepth::Depth8;
                case CBD::Depth16: return ColorBitDepth::Depth16;
                case CBD::Depth16Float: return ColorBitDepth::Depth16Float;
                case CBD::Depth32Float: return ColorBitDepth::Depth32Float;
                case CBD::Depth16Int: return ColorBitDepth::Depth16Int;
                case CBD::Depth32Int: return ColorBitDepth::Depth32Int;
                case CBD::Depth16UInt: return ColorBitDepth::Depth16UInt;
                case CBD::Depth32UInt: return ColorBitDepth::Depth32UInt;
                default: throw std::logic_error("Unhandled case label");
            }
        }(*window.bufferBitDepth);
        setColorBitDepth(bd);
    }
    if (window.isFullScreen) {
        setWindowMode(*window.isFullScreen);
    }
    if (window.isFloating) {
        setFloating(*window.isFloating);
    }
    if (window.alwaysRender) {
        setRenderWhileHidden(*window.alwaysRender);
    }
    if (window.isHidden) {
        setVisible(*window.isHidden);
    }
    if (window.doubleBuffered) {
        setDoubleBuffered(*window.doubleBuffered);
    }
    if (window.msaa) {
        setNumberOfAASamples(*window.msaa);
    }
    if (window.hasAlpha) {
        setAlpha(*window.hasAlpha);
    }
    if (window.useFxaa) {
        setUseFXAA(*window.useFxaa);
    }
    if (window.isDecorated) {
        setWindowDecoration(*window.isDecorated);
    }
    if (window.draw2D) {
        setCallDraw2DFunction(*window.draw2D);
    }
    if (window.draw3D) {
        setCallDraw3DFunction(*window.draw3D);
    }
    if (window.blitPreviousWindow) {
        setBlitPreviousWindow(*window.blitPreviousWindow);
    }
    if (window.monitor) {
        setFullScreenMonitorIndex(*window.monitor);
    }
    if (window.mpcdi) {
        ZoneScopedN("MPCDI")

        mpcdi::ReturnValue r = mpcdi::parseMpcdiConfiguration(*window.mpcdi);
        setWindowPosition(glm::ivec2(0));
        initWindowResolution(r.resolution);
        setFramebufferResolution(r.resolution);
        setFixResolution(true);

        for (const mpcdi::ReturnValue::ViewportInfo& vp : r.viewports) {
            std::unique_ptr<Viewport> v = std::make_unique<Viewport>(this);
            v->applySettings(vp.proj);
            v->setMpcdiWarpMesh(vp.meshData);
            addViewport(std::move(v));
        }
        return;
    }
    if (window.stereo) {
        StereoMode sm = [](config::Window::StereoMode mode) {
            using SM = config::Window::StereoMode;
            switch (mode) {
                case SM::NoStereo: return StereoMode::NoStereo;
                case SM::Active: return StereoMode::Active;
                case SM::AnaglyphRedCyan: return StereoMode::AnaglyphRedCyan;
                case SM::AnaglyphAmberBlue: return StereoMode::AnaglyphAmberBlue;
                case SM::AnaglyphRedCyanWimmer: return StereoMode::AnaglyphRedCyanWimmer;
                case SM::Checkerboard: return StereoMode::Checkerboard;
                case SM::CheckerboardInverted: return StereoMode::CheckerboardInverted;
                case SM::VerticalInterlaced: return StereoMode::VerticalInterlaced;
                case SM::VerticalInterlacedInverted:
                    return StereoMode::VerticalInterlacedInverted;
                case SM::Dummy: return StereoMode::Dummy;
                case SM::SideBySide: return StereoMode::SideBySide;
                case SM::SideBySideInverted: return StereoMode::SideBySideInverted;
                case SM::TopBottom: return StereoMode::TopBottom;
                case SM::TopBottomInverted: return StereoMode::TopBottomInverted;
                default: throw std::logic_error("Unhandled case label");
            }
        }(*window.stereo);
        setStereoMode(sm);
    }
    if (window.pos) {
        setWindowPosition(*window.pos);
    }

    initWindowResolution(window.size);

    if (window.resolution) {
        ZoneScopedN("Resolution")
        
        setFramebufferResolution(*window.resolution);
        setFixResolution(true);
    }

    for (const config::Viewport& viewport : window.viewports) {
        std::unique_ptr<Viewport> vp = std::make_unique<Viewport>(this);
        vp->applyViewport(viewport);
        addViewport(std::move(vp));
    }
}

void Window::setName(std::string name) {
    _name = std::move(name);
}

void Window::setTags(std::vector<std::string> tags) {
    _tags = std::move(tags);
}

const std::string& Window::name() const {
    return _name;
}

bool Window::hasTag(const std::string& tag) const {
    return std::find(_tags.cbegin(), _tags.cend(), tag) != _tags.cend();
}

int Window::id() const {
    return _id;
}

bool Window::isFocused() const {
    return _hasFocus;
}

void Window::close() {
    ZoneScoped

    makeSharedContextCurrent();

    Log::Info("Deleting screen capture data for window %d", _id);
    _screenCaptureLeftOrMono = nullptr;
    _screenCaptureRight = nullptr;

    // delete FBO stuff
    if (_finalFBO) {
        Log::Info("Releasing OpenGL buffers for window %d", _id);
        _finalFBO = nullptr;
        destroyFBOs();
    }

    Log::Info("Deleting VBOs for window %d", _id);
    glDeleteBuffers(1, &_vbo);
    _vbo = 0;

    Log::Info("Deleting VAOs for window %d", _id);
    glDeleteVertexArrays(1, &_vao);
    _vao = 0;

    _stereo.shader.deleteProgram();

    // Current handle must be set at the end to properly destroy the window
    makeOpenGLContextCurrent();

    _viewports.clear();

    glfwSetWindowSizeCallback(_windowHandle, nullptr);
    glfwSetFramebufferSizeCallback(_windowHandle, nullptr);
    glfwSetWindowFocusCallback(_windowHandle, nullptr);
    glfwSetWindowIconifyCallback(_windowHandle, nullptr);

#ifdef WIN32
    if (_useSwapGroups && glfwExtensionSupported("WGL_NV_swap_group")) {
        bind::wglBindSwapBarrierNV(1, 0); // un-bind
        bind::wglJoinSwapGroupNV(hDC, 0); // un-join
    }
#endif
}

void Window::init() {
    ZoneScoped

    if (!_isFullScreen) {
        if (_setWindowPos) {
            glfwSetWindowPos(_windowHandle, _windowPos.x, _windowPos.y);
        }
        glfwSetWindowSizeCallback(_windowHandle, windowResizeCallback);
        glfwSetFramebufferSizeCallback(_windowHandle, frameBufferResizeCallback);
        glfwSetWindowFocusCallback(_windowHandle, windowFocusCallback);
    }

    std::string title = "SGCT node: " +
        ClusterManager::instance().thisNode().address() + " (" +
        (NetworkManager::instance().isComputerServer() ? "master" : "client") +
        ": " + std::to_string(_id) + ")";

    setWindowTitle(_name.empty() ? title.c_str() : _name.c_str());

    {
        // swap the buffers and update the window
        ZoneScopedN("glfwSwapBuffers")
        glfwSwapBuffers(_windowHandle);
    }
}

void Window::initOGL() {
    ZoneScoped

    _colorFormat = GL_BGRA;
    
    std::tie(_internalColorFormat, _colorDataType, _bytesPerColor) =
        [](ColorBitDepth bd) -> std::tuple<GLenum, GLenum, int>
    {
        switch (bd) {
            case ColorBitDepth::Depth8: return { GL_RGBA8, GL_UNSIGNED_BYTE, 1 };
            case ColorBitDepth::Depth16: return { GL_RGBA16, GL_UNSIGNED_SHORT, 2 };
            case ColorBitDepth::Depth16Float: return { GL_RGBA16F, GL_HALF_FLOAT, 2};
            case ColorBitDepth::Depth32Float: return { GL_RGBA32F, GL_FLOAT, 4 };
            case ColorBitDepth::Depth16Int: return { GL_RGBA16I, GL_SHORT, 2 };
            case ColorBitDepth::Depth32Int: return { GL_RGBA32I, GL_INT, 2 };
            case ColorBitDepth::Depth16UInt: return { GL_RGBA16UI, GL_UNSIGNED_SHORT, 2 };
            case ColorBitDepth::Depth32UInt: return { GL_RGBA32UI, GL_UNSIGNED_INT, 4 };
            default: throw std::logic_error("Unhandled case label");
        }
    }(_bufferColorBitDepth);
    
    createTextures();
    createVBOs(); // must be created before FBO
    createFBOs();
    initScreenCapture();
    loadShaders();

    for (const std::unique_ptr<Viewport>& vp : _viewports) {
        if (!vp->hasSubViewports()) {
            continue;
        }

        vp->nonLinearProjection()->setStereo(_stereoMode != StereoMode::NoStereo);
        vp->nonLinearProjection()->init(
            _internalColorFormat,
            _colorFormat,
            _colorDataType,
            _nAASamples
        );

        glm::vec2 viewport = glm::vec2(_framebufferRes) * vp->size();
        vp->nonLinearProjection()->update(std::move(viewport));
    }

#ifdef WIN32
    if (!bind::AreFunctionsResolved && glfwExtensionSupported("WGL_NV_swap_group")) {
        // abock(2019-10-02); I had to hand-resolve these functions as glbindings does not
        // come with build-in support for the wgl.xml functions
        // See https://github.com/cginternals/glbinding/issues/132 for when it is resolved

        bind::wglBindSwapBarrierNV = reinterpret_cast<bind::BindSwapBarrier>(
            glfwGetProcAddress("wglBindSwapBarrierNV")
        );

        assert(bind::wglBindSwapBarrierNV);
        bind::wglJoinSwapGroupNV = reinterpret_cast<bind::JoinSwapGroup>(
            glfwGetProcAddress("wglJoinSwapGroupNV")
        );
        assert(bind::wglJoinSwapGroupNV);
        bind::wglQueryMaxSwapGroupsNV = reinterpret_cast<bind::QueryMaxSwapGroups>(
            glfwGetProcAddress("wglQueryMaxSwapGroupsNV")
        );
        assert(bind::wglQueryMaxSwapGroupsNV);
        bind::wglQueryFrameCountNV = reinterpret_cast<bind::QueryFrameCount>(
            glfwGetProcAddress("wglQueryFrameCountNV")
        );
        assert(bind::wglQueryFrameCountNV);
        bind::wglResetFrameCountNV = reinterpret_cast<bind::ResetFrameCount>(
            glfwGetProcAddress("wglResetFrameCountNV")
        );
        assert(bind::wglResetFrameCountNV);

        if (!bind::wglBindSwapBarrierNV || !bind::wglJoinSwapGroupNV ||
            !bind::wglQueryMaxSwapGroupsNV || !bind::wglQueryFrameCountNV ||
            !bind::wglResetFrameCountNV)
        {
            Log::Error("Error resolving swapgroup functions");
            Log::Info(
                "wglBindSwapBarrierNV(: %p\twglJoinSwapGroupNV: %p\t"
                "wglQueryMaxSwapGroupsNV: %p\twglQueryFrameCountNV: %p\t"
                "wglResetFrameCountNV: %p\twglGetCurrentDC: %p",
                bind::wglBindSwapBarrierNV, bind::wglJoinSwapGroupNV,
                bind::wglQueryMaxSwapGroupsNV, bind::wglQueryFrameCountNV, 
                bind::wglResetFrameCountNV
            );
            throw Err(8000, "Error resolving swapgroup functions");
        };

        bind::AreFunctionsResolved = true;
    }
#endif // WIN32
}

void Window::initContextSpecificOGL() {
    ZoneScoped

    makeOpenGLContextCurrent();
    std::for_each(_viewports.begin(), _viewports.end(), std::mem_fn(&Viewport::loadData));
    _hasAnyMasks = std::any_of(
        _viewports.cbegin(),
        _viewports.cend(),
        [](const std::unique_ptr<Viewport>& vp) {
            return vp->hasBlendMaskTexture() || vp->hasBlackLevelMaskTexture();
        }
    );
}

unsigned int Window::frameBufferTexture(TextureIndex index) {
    ZoneScoped

    // @TODO (abock, 2019-12-04) I think this function should be made constant and we
    // figure out beforehand which textures we need to create. So this function just
    // returns the already created textures instead
    switch (index) {
        case TextureIndex::LeftEye:
            if (_frameBufferTextures.leftEye == 0) {
                generateTexture(_frameBufferTextures.leftEye, TextureType::Color);
            }
            return _frameBufferTextures.leftEye;
        case TextureIndex::RightEye:
            if (_frameBufferTextures.rightEye == 0) {
                generateTexture(_frameBufferTextures.rightEye, TextureType::Color);
            }
            return _frameBufferTextures.rightEye;
        case TextureIndex::Intermediate:
            if (_frameBufferTextures.intermediate == 0) {
                generateTexture(_frameBufferTextures.intermediate, TextureType::Color);
            }
            return _frameBufferTextures.intermediate;
        case TextureIndex::Depth:
            if (_frameBufferTextures.depth == 0) {
                generateTexture(_frameBufferTextures.depth, TextureType::Depth);
            }
            return _frameBufferTextures.depth;
        case TextureIndex::Normals:
            if (_frameBufferTextures.normals == 0) {
                generateTexture(_frameBufferTextures.normals, TextureType::Normal);
            }
            return _frameBufferTextures.normals;
        case TextureIndex::Positions:
            if (_frameBufferTextures.positions == 0) {
                generateTexture(_frameBufferTextures.positions, TextureType::Position);
            }
            return _frameBufferTextures.positions;
        default: throw std::logic_error("Unhandled case label");
    }
}

void Window::setVisible(bool state) {
    _isVisible = state;
    if (state != _isVisible && _windowHandle) {
        if (state) {
            glfwShowWindow(_windowHandle);
        }
        else {
            glfwHideWindow(_windowHandle);
        }
    }
}

void Window::setRenderWhileHidden(bool state) {
    _shouldRenderWhileHidden = state;
}

void Window::setFocused(bool state) {
    _hasFocus = state;
}

void Window::setWindowTitle(const char* title) {
    ZoneScoped
    glfwSetWindowTitle(_windowHandle, title);
}

void Window::setWindowResolution(glm::ivec2 resolution) {
    // In case this callback gets triggered from elsewhere than SGCT's glfwPollEvents, we
    // want to make sure the actual resizing is deferred to the end of the frame. This can
    // happen if some other library pulls events from the operating system for example by
    // calling nextEventMatchingMask (MacOS) or PeekMessage (Windows). If we were to set
    // the actual resolution directly, we may render half a frame with resolution A and
    // the other half with resolution b, which is undefined behaviour.
    // _hasPendingWindowRes is checked in Window::updateResolution, which is called from
    // Engine's render loop after glfwPollEvents.
    _pendingWindowRes = std::move(resolution);
}

void Window::setFramebufferResolution(glm::ivec2 resolution) {
    // Defer actual update of framebuffer resolution until next call to updateResolutions.
    // (Same reason as described for setWindowResolution above.)
    if (!_useFixResolution) {
        _pendingFramebufferRes = std::move(resolution);
    }
}

void Window::swap(bool takeScreenshot) {
    if (!(_isVisible || _shouldRenderWhileHidden)) {
        return;
    }

    makeOpenGLContextCurrent();
        
    if (takeScreenshot) {
        ZoneScopedN("Take Screenshot")
        if (Settings::instance().captureFromBackBuffer() && _isDoubleBuffered) {
            if (_screenCaptureLeftOrMono) {
                _screenCaptureLeftOrMono->saveScreenCapture(
                    0,
                    _stereoMode == StereoMode::Active ?
                        ScreenCapture::CaptureSource::LeftBackBuffer :
                        ScreenCapture::CaptureSource::BackBuffer
                );
            }
            if (_screenCaptureRight && _stereoMode == StereoMode::Active) {
                _screenCaptureLeftOrMono->saveScreenCapture(
                    0,
                    ScreenCapture::CaptureSource::RightBackBuffer
                );
            }
        }
        else {
            if (_screenCaptureLeftOrMono) {
                _screenCaptureLeftOrMono->saveScreenCapture(_frameBufferTextures.leftEye);
            }
            if (_screenCaptureRight && _stereoMode > StereoMode::NoStereo &&
                _stereoMode < Window::StereoMode::SideBySide)
            {
                _screenCaptureRight->saveScreenCapture(_frameBufferTextures.rightEye);
            }
        }
    }

    // swap
    _windowResOld = _windowRes;

    if (_isDoubleBuffered) {
        ZoneScopedN("glfwSwapBuffers")
        glfwSwapBuffers(_windowHandle);
    }
    else {
        ZoneScopedN("glFinish")
        glFinish();
    }
}

void Window::updateResolutions() {
    ZoneScoped

    if (_pendingWindowRes.has_value()) {
        _windowRes = *_pendingWindowRes;
        float ratio = static_cast<float>(_windowRes.x) / static_cast<float>(_windowRes.y);

        // Set field of view of each of this window's viewports to match new aspect ratio,
        // adjusting only the horizontal (x) values
        for (const std::unique_ptr<Viewport>& vp : _viewports) {
            vp->updateFovToMatchAspectRatio(_aspectRatio, ratio);
            Log::Debug("Update aspect ratio in viewport (%f -> %f)", _aspectRatio, ratio);
        }
        _aspectRatio = ratio;

        // Redraw window
        if (_windowHandle) {
            glfwSetWindowSize(_windowHandle, _windowRes.x, _windowRes.y);
        }

        Log::Debug(
            "Resolution changed to %dx%d in window %d", _windowRes.x, _windowRes.y, _id
        );
        _pendingWindowRes = std::nullopt;
    }

    if (_pendingFramebufferRes.has_value()) {
        _framebufferRes = *_pendingFramebufferRes;

        Log::Debug(
            "Framebuffer resolution changed to %dx%d for window %d",
            _framebufferRes.x, _framebufferRes.y, _id
        );

        _pendingFramebufferRes = std::nullopt;
    }
}

void Window::setHorizFieldOfView(float hFovDeg) {
    // Set field of view of each of this window's viewports to match new horiz/vert
    // aspect ratio, adjusting only the horizontal (x) values
    for (const std::unique_ptr<Viewport>& vp : _viewports) {
        vp->setHorizontalFieldOfView(hFovDeg);
    }
    Log::Debug("HFOV changed to %f for window %d", hFovDeg, _viewports.size(), _id);
}

void Window::initWindowResolution(glm::ivec2 resolution) {
    ZoneScoped

    _windowRes = resolution;
    _windowResOld = _windowRes;
    _aspectRatio = static_cast<float>(resolution.x) / static_cast<float>(resolution.y);
    _isWindowResolutionSet = true;

    if (!_useFixResolution) {
        _framebufferRes = resolution;
    }
}

void Window::update() {
    ZoneScoped

    if (!_isVisible || !isWindowResized()) {
        return;
    }
    makeOpenGLContextCurrent();

    resizeFBOs();

    auto resizePBO = [this](ScreenCapture& sc) {
        const int nCaptureChannels = _hasAlpha ? 4 : 3;
        if (Settings::instance().captureFromBackBuffer()) {
            // capture from buffer supports only 8-bit per color component
            sc.setTextureTransferProperties(GL_UNSIGNED_BYTE);
            const glm::ivec2 res = resolution();
            sc.initOrResize(res, nCaptureChannels, 1);
        }
        else {
            // default: capture from texture (supports HDR)
            sc.setTextureTransferProperties(_colorDataType);
            const glm::ivec2 res = framebufferResolution();
            sc.initOrResize(res, nCaptureChannels, _bytesPerColor);
        }
    };
    if (_screenCaptureLeftOrMono) {
        resizePBO(*_screenCaptureLeftOrMono);
    }
    if (_screenCaptureRight) {
        resizePBO(*_screenCaptureRight);
    }

    // resize non linear projection buffers
    for (const std::unique_ptr<Viewport>& vp : _viewports) {
        if (vp->hasSubViewports()) {
            glm::vec2 viewport = glm::vec2(_framebufferRes) * vp->size();
            vp->nonLinearProjection()->update(std::move(viewport));
        }
    }
}

void Window::makeSharedContextCurrent() {
    ZoneScoped

    if (_activeContext == _sharedHandle) {
        // glfwMakeContextCurrent is expensive even if we don't change the context
        return;
    }
    _activeContext = _sharedHandle;
    glfwMakeContextCurrent(_sharedHandle);
}

void Window::makeOpenGLContextCurrent() {
    ZoneScoped

    if (_activeContext == _windowHandle) {
        // glfwMakeContextCurrent is expensive even if we don't change the context
        return;
    }
    _activeContext = _windowHandle;
    glfwMakeContextCurrent(_windowHandle);
}

bool Window::isWindowResized() const {
    return (_windowRes.x != _windowResOld.x || _windowRes.y != _windowResOld.y);
}

bool Window::isBarrierActive() {
    return _isBarrierActive;
}

bool Window::isUsingSwapGroups() {
    return _useSwapGroups;
}

bool Window::isSwapGroupMaster() {
    return _isSwapGroupMaster;
}

bool Window::isFullScreen() const {
    return _isFullScreen;
}

bool Window::isFloating() const {
    return _isFloating;
}

bool Window::isDoubleBuffered() const {
    return _isDoubleBuffered;
}

bool Window::isVisible() const {
    return _isVisible;
}

bool Window::isRenderingWhileHidden() const {
    return _shouldRenderWhileHidden;
}

bool Window::isFixResolution() const {
    return _useFixResolution;
}

bool Window::isStereo() const {
    return _stereoMode != StereoMode::NoStereo;
}

void Window::setWindowPosition(glm::ivec2 positions) {
    _windowPos = std::move(positions);
    _setWindowPos = true;
}

void Window::setWindowMode(bool fullscreen) {
    _isFullScreen = fullscreen;
}

void Window::setFloating(bool floating) {
    _isFloating = floating;
}

void Window::setDoubleBuffered(bool doubleBuffered) {
    _isDoubleBuffered = doubleBuffered;
}

void Window::setWindowDecoration(bool state) {
    _isDecorated = state;
}

void Window::setFullScreenMonitorIndex(int index) {
    _monitorIndex = index;
}

void Window::setBarrier(bool state) {
    if (_useSwapGroups && state != _isBarrierActive) {
        Log::Info("Enabling Nvidia swap barrier");

#ifdef WIN32
        _isBarrierActive = bind::wglBindSwapBarrierNV(1, state ? 1 : 0) == GL_TRUE;
#endif // WIN32
    }
}

void Window::setFixResolution(bool state) {
    _useFixResolution = state;
}

void Window::setUseFXAA(bool state) {
    _useFXAA = state;
    Log::Debug("FXAA status: %s for window %d", state ? "enabled" : "disabled", _id);
}

void Window::setUseQuadbuffer(bool state) {
    _useQuadBuffer = state;
    if (_useQuadBuffer) {
        glfwWindowHint(GLFW_STEREO, GLFW_TRUE);
        Log::Info("Window %d: Enabling quadbuffered rendering", _id);
    }
}

void Window::setCallDraw2DFunction(bool state) {
    _hasCallDraw2DFunction = state;
    if (!_hasCallDraw2DFunction) {
        Log::Info("Window %d: Draw 2D function disabled", _id);
    }
}

void Window::setCallDraw3DFunction(bool state) {
    _hasCallDraw3DFunction = state;
    if (!_hasCallDraw3DFunction) {
        Log::Info("Window %d: Draw 3D function disabled", _id);
    }
}

void Window::setBlitPreviousWindow(bool state) {
    _shouldBitPreviousWindow = state;
    if (_shouldBitPreviousWindow) {
        Log::Info("Window %d: BlitPreviousWindow enabled", _id);
    }
}

void Window::openWindow(GLFWwindow* share, bool isLastWindow) {
    ZoneScoped

    {
        ZoneScopedN("Set GLFW settings")
        glfwWindowHint(GLFW_DEPTH_BITS, 32);
        glfwWindowHint(GLFW_DECORATED, _isDecorated ? GLFW_TRUE : GLFW_FALSE);

        const int antiAliasingSamples = numberOfAASamples();
        glfwWindowHint(GLFW_SAMPLES, antiAliasingSamples > 1 ? antiAliasingSamples : 0);

        glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
        glfwWindowHint(GLFW_FLOATING, _isFloating ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_DOUBLEBUFFER, _isDoubleBuffered ? GLFW_TRUE : GLFW_FALSE);
        if (!_isVisible) {
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        }
    }

    setUseQuadbuffer(_stereoMode == StereoMode::Active);

    GLFWmonitor* mon = nullptr;
    if (_isFullScreen) {
        ZoneScopedN("Fullscreen Settings")
        int count;
        GLFWmonitor** monitors = glfwGetMonitors(&count);

        const int refreshRateHint = Settings::instance().refreshRateHint();
        if (refreshRateHint > 0) {
            glfwWindowHint(GLFW_REFRESH_RATE, refreshRateHint);
        }
        
        if (_monitorIndex > 0 && _monitorIndex < count) {
            mon = monitors[_monitorIndex];
        }
        else {
            mon = glfwGetPrimaryMonitor();
            if (_monitorIndex >= count) {
                Log::Info(
                    "Window(%d): Invalid monitor index (%d). Computer has %d monitors",
                    _id, _monitorIndex, count
                );
            }
        }

        if (!_isWindowResolutionSet) {
            const GLFWvidmode* currentMode = glfwGetVideoMode(mon);
            _windowRes = glm::ivec2(currentMode->width, currentMode->height);
        }
    }
    
    {
        ZoneScopedN("glfwCreateWindow")
        _windowHandle = glfwCreateWindow(_windowRes.x, _windowRes.y, "SGCT", mon, share);
        if (_windowHandle == nullptr) {
            throw Err(8001, "Error opening GLFW window");
        }
    }

    {
        ZoneScopedN("glfwMakeContextCurrent")
        _sharedHandle = share != nullptr ? share : _windowHandle;
        glfwMakeContextCurrent(_windowHandle);
    }

    // Mac for example scales the window size != frame buffer size
    glm::ivec2 bufferSize;
    {
        ZoneScopedN("glfwGetFramebufferSize")
        glfwGetFramebufferSize(_windowHandle, &bufferSize[0], &bufferSize[1]);
    }

    _windowInitialRes = _windowRes;
    _scale = glm::vec2(bufferSize) / glm::vec2(_windowRes);
    if (!_useFixResolution) {
        _framebufferRes = bufferSize;
    }
        
    // Swap inerval:
    //  -1 = adaptive sync
    //   0 = vertical sync off
    //   1 = wait for vertical sync
    //   2 = fix when using swapgroups in xp and running half the framerate

    // If we would set multiple windows to use vsync, we would get a framerate of (monitor
    // refreshrate)/(number of windows), which is something that might really slow down a
    // multi-monitor application. Setting last window to the requested interval, which
    // does mean all other windows will respect the last window in the pipeline.
    glfwSwapInterval(isLastWindow ? Settings::instance().swapInterval() : 0);
    
    // if client, disable mouse pointer
    if (!Engine::instance().isMaster()) {
        glfwSetInputMode(_windowHandle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }

    _hasFocus = glfwGetWindowAttrib(_windowHandle, GLFW_FOCUSED) == GLFW_TRUE;
        
    {
        ZoneScopedN("glfwMakeContextCurrent")
        glfwMakeContextCurrent(_sharedHandle);
    }

    _screenCaptureLeftOrMono = std::make_unique<ScreenCapture>();
    if (useRightEyeTexture()) {
        _screenCaptureRight = std::make_unique<ScreenCapture>();
    }
    _finalFBO = std::make_unique<OffScreenBuffer>();
}

void Window::initNvidiaSwapGroups() {
    ZoneScoped

#ifdef WIN32
    if (glfwExtensionSupported("WGL_NV_swap_group")) {
        Log::Info("Joining Nvidia swap group");

        hDC = wglGetCurrentDC();

        unsigned int maxBarrier = 0;
        unsigned int maxGroup = 0;
        const GLboolean res = bind::wglQueryMaxSwapGroupsNV(hDC, &maxGroup, &maxBarrier);
        if (res == GL_FALSE) {
            throw Err(3006, "Error requesting maximum number of swap groups");
        }
        Log::Info(
            "WGL_NV_swap_group extension is supported. Max number of groups: %d. "
            "Max number of barriers: %d", maxGroup, maxBarrier
        );

        if (maxGroup > 0) {
            _useSwapGroups = bind::wglJoinSwapGroupNV(hDC, 1) == GL_TRUE;
            Log::Info("Joining swapgroup 1 [%s]", _useSwapGroups ? "ok" : "failed");
        }
        else {
            Log::Error("No swap group found. This instance will not use swap groups");
            _useSwapGroups = false;
        }
    }
    else {
        _useSwapGroups = false;
    }
#endif
}

void Window::initScreenCapture() {
    auto initializeCapture = [this](ScreenCapture& sc) {
        const int nCaptureChannels = _hasAlpha ? 4 : 3;
        if (Settings::instance().captureFromBackBuffer()) {
            // capturing from buffer supports only 8-bit per color component capture
            const glm::ivec2 res = resolution();
            sc.initOrResize(res, nCaptureChannels, 1);
            sc.setTextureTransferProperties(GL_UNSIGNED_BYTE);
        }
        else {
            // default: capture from texture (supports HDR)
            const glm::ivec2 res = framebufferResolution();
            sc.initOrResize(res, nCaptureChannels, _bytesPerColor);
            sc.setTextureTransferProperties(_colorDataType);
        }

        Settings::CaptureFormat format = Settings::instance().captureFormat();
        ScreenCapture::CaptureFormat scf = [](Settings::CaptureFormat f) {
            using CF = Settings::CaptureFormat;
            switch (f) {
                case CF::PNG: return ScreenCapture::CaptureFormat::PNG;
                case CF::TGA: return ScreenCapture::CaptureFormat::TGA;
                case CF::JPG: return ScreenCapture::CaptureFormat::JPEG;
                default: throw std::logic_error("Unhandled case label");
            }
        }(format);
        sc.setCaptureFormat(scf);
    };

    if (_screenCaptureLeftOrMono) {
        if (useRightEyeTexture()) {
            _screenCaptureLeftOrMono->init(_id, ScreenCapture::EyeIndex::StereoLeft);
        }
        else {
            _screenCaptureLeftOrMono->init(_id, ScreenCapture::EyeIndex::Mono);
        }
        initializeCapture(*_screenCaptureLeftOrMono);
    }

    if (_screenCaptureRight) {
        _screenCaptureRight->init(_id, ScreenCapture::EyeIndex::StereoRight);
        initializeCapture(*_screenCaptureRight);
    }
}

unsigned int Window::swapGroupFrameNumber() {
    unsigned int frameNumber = 0;

#ifdef WIN32
    if (_isBarrierActive && glfwExtensionSupported("WGL_NV_swap_group")) {
        bind::wglQueryFrameCountNV(hDC, &frameNumber);
    }
#endif
    return frameNumber;
}

void Window::resetSwapGroupFrameNumber() {
#ifdef WIN32
    if (_isBarrierActive) {
        _isSwapGroupMaster = glfwExtensionSupported("WGL_NV_swap_group") &&
                             bind::wglResetFrameCountNV(hDC);
        if (_isSwapGroupMaster) {
            Log::Info("Resetting frame counter");
        }
        else {
            Log::Info("Resetting frame counter failed");
        }
    }
#endif
}

void Window::createTextures() {
    ZoneScoped
    TracyGpuZone("Create Textures")

    GLint max;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max);
    if (_framebufferRes.x > max || _framebufferRes.y > max) {
        Log::Error("Window %d: Requested framebuffer too big (Max: %d)", _id, max);
        return;
    }

    // Create left and right color & depth textures; don't allocate the right eye image if
    // stereo is not used create a postFX texture for effects
    generateTexture(_frameBufferTextures.leftEye, TextureType::Color);
    if (useRightEyeTexture()) {
        generateTexture(_frameBufferTextures.rightEye, TextureType::Color);
    }
    if (Settings::instance().useDepthTexture()) {
        generateTexture(_frameBufferTextures.depth, TextureType::Depth);
    }
    if (_useFXAA) {
        generateTexture(_frameBufferTextures.intermediate, TextureType::Color);
    }
    if (Settings::instance().useNormalTexture()) {
        generateTexture(_frameBufferTextures.normals, TextureType::Normal);
    }
    if (Settings::instance().usePositionTexture()) {
        generateTexture(_frameBufferTextures.positions, TextureType::Position);
    }

    Log::Debug("Targets initialized successfully for window %d", _id);
}

void Window::generateTexture(unsigned int& id, Window::TextureType type) {
    ZoneScoped
    TracyGpuZone("Generate Textures")

    glDeleteTextures(1, &id);
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    
    // Determine the internal texture format, the texture format, and the pixel type
    const GLenum internalFormat = [this](Window::TextureType t) -> GLenum {
        switch (t) {
            case TextureType::Color: return _internalColorFormat;
            case TextureType::Depth: return GL_DEPTH_COMPONENT32;
            case TextureType::Normal:
            case TextureType::Position:
                return Settings::instance().bufferFloatPrecision();
            default: throw std::logic_error("Unhandled case label");
        }
    }(type);

    const glm::ivec2 res = _framebufferRes;
    glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, res.x, res.y);
    Log::Debug("%dx%d texture generated for window %d", res.x, res.y, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
}

void Window::createFBOs() {
    ZoneScoped
    TracyGpuZone("Create FBOs")

    _finalFBO->setInternalColorFormat(_internalColorFormat);
    _finalFBO->createFBO(_framebufferRes.x, _framebufferRes.y, _nAASamples);

    Log::Debug(
        "Window %d: FBO initiated successfully. Number of samples: %d",
        _id, _finalFBO->isMultiSampled() ? _nAASamples : 1
    );
}

void Window::createVBOs() {
    ZoneScoped
    TracyGpuZone("Create VBOs")

    constexpr const std::array<const float, 20> QuadVerts = {
        0.f, 0.f, -1.f, -1.f, -1.f,
        1.f, 0.f,  1.f, -1.f, -1.f,
        0.f, 1.f, -1.f,  1.f, -1.f,
        1.f, 1.f,  1.f,  1.f, -1.f
    };

    glGenVertexArrays(1, &_vao);
    Log::Debug("Window: Generating VAO: %d", _vao);

    glGenBuffers(1, &_vbo);
    Log::Debug("Window: Generating VBO: %d", _vbo);

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), QuadVerts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        reinterpret_cast<void*>(8)
    );

    glBindVertexArray(0);
}

void Window::loadShaders() {
    ZoneScoped
    TracyGpuZone("Load Shaders")

    if (_stereoMode <= StereoMode::Active || _stereoMode >= StereoMode::SideBySide) {
        return;
    }

    // reload shader program if it exists
    _stereo.shader.deleteProgram();

    std::string stereoVertShader = shaders::AnaglyphVert;
    std::string stereoFragShader = [](sgct::Window::StereoMode mode) {
        using SM = StereoMode;
        switch (mode) {
            case SM::AnaglyphRedCyan: return shaders::AnaglyphRedCyanFrag;
            case SM::AnaglyphAmberBlue: return shaders::AnaglyphAmberBlueFrag;
            case SM::AnaglyphRedCyanWimmer: return shaders::AnaglyphRedCyanWimmerFrag;
            case SM::Checkerboard: return shaders::CheckerBoardFrag;
            case SM::CheckerboardInverted: return shaders::CheckerBoardInvertedFrag;
            case SM::VerticalInterlaced: return shaders::VerticalInterlacedFrag;
            case SM::VerticalInterlacedInverted:
                return shaders::VerticalInterlacedInvertedFrag;
            case SM::Dummy: return shaders::DummyStereoFrag;
            default: throw std::logic_error("Unhandled case label");
        }
    }(_stereoMode);

    _stereo.shader = ShaderProgram("StereoShader");
    _stereo.shader.addShaderSource(stereoVertShader, stereoFragShader);
    _stereo.shader.createAndLinkProgram();
    _stereo.shader.bind();
    _stereo.leftTexLoc = glGetUniformLocation(_stereo.shader.id(), "leftTex");
    glUniform1i(_stereo.leftTexLoc, 0);
    _stereo.rightTexLoc = glGetUniformLocation(_stereo.shader.id(), "rightTex");
    glUniform1i(_stereo.rightTexLoc, 1);
    ShaderProgram::unbind();
}

void Window::renderScreenQuad() const {
    TracyGpuZone("Render Screen Quad")

    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

OffScreenBuffer* Window::fbo() const {
    return _finalFBO.get();
}

GLFWwindow* Window::windowHandle() const {
    return _windowHandle;
}

glm::ivec2 Window::finalFBODimensions() const {
    return _framebufferRes;
}

void Window::resizeFBOs() {
    if (_useFixResolution) {
        return;
    }

    makeSharedContextCurrent();
    destroyFBOs();
    createTextures();

    _finalFBO->resizeFBO(_framebufferRes.x, _framebufferRes.y, _nAASamples);
        
    if (!_finalFBO->isMultiSampled()) {
        _finalFBO->bind();
        _finalFBO->attachColorTexture(_frameBufferTextures.leftEye);
        _finalFBO->unbind();
    }
}

void Window::destroyFBOs() {
    glDeleteTextures(1, &_frameBufferTextures.leftEye);
    _frameBufferTextures.leftEye = 0;
    glDeleteTextures(1, &_frameBufferTextures.rightEye);
    _frameBufferTextures.rightEye = 0;
    glDeleteTextures(1, &_frameBufferTextures.depth);
    _frameBufferTextures.depth = 0;
    glDeleteTextures(1, &_frameBufferTextures.intermediate);
    _frameBufferTextures.intermediate = 0;
    glDeleteTextures(1, &_frameBufferTextures.positions);
    _frameBufferTextures.positions = 0;
}

Window::StereoMode Window::stereoMode() const {
    return _stereoMode;
}

void Window::addViewport(std::unique_ptr<Viewport> vpPtr) {
    _viewports.push_back(std::move(vpPtr));
    Log::Debug("Adding viewport (total %d)", _viewports.size());
}

const std::vector<std::unique_ptr<Viewport>>& Window::viewports() const {
    return _viewports;
}

void Window::setNumberOfAASamples(int samples) {
    _nAASamples = samples;
}

int Window::numberOfAASamples() const {
    return _nAASamples;
}

void Window::setStereoMode(StereoMode sm) {
    _stereoMode = sm;
    if (_windowHandle) {
        loadShaders();
    }
}

ScreenCapture* Window::screenCapturePointer(Eye eye) const {
    switch (eye) {
        case Eye::MonoOrLeft: return _screenCaptureLeftOrMono.get();
        case Eye::Right:      return _screenCaptureRight.get();
        default:              throw std::logic_error("Unhandled case label");
    }
}

bool Window::useRightEyeTexture() const {
    return _stereoMode != StereoMode::NoStereo && _stereoMode < StereoMode::SideBySide;
}

void Window::setAlpha(bool state) {
    _hasAlpha = state;
}

bool Window::hasAlpha() const {
    return _hasAlpha;
}

void Window::setColorBitDepth(ColorBitDepth cbd) {
    _bufferColorBitDepth = cbd;
}

Window::ColorBitDepth Window::colorBitDepth() const {
    return _bufferColorBitDepth;
}

float Window::horizFieldOfViewDegrees() const {
    return _viewports[0]->horizontalFieldOfViewDegrees();
}

glm::ivec2 Window::resolution() const {
    return _windowRes;
}

glm::ivec2 Window::framebufferResolution() const {
    return _framebufferRes;
}

glm::ivec2 Window::initialResolution() const {
    return _windowInitialRes;
}

glm::vec2 Window::scale() const {
    return _scale;
}

float Window::aspectRatio() const {
    return _aspectRatio;
}

int Window::framebufferBPCC() const {
    return _bytesPerColor;
}

bool Window::hasAnyMasks() const {
    return _hasAnyMasks;
}

bool Window::useFXAA() const {
    return _useFXAA;
}

void Window::bindStereoShaderProgram(unsigned int leftTex, unsigned int rightTex) const {
    _stereo.shader.bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, leftTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, rightTex);

    glUniform1i(_stereo.leftTexLoc, 0);
    glUniform1i(_stereo.rightTexLoc, 1);
}

bool Window::shouldCallDraw2DFunction() const {
    return _hasCallDraw2DFunction;
}

bool Window::shouldCallDraw3DFunction() const {
    return _hasCallDraw3DFunction;
}

bool Window::shouldBlitPreviousWindow() const {
    return _shouldBitPreviousWindow;
}

} // namespace sgct
