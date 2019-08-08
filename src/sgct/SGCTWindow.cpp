/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/SGCTWindow.h>

#include <sgct/Engine.h>
#include <sgct/TextureManager.h>
#include <sgct/MessageHandler.h>
#include <sgct/ClusterManager.h>
#include <sgct/SGCTSettings.h>
#include <sgct/shaders/SGCTInternalShaders.h>
#include <sgct/shaders/SGCTInternalShaders_modern.h>
#include <sgct/helpers/SGCTStringFunctions.h>
#include <glm/gtc/matrix_transform.hpp>
#include <stdio.h>
#include <algorithm>
#include <sstream>

/*
    Apple doesn't support advanced sync features

    Nvidia Quadro Sync technology is only supported in Windows or Linux
*/
#ifdef __WIN32__
HDC hDC;
#elif defined __LINUX__
GLXDrawable hDC;
Display * disp;
#ifdef GLEW_MX
GLXEWContext * glxewGetContext();
#endif
#endif

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

    //FBO targets init
    for (int i = 0; i < NUMBER_OF_TEXTURES; i++) {
        mFrameBufferTextures[i] = 0;
    }

    //pointers
    mSharedHandle = nullptr;
}

/*!
Name this window
*/
void SGCTWindow::setName(std::string name) {
    mName = std::move(name);
}

/*!
Tag this window
Tags are seperated by comma
*/
void SGCTWindow::setTags(std::string tags) {
    std::stringstream ss(std::move(tags));
    while (ss.good()) {
        std::string substr;
        getline(ss, substr, ',');
        mTags.push_back(substr);
    }
}

/*!
\returns the name of this window
*/
const std::string& SGCTWindow::getName() const {
    return mName;
}

/*!
\returns the tags of this window
*/
const std::vector<std::string>& SGCTWindow::getTags() const {
    return mTags;
}

/*!
\returns true if a specific tag exists
\tags are seperated by comma
*/
bool SGCTWindow::checkIfTagExists(const std::string& tag) const {
    return std::find(mTags.begin(), mTags.end(), tag) != mTags.end();
}

/*!
    \returns this window's id
*/
int SGCTWindow::getId() const {
    return mId;
}

/*!
    \returns this window's focused flag
*/
bool SGCTWindow::isFocused() const {
    return mFocused;
}

/*!
    \returns this window's inconify flag 
*/
bool SGCTWindow::isIconified() const {
    return mIconified;
}

void SGCTWindow::close() {
    makeOpenGLContextCurrent(Shared_Context);

    //delete postFX
    for (size_t i = 0; i < getNumberOfPostFXs(); i++) {
        mPostFXPasses[i].destroy();
    }
    mPostFXPasses.clear();

    MessageHandler::instance()->print(
        MessageHandler::NOTIFY_INFO,
        "Deleting screen capture data for window %d...\n", mId
    );
    for (int i = 0; i < 2; i++) {
        if (mScreenCapture[i]) {
            delete mScreenCapture[i];
            mScreenCapture[i] = nullptr;
        }
    }

    //delete FBO stuff
    if (mFinalFBO_Ptr != nullptr && SGCTSettings::instance()->useFBO()) {
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_INFO,
            "Releasing OpenGL buffers for window %d...\n", mId
        );
        mFinalFBO_Ptr->destroy();

        delete mFinalFBO_Ptr;
        mFinalFBO_Ptr = nullptr;

        for (unsigned int i = 0; i < NUMBER_OF_TEXTURES; i++) {
            if (mFrameBufferTextures[i] != 0) {
                glDeleteTextures(1, &mFrameBufferTextures[i]);
                mFrameBufferTextures[i] = 0;
            }
        }
    }

    if (mVBO) {
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_INFO,
            "Deleting VBOs for window %d...\n", mId
        );
        glDeleteBuffers(1, &mVBO);
        mVBO = 0;
    }

    if (mVAO) {
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_INFO,
            "Deleting VAOs for window %d...\n", mId
        );
        glDeleteVertexArrays(1, &mVAO);
        mVAO = 0;
    }

    //delete shaders
    mStereoShader.deleteProgram();

    /*
    Current handle must be set at the end to propely destroy the window
    */
    makeOpenGLContextCurrent(Window_Context);

    deleteAllViewports();

    if (mUseSwapGroups) {
//#ifdef __WITHSWAPBARRIERS__

#ifdef __WIN32__
        if (glfwExtensionSupported("WGL_NV_swap_group")) {
            //un-bind
            wglBindSwapBarrierNV(1, 0);
            //un-join
            wglJoinSwapGroupNV(hDC, 0);
        }
#else
    #ifndef __APPLE__
        if (glfwExtensionSupported("GLX_NV_swap_group")) {
            //un-bind
            glXBindSwapBarrierNV(disp, 1, 0);
            //un-join
            glXJoinSwapGroupNV(disp, hDC, 0);
        }
    #endif
#endif

    }
}

void SGCTWindow::init() {
    if (!mFullScreen) {
        if (mSetWindowPos) {
            glfwSetWindowPos(mWindowHandle, mWindowPos[0], mWindowPos[1]);
        }
        glfwSetWindowSizeCallback(mWindowHandle, windowResizeCallback);
        glfwSetFramebufferSizeCallback(mWindowHandle, frameBufferResizeCallback);
        glfwSetWindowFocusCallback(mWindowHandle, windowFocusCallback);
        glfwSetWindowIconifyCallback(mWindowHandle, windowIconifyCallback);
    }

    std::string title = "SGCT node: " +
        sgct_core::ClusterManager::instance()->getThisNodePtr()->getAddress() +
        " (" + (sgct_core::NetworkManager::instance()->isComputerServer() ? "master" : "slave") +
        + ": " + std::to_string(mId) + ")";

    setWindowTitle(mName.empty() ? title.c_str() : mName.c_str());

    //swap the buffers and update the window
    glfwSwapBuffers(mWindowHandle);

    //initNvidiaSwapGroups();
}

/*!
    Init window buffers such as textures, FBOs, VAOs, VBOs and PBOs.
*/
void SGCTWindow::initOGL() {
    updateColorBufferData();
    
    createTextures();
    createVBOs(); //must be created before FBO
    createFBOs();
    initScreenCapture();
    loadShaders();

    for (sgct_core::Viewport* vp : mViewports) {
        if (vp->hasSubViewports()) {
            setCurrentViewport(vp);
            vp->getNonLinearProjectionPtr()->setStereo(mStereoMode != No_Stereo);
            vp->getNonLinearProjectionPtr()->setPreferedMonoFrustumMode(vp->getEye());
            vp->getNonLinearProjectionPtr()->init(
                mInternalColorFormat,
                mColorFormat,
                mColorDataType,
                mNumberOfAASamples
            );

            float viewPortWidth = mFramebufferResolution[0] * vp->getXSize();
            float viewPortHeight = mFramebufferResolution[1] * vp->getYSize();
            vp->getNonLinearProjectionPtr()->update(viewPortWidth, viewPortHeight);
        }
    }
}

/*!
    Init context specific data such as viewport corrections/warping meshes
*/
void SGCTWindow::initContextSpecificOGL() {
    makeOpenGLContextCurrent(Window_Context);
    unsigned int numberOfMasks = 0;
    TextureManager::CompressionMode cm = TextureManager::instance()->getCompression();
    // must be uncompressed otherwise arifacts will occur in gradients
    TextureManager::instance()->setCompression(TextureManager::No_Compression);

    for (size_t j = 0; j < getNumberOfViewports(); j++) {
        sgct_core::Viewport* vpPtr = getViewport(j);
        vpPtr->loadData();
        if (vpPtr->hasBlendMaskTexture()) {
            numberOfMasks++;
        }

        if (vpPtr->hasBlackLevelMaskTexture()) {
            numberOfMasks++;
        }
    }

    //restore
    TextureManager::instance()->setCompression(cm);

    if (numberOfMasks > 0) {
        mHasAnyMasks = true;
    }
}

