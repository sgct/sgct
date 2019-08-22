/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/SGCTWindow.h>

#include <sgct/ClusterManager.h>
#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
#include <sgct/SGCTSettings.h>
#include <sgct/TextureManager.h>
#include <sgct/Viewport.h>
#include <sgct/shaders/SGCTInternalShaders.h>
#include <sgct/shaders/SGCTInternalShaders_modern.h>
#include <sgct/helpers/SGCTStringFunctions.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

// Apple doesn't support advanced sync features
// Nvidia Quadro Sync technology is only supported in Windows or Linux
#ifdef __WIN32__
HDC hDC;
#elif defined __LINUX__
GLXDrawable hDC;
Display * disp;
#ifdef GLEW_MX
GLXEWContext* glxewGetContext();
#endif // GLEW_MX
#endif

namespace {

constexpr const float QuadVerts[20] = {
    0.f, 0.f, -1.f, -1.f, -1.f,
    1.f, 0.f,  1.f, -1.f, -1.f,
    0.f, 1.f, -1.f,  1.f, -1.f,
    1.f, 1.f,  1.f,  1.f, -1.f
};

void windowResizeCallback(GLFWwindow* window, int width, int height) {
    sgct_core::SGCTNode* node = sgct_core::ClusterManager::instance()->getThisNodePtr();
    if (!node) {
        return;
    }

    width = std::max(width, 1);
    height = std::max(height, 1);
    for (int i = 0; i < node->getNumberOfWindows(); i++) {
        if (node->getWindowPtr(i).getWindowHandle() == window) {
            node->getWindowPtr(i).setWindowResolution(glm::ivec2(width, height));
        }
    }
}

void frameBufferResizeCallback(GLFWwindow* window, int width, int height) {
    sgct_core::SGCTNode* node = sgct_core::ClusterManager::instance()->getThisNodePtr();
    if (!node) {
        return;
    }

    width = std::max(width, 1);
    height = std::max(height, 1);
    for (int i = 0; i < node->getNumberOfWindows(); i++) {
        if (node->getWindowPtr(i).getWindowHandle() == window) {
            node->getWindowPtr(i).setFramebufferResolution(glm::ivec2(width, height));
        }
    }
}

void windowFocusCallback(GLFWwindow* window, int state) {
    sgct_core::SGCTNode* node = sgct_core::ClusterManager::instance()->getThisNodePtr();
    if (!node) {
        return;
    }

    for (int i = 0; i < node->getNumberOfWindows(); i++) {
        if (node->getWindowPtr(i).getWindowHandle() == window) {
            node->getWindowPtr(i).setFocused(state == GL_TRUE);
        }
    }
}

void windowIconifyCallback(GLFWwindow* window, int state) {
    sgct_core::SGCTNode* node = sgct_core::ClusterManager::instance()->getThisNodePtr();
    if (!node) {
        return;
    }

    for (int i = 0; i < node->getNumberOfWindows(); i++) {
        if (node->getWindowPtr(i).getWindowHandle() == window) {
            node->getWindowPtr(i).setIconified(state == GL_TRUE);
        }
    }
}

} // namespace

namespace sgct {

bool SGCTWindow::mUseSwapGroups = false;
bool SGCTWindow::mBarrier = false;
bool SGCTWindow::mSwapGroupMaster = false;
GLFWwindow* SGCTWindow::mCurrentContextOwner = nullptr;
GLFWwindow* SGCTWindow::mSharedHandle = nullptr;

SGCTWindow::SGCTWindow(int id)
    : mId(id)
{
    mUseFXAA = SGCTSettings::instance()->getDefaultFXAAState();
    mNumberOfAASamples = SGCTSettings::instance()->getDefaultNumberOfAASamples();

    // pointers
    mSharedHandle = nullptr;
}

void SGCTWindow::setName(std::string name) {
    mName = std::move(name);
}

void SGCTWindow::setTags(std::vector<std::string> tags) {
    mTags = std::move(tags);
}

const std::string& SGCTWindow::getName() const {
    return mName;
}

const std::vector<std::string>& SGCTWindow::getTags() const {
    return mTags;
}

bool SGCTWindow::hasTag(const std::string& tag) const {
    return std::find(mTags.cbegin(), mTags.cend(), tag) != mTags.cend();
}

int SGCTWindow::getId() const {
    return mId;
}

bool SGCTWindow::isFocused() const {
    return mFocused;
}

bool SGCTWindow::isIconified() const {
    return mIconified;
}

void SGCTWindow::close() {
    makeOpenGLContextCurrent(Context::Shared);

    for (sgct::PostFX& pfx : mPostFXPasses) {
        pfx.destroy();
    }
    mPostFXPasses.clear();

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Deleting screen capture data for window %d...\n", mId
    );
    mScreenCaptureLeftOrMono = nullptr;
    mScreenCaptureRight = nullptr;

    // delete FBO stuff
    if (mFinalFBO != nullptr && SGCTSettings::instance()->useFBO()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Releasing OpenGL buffers for window %d...\n", mId
        );
        mFinalFBO->destroy();
        mFinalFBO = nullptr;

        destroyFBOs();
    }

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Deleting VBOs for window %d...\n", mId
    );
    glDeleteBuffers(1, &mVBO);
    mVBO = 0;

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Deleting VAOs for window %d...\n", mId
    );
    glDeleteVertexArrays(1, &mVAO);
    mVAO = 0;

    // delete shaders
    stereo.shader.deleteProgram();

    // Current handle must be set at the end to propely destroy the window
    makeOpenGLContextCurrent(Context::Window);

    mCurrentViewport = nullptr;
    mViewports.clear();

    if (mUseSwapGroups) {
#ifdef __WIN32__
        if (glfwExtensionSupported("WGL_NV_swap_group")) {
            wglBindSwapBarrierNV(1, 0); // un-bind
            wglJoinSwapGroupNV(hDC, 0); // un-join
        }
#else
    #ifndef __APPLE__
        if (glfwExtensionSupported("GLX_NV_swap_group")) {
            glXBindSwapBarrierNV(disp, 1, 0); // un-bind
            glXJoinSwapGroupNV(disp, hDC, 0); // un-join
        }
    #endif
#endif
    }
}

void SGCTWindow::init() {
    if (!mFullScreen) {
        if (mSetWindowPos) {
            glfwSetWindowPos(mWindowHandle, mWindowPos.x, mWindowPos.y);
        }
        glfwSetWindowSizeCallback(mWindowHandle, windowResizeCallback);
        glfwSetFramebufferSizeCallback(mWindowHandle, frameBufferResizeCallback);
        glfwSetWindowFocusCallback(mWindowHandle, windowFocusCallback);
        glfwSetWindowIconifyCallback(mWindowHandle, windowIconifyCallback);
    }

    using namespace sgct_core;
    std::string title = "SGCT node: " +
        ClusterManager::instance()->getThisNodePtr()->getAddress() +
        " (" + (NetworkManager::instance()->isComputerServer() ? "master" : "slave") +
        + ": " + std::to_string(mId) + ")";

    setWindowTitle(mName.empty() ? title.c_str() : mName.c_str());

    // swap the buffers and update the window
    glfwSwapBuffers(mWindowHandle);

    // initNvidiaSwapGroups();
}