/*!
    Get a frame buffer texture. If the texture doesn't exists then it will be created.

    \param index Index or Engine::TextureIndexes enum
    \returns texture index of selected frame buffer texture
*/
unsigned int SGCTWindow::getFrameBufferTexture(unsigned int index) {
    if (index >= NUMBER_OF_TEXTURES) {
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_ERROR,
            "SGCTWindow: Requested framebuffer texture index %d is out of bounds!\n",
            index
        );
        return 0;
    }

    if (mFrameBufferTextures[index] == 0) {
        switch(index) {
            case Engine::LeftEye:
            case Engine::RightEye:
            case Engine::Intermediate:
            case Engine::FX1:
            case Engine::FX2:
                generateTexture(
                    index,
                    mFramebufferResolution[0],
                    mFramebufferResolution[1],
                    ColorTexture,
                    true
                );
                break;
            case Engine::Depth:
                generateTexture(
                    index,
                    mFramebufferResolution[0],
                    mFramebufferResolution[1],
                    DepthTexture,
                    true
                );
                break;
            case Engine::Normals:
                generateTexture(
                    index,
                    mFramebufferResolution[0],
                    mFramebufferResolution[1],
                    NormalTexture,
                    true
                );
                break;
            case Engine::Positions:
                generateTexture(
                    index,
                    mFramebufferResolution[0],
                    mFramebufferResolution[1],
                    PositionTexture,
                    true
                );
                break;            
            default:
                break;
        }
    }

    return mFrameBufferTextures[index];
}

/*!
    Set the visibility state of this window. If a window is hidden the rendering for that window will be paused unless it's forced to render while hidden by using setRenderWhileHidden().
*/
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

/*!
    Set if window should render while hidden. Normally a window pauses the rendering if it's hidden.
*/
void SGCTWindow::setRenderWhileHidden(bool state) {
    mRenderWhileHidden = state;
}

/*!
    Set the focued flag for this window (should not be done by user)
*/
void SGCTWindow::setFocused(bool state) {
    mFocused = state;
    //MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "SGCTWindow %d: Focused=%d.\n", mId, mFocused);
}

/*!
    Set the inonified flag for this window (should not be done by user)
*/
void SGCTWindow::setIconified(bool state) {
    mIconified = state;
}

/*!
    Set the window title
    @param title The title of the window.
*/
void SGCTWindow::setWindowTitle(const char * title) {
    glfwSetWindowTitle(mWindowHandle, title);
}

/*!
    Sets the window resolution.

    @param x The width of the window in pixels.
    @param y The height of the window in pixels.
*/
void SGCTWindow::setWindowResolution(int x, int y) {
    // In case this callback gets triggered from elsewhere than sgct's glfwPollEvents,
    // we want to make sure the actual resizing is deferred to the end of the frame.
    // This can happen if some other library pulls events from the operating system
    // for example by calling nextEventMatchingMask (MacOS) or PeekMessageW (Windows).
    // If we were to set the actual mWindowRes directly, we may render half a frame with
    // resolution a and the other half with resolution b, which is undefined behaviour.
    // mHasNewPendingWindowRes is checked in SGCTWindow::updateResolution,
    // which is called from SGCTEngine's render loop after glfwPollEvents.

    mHasPendingWindowRes = true;
    mPendingWindowRes[0] = x;
    mPendingWindowRes[1] = y;
}

/*!
    Sets the framebuffer resolution. Theese parameters will only be used if a fixed resolution is used that is different from the window resolution.
    This might be useful in fullscreen mode on Apples retina displays to force 1080p resolution or similar.

    @param x The width of the frame buffer in pixels.
    @param y The height of the frame buffer in pixels.
*/
void SGCTWindow::setFramebufferResolution(int x, int y) {
    // Defer actual update of framebuffer resolution until next call to updateResolutions.
    // (Same reason as described for setWindowResolution above.)
    if (!mUseFixResolution) {
        mHasPendingFramebufferRes = true;
        mPendingFramebufferRes[0] = x;
        mPendingFramebufferRes[1] = y;
    }
}

/*!
    Swap previus data and current data. This is done at the end of the render loop.
*/
void SGCTWindow::swap(bool takeScreenshot) {
    if ((mVisible || mRenderWhileHidden) && mAllowCapture) {
        makeOpenGLContextCurrent( Window_Context );
        
        if (takeScreenshot) {
            if (SGCTSettings::instance()->getCaptureFromBackBuffer() && mDoubleBuffered) {
                if (mScreenCapture[0] != nullptr) {
                    mScreenCapture[0]->saveScreenCapture(
                        0,
                        mStereoMode == Active_Stereo ?
                            sgct_core::ScreenCapture::CAPTURE_LEFT_BACK_BUFFER :
                            sgct_core::ScreenCapture::CAPTURE_BACK_BUFFER
                    );
                }

                if (mScreenCapture[1] != nullptr && mStereoMode == Active_Stereo) {
                    mScreenCapture[0]->saveScreenCapture(
                        0,
                        sgct_core::ScreenCapture::CAPTURE_RIGHT_BACK_BUFFER
                    );
                }
            }
            else {
                if (mScreenCapture[0] != nullptr) {
                    mScreenCapture[0]->saveScreenCapture(
                        mFrameBufferTextures[Engine::LeftEye]
                    );
                }
                if (mScreenCapture[1] != nullptr && mStereoMode > No_Stereo &&
                    mStereoMode < SGCTWindow::Side_By_Side_Stereo)
                {
                    mScreenCapture[1]->saveScreenCapture(
                        mFrameBufferTextures[Engine::RightEye]
                    );
                }
            }
        }

        //swap
        mWindowResOld[0] = mWindowRes[0];
        mWindowResOld[1] = mWindowRes[1];

        mDoubleBuffered ? glfwSwapBuffers(mWindowHandle): glFinish();
    }
}

void SGCTWindow::updateResolutions() {
    if (mHasPendingWindowRes) {
        mWindowRes[0] = mPendingWindowRes[0];
        mWindowRes[1] = mPendingWindowRes[1];
        float newAspectRatio =
            static_cast<float>(mWindowRes[0]) / static_cast<float>(mWindowRes[1]);

        // Set field of view of each of this window's viewports to match new
        // aspect ratio, adjusting only the horizontal (x) values.
        for (std::size_t j = 0; j < getNumberOfViewports(); ++j) {
            sgct_core::Viewport* vpPtr = getViewport(j);
            vpPtr->updateFovToMatchAspectRatio(mAspectRatio, newAspectRatio);
            MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG,
                "SGCTWindow: update aspect ratio in viewport# %d (%f --> %f)...\n",
                j,
                mAspectRatio,
                newAspectRatio
            );
        }
        mAspectRatio = newAspectRatio;

        // Redraw window
        if (mWindowHandle) {
            glfwSetWindowSize(mWindowHandle, mWindowRes[0], mWindowRes[1]);
        }

        MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG,
            "SGCTWindow: Resolution changed to %dx%d in window %d...\n",
            mWindowRes[0],
            mWindowRes[1],
            mId
        );

        mHasPendingWindowRes = false;
    }
    if (mHasPendingFramebufferRes) {
        mFramebufferResolution[0] = mPendingFramebufferRes[0];
        mFramebufferResolution[1] = mPendingFramebufferRes[1];

        MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG,
            "SGCTWindow: Framebuffer resolution changed to %dx%d for window %d...\n",
            mFramebufferResolution[0],
            mFramebufferResolution[1],
            mId
        );

        mHasPendingFramebufferRes = false;
    }
}

void SGCTWindow::setHorizFieldOfView(float hFovDeg) {
    // Set field of view of each of this window's viewports to match new horiz/vert
    // aspect ratio, adjusting only the horizontal (x) values.
    for (size_t j = 0; j < getNumberOfViewports(); ++j) {
        sgct_core::Viewport* vpPtr = getViewport(j);
        vpPtr->setHorizontalFieldOfView(hFovDeg, mAspectRatio);
    }
    MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG,
        "SGCTWindow: Horizontal FOV changed to %f deg. in %d viewports for window %d "
        "using aspect ratio %f...\n",
        hFovDeg,
        getNumberOfViewports(),
        mId,
        mAspectRatio
    );
}

/*!
    Don't use this function if you want to set the window resolution. Use setWindowResolution(const int x, const int y) instead.
    This function is called within sgct when the window is created.
*/
void SGCTWindow::initWindowResolution(int x, int y) {
    mWindowRes[0] = x;
    mWindowRes[1] = y;
    mWindowResOld[0] = mWindowRes[0];
    mWindowResOld[1] = mWindowRes[1];

    mAspectRatio = static_cast<float>(x) / static_cast<float>(y);

    mIsWindowResSet = true;

    if (!mUseFixResolution) {
        mFramebufferResolution[0] = x;
        mFramebufferResolution[1] = y;
    }
}

/*
\returns true if frame buffer is resized and window is visible.
*/
bool SGCTWindow::update() {
    if (!mVisible || !isWindowResized()) {
        return false;
    }
    makeOpenGLContextCurrent(Window_Context);

    //resize FBOs
    resizeFBOs();

    //resize PBOs
    for (int i = 0; i < 2; i++) {
        sgct_core::ScreenCapture* sc = mScreenCapture[i];
        if (sc == nullptr) {
            continue;
        }
        int numberOfCaputeChannels = mAlpha ? 4 : 3;
        if (SGCTSettings::instance()->getCaptureFromBackBuffer()) {
            // capture from buffer supports only 8-bit per color component
            sc->setTextureTransferProperties(GL_UNSIGNED_BYTE, mPreferBGR);
            sc->initOrResize(
                getXResolution(),
                getYResolution(),
                numberOfCaputeChannels,
                1
            );
        }
        else  {
            //default: capture from texture (supports HDR)
            sc->setTextureTransferProperties(mColorDataType, mPreferBGR);
            sc->initOrResize(
                getXFramebufferResolution(),
                getYFramebufferResolution(),
                numberOfCaputeChannels,
                mBytesPerColor
            );
        }
    }

    //resize non linear projection buffers
    for (size_t i = 0; i < mViewports.size(); i++) {
        if (mViewports[i]->hasSubViewports()) {
            float w = static_cast<float>(mFramebufferResolution[0]) *
                                    mViewports[i]->getXSize();
            float h = static_cast<float>(mFramebufferResolution[1]) *
                                    mViewports[i]->getYSize();
            mViewports[i]->getNonLinearProjectionPtr()->update(w, h);
        }
    }

    return true;
}

/*!
    Set this window's OpenGL context or the shared context as current. This function keeps
    track of which context is in use and only set the context to current if it's not.
*/
void SGCTWindow::makeOpenGLContextCurrent(OGL_Context context) {
    if (context == Shared_Context && mCurrentContextOwner != mSharedHandle) {
        glfwMakeContextCurrent(mSharedHandle);
        mCurrentContextOwner = mSharedHandle;
    }
    else if (context == Window_Context && mCurrentContextOwner != mWindowHandle) {
        glfwMakeContextCurrent(mWindowHandle);
        mCurrentContextOwner = mWindowHandle;
    }
}

/*!
    Force a restore of the shared openGL context
*/
void SGCTWindow::restoreSharedContext() {
    glfwMakeContextCurrent(mSharedHandle);
}

/*!
    \returns true if this window is resized
*/
bool SGCTWindow::isWindowResized() const {
    return (mWindowRes[0] != mWindowResOld[0] || mWindowRes[1] != mWindowResOld[1]);
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

/*!
    \returns true if full screen rendering is enabled
*/
bool SGCTWindow::isFullScreen() const {
    return mFullScreen;
}

/*!
    \return true if window is floating/allways on top/topmost
*/
bool SGCTWindow::isFloating() const {
    return mFloating;
}

/*!
\return true if window is double-buffered
*/
bool SGCTWindow::isDoubleBuffered() const {
    return mDoubleBuffered;
}

/*!
    \returns if the window is visible or not
*/
bool SGCTWindow::isVisible() const {
    return mVisible;
}

/*!
    \returns true if the window is set to render while hidden
*/
bool SGCTWindow::isRenderingWhileHidden() const {
    return mRenderWhileHidden;
}

/*!
    \returns If the frame buffer has a fix resolution this function returns true.
*/
bool SGCTWindow::isFixResolution() const {
    return mUseFixResolution;
}

/*!
\returns If the window resolution was set in configuration file this function returns true.
*/
bool SGCTWindow::isWindowResolutionSet() const {
    return mIsWindowResSet;
}

/*!
    Returns true if any kind of stereo is enabled
*/
bool SGCTWindow::isStereo() const {
    return mStereoMode != No_Stereo;
}

/*!
    Set this window's position in screen coordinates
    \param x horisontal position in pixels
    \param y vertical position in pixels
*/
void SGCTWindow::setWindowPosition(int x, int y) {
    mWindowPos[0] = x;
    mWindowPos[1] = y;
    mSetWindowPos = true;
}

/*!
Set if fullscreen mode should be used
*/
void SGCTWindow::setWindowMode(bool fullscreen) {
    mFullScreen = fullscreen;
}

/*!
Set if the window should float (be on top / topmost)
*/
void SGCTWindow::setFloating(bool floating) {
    mFloating = floating;
}

/*!
Set if the window should be double buffered (can only be set before window is created)
*/
void SGCTWindow::setDoubleBuffered(bool doubleBuffered) {
    mDoubleBuffered = doubleBuffered;
}

/*!
Set if window borders should be visible
*/
void SGCTWindow::setWindowDecoration(bool state) {
    mDecorated = state;
}

/*!
Set which monitor that should be used for fullscreen mode
*/
void SGCTWindow::setFullScreenMonitorIndex(int index) {
    mMonitorIndex = index;
}

void SGCTWindow::setBarrier(bool state) {
//#ifdef __WITHSWAPBARRIERS__

    if (mUseSwapGroups && state != mBarrier) {
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_INFO,
            "SGCTWindow: Enabling Nvidia swap barrier...\n"
        );

#ifdef __WIN32__ //Windows uses wglew.h
        mBarrier = wglBindSwapBarrierNV(1, state ? 1 : 0) ? 1 : 0;
#else //Apple and Linux uses glext.h
    #ifndef __APPLE__
        mBarrier = glXBindSwapBarrierNV(disp, 1, state ? 1 : 0) ? 1 : 0;
    #endif
#endif
    }
//#endif
}

/*!
    Force the frame buffer to have a fixed size which may be different from the window size.
*/
void SGCTWindow::setFixResolution(bool state) {
    mUseFixResolution = state;
}

/*!
Set if post effects should be used.
*/
void SGCTWindow::setUsePostFX(bool state) {
    mUsePostFX = state;
    if (!state) {
        mUseFXAA = false;
    }
}

/*!
Set if FXAA should be used.
*/
void SGCTWindow::setUseFXAA(bool state) {
    mUseFXAA = state;
    if (mUseFXAA) {
        mUsePostFX = true;
    }
    else {
        mUsePostFX = !mPostFXPasses.empty();
    }
    MessageHandler::instance()->print(
        MessageHandler::NOTIFY_DEBUG,
        "FXAA status: %s for window %d\n", state ? "enabled" : "disabled", mId
    );
}

/*!
    Use quad buffer (hardware stereoscopic rendering).
    This function can only be used before the window is created.
    The quad buffer feature is only supported on professional CAD graphics cards such as
    Nvidia Quadro or AMD/ATI FireGL.
*/
void SGCTWindow::setUseQuadbuffer(bool state) {
    mUseQuadBuffer = state;
    if (mUseQuadBuffer) {
        glfwWindowHint(GLFW_STEREO, GL_TRUE);
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_INFO,
            "Window %d: Enabling quadbuffered rendering.\n", mId
        );
    }
}

/*!
Set if the specifed Draw2D function pointer should be called for this window.
*/
void SGCTWindow::setCallDraw2DFunction(bool state) {
    mCallDraw2DFunction = state;
    if (!mCallDraw2DFunction) {
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_INFO,
            "Window %d: Draw 2D function disabled for this window.\n", mId
        );
    }
}

/*!
Set if the specifed Draw3D function pointer should be called for this window.
*/
void SGCTWindow::setCallDraw3DFunction(bool state) {
    mCallDraw3DFunction = state;
    if (!mCallDraw3DFunction) {
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_INFO,
            "Window %d: Draw (3D) function disabled for this window.\n", mId
        );
    }
}