void SGCTWindow::initOGL() {
    updateColorBufferData();
    
    createTextures();
    createVBOs(); //must be created before FBO
    createFBOs();
    initScreenCapture();
    loadShaders();

    for (const std::unique_ptr<sgct_core::Viewport>& vp : mViewports) {
        if (!vp->hasSubViewports()) {
            continue;
        }

        setCurrentViewport(vp.get());
        vp->getNonLinearProjectionPtr()->setStereo(mStereoMode != StereoMode::NoStereo);
        vp->getNonLinearProjectionPtr()->setPreferedMonoFrustumMode(vp->getEye());
        vp->getNonLinearProjectionPtr()->init(
            mInternalColorFormat,
            mColorFormat,
            mColorDataType,
            mNumberOfAASamples
        );

        const float viewPortWidth = mFramebufferRes.x * vp->getXSize();
        const float viewPortHeight = mFramebufferRes.y * vp->getYSize();
        vp->getNonLinearProjectionPtr()->update(viewPortWidth, viewPortHeight);
    }
}

void SGCTWindow::initContextSpecificOGL() {
    makeOpenGLContextCurrent(Context::Window);
    TextureManager::CompressionMode cm = TextureManager::instance()->getCompression();
    // must be uncompressed otherwise artefacts will occur in gradients
    TextureManager::instance()->setCompression(TextureManager::CompressionMode::None);

    for (size_t j = 0; j < getNumberOfViewports(); j++) {
        sgct_core::Viewport& vp = getViewport(j);
        vp.loadData();
        if (vp.hasBlendMaskTexture() || vp.hasBlackLevelMaskTexture()) {
            mHasAnyMasks = true;
        }
    }

    // restore old state
    TextureManager::instance()->setCompression(cm);
}

unsigned int SGCTWindow::getFrameBufferTexture(unsigned int index) {
    switch (index) {
        case Engine::LeftEye:
            if (mFrameBufferTextures.leftEye == 0) {
                generateTexture(mFrameBufferTextures.leftEye, TextureType::Color);
            }
            return mFrameBufferTextures.leftEye;
        case Engine::RightEye:
            if (mFrameBufferTextures.rightEye == 0) {
                generateTexture(mFrameBufferTextures.rightEye, TextureType::Color);
            }
            return mFrameBufferTextures.rightEye;
        case Engine::Intermediate:
            if (mFrameBufferTextures.intermediate == 0) {
                generateTexture(mFrameBufferTextures.intermediate, TextureType::Color);
            }
            return mFrameBufferTextures.intermediate;
        case Engine::FX1:
            if (mFrameBufferTextures.fx1 == 0) {
                generateTexture(mFrameBufferTextures.fx1, TextureType::Color);
            }
            return mFrameBufferTextures.fx1;
        case Engine::FX2:
            if (mFrameBufferTextures.fx2 == 0) {
                generateTexture(mFrameBufferTextures.fx2, TextureType::Color);
            }
            return mFrameBufferTextures.fx2;
        case Engine::Depth:
            if (mFrameBufferTextures.depth == 0) {
                generateTexture(mFrameBufferTextures.depth, TextureType::Depth);
            }
            return mFrameBufferTextures.depth;
        case Engine::Normals:
            if (mFrameBufferTextures.normals == 0) {
                generateTexture(mFrameBufferTextures.normals, TextureType::Normal);
            }
            return mFrameBufferTextures.normals;
        case Engine::Positions:
            if (mFrameBufferTextures.positions == 0) {
                generateTexture(mFrameBufferTextures.positions, TextureType::Position);
            }
            return mFrameBufferTextures.positions;
        default:
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "SGCTWindow: Requested framebuffer texture index %d is out of bounds!\n",
                index
            );
            return 0;
    }
}

void SGCTWindow::setVisibility(bool state) {
    if (state != mVisible) {
        if (mWindowHandle) {
            if (state) {
                glfwShowWindow(mWindowHandle);
            }
            else {
                glfwHideWindow(mWindowHandle);
            }
        }
        mVisible = state;
    }
}

void SGCTWindow::setRenderWhileHidden(bool state) {
    mRenderWhileHidden = state;
}

void SGCTWindow::setFocused(bool state) {
    mFocused = state;
}

void SGCTWindow::setIconified(bool state) {
    mIconified = state;
}

void SGCTWindow::setWindowTitle(const char* title) {
    glfwSetWindowTitle(mWindowHandle, title);
}

void SGCTWindow::setWindowResolution(glm::ivec2 resolution) {
    // In case this callback gets triggered from elsewhere than sgct's glfwPollEvents, we
    // want to make sure the actual resizing is deferred to the end of the frame. This can
    // happen if some other library pulls events from the operating system for example by
    // calling nextEventMatchingMask (MacOS) or PeekMessageW (Windows). If we were to set
    // the actual mWindowRes directly, we may render half a frame with resolution and the
    // other half with resolution b, which is undefined behaviour. mHasNewPendingWindowRes
    // is checked in SGCTWindow::updateResolution, which is called from SGCTEngine's
    // render loop after glfwPollEvents.

    mHasPendingWindowRes = true;
    mPendingWindowRes = std::move(resolution);
}

void SGCTWindow::setFramebufferResolution(glm::ivec2 resolution) {
    // Defer actual update of framebuffer resolution until next call to updateResolutions.
    // (Same reason as described for setWindowResolution above.)
    if (!mUseFixResolution) {
        mHasPendingFramebufferRes = true;
        mPendingFramebufferRes = std::move(resolution);
    }
}

void SGCTWindow::swap(bool takeScreenshot) {
    if (!((mVisible || mRenderWhileHidden) && mAllowCapture)) {
        return;
    }

    makeOpenGLContextCurrent(Context::Window);
        
    if (takeScreenshot) {
        if (SGCTSettings::instance()->getCaptureFromBackBuffer() && mDoubleBuffered) {
            if (mScreenCaptureLeftOrMono != nullptr) {
                mScreenCaptureLeftOrMono->saveScreenCapture(
                    0,
                    mStereoMode == StereoMode::Active ?
                        sgct_core::ScreenCapture::CaptureSource::LeftBackBuffer :
                        sgct_core::ScreenCapture::CaptureSource::BackBuffer
                );
            }

            if (mScreenCaptureRight && mStereoMode == StereoMode::Active) {
                // @TODO(abock) This was mScreenCapture[0] before, but it seems like
                //              it should have been mScreenCapture[1] instead?!
                mScreenCaptureLeftOrMono->saveScreenCapture(
                    0,
                    sgct_core::ScreenCapture::CaptureSource::RightBackBuffer
                );
            }
        }
        else {
            if (mScreenCaptureLeftOrMono) {
                mScreenCaptureLeftOrMono->saveScreenCapture(mFrameBufferTextures.leftEye);
            }
            if (mScreenCaptureRight && mStereoMode > StereoMode::NoStereo &&
                mStereoMode < SGCTWindow::StereoMode::SideBySide)
            {
                mScreenCaptureRight->saveScreenCapture(mFrameBufferTextures.rightEye);
            }
        }
    }

    // swap
    mWindowResOld = mWindowRes;

    if (mDoubleBuffered) {
        glfwSwapBuffers(mWindowHandle);
    }
    else {
        glFinish();
    }
}

void SGCTWindow::updateResolutions() {
    if (mHasPendingWindowRes) {
        mWindowRes = mPendingWindowRes;
        float newAspectRatio =
            static_cast<float>(mWindowRes.x) / static_cast<float>(mWindowRes.y);

        // Set field of view of each of this window's viewports to match new
        // aspect ratio, adjusting only the horizontal (x) values.
        for (std::size_t j = 0; j < getNumberOfViewports(); ++j) {
            sgct_core::Viewport& vp = getViewport(j);
            vp.updateFovToMatchAspectRatio(mAspectRatio, newAspectRatio);
            MessageHandler::instance()->print(
                MessageHandler::Level::Debug,
                "SGCTWindow: update aspect ratio in viewport# %d (%f --> %f)...\n",
                j, mAspectRatio, newAspectRatio
            );
        }
        mAspectRatio = newAspectRatio;

        // Redraw window
        if (mWindowHandle) {
            glfwSetWindowSize(mWindowHandle, mWindowRes.x, mWindowRes.y);
        }

        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "SGCTWindow: Resolution changed to %dx%d in window %d...\n",
            mWindowRes.x, mWindowRes.y, mId
        );

        mHasPendingWindowRes = false;
    }

    if (mHasPendingFramebufferRes) {
        mFramebufferRes = mPendingFramebufferRes;

        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "SGCTWindow: Framebuffer resolution changed to %dx%d for window %d...\n",
            mFramebufferRes.x, mFramebufferRes.y, mId
        );

        mHasPendingFramebufferRes = false;
    }
}

void SGCTWindow::setHorizFieldOfView(float hFovDeg) {
    // Set field of view of each of this window's viewports to match new horiz/vert
    // aspect ratio, adjusting only the horizontal (x) values.
    for (size_t j = 0; j < getNumberOfViewports(); ++j) {
        sgct_core::Viewport& vp = getViewport(j);
        vp.setHorizontalFieldOfView(hFovDeg, mAspectRatio);
    }
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "SGCTWindow: Horizontal FOV changed to %f deg. in %d viewports for window %d "
        "using aspect ratio %f...\n",
        hFovDeg, getNumberOfViewports(), mId, mAspectRatio
    );
}

void SGCTWindow::initWindowResolution(glm::ivec2 resolution) {
    mWindowRes = resolution;
    mWindowResOld = mWindowRes;
    mAspectRatio = static_cast<float>(resolution.x) / static_cast<float>(resolution.y);
    mIsWindowResSet = true;

    if (!mUseFixResolution) {
        mFramebufferRes = resolution;
    }
}

bool SGCTWindow::update() {
    if (!mVisible || !isWindowResized()) {
        return false;
    }
    makeOpenGLContextCurrent(Context::Window);

    resizeFBOs();

    auto resizePBO = [this](sgct_core::ScreenCapture& sc) {
        const int nCaptureChannels = mAlpha ? 4 : 3;
        if (SGCTSettings::instance()->getCaptureFromBackBuffer()) {
            // capture from buffer supports only 8-bit per color component
            sc.setTextureTransferProperties(GL_UNSIGNED_BYTE, mPreferBGR);
            const glm::ivec2 res = getResolution();
            sc.initOrResize(res.x, res.y, nCaptureChannels, 1 );
        }
        else {
            // default: capture from texture (supports HDR)
            sc.setTextureTransferProperties(mColorDataType, mPreferBGR);
            const glm::ivec2 res = getFramebufferResolution();
            sc.initOrResize(res.x, res.y, nCaptureChannels, mBytesPerColor);
        }
    };
    if (mScreenCaptureLeftOrMono) {
        resizePBO(*mScreenCaptureLeftOrMono);
    }
    if (mScreenCaptureRight) {
        resizePBO(*mScreenCaptureRight);
    }

    // resize non linear projection buffers
    for (const std::unique_ptr<sgct_core::Viewport>& vp : mViewports) {
        if (vp->hasSubViewports()) {
            const float w = static_cast<float>(mFramebufferRes.x) * vp->getXSize();
            const float h = static_cast<float>(mFramebufferRes.y) * vp->getYSize();
            vp->getNonLinearProjectionPtr()->update(w, h);
        }
    }

    return true;
}

void SGCTWindow::makeOpenGLContextCurrent(Context context) {
    if (context == Context::Shared && mCurrentContextOwner != mSharedHandle) {
        glfwMakeContextCurrent(mSharedHandle);
        mCurrentContextOwner = mSharedHandle;
    }
    else if (context == Context::Window && mCurrentContextOwner != mWindowHandle) {
        glfwMakeContextCurrent(mWindowHandle);
        mCurrentContextOwner = mWindowHandle;
    }
}

void SGCTWindow::restoreSharedContext() {
    glfwMakeContextCurrent(mSharedHandle);
}

bool SGCTWindow::isWindowResized() const {
    return (mWindowRes.x != mWindowResOld.x || mWindowRes.y != mWindowResOld.y);
}

bool SGCTWindow::isBarrierActive() {
    return mBarrier;
}

bool SGCTWindow::isUsingSwapGroups() {
    return mUseSwapGroups;
}

bool SGCTWindow::isSwapGroupMaster() {
    return mSwapGroupMaster;
}

bool SGCTWindow::isFullScreen() const {
    return mFullScreen;
}

bool SGCTWindow::isFloating() const {
    return mFloating;
}

bool SGCTWindow::isDoubleBuffered() const {
    return mDoubleBuffered;
}

bool SGCTWindow::isVisible() const {
    return mVisible;
}

bool SGCTWindow::isRenderingWhileHidden() const {
    return mRenderWhileHidden;
}

bool SGCTWindow::isFixResolution() const {
    return mUseFixResolution;
}

bool SGCTWindow::isWindowResolutionSet() const {
    return mIsWindowResSet;
}

bool SGCTWindow::isStereo() const {
    return mStereoMode != StereoMode::NoStereo;
}

void SGCTWindow::setWindowPosition(glm::ivec2 positions) {
    mWindowPos = std::move(positions);
    mSetWindowPos = true;
}

void SGCTWindow::setWindowMode(bool fullscreen) {
    mFullScreen = fullscreen;
}

void SGCTWindow::setFloating(bool floating) {
    mFloating = floating;
}

void SGCTWindow::setDoubleBuffered(bool doubleBuffered) {
    mDoubleBuffered = doubleBuffered;
}