/*!
Set if the specifed Draw2D functin pointer should be called for this window.
*/
void SGCTWindow::setCopyPreviousWindowToCurrentWindow(bool state) {
    mCopyPreviousWindowToCurrentWindow = state;
    if (mCopyPreviousWindowToCurrentWindow) {
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_INFO,
            "Window %d: CopyPreviousWindowToCurrentWindow enabled for this window.\n", mId
        );
    }
}

/*!
    This function is used internally within sgct to open the window.

    /returns True if window was created successfully.
*/
bool SGCTWindow::openWindow(GLFWwindow* share, size_t lastWindowIdx) {
    glfwWindowHint(GLFW_DEPTH_BITS, 32);
    glfwWindowHint(GLFW_DECORATED, mDecorated ? GL_TRUE : GL_FALSE);

    int antiAliasingSamples = getNumberOfAASamples();
    if (antiAliasingSamples > 1 && !SGCTSettings::instance()->useFBO()) {
        //if multisample is used
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

    setUseQuadbuffer(mStereoMode == Active_Stereo);

    if (mFullScreen) {
        int count;
        GLFWmonitor** monitors = glfwGetMonitors(&count);

        if (SGCTSettings::instance()->getRefreshRateHint() > 0) {
            glfwWindowHint(
                GLFW_REFRESH_RATE,
                SGCTSettings::instance()->getRefreshRateHint()
            );
        }
        
        if (mMonitorIndex > 0 && mMonitorIndex < count) {
            mMonitor = monitors[mMonitorIndex];
        }
        else {
            mMonitor = glfwGetPrimaryMonitor();
            if (mMonitorIndex >= count) {
                MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO,
                    "SGCTWindow(%d): Invalid monitor index (%d). This computer has %d monitors.\n",
                    mId, mMonitorIndex, count);
            }
        }

        if (!mIsWindowResSet) {
            const GLFWvidmode* currentMode = glfwGetVideoMode(mMonitor);
            mWindowRes[0] = currentMode->width;
            mWindowRes[1] = currentMode->height;
        }
    }

    mWindowHandle = glfwCreateWindow(
        mWindowRes[0],
        mWindowRes[1],
        "SGCT",
        mMonitor,
        share
    );

    if (mWindowHandle != nullptr) {
        mSharedHandle = share != nullptr ? share : mWindowHandle;
        glfwMakeContextCurrent(mWindowHandle);

        /*
            Mac for example scales the window size != frame buffer size
        */
        int buffer_width, buffer_height;
        glfwGetFramebufferSize(mWindowHandle, &buffer_width, &buffer_height);

        mWindowInitialRes[0] = mWindowRes[0];
        mWindowInitialRes[1] = mWindowRes[1];
        mScale[0] = static_cast<float>(buffer_width) / static_cast<float>(mWindowRes[0]);
        mScale[1] = static_cast<float>(buffer_height) / static_cast<float>(mWindowRes[1]);
        if (!mUseFixResolution) {
            mFramebufferResolution[0] = buffer_width;
            mFramebufferResolution[1] = buffer_height;
        }
        
        /*
         Verified that sizes are set correctly
         */

        /*
            Swap inerval:
            -1 = adaptive sync
            0  = vertical sync off
            1  = wait for vertical sync
            2  = fix when using swapgroups in xp and running half the framerate
        */

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
        
        //clear directly otherwise junk will be displayed on some OSs (OS X Yosemite)
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwMakeContextCurrent(mSharedHandle);

        if (SGCTSettings::instance()->useFBO()) {
            mScreenCapture[0] = new sgct_core::ScreenCapture();

            if (mUseRightEyeTexture) {
                mScreenCapture[1] = new sgct_core::ScreenCapture();
            }
        }

        mFinalFBO_Ptr = new sgct_core::OffScreenBuffer();

        return true;
    }
    else {
        return false;
    }
}