void SGCTWindow::setWindowDecoration(bool state) {
    mDecorated = state;
}

void SGCTWindow::setFullScreenMonitorIndex(int index) {
    mMonitorIndex = index;
}

void SGCTWindow::setBarrier(bool state) {
    if (mUseSwapGroups && state != mBarrier) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info, "SGCTWindow: Enabling Nvidia swap barrier...\n"
        );

#ifdef __WIN32__ //Windows uses wglew.h
        mBarrier = wglBindSwapBarrierNV(1, state ? 1 : 0);
#else //Apple and Linux uses glext.h
    #ifndef __APPLE__
        mBarrier = glXBindSwapBarrierNV(disp, 1, state ? 1 : 0);
    #endif
#endif
    }
}

void SGCTWindow::setFixResolution(bool state) {
    mUseFixResolution = state;
}

void SGCTWindow::setUsePostFX(bool state) {
    mUsePostFX = state;
    if (!state) {
        mUseFXAA = false;
    }
}

void SGCTWindow::setUseFXAA(bool state) {
    mUseFXAA = state;
    if (mUseFXAA) {
        mUsePostFX = true;
    }
    else {
        mUsePostFX = !mPostFXPasses.empty();
    }
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "FXAA status: %s for window %d\n", state ? "enabled" : "disabled", mId
    );
}

void SGCTWindow::setUseQuadbuffer(bool state) {
    mUseQuadBuffer = state;
    if (mUseQuadBuffer) {
        glfwWindowHint(GLFW_STEREO, GL_TRUE);
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Window %d: Enabling quadbuffered rendering.\n", mId
        );
    }
}

void SGCTWindow::setCallDraw2DFunction(bool state) {
    mCallDraw2DFunction = state;
    if (!mCallDraw2DFunction) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Window %d: Draw 2D function disabled for this window.\n", mId
        );
    }
}

void SGCTWindow::setCallDraw3DFunction(bool state) {
    mCallDraw3DFunction = state;
    if (!mCallDraw3DFunction) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Window %d: Draw (3D) function disabled for this window.\n", mId
        );
    }
}

void SGCTWindow::setCopyPreviousWindowToCurrentWindow(bool state) {
    mCopyPreviousWindowToCurrentWindow = state;
    if (mCopyPreviousWindowToCurrentWindow) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Window %d: CopyPreviousWindowToCurrentWindow enabled for this window.\n", mId
        );
    }
}

bool SGCTWindow::openWindow(GLFWwindow* share, size_t lastWindowIdx) {
    glfwWindowHint(GLFW_DEPTH_BITS, 32);
    glfwWindowHint(GLFW_DECORATED, mDecorated ? GL_TRUE : GL_FALSE);

    const int antiAliasingSamples = getNumberOfAASamples();
    if (antiAliasingSamples > 1 && !SGCTSettings::instance()->useFBO()) {
        // if multisample is used
        glfwWindowHint(GLFW_SAMPLES, antiAliasingSamples);
    }
    else {
        glfwWindowHint(GLFW_SAMPLES, 0);
    }

    glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);
    glfwWindowHint(GLFW_FLOATING, mFloating ? GL_TRUE : GL_FALSE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, mDoubleBuffered ? GL_TRUE : GL_FALSE);
    if (!mVisible) {
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    }

    setUseQuadbuffer(mStereoMode == StereoMode::Active);

    if (mFullScreen) {
        int count;
        GLFWmonitor** monitors = glfwGetMonitors(&count);

        const int refreshRateHint = SGCTSettings::instance()->getRefreshRateHint();
        if (refreshRateHint > 0) {
            glfwWindowHint(GLFW_REFRESH_RATE, refreshRateHint);
        }
        
        if (mMonitorIndex > 0 && mMonitorIndex < count) {
            mMonitor = monitors[mMonitorIndex];
        }
        else {
            mMonitor = glfwGetPrimaryMonitor();
            if (mMonitorIndex >= count) {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Info,
                    "SGCTWindow(%d): Invalid monitor index (%d). "
                    "This computer has %d monitors.\n",
                    mId, mMonitorIndex, count);
            }
        }

        if (!mIsWindowResSet) {
            const GLFWvidmode* currentMode = glfwGetVideoMode(mMonitor);
            mWindowRes = glm::ivec2(currentMode->width, currentMode->height);
        }
    }

    mWindowHandle = glfwCreateWindow(mWindowRes.x, mWindowRes.y, "SGCT", mMonitor, share);
    if (mWindowHandle == nullptr) {
        return false;
    }

    mSharedHandle = share != nullptr ? share : mWindowHandle;
    glfwMakeContextCurrent(mWindowHandle);

    // Mac for example scales the window size != frame buffer size
    glm::ivec2 bufferSize;
    glfwGetFramebufferSize(mWindowHandle, &bufferSize[0], &bufferSize[1]);

    mWindowInitialRes = mWindowRes;
    mScale = glm::vec2(bufferSize) / glm::vec2(mWindowRes);
    if (!mUseFixResolution) {
        mFramebufferRes = bufferSize;
    }
        
    //
    // Swap inerval:
    //  -1 = adaptive sync
    //   0  = vertical sync off
    //   1  = wait for vertical sync
    //   2  = fix when using swapgroups in xp and running half the framerate

    // If we would set multiple windows to use vsync, with would get a framerate of
    // (monitor refreshrate)/(number of windows),
    // which is something that might really slow down a multi-monitor application.
    // Setting last window to the requested interval, which does mean all other
    // windows will respect the last window in the pipeline.
    if (getId() == lastWindowIdx) {
        glfwSwapInterval(SGCTSettings::instance()->getSwapInterval());
    }
    else {
        glfwSwapInterval(0);
    }

    updateTransferCurve();

    //if slave disable mouse pointer
    if (!Engine::instance()->isMaster()) {
        glfwSetInputMode(mWindowHandle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }

    mFocused = glfwGetWindowAttrib(mWindowHandle, GLFW_FOCUSED) == GL_TRUE;
    mIconified = glfwGetWindowAttrib(mWindowHandle, GLFW_ICONIFIED) == GL_TRUE;
        
    // clear directly otherwise junk will be displayed on some OSs (OS X Yosemite)
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glfwMakeContextCurrent(mSharedHandle);

    if (SGCTSettings::instance()->useFBO()) {
        mScreenCaptureLeftOrMono = std::make_unique<sgct_core::ScreenCapture>();

        if (useRightEyeTexture()) {
            mScreenCaptureRight = std::make_unique<sgct_core::ScreenCapture>();
        }
    }

    mFinalFBO = std::make_unique<sgct_core::OffScreenBuffer>();

    return true;
}

void SGCTWindow::initNvidiaSwapGroups() {    
#ifdef __WIN32__ // Windows uses wglew.h
    if (glfwExtensionSupported("WGL_NV_swap_group")) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "SGCTWindow: Joining Nvidia swap group.\n"
        );

        hDC = wglGetCurrentDC();

        unsigned int maxBarrier = 0;
        unsigned int maxGroup = 0;
        wglQueryMaxSwapGroupsNV(hDC, &maxGroup, &maxBarrier);
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "WGL_NV_swap_group extension is supported.\n\tMax number of groups: %d\n\t"
            "Max number of barriers: %d\n",
            maxGroup, maxBarrier
        );

        // wglJoinSwapGroupNV adds <hDC> to the swap group specified by <group>.
        // If <hDC> is already a member of a different group, it is
        // implicitly removed from that group first. A swap group is specified as
        // an integer value between 0 and the value returned in <maxGroups> by
        // wglQueryMaxSwapGroupsNV. If <group> is zero, the hDC is unbound from its
        // current group, if any. If <group> is larger than <maxGroups>,
        // wglJoinSwapGroupNV fails.
        if (wglJoinSwapGroupNV(hDC, 1)) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Info, "SGCTWindow: Joining swapgroup 1 [ok].\n"
            );
            mUseSwapGroups = true;
        }
        else {
            MessageHandler::instance()->print(
                MessageHandler::Level::Info, "SGCTWindow: Joining swapgroup 1 [failed].\n"
            );
            mUseSwapGroups = false;
        }
    }
    else {
        mUseSwapGroups = false;
    }
#else //Apple and Linux uses glext.h
    #ifndef __APPLE__

    if (glfwExtensionSupported("GLX_NV_swap_group")) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info, "SGCTWindow: Joining Nvidia swap group.\n"
        );

        hDC = glXGetCurrentDrawable();
        disp = glXGetCurrentDisplay();

        unsigned int maxBarrier = 0;
        unsigned int maxGroup = 0;
        glXQueryMaxSwapGroupsNV(disp, hDC, &maxGroup, &maxBarrier);
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "GLX_NV_swap_group extension is supported.\n\tMax number of groups: %d\n\t"
            "Max number of barriers: %d\n",
            maxGroup, maxBarrier
        );

        if (glXJoinSwapGroupNV(disp, hDC, 1)) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Info, "SGCTWindow: Joining swapgroup 1 [ok].\n"
            );
            mUseSwapGroups = true;
        }
        else {
            MessageHandler::instance()->print(
                MessageHandler::Level::Info, "SGCTWindow: Joining swapgroup 1 [failed].\n"
            );
            mUseSwapGroups = false;
        }
    }
    else {
        mUseSwapGroups = false;
    }
    #endif
#endif
}

void SGCTWindow::initScreenCapture() {
    auto initializeCapture = [this](sgct_core::ScreenCapture& sc) {
        // a workaround for devices that support pbos but not showing it, like OSX (Intel)
        if (Engine::instance()->isOGLPipelineFixed()) {
            sc.setUsePBO(
                // if supported then use them
                glfwExtensionSupported("GL_ARB_pixel_buffer_object") == GL_TRUE &&
                SGCTSettings::instance()->getUsePBO()
            );
        }
        else {
            // in modern OpenGL pbos must be supported
            sc.setUsePBO(SGCTSettings::instance()->getUsePBO());
        }

        const int nCaptureChannels = mAlpha ? 4 : 3;
        if (SGCTSettings::instance()->getCaptureFromBackBuffer()) {
            // capturing from buffer supports only 8-bit per color component capture
            sc.setTextureTransferProperties(GL_UNSIGNED_BYTE, mPreferBGR);
            const glm::ivec2 res = getResolution();
            sc.initOrResize(res.x, res.y, nCaptureChannels, 1);
        }
        else {
            // default: capture from texture (supports HDR)
            sc.setTextureTransferProperties(mColorDataType, mPreferBGR);
            const glm::ivec2 res = getFramebufferResolution();
            sc.initOrResize(res.x, res.y, nCaptureChannels, mBytesPerColor);
        }

        SGCTSettings::CaptureFormat format = SGCTSettings::instance()->getCaptureFormat();
        switch (format) {
            case SGCTSettings::CaptureFormat::PNG:
                sc.setCaptureFormat(sgct_core::ScreenCapture::CaptureFormat::PNG);
                break;
            case SGCTSettings::CaptureFormat::TGA:
                sc.setCaptureFormat(sgct_core::ScreenCapture::CaptureFormat::TGA);
                break;
            case SGCTSettings::CaptureFormat::JPG:
                sc.setCaptureFormat(sgct_core::ScreenCapture::CaptureFormat::JPEG);
                break;
        }

        if (!Engine::checkForOGLErrors()) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "SGCTWindow %d: OpenGL error occured in screen capture init!\n", mId
            );
        }
    };


    if (mScreenCaptureLeftOrMono) {
        using namespace sgct_core;
        if (useRightEyeTexture()) {
            mScreenCaptureLeftOrMono->init(mId, ScreenCapture::EyeIndex::StereoLeft);
        }
        else {
            mScreenCaptureLeftOrMono->init(mId, ScreenCapture::EyeIndex::Mono);
        }
        initializeCapture(*mScreenCaptureLeftOrMono);
    }

    if (mScreenCaptureRight) {
        mScreenCaptureRight->init(mId, sgct_core::ScreenCapture::EyeIndex::StereoRight);
        initializeCapture(*mScreenCaptureRight);
    }
}

unsigned int SGCTWindow::getSwapGroupFrameNumber() {
    unsigned int frameNumber = 0;

    if (mBarrier) {
    #ifdef __WIN32__ //Windows uses wglew.h
        if (glfwExtensionSupported("WGL_NV_swap_group")) {
            wglQueryFrameCountNV(hDC, &frameNumber);
        }
    #else //Apple and Linux uses glext.h
        #ifndef __APPLE__
        if (glfwExtensionSupported("GLX_NV_swap_group")) {
            glXQueryFrameCountNV(disp, hDC, &frameNumber);
        }
        #endif
    #endif
    }
    return frameNumber;
}

void SGCTWindow::resetSwapGroupFrameNumber() {
    if (mBarrier) {
#ifdef __WIN32__
        bool success = glfwExtensionSupported("WGL_NV_swap_group") &&
                       wglResetFrameCountNV(hDC);
#else
    #ifdef __APPLE__
        bool success = false;
    #else //linux
        bool success = glfwExtensionSupported("GLX_NV_swap_group") &&
                       glXResetFrameCountNV(disp, hDC);
    #endif
#endif
        if (success) {
            mSwapGroupMaster = true;
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Resetting frame counter. This computer is the master.\n"
            );
        }
        else {
            mSwapGroupMaster = false;
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Resetting frame counter failed. This computer is the slave.\n"
            );
        }
    }
}