/*!
Init Nvidia swap groups if they are supported by hardware. Supported hardware is Nvidia
Quadro graphics card + sync card or AMD/ATI FireGL graphics card + sync card.
*/
void SGCTWindow::initNvidiaSwapGroups() {    

#ifdef __WIN32__ //Windows uses wglew.h
    if (glfwExtensionSupported("WGL_NV_swap_group")) {
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_INFO,
            "SGCTWindow: Joining Nvidia swap group.\n"
        );

        hDC = wglGetCurrentDC();

        unsigned int maxBarrier = 0;
        unsigned int maxGroup = 0;
        wglQueryMaxSwapGroupsNV(hDC, &maxGroup, &maxBarrier);
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_INFO,
            "WGL_NV_swap_group extension is supported.\n\tMax number of groups: %d\n\t"
            "Max number of barriers: %d\n",
            maxGroup, maxBarrier
        );

        /*
        wglJoinSwapGroupNV adds <hDC> to the swap group specified by <group>.
        If <hDC> is already a member of a different group, it is
        implicitly removed from that group first. A swap group is specified as
        an integer value between 0 and the value returned in <maxGroups> by
        wglQueryMaxSwapGroupsNV. If <group> is zero, the hDC is unbound from its
        current group, if any. If <group> is larger than <maxGroups>,
        wglJoinSwapGroupNV fails.

        */
        if (wglJoinSwapGroupNV(hDC, 1)) {
            MessageHandler::instance()->print(
                MessageHandler::NOTIFY_INFO,
                "SGCTWindow: Joining swapgroup 1 [ok].\n"
            );
            mUseSwapGroups = true;
        }
        else {
            MessageHandler::instance()->print(
                MessageHandler::NOTIFY_INFO,
                "SGCTWindow: Joining swapgroup 1 [failed].\n"
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
            MessageHandler::NOTIFY_INFO,
            "SGCTWindow: Joining Nvidia swap group.\n"
        );

        hDC = glXGetCurrentDrawable();
        disp = glXGetCurrentDisplay();

        unsigned int maxBarrier = 0;
        unsigned int maxGroup = 0;
        glXQueryMaxSwapGroupsNV(disp, hDC, &maxGroup, &maxBarrier);
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_INFO,
            "GLX_NV_swap_group extension is supported.\n\tMax number of groups: %d\n\t"
            "Max number of barriers: %d\n",
            maxGroup, maxBarrier
        );

        if (glXJoinSwapGroupNV(disp, hDC, 1)) {
            MessageHandler::instance()->print(
                MessageHandler::NOTIFY_INFO,
                "SGCTWindow: Joining swapgroup 1 [ok].\n"
            );
            mUseSwapGroups = true;
        }
        else {
            MessageHandler::instance()->print(
                MessageHandler::NOTIFY_INFO,
                "SGCTWindow: Joining swapgroup 1 [failed].\n"
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

void SGCTWindow::windowResizeCallback(GLFWwindow* window, int width, int height) {
    sgct_core::SGCTNode* thisNode = sgct_core::ClusterManager::instance()->getThisNodePtr();

    if (thisNode != nullptr) {
        //find the correct window to update
        for (std::size_t i = 0; i < thisNode->getNumberOfWindows(); i++) {
            if (thisNode->getWindowPtr(i)->getWindowHandle() == window) {
                thisNode->getWindowPtr(i)->setWindowResolution(
                    std::max(width, 1),
                    std::max(height, 1)
                );
            }
        }
    }
}

void SGCTWindow::frameBufferResizeCallback(GLFWwindow * window, int width, int height) {
    sgct_core::SGCTNode* thisNode = sgct_core::ClusterManager::instance()->getThisNodePtr();
    
    if (thisNode != nullptr) {
        //find the correct window to update
        for (size_t i = 0; i < thisNode->getNumberOfWindows(); i++) {
            if (thisNode->getWindowPtr(i)->getWindowHandle() == window) {
                thisNode->getWindowPtr(i)->setFramebufferResolution(
                    std::max(width, 1),
                    std::max(height, 1)
                );
            }
        }
    }
}

void SGCTWindow::windowFocusCallback(GLFWwindow * window, int state) {
    sgct_core::SGCTNode* thisNode = sgct_core::ClusterManager::instance()->getThisNodePtr();

    if (thisNode != nullptr) {
        //find the correct window to update
        for (size_t i = 0; i < thisNode->getNumberOfWindows(); i++) {
            if (thisNode->getWindowPtr(i)->getWindowHandle() == window)
                thisNode->getWindowPtr(i)->setFocused(state == GL_TRUE);
        }
    }
}

void SGCTWindow::windowIconifyCallback(GLFWwindow * window, int state) {
    sgct_core::SGCTNode* thisNode = sgct_core::ClusterManager::instance()->getThisNodePtr();

    if (thisNode != nullptr) {
        //find the correct window to update
        for (size_t i = 0; i < thisNode->getNumberOfWindows(); i++) {
            if (thisNode->getWindowPtr(i)->getWindowHandle() == window)
                thisNode->getWindowPtr(i)->setIconified(state == GL_TRUE);
        }
    }
}

void SGCTWindow::initScreenCapture() {
    for (int i = 0; i < 2; i++) {
        if (mScreenCapture[i] == nullptr) {
            continue;
        }
        sgct_core::ScreenCapture& sc = *mScreenCapture[i];

        //init PBO in screen capture
        if (i == 0) {
            if (mUseRightEyeTexture) {
                sc.init(mId, sgct_core::ScreenCapture::STEREO_LEFT);
            }
            else {
                sc.init(mId, sgct_core::ScreenCapture::MONO);
            }
        }
        else {
            sc.init(mId, sgct_core::ScreenCapture::STEREO_RIGHT);
        }

        //a workaround for devices that are supporting pbos but not showing it, like OS X (Intel)
        if (Engine::instance()->isOGLPipelineFixed()) {
            sc.setUsePBO(
                // if supported then use them
                glfwExtensionSupported("GL_ARB_pixel_buffer_object") == GL_TRUE &&
                SGCTSettings::instance()->getUsePBO()
            ); 
        }
        else {
            //in modern openGL pbos must be supported
            sc.setUsePBO(SGCTSettings::instance()->getUsePBO());
        }

        int numberOfCaputeChannels = mAlpha ? 4 : 3;
        if (SGCTSettings::instance()->getCaptureFromBackBuffer()) {
            // capturefrom buffer supports only 8-bit per color component capture (unsigned byte)
            sc.setTextureTransferProperties(GL_UNSIGNED_BYTE, mPreferBGR);
            sc.initOrResize(getXResolution(), getYResolution(), numberOfCaputeChannels, 1);
        }
        else {
            //default: capture from texture (supports HDR)
            sc.setTextureTransferProperties(mColorDataType, mPreferBGR);
            sc.initOrResize(
                getXFramebufferResolution(),
                getYFramebufferResolution(),
                numberOfCaputeChannels,
                mBytesPerColor
            );
        }

        if (SGCTSettings::instance()->getCaptureFormat() != sgct_core::ScreenCapture::NOT_SET) {
            sc.setCaptureFormat(
                static_cast<sgct_core::ScreenCapture::CaptureFormat>(
                    SGCTSettings::instance()->getCaptureFormat()
                )
            );
        }

        if (!Engine::checkForOGLErrors()) {
            MessageHandler::instance()->print(
                MessageHandler::NOTIFY_ERROR,
                "SGCTWindow %d: OpenGL error occured in screen capture %d init!\n", mId, i
            );
        }
    }
}

void SGCTWindow::getSwapGroupFrameNumber(unsigned int& frameNumber) {
    frameNumber = 0;

//#ifdef __WITHSWAPBARRIERS__

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
//#endif
}

void SGCTWindow::resetSwapGroupFrameNumber() {
//#ifdef __WITHSWAPBARRIERS__
    if (mBarrier) {
#ifdef __WIN32__
        if (glfwExtensionSupported("WGL_NV_swap_group") && wglResetFrameCountNV(hDC))
#else
    #ifdef __APPLE__
        if (false)
    #else //linux
        if (glfwExtensionSupported("GLX_NV_swap_group") && glXResetFrameCountNV(disp,hDC))
    #endif
#endif
        {
            mSwapGroupMaster = true;
            MessageHandler::instance()->print(
                MessageHandler::NOTIFY_INFO,
                "Resetting frame counter. This computer is the master.\n"
            );
        }
        else {
            mSwapGroupMaster = false;
            MessageHandler::instance()->print(
                MessageHandler::NOTIFY_INFO,
                "Resetting frame counter failed. This computer is the slave.\n"
            );
        }
    }

//#endif
}

/*!
    This function creates textures that will act as FBO targets.
*/
void SGCTWindow::createTextures() {
    //no target textures needed if not using FBO
    if (!SGCTSettings::instance()->useFBO()) {
        return;
    }

    if (Engine::instance()->getRunMode() <= Engine::OpenGL_Compablity_Profile) {
        glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT);
        glEnable(GL_TEXTURE_2D);
    }

    GLint maxTexSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
    if (mFramebufferResolution[0] > maxTexSize || mFramebufferResolution[1] > maxTexSize) {
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_ERROR,
            "SGCTWindow %d: Requested framebuffer is to big (Max: %dx%d)!\n",
            mId, maxTexSize, maxTexSize
        );
        return;
    }

    /*
        Create left and right color & depth textures.
    */
    //don't allocate the right eye image if stereo is not used
    //create a postFX texture for effects
    for (int i = 0; i < NUMBER_OF_TEXTURES; i++) {
        switch (i) {
            case Engine::RightEye:
                if (mUseRightEyeTexture) {
                    generateTexture(
                        i,
                        mFramebufferResolution[0],
                        mFramebufferResolution[1],
                        ColorTexture,
                        true
                    );
                }
                break;
            case Engine::Depth:
                if (SGCTSettings::instance()->useDepthTexture()) {
                    generateTexture(
                        i,
                        mFramebufferResolution[0],
                        mFramebufferResolution[1],
                        DepthTexture,
                        true
                    );
                }
                break;
            case Engine::FX1:
                if (!mPostFXPasses.empty()) {
                    generateTexture(
                        i,
                        mFramebufferResolution[0],
                        mFramebufferResolution[1],
                        ColorTexture,
                        true
                    );
                }
                break;
            case Engine::FX2:
                if (mPostFXPasses.size() > 1) {
                    generateTexture(
                        i,
                        mFramebufferResolution[0],
                        mFramebufferResolution[1],
                        ColorTexture,
                        true
                    );
                }
            case Engine::Intermediate:
                if (mUsePostFX) {
                    generateTexture(
                        i,
                        mFramebufferResolution[0],
                        mFramebufferResolution[1],
                        ColorTexture,
                        true
                    );
                }
                break;
            case Engine::Normals:
                if (SGCTSettings::instance()->useNormalTexture()) {
                    generateTexture(
                        i,
                        mFramebufferResolution[0],
                        mFramebufferResolution[1],
                        NormalTexture,
                        true
                    );
                }
                break;
            case Engine::Positions:
                if (SGCTSettings::instance()->usePositionTexture()) {
                    generateTexture(
                        i,
                        mFramebufferResolution[0],
                        mFramebufferResolution[1],
                        PositionTexture,
                        true
                    );
                }
                break;
            default:
                generateTexture(
                    i,
                    mFramebufferResolution[0],
                    mFramebufferResolution[1],
                    ColorTexture,
                    true
                );
                break;
        }
    }

    if (Engine::instance()->getRunMode() <= Engine::OpenGL_Compablity_Profile) {
        glPopAttrib();
    }

    if (Engine::checkForOGLErrors()) {
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_DEBUG,
            "Texture targets initiated successfully for window %d!\n",
            mId
        );
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_ERROR,
            "Texture targets failed to initialize for window %d!\n",
            mId
        );
    }
}