void SGCTWindow::createTextures() {
    // no target textures needed if not using FBO
    if (!SGCTSettings::instance()->useFBO()) {
        return;
    }

    if (Engine::instance()->getRunMode() <= Engine::OpenGL_Compatibility_Profile) {
        glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT);
        glEnable(GL_TEXTURE_2D);
    }

    GLint maxTexSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
    if (mFramebufferRes.x > maxTexSize || mFramebufferRes.y > maxTexSize) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "SGCTWindow %d: Requested framebuffer is to big (Max: %dx%d)!\n",
            mId, maxTexSize, maxTexSize
        );
        return;
    }

    // Create left and right color & depth textures; don't allocate the right eye image if
    // stereo is not used create a postFX texture for effects
    generateTexture(mFrameBufferTextures.leftEye, TextureType::Color);
    if (useRightEyeTexture()) {
        generateTexture(mFrameBufferTextures.rightEye, TextureType::Color);
    }
    if (SGCTSettings::instance()->useDepthTexture()) {
        generateTexture(mFrameBufferTextures.depth, TextureType::Depth);
    }
    if (!mPostFXPasses.empty()) {
        generateTexture(mFrameBufferTextures.fx1, TextureType::Color);
    }
    if (mPostFXPasses.size() > 1) {
        generateTexture(mFrameBufferTextures.fx2, TextureType::Color);
    }
    if (mUsePostFX) {
        generateTexture(mFrameBufferTextures.intermediate, TextureType::Color);
    }
    if (SGCTSettings::instance()->useNormalTexture()) {
        generateTexture(mFrameBufferTextures.normals, TextureType::Normal);
    }
    if (SGCTSettings::instance()->usePositionTexture()) {
        generateTexture(mFrameBufferTextures.positions, TextureType::Position);
    }

    if (Engine::instance()->getRunMode() <= Engine::OpenGL_Compatibility_Profile) {
        glPopAttrib();
    }

    if (Engine::checkForOGLErrors()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "Texture targets initialized successfully for window %d!\n", mId
        );
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Texture targets failed to initialize for window %d!\n", mId
        );
    }
}

void SGCTWindow::generateTexture(unsigned int& id, SGCTWindow::TextureType type) {
    // clean up if needed
    glDeleteTextures(1, &id);
    id = 0;

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    
    // Disable mipmaps
    if (Engine::instance()->isOGLPipelineFixed()) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    }

    // Determine the internal texture format, the texture format, and the pixel type
    const auto [internalFormat, format, pType] =
        [this, type](SGCTWindow::TextureType type) -> std::tuple<int, unsigned int, int> {
            switch (type) {
                case TextureType::Color:
                    return { mInternalColorFormat, mColorFormat, mColorDataType };
                case TextureType::Depth:
                    return { GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT };
                case TextureType::Normal:
                case TextureType::Position:
                    return {
                         SGCTSettings::instance()->getBufferFloatPrecisionAsGLint(),
                         GL_RGB,
                         GL_FLOAT
                    };
            }
        }(type);

    const glm::ivec2 res = mFramebufferRes;
    const bool fixedPipeline = Engine::instance()->isOGLPipelineFixed();
    const bool forceGlTex = SGCTSettings::instance()->getForceGlTexImage2D();
    if (fixedPipeline || forceGlTex) {
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, res.x, res.y, 0, format, pType, 0);
    }
    else {
        glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, res.x, res.y);
    }

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "%dx%d texture (id: %d) generated for window %d!\n",
        mFramebufferRes.x, mFramebufferRes.y, id, mId
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
}

void SGCTWindow::createFBOs() {
    if (!SGCTSettings::instance()->useFBO()) {
        // disable anaglyph & checkerboard stereo if FBOs are not used
        if (mStereoMode > StereoMode::Active) {
            mStereoMode = StereoMode::NoStereo;
        }
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Warning! FBO rendering is not supported or enabled!\nPostFX, fisheye and "
            "some stereo modes are disabled.\n"
        );
        return;
    }

    mFinalFBO->setInternalColorFormat(mInternalColorFormat);
    mFinalFBO->createFBO(mFramebufferRes.x, mFramebufferRes.y, mNumberOfAASamples);
            
    if (mFinalFBO->checkForErrors()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "Window %d: FBO initiated successfully. Number of samples: %d\n",
            mId, mFinalFBO->isMultiSampled() ? mNumberOfAASamples : 1
        );
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Window %d: FBO initiated with errors! Number of samples: %d\n",
            mId, mFinalFBO->isMultiSampled() ? mNumberOfAASamples : 1
        );
    }
}

void SGCTWindow::createVBOs() {
    if (!Engine::instance()->isOGLPipelineFixed()) {
        glGenVertexArrays(1, &mVAO);
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug, "SGCTWindow: Generating VAO: %d\n", mVAO
        );
    }

    glGenBuffers(1, &mVBO);
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug, "SGCTWindow: Generating VBO: %d\n", mVBO
    );

    if (!Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(mVAO);
    }
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    //2TF + 3VF = 2*4 + 3*4 = 20
    glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), QuadVerts, GL_STATIC_DRAW);
    if (!Engine::instance()->isOGLPipelineFixed()) {
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,
            2,
            GL_FLOAT,
            GL_FALSE,
            5 * sizeof(float),
            reinterpret_cast<void*>(0)
        );

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            5 * sizeof(float),
            reinterpret_cast<void*>(8)
        );
    }

    // unbind
    if (!Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SGCTWindow::loadShaders() {
    // load shaders
    if (mStereoMode <= StereoMode::Active || mStereoMode >= StereoMode::SideBySide) {
        return;
    }

    // reload shader program if it exists
    if (stereo.shader.isLinked()) {
        stereo.shader.deleteProgram();
    }

    const bool fixed = Engine::instance()->isOGLPipelineFixed();
    using namespace sgct_core;

    std::string stereoVertShader = fixed ?
        shaders::AnaglyphVert :
        shaders_modern::AnaglyphVert;

    std::string stereoFragShader = [this, fixed]() {
        switch (mStereoMode) {
        case StereoMode::AnaglyphRedCyan:
            return fixed ?
                shaders::AnaglyphRedCyanFrag :
                shaders_modern::AnaglyphRedCyanFrag;
            break;
        case StereoMode::AnaglyphAmberBlue:
            return fixed ?
                shaders::AnaglyphAmberBlueFrag :
                shaders_modern::AnaglyphAmberBlueFrag;
            break;
        case StereoMode::AnaglyphRedCyanWimmer:
            return fixed ?
                shaders::AnaglyphRedCyanWimmerFrag :
                shaders_modern::AnaglyphRedCyanWimmerFrag;
            break;
        case StereoMode::Checkerboard:
            return fixed ? shaders::CheckerBoardFrag : shaders_modern::CheckerBoardFrag;
            break;
        case StereoMode::CheckerboardInverted:
            return fixed ?
                shaders::CheckerBoardInvertedFrag :
                shaders_modern::CheckerBoardInvertedFrag;
            break;
        case StereoMode::VerticalInterlaced:
            return fixed ?
                shaders::VerticalInterlacedFrag :
                shaders_modern::VerticalInterlacedFrag;
            break;
        case StereoMode::VerticalInterlacedInverted:
            return fixed ?
                shaders::VerticalInterlacedInvertedFrag :
                shaders_modern::VerticalInterlacedInvertedFrag;
            break;
        default:
            return fixed ? shaders::DummyStereoFrag : shaders_modern::DummyStereoFrag;
            break;
        }
    }();

    const std::string glslVersion = Engine::instance()->getGLSLVersion();

    sgct_helpers::findAndReplace(stereoVertShader, "**glsl_version**", glslVersion);
    const bool vertShader = stereo.shader.addShaderSrc(
        stereoVertShader,
        GL_VERTEX_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!vertShader) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Failed to load stereo vertex shader\n"
        );
    }

    sgct_helpers::findAndReplace(stereoFragShader, "**glsl_version**", glslVersion);
    const bool fragShader = stereo.shader.addShaderSrc(
        stereoFragShader,
        GL_FRAGMENT_SHADER,
        ShaderProgram::ShaderSourceType::String
    );
    if (!fragShader) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Failed to load stereo fragment shader\n"
        );
    }

    stereo.shader.setName("StereoShader");
    stereo.shader.createAndLinkProgram();
    stereo.shader.bind();
    if (!Engine::instance()->isOGLPipelineFixed()) {
        stereo.mvpLoc = stereo.shader.getUniformLocation("MVP");
    }
    stereo.leftTexLoc = stereo.shader.getUniformLocation("LeftTex");
    stereo.rightTexLoc = stereo.shader.getUniformLocation("RightTex");
    glUniform1i(stereo.leftTexLoc, 0);
    glUniform1i(stereo.rightTexLoc, 1);
    ShaderProgram::unbind();

    if (!Engine::checkForOGLErrors()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "SGCTWindow %d: OpenGL error occured while loading shaders!\n", mId
        );
    }
}