void SGCTWindow::generateTexture(unsigned int id, int xSize, int ySize,
                                 SGCTWindow::TextureType type, bool interpolate)
{
    //clean up if needed
    if (mFrameBufferTextures[id] != 0) {
        glDeleteTextures(1, &mFrameBufferTextures[id]);
        mFrameBufferTextures[id] = 0;
    }

    glGenTextures(1, &mFrameBufferTextures[id]);
    glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[id]);
    
    //---------------------
    // Disable mipmaps
    //---------------------
    if (Engine::instance()->isOGLPipelineFixed()) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    }

    if (type == DepthTexture) {
        if (Engine::instance()->isOGLPipelineFixed() ||
            SGCTSettings::instance()->getForceGlTexImage2D())
        {
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_DEPTH_COMPONENT32,
                xSize,
                ySize,
                0,
                GL_DEPTH_COMPONENT,
                GL_FLOAT,
                nullptr
            );
        }
        else {
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32, xSize, ySize);
        }
        
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_DEBUG,
            "%dx%d depth texture (id: %d, type %d) generated for window %d!\n",
            xSize, ySize, mFrameBufferTextures[id], id, mId
        );
    }
    else if (type == NormalTexture) {
        if (Engine::instance()->isOGLPipelineFixed() ||
            SGCTSettings::instance()->getForceGlTexImage2D())
        {
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                SGCTSettings::instance()->getBufferFloatPrecisionAsGLint(),
                xSize,
                ySize,
                0,
                GL_RGB,
                GL_FLOAT,
                nullptr
            );
        }
        else {
            glTexStorage2D(
                GL_TEXTURE_2D,
                1,
                SGCTSettings::instance()->getBufferFloatPrecisionAsGLint(),
                xSize,
                ySize
            );
        }
        
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_DEBUG,
            "%dx%d normal texture (id: %d, type %d) generated for window %d!\n",
            xSize, ySize, mFrameBufferTextures[id], id, mId
        );
    }
    else if (type == PositionTexture) {
        if (Engine::instance()->isOGLPipelineFixed() ||
            SGCTSettings::instance()->getForceGlTexImage2D())
        {
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                SGCTSettings::instance()->getBufferFloatPrecisionAsGLint(),
                xSize,
                ySize,
                0,
                GL_RGB,
                GL_FLOAT,
                nullptr
            );
        }
        else {
            glTexStorage2D(
                GL_TEXTURE_2D,
                1,
                SGCTSettings::instance()->getBufferFloatPrecisionAsGLint(),
                xSize,
                ySize
            );
        }
        
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_DEBUG,
            "%dx%d position texture (id: %d, type %d) generated for window %d!\n",
            xSize, ySize, mFrameBufferTextures[id], id, mId
        );
    }
    else {
        if (Engine::instance()->isOGLPipelineFixed() ||
            SGCTSettings::instance()->getForceGlTexImage2D())
        {
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                mInternalColorFormat,
                xSize,
                ySize,
                0,
                mColorFormat,
                mColorDataType,
                nullptr
            );
        }
        else {
            glTexStorage2D(GL_TEXTURE_2D, 1, mInternalColorFormat, xSize, ySize);
        }
        
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_DEBUG,
            "%dx%d BGRA texture (id: %d, type %d) generated for window %d!\n",
            xSize, ySize, mFrameBufferTextures[id], id, mId
        );
    }
    
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        interpolate ? GL_LINEAR : GL_NEAREST
    );
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        interpolate ? GL_LINEAR : GL_NEAREST
    );
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

/*!
    This function creates FBOs if they are supported.
    This is done in the initOGL function.
*/
void SGCTWindow::createFBOs() {
    if (!SGCTSettings::instance()->useFBO()) {
        //disable anaglyph & checkerboard stereo if FBOs are not used
        if (mStereoMode > Active_Stereo) {
            mStereoMode = No_Stereo;
        }
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_WARNING,
            "Warning! FBO rendering is not supported or enabled!\nPostFX, fisheye and "
            "some stereo modes are disabled.\n"
        );
    }
    else {
        mFinalFBO_Ptr->setInternalColorFormat(mInternalColorFormat);
        mFinalFBO_Ptr->createFBO(
            mFramebufferResolution[0],
            mFramebufferResolution[1],
            mNumberOfAASamples
        );
            
        if (mFinalFBO_Ptr->checkForErrors()) {
            MessageHandler::instance()->print(
                MessageHandler::NOTIFY_DEBUG,
                "Window %d: FBO initiated successfully. Number of samples: %d\n",
                mId, mFinalFBO_Ptr->isMultiSampled() ? mNumberOfAASamples : 1
            );
        }
        else {
            MessageHandler::instance()->print(
                MessageHandler::NOTIFY_ERROR,
                "Window %d: FBO initiated with errors! Number of samples: %d\n",
                mId, mFinalFBO_Ptr->isMultiSampled() ? mNumberOfAASamples : 1
            );
        }
    }
}

/*!
    Create vertex buffer objects used to render framebuffer quad
*/
void SGCTWindow::createVBOs() {
    if (!Engine::instance()->isOGLPipelineFixed()) {
        glGenVertexArrays(1, &mVAO);
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_DEBUG,
            "SGCTWindow: Generating VAO: %d\n", mVAO
        );
    }

    glGenBuffers(1, &mVBO);
    MessageHandler::instance()->print(
        MessageHandler::NOTIFY_DEBUG,
        "SGCTWindow: Generating VBO: %d\n", mVBO
    );

    if (!Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(mVAO);
    }
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    //2TF + 3VF = 2*4 + 3*4 = 20
    glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), mQuadVerts, GL_STATIC_DRAW);
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

    //unbind
    if (!Engine::instance()->isOGLPipelineFixed()) {
        glBindVertexArray(0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SGCTWindow::loadShaders() {
    //load shaders
    if (mStereoMode > Active_Stereo && mStereoMode < Side_By_Side_Stereo) {
        std::string stereo_frag_shader;
        std::string stereo_vert_shader;
        
        //reload shader program if it exists
        if (mStereoShader.isLinked()) {
            mStereoShader.deleteProgram();
        }

        stereo_vert_shader = Engine::instance()->isOGLPipelineFixed() ?
            sgct_core::shaders::Anaglyph_Vert_Shader :
            sgct_core::shaders_modern::Anaglyph_Vert_Shader;

        if (mStereoMode == Anaglyph_Red_Cyan_Stereo) {
            stereo_frag_shader = Engine::instance()->isOGLPipelineFixed() ?
                sgct_core::shaders::Anaglyph_Red_Cyan_Stereo_Frag_Shader :
                sgct_core::shaders_modern::Anaglyph_Red_Cyan_Stereo_Frag_Shader;
        }
        else if (mStereoMode == Anaglyph_Amber_Blue_Stereo) {
            stereo_frag_shader = Engine::instance()->isOGLPipelineFixed() ?
                sgct_core::shaders::Anaglyph_Amber_Blue_Stereo_Frag_Shader :
                sgct_core::shaders_modern::Anaglyph_Amber_Blue_Stereo_Frag_Shader;
        }
        else if (mStereoMode == Anaglyph_Red_Cyan_Wimmer_Stereo) {
            stereo_frag_shader = Engine::instance()->isOGLPipelineFixed() ?
                sgct_core::shaders::Anaglyph_Red_Cyan_Stereo_Frag_Shader_Wimmer :
                sgct_core::shaders_modern::Anaglyph_Red_Cyan_Stereo_Frag_Shader_Wimmer;
        }
        else if (mStereoMode == Checkerboard_Stereo) {
            stereo_frag_shader = Engine::instance()->isOGLPipelineFixed() ?
                sgct_core::shaders::CheckerBoard_Frag_Shader :
                sgct_core::shaders_modern::CheckerBoard_Frag_Shader;
        }
        else if (mStereoMode == Checkerboard_Inverted_Stereo) {
            stereo_frag_shader = Engine::instance()->isOGLPipelineFixed() ?
                sgct_core::shaders::CheckerBoard_Inverted_Frag_Shader :
                sgct_core::shaders_modern::CheckerBoard_Inverted_Frag_Shader;
        }
        else if (mStereoMode == Vertical_Interlaced_Stereo) {
            stereo_frag_shader = Engine::instance()->isOGLPipelineFixed() ?
                sgct_core::shaders::Vertical_Interlaced_Stereo_Frag_Shader :
                sgct_core::shaders_modern::Vertical_Interlaced_Stereo_Frag_Shader;
        }
        else if (mStereoMode == Vertical_Interlaced_Inverted_Stereo) {
            stereo_frag_shader = Engine::instance()->isOGLPipelineFixed() ?
                sgct_core::shaders::Vertical_Interlaced_Inverted_Stereo_Frag_Shader :
                sgct_core::shaders_modern::Vertical_Interlaced_Inverted_Stereo_Frag_Shader;
        }
        else {
            stereo_frag_shader = Engine::instance()->isOGLPipelineFixed() ?
                sgct_core::shaders::Dummy_Stereo_Frag_Shader :
                sgct_core::shaders_modern::Dummy_Stereo_Frag_Shader;
        }

        //replace glsl version
        sgct_helpers::findAndReplace(
            stereo_frag_shader,
            "**glsl_version**",
            Engine::instance()->getGLSLVersion()
        );
        sgct_helpers::findAndReplace(
            stereo_vert_shader,
            "**glsl_version**",
            Engine::instance()->getGLSLVersion()
        );

        bool vertShader = mStereoShader.addShaderSrc(
            stereo_vert_shader,
            GL_VERTEX_SHADER,
            ShaderProgram::SHADER_SRC_STRING
        );
        if (!vertShader) {
            MessageHandler::instance()->print(
                MessageHandler::NOTIFY_ERROR,
                "Failed to load stereo vertex shader\n"
            );
        }
        bool fragShader = mStereoShader.addShaderSrc(
            stereo_frag_shader,
            GL_FRAGMENT_SHADER,
            ShaderProgram::SHADER_SRC_STRING
        );
        if (!fragShader) {
            MessageHandler::instance()->print(
                MessageHandler::NOTIFY_ERROR,
                "Failed to load stereo fragment shader\n"
            );
        }

        mStereoShader.setName("StereoShader");
        mStereoShader.createAndLinkProgram();
        mStereoShader.bind();
        if (!Engine::instance()->isOGLPipelineFixed()) {
            StereoMVP = mStereoShader.getUniformLocation("MVP");
        }
        StereoLeftTex = mStereoShader.getUniformLocation("LeftTex");
        StereoRightTex = mStereoShader.getUniformLocation("RightTex");
        glUniform1i(StereoLeftTex, 0);
        glUniform1i(StereoRightTex, 1);
        ShaderProgram::unbind();

        if (!Engine::checkForOGLErrors()) {
            MessageHandler::instance()->print(
                MessageHandler::NOTIFY_ERROR,
                "SGCTWindow %d: OpenGL error occured while loading shaders!\n", mId
            );
        }
    }
}

void SGCTWindow::bindVAO() const {
    glBindVertexArray(mVAO);
}

void SGCTWindow::bindVBO() const {
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
}

void SGCTWindow::unbindVBO() const {
    glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);
}

void SGCTWindow::unbindVAO() const {
    glBindVertexArray( GL_FALSE );
}

/*!
    Returns pointer to FBO container
*/
sgct_core::OffScreenBuffer* SGCTWindow::getFBOPtr() const {
    return mFinalFBO_Ptr;
}

/*!
    \returns pointer to GLFW monitor
*/
GLFWmonitor* SGCTWindow::getMonitor() const {
    return mMonitor;
}

/*!
    \returns pointer to GLFW window
*/
GLFWwindow* SGCTWindow::getWindowHandle() const {
    return mWindowHandle;
}

/*!
    Get the dimensions of the final FBO. Regular viewport rendering renders directly to this FBO but a fisheye renders first a cubemap and then to the final FBO.
    Post effects are rendered using these dimensions.
*/
void SGCTWindow::getFinalFBODimensions(int& width, int& height) const {
    width = mFramebufferResolution[0];
    height = mFramebufferResolution[1];
}

/*!
    Add a post effect for this window
*/
void SGCTWindow::addPostFX(PostFX& fx) {
    mPostFXPasses.push_back(fx);
}

/*!
    This function resizes the FBOs when the window is resized to achive 1:1 pixel-texel mapping.
*/
void SGCTWindow::resizeFBOs() {
    if (!mUseFixResolution && SGCTSettings::instance()->useFBO()) {
        makeOpenGLContextCurrent(Shared_Context);
        for (unsigned int i = 0; i < NUMBER_OF_TEXTURES; i++) {
            if (mFrameBufferTextures[i] != 0) {
                glDeleteTextures(1, &mFrameBufferTextures[i]);
                mFrameBufferTextures[i] = GL_FALSE;
            }
        }
        createTextures();

        mFinalFBO_Ptr->resizeFBO(
            mFramebufferResolution[0],
            mFramebufferResolution[1],
            mNumberOfAASamples
        );
        
        if (!mFinalFBO_Ptr->isMultiSampled()) {
            //attatch color buffer to prevent GL errors
            mFinalFBO_Ptr->bind();
            mFinalFBO_Ptr->attachColorTexture(mFrameBufferTextures[Engine::LeftEye]);
            mFinalFBO_Ptr->unBind();
        }

        if (mFinalFBO_Ptr->checkForErrors()) {
            MessageHandler::instance()->print(
                MessageHandler::NOTIFY_DEBUG,
                "Window %d: FBOs resized successfully.\n", mId
            );
        }
        else {
            MessageHandler::instance()->print(
                MessageHandler::NOTIFY_ERROR,
                "Window %d: FBOs resized with GL errors!\n", mId
            );
        }
    }
}

/*!
    Returns the stereo mode. The value can be compared to the sgct_core::ClusterManager::StereoMode enum
*/
SGCTWindow::StereoMode SGCTWindow::getStereoMode() const {
    return mStereoMode;
}

void SGCTWindow::addViewport(float left, float right, float bottom, float top) {
    sgct_core::Viewport* vpPtr = new sgct_core::Viewport(left, right, bottom, top);
    mViewports.push_back(vpPtr);
    MessageHandler::instance()->print(
        MessageHandler::NOTIFY_DEBUG,
        "Adding viewport (total %d)\n", mViewports.size()
    );
}

void SGCTWindow::addViewport(sgct_core::Viewport* vpPtr) {
    mViewports.push_back(vpPtr);
    MessageHandler::instance()->print(
        MessageHandler::NOTIFY_DEBUG,
        "Adding viewport (total %d)\n", mViewports.size()
    );
}

/*!
    Clears the vector containing all viewport data.
*/
void SGCTWindow::deleteAllViewports() {
    mCurrentViewport = nullptr;

    for (unsigned int i = 0; i < mViewports.size(); i++) {
        delete mViewports[i];
    }
    mViewports.clear();
}

/*!
\returns a pointer to the viewport that is beeing rendered to at the moment
*/
sgct_core::BaseViewport* SGCTWindow::getCurrentViewport() const {
    if (mCurrentViewport == nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::NOTIFY_ERROR,
            "Window %d error: Current viewport is nullptr!\n", mId
        );
    }
    return mCurrentViewport;
}

/*!
\returns a pointer to a specific viewport
*/
sgct_core::Viewport* SGCTWindow::getViewport(size_t index) const {
    return mViewports[index];
}

/*!
Get the current viewport data in pixels.
*/
void SGCTWindow::getCurrentViewportPixelCoords(int& x, int& y, int& xSize,
                                               int& ySize) const
{
    x = static_cast<int>(getCurrentViewport()->getX() *
        static_cast<float>(mFramebufferResolution[0]));
    y = static_cast<int>(getCurrentViewport()->getY() *
        static_cast<float>(mFramebufferResolution[1]));
    xSize = static_cast<int>(getCurrentViewport()->getXSize() *
        static_cast<float>(mFramebufferResolution[0]));
    ySize = static_cast<int>(getCurrentViewport()->getYSize() *
        static_cast<float>(mFramebufferResolution[1]));
}

/*!
\returns the viewport count for this window
*/
size_t SGCTWindow::getNumberOfViewports() const {
    return mViewports.size();
}

/*!
    Set the number of samples used in multisampled anti-aliasing
*/
void SGCTWindow::setNumberOfAASamples(int samples) {
    mNumberOfAASamples = samples;
}

/*!
    \returns the number of samples used in multisampled anti-aliasing
*/
int SGCTWindow::getNumberOfAASamples() const {
    return mNumberOfAASamples;
}

/*!
    Set the stereo mode. Set this mode in your init callback or during runtime in the post-sync-pre-draw callback.
    GLSL shaders will be recompliled if needed.
*/
void SGCTWindow::setStereoMode(StereoMode sm) {
    mStereoMode = sm;

    MessageHandler::instance()->print(
        MessageHandler::NOTIFY_DEBUG,
        "SGCTWindow: Setting stereo mode to '%s' for window %d.\n",
        getStereoModeStr().c_str(), mId
    );

    mUseRightEyeTexture = mStereoMode != No_Stereo && mStereoMode < Side_By_Side_Stereo;

    if (mWindowHandle) {
        loadShaders();
    }
}