void SGCTWindow::bindVAO() const {
    glBindVertexArray(mVAO);
}

void SGCTWindow::bindVBO() const {
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
}

void SGCTWindow::unbindVBO() const {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SGCTWindow::unbindVAO() const {
    glBindVertexArray(0);
}

sgct_core::OffScreenBuffer* SGCTWindow::getFBOPtr() const {
    return mFinalFBO.get();
}

GLFWmonitor* SGCTWindow::getMonitor() const {
    return mMonitor;
}

GLFWwindow* SGCTWindow::getWindowHandle() const {
    return mWindowHandle;
}

glm::ivec2 SGCTWindow::getFinalFBODimensions() const {
    return mFramebufferRes;
}

void SGCTWindow::addPostFX(PostFX fx) {
    mPostFXPasses.push_back(std::move(fx));
}

void SGCTWindow::resizeFBOs() {
    if (mUseFixResolution || !SGCTSettings::instance()->useFBO()) {
        return;
    }

    makeOpenGLContextCurrent(Context::Shared);
    destroyFBOs();
    createTextures();

    mFinalFBO->resizeFBO(mFramebufferRes.x, mFramebufferRes.y, mNumberOfAASamples);
        
    if (!mFinalFBO->isMultiSampled()) {
        //attatch color buffer to prevent GL errors
        mFinalFBO->bind();
        mFinalFBO->attachColorTexture(mFrameBufferTextures.leftEye);
        mFinalFBO->unBind();
    }

    if (mFinalFBO->checkForErrors()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug, "Window %d: FBOs resized successfully.\n", mId
        );
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error, "Window %d: FBOs resized with GL errors!\n", mId
        );
    }
}

void SGCTWindow::destroyFBOs() {
    glDeleteTextures(1, &mFrameBufferTextures.leftEye);
    mFrameBufferTextures.leftEye = 0;
    glDeleteTextures(1, &mFrameBufferTextures.rightEye);
    mFrameBufferTextures.rightEye = 0;
    glDeleteTextures(1, &mFrameBufferTextures.depth);
    mFrameBufferTextures.depth = 0;
    glDeleteTextures(1, &mFrameBufferTextures.fx1);
    mFrameBufferTextures.fx1 = 0;
    glDeleteTextures(1, &mFrameBufferTextures.fx2);
    mFrameBufferTextures.fx2 = 0;
    glDeleteTextures(1, &mFrameBufferTextures.intermediate);
    mFrameBufferTextures.intermediate = 0;
    glDeleteTextures(1, &mFrameBufferTextures.positions);
    mFrameBufferTextures.positions = 0;
}

SGCTWindow::StereoMode SGCTWindow::getStereoMode() const {
    return mStereoMode;
}

void SGCTWindow::addViewport(std::unique_ptr<sgct_core::Viewport> vpPtr) {
    mViewports.push_back(std::move(vpPtr));
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug, "Adding viewport (total %d)\n", mViewports.size()
    );
}

sgct_core::BaseViewport* SGCTWindow::getCurrentViewport() const {
    if (mCurrentViewport == nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "Window %d error: Current viewport is nullptr!\n", mId
        );
    }
    return mCurrentViewport;
}

const sgct_core::Viewport& SGCTWindow::getViewport(size_t index) const {
    return *mViewports[index];
}

sgct_core::Viewport& SGCTWindow::getViewport(size_t index) {
    return *mViewports[index];
}

glm::ivec4 SGCTWindow::getCurrentViewportPixelCoords() const {
    return glm::ivec4(
        static_cast<int>(getCurrentViewport()->getX() * mFramebufferRes.x),
        static_cast<int>(getCurrentViewport()->getY() * mFramebufferRes.y),
        static_cast<int>(getCurrentViewport()->getXSize() * mFramebufferRes.x),
        static_cast<int>(getCurrentViewport()->getYSize() * mFramebufferRes.y)
    );
}

size_t SGCTWindow::getNumberOfViewports() const {
    return mViewports.size();
}

void SGCTWindow::setNumberOfAASamples(int samples) {
    mNumberOfAASamples = samples;
}

int SGCTWindow::getNumberOfAASamples() const {
    return mNumberOfAASamples;
}

void SGCTWindow::setStereoMode(StereoMode sm) {
    mStereoMode = sm;

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "SGCTWindow: Setting stereo mode to '%s' for window %d.\n",
        getStereoModeStr().c_str(), mId
    );

    if (mWindowHandle) {
        loadShaders();
    }
}

sgct_core::ScreenCapture* SGCTWindow::getScreenCapturePointer(Eye eye) const {
    switch (eye) {
        case Eye::MonoOrLeft:
            return mScreenCaptureLeftOrMono.get();
        case Eye::Right:
            return mScreenCaptureRight.get();
        default:
            return nullptr;
    }
}

void SGCTWindow::setCurrentViewport(size_t index) {
    mCurrentViewport = mViewports[index].get();
}

void SGCTWindow::setCurrentViewport(sgct_core::BaseViewport* vp) {
    mCurrentViewport = vp;
}

std::string SGCTWindow::getStereoModeStr() const {
    switch (mStereoMode) {
        case StereoMode::Active:
            return "active";
        case StereoMode::AnaglyphRedCyan:
            return "anaglyph_red_cyan";
        case StereoMode::AnaglyphAmberBlue:
            return "anaglyph_amber_blue";
        case StereoMode::AnaglyphRedCyanWimmer:
            return "anaglyph_wimmer";
        case StereoMode::Checkerboard:
            return "checkerboard";
        case StereoMode::CheckerboardInverted:
            return "checkerboard_inverted";
        case StereoMode::VerticalInterlaced:
            return "vertical_interlaced";
        case StereoMode::VerticalInterlacedInverted:
            return "vertical_interlaced_inverted";
        case StereoMode::Dummy:
            return "dummy";
        case StereoMode::SideBySide:
            return "side_by_side";
        case StereoMode::SideBySideInverted:
            return "side_by_side_inverted";
        case StereoMode::TopBottom:
            return "top_bottom";
        case StereoMode::TopBottomInverted:
            return "top_bottom_inverted";
        default:
            return "none";
    }
}

void SGCTWindow::updateTransferCurve() {
    if (!mMonitor) {
        return;
    }

    GLFWgammaramp ramp;
    unsigned short red[256];
    unsigned short green[256];
    unsigned short blue[256];
    
    ramp.size = 256;
    ramp.red = red;
    ramp.green = green;
    ramp.blue = blue;

    float gammaExp = 1.f / mGamma;

    for (unsigned int i = 0; i < ramp.size; i++) {
        float c = ((static_cast<float>(i) / 255.f) - 0.5f) * mContrast + 0.5f;
        float b = c + (mBrightness - 1.f);
        float g = powf(b, gammaExp);

        //transform back
        //unsigned short t = static_cast<unsigned short>(roundf(256.0f * g));

        unsigned short t = static_cast<unsigned short>(
            glm::clamp(65535.f * g, 0.f, 65535.f) + 0.5f
        );

        ramp.red[i] = t;
        ramp.green[i] = t;
        ramp.blue[i] = t;
    }

    glfwSetGammaRamp(mMonitor, &ramp);
}

void SGCTWindow::updateColorBufferData() {
    mColorFormat = GL_BGRA;
    
    switch (mBufferColorBitDepth) {
        default:
        case ColorBitDepth::Depth8:
            mInternalColorFormat = GL_RGBA8;
            mColorDataType = GL_UNSIGNED_BYTE;
            mBytesPerColor = 1;
            break;
        case ColorBitDepth::Depth16:
            mInternalColorFormat = GL_RGBA16;
            mColorDataType = GL_UNSIGNED_SHORT;
            mBytesPerColor = 2;
            break;
        case ColorBitDepth::Depth16Float:
            mInternalColorFormat = GL_RGBA16F;
            mColorDataType = GL_HALF_FLOAT;
            mBytesPerColor = 2;
            break;
        case ColorBitDepth::Depth32Float:
            mInternalColorFormat = GL_RGBA32F;
            mColorDataType = GL_FLOAT;
            mBytesPerColor = 4;
            break;
        case ColorBitDepth::Depth16Int:
            mInternalColorFormat = GL_RGBA16I;
            mColorDataType = GL_SHORT;
            mBytesPerColor = 2;
            break;
        case ColorBitDepth::Depth32Int:
            mInternalColorFormat = GL_RGBA32I;
            mColorDataType = GL_INT;
            mBytesPerColor = 2;
            break;
        case ColorBitDepth::Depth16UInt:
            mInternalColorFormat = GL_RGBA16UI;
            mColorDataType = GL_UNSIGNED_SHORT;
            mBytesPerColor = 2;
            break;
        case ColorBitDepth::Depth32UInt:
            mInternalColorFormat = GL_RGBA32UI;
            mColorDataType = GL_UNSIGNED_INT;
            mBytesPerColor = 4;
            break;
    }
}

bool SGCTWindow::useRightEyeTexture() const {
    return mStereoMode != StereoMode::NoStereo && mStereoMode < StereoMode::SideBySide;
}

void SGCTWindow::setAlpha(bool state) {
    mAlpha = state;
}

bool SGCTWindow::getAlpha() const {
    return mAlpha;
}

void SGCTWindow::setGamma(float gamma) {
    mGamma = gamma;
    updateTransferCurve();
}

float SGCTWindow::getGamma() const {
    return mGamma;
}

void SGCTWindow::setContrast(float contrast) {
    mContrast = contrast;
    updateTransferCurve();
}

float SGCTWindow::getContrast() const {
    return mContrast;
}

void SGCTWindow::setBrightness(float brightness) {
    mBrightness = brightness;
    updateTransferCurve();
}

void SGCTWindow::setColorBitDepth(ColorBitDepth cbd) {
    mBufferColorBitDepth = cbd;
}

SGCTWindow::ColorBitDepth SGCTWindow::getColorBitDepth() const {
    return mBufferColorBitDepth;
}

void SGCTWindow::setPreferBGR(bool state) {
    mPreferBGR = state;
}

void SGCTWindow::setAllowCapture(bool state) {
    mAllowCapture = state;
}

bool SGCTWindow::isBGRPreferred() const {
    return mPreferBGR;
}

bool SGCTWindow::isCapturingAllowed() const {
    return mAllowCapture;
}

float SGCTWindow::getBrightness() const {
    return mBrightness;
}

float SGCTWindow::getHorizFieldOfViewDegrees() const {
    return mViewports[0]->getHorizontalFieldOfViewDegrees();
}

PostFX& SGCTWindow::getPostFX(size_t index) {
    return mPostFXPasses[index];
}

size_t SGCTWindow::getNumberOfPostFXs() const {
    return mPostFXPasses.size();
}

glm::ivec2 SGCTWindow::getResolution() const {
    return mWindowRes;
}

glm::ivec2 SGCTWindow::getFramebufferResolution() const {
    return mFramebufferRes;
}

glm::ivec2 SGCTWindow::getInitialResolution() const {
    return mWindowInitialRes;
}

glm::vec2 SGCTWindow::getScale() const {
    return mScale;
}

float SGCTWindow::getAspectRatio() const {
    return mAspectRatio;
}

int SGCTWindow::getFramebufferBPCC() const {
    return mBytesPerColor;
}

bool SGCTWindow::hasAnyMasks() const {
    return mHasAnyMasks;
}

bool SGCTWindow::useFXAA() const {
    return mUseFXAA;
}

bool SGCTWindow::usePostFX() const {
    return mUsePostFX;
}

void SGCTWindow::bindStereoShaderProgram() const {
    stereo.shader.bind();
}

int SGCTWindow::getStereoShaderMVPLoc() const {
    return stereo.mvpLoc;
}

int SGCTWindow::getStereoShaderLeftTexLoc() const {
    return stereo.leftTexLoc;
}

int SGCTWindow::getStereoShaderRightTexLoc() const {
    return stereo.rightTexLoc;
}

bool SGCTWindow::getCallDraw2DFunction() const {
    return mCallDraw2DFunction;
}

bool SGCTWindow::getCallDraw3DFunction() const {
    return mCallDraw3DFunction;
}

bool SGCTWindow::getCopyPreviousWindowToCurrentWindow() const {
    return mCopyPreviousWindowToCurrentWindow;
}

} // namespace sgct