/*!
    This function returns the screen capture pointer if it's set otherwise nullptr.

    @param eye can either be 0 (left) or 1 (right)
    
    Returns pointer to screen capture ptr
*/
sgct_core::ScreenCapture* SGCTWindow::getScreenCapturePointer(unsigned int eye) const {
    return eye < 2 ? mScreenCapture[eye] : nullptr;
}

/*!
    Set the which viewport that is the current. This is done from the Engine and end users shouldn't change this
*/
void SGCTWindow::setCurrentViewport(size_t index) {
    mCurrentViewport = mViewports[index];
}

/*!
Set the which viewport that is the current. This is done internally from SGCT and end users shouldn't change this
*/
void SGCTWindow::setCurrentViewport(sgct_core::BaseViewport* vp) {
    mCurrentViewport = vp;
}

std::string SGCTWindow::getStereoModeStr() const {
    switch (mStereoMode) {
        case Active_Stereo:
            return "active";
        case Anaglyph_Red_Cyan_Stereo:
            return "anaglyph_red_cyan";
        case Anaglyph_Amber_Blue_Stereo:
            return "anaglyph_amber_blue";
        case Anaglyph_Red_Cyan_Wimmer_Stereo:
            return "anaglyph_wimmer";
        case Checkerboard_Stereo:
            return "checkerboard";
        case Checkerboard_Inverted_Stereo:
            return "checkerboard_inverted";
        case Vertical_Interlaced_Stereo:
            return "vertical_interlaced";
        case Vertical_Interlaced_Inverted_Stereo:
            return "vertical_interlaced_inverted";
        case Dummy_Stereo:
            return "dummy";
        case Side_By_Side_Stereo:
            return "side_by_side";
        case Side_By_Side_Inverted_Stereo:
            return "side_by_side_inverted";
        case Top_Bottom_Stereo:
            return "top_bottom";
        case Top_Bottom_Inverted_Stereo:
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
    unsigned short red[256], green[256], blue[256];
    
    ramp.size = 256;
    ramp.red = red;
    ramp.green = green;
    ramp.blue = blue;

    float gamma_exp = 1.0f / mGamma;

    for (unsigned int i = 0; i < ramp.size; i++) {
        float c = ((static_cast<float>(i) / 255.f) - 0.5f) * mContrast + 0.5f;
        float b = c + (mBrightness - 1.f);
        float g = powf(b, gamma_exp);

        //transform back
        //unsigned short t = static_cast<unsigned short>(roundf(256.0f * g));

        unsigned short t = static_cast<unsigned short>(
            std::max(0.0f, std::min(65535.f * g, 65535.f)) //clamp to range
            + 0.5f
            ); //round to closest integer

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
        case BufferColorBitDepth8:
            mInternalColorFormat = GL_RGBA8;
            mColorDataType = GL_UNSIGNED_BYTE;
            mBytesPerColor = 1;
            break;
        case BufferColorBitDepth16:
            mInternalColorFormat = GL_RGBA16;
            mColorDataType = GL_UNSIGNED_SHORT;
            mBytesPerColor = 2;
            break;
        case BufferColorBitDepth16Float:
            mInternalColorFormat = GL_RGBA16F;
            mColorDataType = GL_HALF_FLOAT;
            mBytesPerColor = 2;
            break;
        case BufferColorBitDepth32Float:
            mInternalColorFormat = GL_RGBA32F;
            mColorDataType = GL_FLOAT;
            mBytesPerColor = 4;
            break;
        case BufferColorBitDepth16Int:
            mInternalColorFormat = GL_RGBA16I;
            mColorDataType = GL_SHORT;
            mBytesPerColor = 2;
            break;
        case BufferColorBitDepth32Int:
            mInternalColorFormat = GL_RGBA32I;
            mColorDataType = GL_INT;
            mBytesPerColor = 2;
            break;
        case BufferColorBitDepth16UInt:
            mInternalColorFormat = GL_RGBA16UI;
            mColorDataType = GL_UNSIGNED_SHORT;
            mBytesPerColor = 2;
            break;
        case BufferColorBitDepth32UInt:
            mInternalColorFormat = GL_RGBA32UI;
            mColorDataType = GL_UNSIGNED_INT;
            mBytesPerColor = 4;
            break;
    }
}

/*!
Set if fisheye alpha state. Should only be set using XML config of before calling
Engine::init.
*/
void SGCTWindow::setAlpha(bool state) {
    mAlpha = state;
}

/*!
Enable alpha clear color and 4-component screenshots
*/
bool SGCTWindow::getAlpha() const {
    return mAlpha;
}

/*!
Set monitor gamma (works only if fullscreen)
*/
void SGCTWindow::setGamma(float gamma) {
    mGamma = gamma;
    updateTransferCurve();
}

/*!
Get monitor gamma value (works only if fullscreen)
*/
float SGCTWindow::getGamma() const {
    return mGamma;
}

/*!
Set monitor contrast in range [0.5, 1.5] (works only if fullscreen)
*/
void SGCTWindow::setContrast(float contrast) {
    mContrast = contrast;
    updateTransferCurve();
}

/*!
Get monitor contrast value (works only if fullscreen)
*/
float SGCTWindow::getContrast() const {
    return mContrast;
}

/*!
Set monitor brightness in range [0.5, 1.5] (works only if fullscreen)
*/
void SGCTWindow::setBrightness(float brightness) {
    mBrightness = brightness;
    updateTransferCurve();
}

/*!
Set the color bit depth of the FBO and Screencapture.
*/
void SGCTWindow::setColorBitDepth(ColorBitDepth cbd) {
    mBufferColorBitDepth = cbd;
}

/*!
Get the color bit depth of the FBO and Screencapture.
*/
SGCTWindow::ColorBitDepth SGCTWindow::getColorBitDepth() const {
    return mBufferColorBitDepth;
}

/*!
Set if BGR(A) or RGB(A) rendering should be used. Default is BGR(A), which is usually the native order on GPU hardware.
This setting affects the screencapture which will return the prefered color order.
*/
void SGCTWindow::setPreferBGR(bool state) {
    mPreferBGR = state;
}

/*!
Set if screen capturing is allowed.
*/
void SGCTWindow::setAllowCapture(bool state) {
    mAllowCapture = state;
}

/*!
Get if buffer is rendered using BGR(A) or RGB(A).
*/
bool SGCTWindow::isBGRPrefered() const {
    return mPreferBGR;
}

/*!
Get if (screen) capturing is allowed.
*/
bool SGCTWindow::isCapturingAllowed() const {
    return mAllowCapture;
}

/*!
Get monitor brightness value (works only if fullscreen)
*/
float SGCTWindow::getBrightness() const {
    return mBrightness;
}

/*!
Get FOV of viewport[0]
*/
float SGCTWindow::getHorizFieldOfViewDegrees() {
    if (mViewports[0]) {
        mHorizontalFovDegrees = mViewports[0]->getHorizontalFieldOfViewDegrees();
    }
    return mHorizontalFovDegrees;
}

PostFX * SGCTWindow::getPostFXPtr(std::size_t index) {
    return &mPostFXPasses[index];
}

size_t SGCTWindow::getNumberOfPostFXs() const {
    return mPostFXPasses.size();
}

int SGCTWindow::getXResolution() const {
    return mWindowRes[0];
}

int SGCTWindow::getYResolution() const {
    return mWindowRes[1];
}

int SGCTWindow::getXFramebufferResolution() const {
    return mFramebufferResolution[0];
}

int SGCTWindow::getYFramebufferResolution() const {
    return mFramebufferResolution[1];
}

int SGCTWindow::getXInitialResolution() const {
    return mWindowInitialRes[0];
}

int SGCTWindow::getYInitialResolution() const {
    return mWindowInitialRes[1];
}

float SGCTWindow::getXScale() const {
    return mScale[0];
}

float SGCTWindow::getYScale() const {
    return mScale[1];
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
    mStereoShader.bind();
}

int SGCTWindow::getStereoShaderMVPLoc() const {
    return StereoMVP;
}

int SGCTWindow::getStereoShaderLeftTexLoc() const {
    return StereoLeftTex;
}

int SGCTWindow::getStereoShaderRightTexLoc() const {
    return StereoRightTex;
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
