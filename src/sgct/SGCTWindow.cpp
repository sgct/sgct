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

bool sgct::SGCTWindow::mUseSwapGroups = false;
bool sgct::SGCTWindow::mBarrier = false;
bool sgct::SGCTWindow::mSwapGroupMaster = false;
GLFWwindow * sgct::SGCTWindow::mCurrentContextOwner = NULL;
GLFWwindow * sgct::SGCTWindow::mSharedHandle = NULL;

sgct::SGCTWindow::SGCTWindow(int id)
{
    mId = id;
    mUseFixResolution = false;
    mUseQuadBuffer = false;
    mFullScreen = false;
    mFloating = false;
    mDoubleBuffered = true;
    mSetWindowPos = false;
    mDecorated = true;
    mAlpha = false;
    mVisible = true;
    mRenderWhileHidden = false;
    mAllowCapture = true;
    mUseFXAA = SGCTSettings::instance()->getDefaultFXAAState();
    mUsePostFX = false;
    mFocused = false;
    mIconified = false;
    mPreferBGR = true; //BGR is native on GPU

    mBufferColorBitDepth = BufferColorBitDepth8;
    
    mWindowHandle = NULL;

    mWindowRes[0] = 640;
    mWindowRes[1] = 480;
    mWindowResOld[0] = mWindowRes[0];
    mWindowResOld[1] = mWindowRes[1];
    mWindowInitialRes[0] = mWindowRes[0];
    mWindowInitialRes[1] = mWindowRes[1];
    mWindowPos[0] = 0;
    mWindowPos[1] = 0;
    mScale[0] = 0.0f;
    mScale[1] = 0.0f;
    mMonitorIndex = 0;
    mFramebufferResolution[0] = 512;
    mFramebufferResolution[1] = 256;
    mAspectRatio = 1.0f;
    mGamma = 1.0f;
    mContrast = 1.0f;
    mBrightness = 1.0f;

    StereoMVP        = -1;
    StereoLeftTex    = -1;
    StereoRightTex    = -1;

    //2 texels + 3 vertex
    mQuadVerts[0] = 0.0f;
    mQuadVerts[1] = 0.0f;
    mQuadVerts[2] = -1.0f;
    mQuadVerts[3] = -1.0f;
    mQuadVerts[4] = -1.0f;

    mQuadVerts[5] = 1.0f;
    mQuadVerts[6] = 0.0f;
    mQuadVerts[7] = 1.0f;
    mQuadVerts[8] = -1.0f;
    mQuadVerts[9] = -1.0f;

    mQuadVerts[10] = 0.0f;
    mQuadVerts[11] = 1.0f;
    mQuadVerts[12] = -1.0f;
    mQuadVerts[13] = 1.0f;
    mQuadVerts[14] = -1.0f;

    mQuadVerts[15] = 1.0f;
    mQuadVerts[16] = 1.0f;
    mQuadVerts[17] = 1.0f;
    mQuadVerts[18] = 1.0f;
    mQuadVerts[19] = -1.0f;

    mVBO    = GL_FALSE; //default to openGL false
    mVAO    = GL_FALSE; //default to openGL false

    mStereoMode = No_Stereo;
    mNumberOfAASamples = SGCTSettings::instance()->getDefaultNumberOfAASamples();

    //FBO targets init
    for(int i=0; i<NUMBER_OF_TEXTURES; i++)
        mFrameBufferTextures[i] = GL_FALSE;

    //pointers
    mMonitor = NULL;
    mWindowHandle = NULL;
    mSharedHandle = NULL;
    mFinalFBO_Ptr = NULL;
    mScreenCapture[0] = NULL;
    mScreenCapture[1] = NULL;

    mCurrentViewport = NULL;
    mHasAnyMasks = false;
    mUseRightEyeTexture = false;
}

/*!
Name this window
*/
void sgct::SGCTWindow::setName(const std::string & name)
{
    mName = name;
}

/*!
Tag this window
Tags are seperated by comma
*/
void sgct::SGCTWindow::setTags(const std::string & tags)
{
    std::stringstream ss(tags);
    while (ss.good())
    {
        std::string substr;
        getline(ss, substr, ',');
        mTags.push_back(substr);
    }
}

/*!
\returns the name of this window
*/
const std::string & sgct::SGCTWindow::getName() const
{
    return mName;
}

/*!
\returns the tags of this window
*/
const std::vector<std::string> & sgct::SGCTWindow::getTags() const
{
    return mTags;
}

/*!
\returns true if a specific tag exists
\tags are seperated by comma
*/
bool sgct::SGCTWindow::checkIfTagExists(std::string tag) const
{
    if (std::find(mTags.begin(), mTags.end(), tag) != mTags.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

/*!
    \returns this window's id
*/
const int & sgct::SGCTWindow::getId() const
{
    return mId;
}

/*!
    \returns this window's focused flag
*/
const bool & sgct::SGCTWindow::isFocused() const
{
    return mFocused;
}

/*!
    \returns this window's inconify flag 
*/
const bool & sgct::SGCTWindow::isIconified() const
{
    return mIconified;
}

void sgct::SGCTWindow::close()
{
    makeOpenGLContextCurrent( Shared_Context );

    //delete postFX
    for(std::size_t i=0; i<getNumberOfPostFXs(); i++)
        mPostFXPasses[i].destroy();
    mPostFXPasses.clear();

    MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Deleting screen capture data for window %d...\n", mId);
    for (int i = 0; i < 2; i++)
        if( mScreenCapture[i] )
        {
            delete mScreenCapture[i];
            mScreenCapture[i] = NULL;
        }

    //delete FBO stuff
    if(mFinalFBO_Ptr != NULL &&
        SGCTSettings::instance()->useFBO() )
    {
        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Releasing OpenGL buffers for window %d...\n", mId);
        mFinalFBO_Ptr->destroy();

        delete mFinalFBO_Ptr;
        mFinalFBO_Ptr = NULL;

        for(unsigned int i=0; i<NUMBER_OF_TEXTURES; i++)
        {
            if( mFrameBufferTextures[i] != GL_FALSE )
            {
                glDeleteTextures(1, &mFrameBufferTextures[i]);
                mFrameBufferTextures[i] = GL_FALSE;
            }
        }
    }

    if( mVBO )
    {
        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Deleting VBOs for window %d...\n", mId);
        glDeleteBuffers(1, &mVBO);
        mVBO = GL_FALSE;
    }

    if( mVAO )
    {
        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Deleting VAOs for window %d...\n", mId);
        glDeleteVertexArrays(1, &mVAO);
        mVAO = GL_FALSE;
    }

    //delete shaders
    mStereoShader.deleteProgram();

    /*
    Current handle must be set at the end to propely destroy the window
    */
    makeOpenGLContextCurrent( Window_Context );

    deleteAllViewports();

    if( mUseSwapGroups )
    {
//#ifdef __WITHSWAPBARRIERS__

#ifdef __WIN32__
        if (glfwExtensionSupported("WGL_NV_swap_group"))
        {
            //un-bind
            wglBindSwapBarrierNV(1,0);
            //un-join
            wglJoinSwapGroupNV(hDC,0);
        }
#else
    #ifndef __APPLE__
        if( glfwExtensionSupported("GLX_NV_swap_group") )
        {
            //un-bind
            glXBindSwapBarrierNV(disp,1,0);
            //un-join
            glXJoinSwapGroupNV(disp,hDC,0);
        }
    #endif
#endif

    }
}

void sgct::SGCTWindow::init()
{
    if(!mFullScreen)
    {
        if(mSetWindowPos)
            glfwSetWindowPos( mWindowHandle, mWindowPos[0], mWindowPos[1] );
        glfwSetWindowSizeCallback( mWindowHandle, windowResizeCallback );
        glfwSetFramebufferSizeCallback( mWindowHandle, frameBufferResizeCallback );
        glfwSetWindowFocusCallback( mWindowHandle, windowFocusCallback );
        glfwSetWindowIconifyCallback( mWindowHandle, windowIconifyCallback );
    }

    std::stringstream ss;
    ss << "SGCT node: ";
    ss << sgct_core::ClusterManager::instance()->getThisNodePtr()->getAddress() << " ";
    ss << "(" << (sgct_core::NetworkManager::instance()->isComputerServer() ? "master" : "slave") << ":" << mId << ")";
    
    mName.empty() ? setWindowTitle( ss.str().c_str() ) : setWindowTitle( mName.c_str() );

    //swap the buffers and update the window
    glfwSwapBuffers( mWindowHandle );

    //initNvidiaSwapGroups();
}

/*!
    Init window buffers such as textures, FBOs, VAOs, VBOs and PBOs.
*/
void sgct::SGCTWindow::initOGL()
{
    updateColorBufferData();
    
    createTextures();
    createVBOs(); //must be created before FBO
    createFBOs();
    initScreenCapture();
    loadShaders();

    for (std::size_t i = 0; i < mViewports.size(); i++)
        if (mViewports[i]->hasSubViewports())
        {
            setCurrentViewport(mViewports[i]);
            mViewports[i]->getNonLinearProjectionPtr()->setStereo(mStereoMode != No_Stereo);
            mViewports[i]->getNonLinearProjectionPtr()->setPreferedMonoFrustumMode(mViewports[i]->getEye());
            mViewports[i]->getNonLinearProjectionPtr()->init(mInternalColorFormat, mColorFormat, mColorDataType, mNumberOfAASamples);
            
            float viewPortWidth = static_cast<float>(mFramebufferResolution[0]) * mViewports[i]->getXSize();
            float viewPortHeight = static_cast<float>(mFramebufferResolution[1]) * mViewports[i]->getYSize();
            mViewports[i]->getNonLinearProjectionPtr()->update(viewPortWidth, viewPortHeight);
        }
}

/*!
    Init context specific data such as viewport corrections/warping meshes
*/
void sgct::SGCTWindow::initContextSpecificOGL()
{
    makeOpenGLContextCurrent(Window_Context);
    unsigned int numberOfMasks = 0;
    TextureManager::CompressionMode cm = TextureManager::instance()->getCompression();
    TextureManager::instance()->setCompression(TextureManager::No_Compression); //must be uncompressed otherwise arifacts will occur in gradients

    for (std::size_t j = 0; j < getNumberOfViewports(); j++)
    {
        sgct_core::Viewport * vpPtr = getViewport(j);
        vpPtr->loadData();
        if (vpPtr->hasBlendMaskTexture())
            numberOfMasks++;

        if (vpPtr->hasBlackLevelMaskTexture())
            numberOfMasks++;
    }

    //restore
    TextureManager::instance()->setCompression(cm);

    if (numberOfMasks > 0)
        mHasAnyMasks = true;
}

/*!
    Get a frame buffer texture. If the texture doesn't exists then it will be created.

    \param index Index or Engine::TextureIndexes enum
    \returns texture index of selected frame buffer texture
*/
unsigned int sgct::SGCTWindow::getFrameBufferTexture(unsigned int index)
{
    if(index < NUMBER_OF_TEXTURES)
    {
        if( mFrameBufferTextures[index] == GL_FALSE )
        {
            switch(index)
            {
            case Engine::LeftEye:
            case Engine::RightEye:
                generateTexture(index, mFramebufferResolution[0], mFramebufferResolution[1], ColorTexture, true);
                break;

            case Engine::Intermediate:
            case Engine::FX1:
            case Engine::FX2:
                generateTexture(index, mFramebufferResolution[0], mFramebufferResolution[1], ColorTexture, true);
                break;

            case Engine::Depth:
                generateTexture(index, mFramebufferResolution[0], mFramebufferResolution[1], DepthTexture, true);
                break;

            case Engine::Normals:
                generateTexture(index, mFramebufferResolution[0], mFramebufferResolution[1], NormalTexture, true);
                break;

            case Engine::Positions:
                generateTexture(index, mFramebufferResolution[0], mFramebufferResolution[1], PositionTexture, true);
                break;            

            default:
                break;
            }
        }

        return mFrameBufferTextures[index];
    }
    else
    {
        MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "SGCTWindow: Requested framebuffer texture index %d is out of bounds!\n", index);
        return GL_FALSE;
    }
}

/*!
    Set the visibility state of this window. If a window is hidden the rendering for that window will be paused unless it's forced to render while hidden by using setRenderWhileHidden().
*/
void sgct::SGCTWindow::setVisibility(bool state)
{
    if( state != mVisible )
    {
        if (mWindowHandle)
        {
            state ? glfwShowWindow(mWindowHandle) : glfwHideWindow(mWindowHandle);
        }
        mVisible = state;
    }
}

/*!
    Set if window should render while hidden. Normally a window pauses the rendering if it's hidden.
*/
void sgct::SGCTWindow::setRenderWhileHidden(bool state)
{
    mRenderWhileHidden = state;
}

/*!
    Set the focued flag for this window (should not be done by user)
*/
void sgct::SGCTWindow::setFocused(bool state)
{
    mFocused = state;
    //MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "SGCTWindow %d: Focused=%d.\n", mId, mFocused);
}

/*!
    Set the inonified flag for this window (should not be done by user)
*/
void sgct::SGCTWindow::setIconified(bool state)
{
    mIconified = state;
}

/*!
    Set the window title
    @param title The title of the window.
*/
void sgct::SGCTWindow::setWindowTitle(const char * title)
{
    glfwSetWindowTitle( mWindowHandle, title );
}

/*!
    Sets the window resolution.

    @param x The width of the window in pixels.
    @param y The height of the window in pixels.
*/
void sgct::SGCTWindow::setWindowResolution(const int x, const int y)
{
    mWindowRes[0] = x;
    mWindowRes[1] = y;
    mAspectRatio = static_cast<float>( x ) /
            static_cast<float>( y );

	//redraw window
	if (mWindowHandle)
		glfwSetWindowSize(mWindowHandle, x, y);

	MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "SGCTWindow: Resolution changed to %dx%d for window %d...\n", mWindowRes[0], mWindowRes[1], mId);
}

/*!
    Sets the framebuffer resolution. Theese parameters will only be used if a fixed resolution is used that is different from the window resolution.
    This might be useful in fullscreen mode on Apples retina displays to force 1080p resolution or similar.

    @param x The width of the frame buffer in pixels.
    @param y The height of the frame buffer in pixels.
*/
void sgct::SGCTWindow::setFramebufferResolution(const int x, const int y)
{
    if( !mUseFixResolution )
    {
        mFramebufferResolution[0] = x;
        mFramebufferResolution[1] = y;

        MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "SGCTWindow: Framebuffer resolution changed to %dx%d for window %d...\n", mFramebufferResolution[0], mFramebufferResolution[1], mId);
    }
}

/*!
    Swap previus data and current data. This is done at the end of the render loop.
*/
void sgct::SGCTWindow::swap(bool takeScreenshot)
{
    if ((mVisible || mRenderWhileHidden) && mAllowCapture)
    {
        makeOpenGLContextCurrent( Window_Context );
        
        if (takeScreenshot)
        {
            if (sgct::SGCTSettings::instance()->getCaptureFromBackBuffer() && mDoubleBuffered)
            {
                if (mScreenCapture[0] != NULL)
                {
                    mScreenCapture[0]->saveScreenCapture(0,
                        mStereoMode == Active_Stereo ? sgct_core::ScreenCapture::CAPTURE_LEFT_BACK_BUFFER : sgct_core::ScreenCapture::CAPTURE_BACK_BUFFER);
                }

                if (mScreenCapture[1] != NULL && mStereoMode == Active_Stereo)
                {
                    mScreenCapture[0]->saveScreenCapture(0, sgct_core::ScreenCapture::CAPTURE_RIGHT_BACK_BUFFER);
                }
            }
            else
            {
                if (mScreenCapture[0] != NULL)
                    mScreenCapture[0]->saveScreenCapture(mFrameBufferTextures[Engine::LeftEye]);
                if (mScreenCapture[1] != NULL && mStereoMode > No_Stereo && mStereoMode < sgct::SGCTWindow::Side_By_Side_Stereo)
                    mScreenCapture[1]->saveScreenCapture(mFrameBufferTextures[Engine::RightEye]);
            }
        }
        
        //swap
        mWindowResOld[0] = mWindowRes[0];
        mWindowResOld[1] = mWindowRes[1];

        mDoubleBuffered ? glfwSwapBuffers(mWindowHandle): glFinish();
    }
}

/*!
    Don't use this function if you want to set the window resolution. Use setWindowResolution(const int x, const int y) instead.
    This function is called within sgct when the window is created.
*/
void sgct::SGCTWindow::initWindowResolution(const int x, const int y)
{
    mWindowRes[0] = x;
    mWindowRes[1] = y;
    mWindowResOld[0] = mWindowRes[0];
    mWindowResOld[1] = mWindowRes[1];

    mAspectRatio = static_cast<float>( x ) /
            static_cast<float>( y );

    if( !mUseFixResolution )
    {
        mFramebufferResolution[0] = x;
        mFramebufferResolution[1] = y;
    }
}

/*
\returns true if frame buffer is resized and window is visible.
*/
bool sgct::SGCTWindow::update()
{
    if (mVisible && isWindowResized())
    {
        makeOpenGLContextCurrent(Window_Context);

        //resize FBOs
        resizeFBOs();

        //resize PBOs
        for (int i = 0; i < 2; i++)
            if (mScreenCapture[i] != NULL)
            {
                int numberOfCaputeChannels = mAlpha ? 4 : 3;
                if (sgct::SGCTSettings::instance()->getCaptureFromBackBuffer()) //capute from buffer supports only 8-bit per color component capture (unsigned byte)
                {
                    mScreenCapture[i]->setTextureTransferProperties(GL_UNSIGNED_BYTE, mPreferBGR);
                    mScreenCapture[i]->initOrResize(getXResolution(), getYResolution(), numberOfCaputeChannels, 1);
                }
                else //default: capture from texture (supports HDR)
                {
                    mScreenCapture[i]->setTextureTransferProperties(mColorDataType, mPreferBGR);
                    mScreenCapture[i]->initOrResize(getXFramebufferResolution(), getYFramebufferResolution(), numberOfCaputeChannels, mBytesPerColor);
                }
            }

        //resize non linear projection buffers
        for (std::size_t i = 0; i < mViewports.size(); i++)
            if (mViewports[i]->hasSubViewports())
            {
                float viewPortWidth = static_cast<float>(mFramebufferResolution[0]) * mViewports[i]->getXSize();
                float viewPortHeight = static_cast<float>(mFramebufferResolution[1]) * mViewports[i]->getYSize();
                mViewports[i]->getNonLinearProjectionPtr()->update(viewPortWidth, viewPortHeight);
            }

        return true;
    }
    else
        return false;
}

/*!
    Set this window's OpenGL context or the shared context as current. This function keeps track of which context is in use and only set the context to current if it's not.
*/
void sgct::SGCTWindow::makeOpenGLContextCurrent(OGL_Context context)
{
    if( context == Shared_Context && mCurrentContextOwner != mSharedHandle )
    {
        glfwMakeContextCurrent( mSharedHandle );
        mCurrentContextOwner = mSharedHandle;
    }
    else if( context == Window_Context && mCurrentContextOwner != mWindowHandle )
    {
        glfwMakeContextCurrent( mWindowHandle );
        mCurrentContextOwner = mWindowHandle;
    }
}

/*!
    Force a restore of the shared openGL context
*/
void sgct::SGCTWindow::restoreSharedContext()
{
    glfwMakeContextCurrent( mSharedHandle );
}

/*!
    \returns true if this window is resized
*/
bool sgct::SGCTWindow::isWindowResized() const
{
    return (mWindowRes[0] != mWindowResOld[0] || mWindowRes[1] != mWindowResOld[1]);
}

/*!
    \returns true if full screen rendering is enabled
*/
const bool & sgct::SGCTWindow::isFullScreen() const
{
    return mFullScreen;
}

/*!
    \return true if window is floating/allways on top/topmost
*/
const bool & sgct::SGCTWindow::isFloating() const
{
    return mFloating;
}

/*!
\return true if window is double-buffered
*/
const bool & sgct::SGCTWindow::isDoubleBuffered() const
{
    return mDoubleBuffered;
}

/*!
    \returns if the window is visible or not
*/
const bool & sgct::SGCTWindow::isVisible() const
{
    return mVisible;
}

/*!
    \returns true if the window is set to render while hidden
*/
const bool & sgct::SGCTWindow::isRenderingWhileHidden() const
{
    return mRenderWhileHidden;
}

/*!
    \returns If the frame buffer has a fix resolution this function returns true.
*/
const bool & sgct::SGCTWindow::isFixResolution() const
{
    return mUseFixResolution;
}

/*!
    Returns true if any kind of stereo is enabled
*/
bool sgct::SGCTWindow::isStereo() const
{
    return mStereoMode != No_Stereo;
}

/*!
    Set this window's position in screen coordinates
    \param x horisontal position in pixels
    \param y vertical position in pixels
*/
void sgct::SGCTWindow::setWindowPosition(const int x, const int y)
{
    mWindowPos[0] = x;
    mWindowPos[1] = y;
    mSetWindowPos = true;
}

/*!
Set if fullscreen mode should be used
*/
void sgct::SGCTWindow::setWindowMode(bool fullscreen)
{
    mFullScreen = fullscreen;
}

/*!
Set if the window should float (be on top / topmost)
*/
void sgct::SGCTWindow::setFloating(bool floating)
{
    mFloating = floating;
}

/*!
Set if the window should be double buffered (can only be set before window is created)
*/
void sgct::SGCTWindow::setDoubleBuffered(bool doubleBuffered)
{
    mDoubleBuffered = doubleBuffered;
}

/*!
Set if window borders should be visible
*/
void sgct::SGCTWindow::setWindowDecoration(bool state)
{
    mDecorated = state;
}

/*!
Set which monitor that should be used for fullscreen mode
*/
void sgct::SGCTWindow::setFullScreenMonitorIndex( int index )
{
    mMonitorIndex = index;
}

void sgct::SGCTWindow::setBarrier(const bool state)
{
//#ifdef __WITHSWAPBARRIERS__

    if( mUseSwapGroups && state != mBarrier)
    {
        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "SGCTWindow: Enabling Nvidia swap barrier...\n");

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
void sgct::SGCTWindow::setFixResolution(const bool state)
{
    mUseFixResolution = state;
}

/*!
Set if post effects should be used.
*/
void sgct::SGCTWindow::setUsePostFX(bool state)
{
    mUsePostFX = state;
    if( !state )
        mUseFXAA = false;
}

/*!
Set if FXAA should be used.
*/
void sgct::SGCTWindow::setUseFXAA(bool state)
{
    mUseFXAA = state;
    if( mUseFXAA )
        mUsePostFX = true;
    else
        mUsePostFX = (mPostFXPasses.size() > 0);
    MessageHandler::instance()->print( MessageHandler::NOTIFY_DEBUG, "FXAA status: %s for window %d\n", state ? "enabled" : "disabled", mId);
}

/*!
    Use quad buffer (hardware stereoscopic rendering).
    This function can only be used before the window is created.
    The quad buffer feature is only supported on professional CAD graphics cards such as
    Nvidia Quadro or AMD/ATI FireGL.
*/
void sgct::SGCTWindow::setUseQuadbuffer(const bool state)
{
    mUseQuadBuffer = state;
    if( mUseQuadBuffer )
    {
        glfwWindowHint(GLFW_STEREO, GL_TRUE);
        MessageHandler::instance()->print( MessageHandler::NOTIFY_INFO, "Window %d: Enabling quadbuffered rendering.\n", mId);
    }
}

/*!
    This function is used internally within sgct to open the window.

    /returns True if window was created successfully.
*/
bool sgct::SGCTWindow::openWindow(GLFWwindow* share)
{
    glfwWindowHint(GLFW_DEPTH_BITS, 32);
    glfwWindowHint(GLFW_DECORATED, mDecorated ? GL_TRUE : GL_FALSE);

    int antiAliasingSamples = getNumberOfAASamples();
    if( antiAliasingSamples > 1 && !SGCTSettings::instance()->useFBO() ) //if multisample is used
         glfwWindowHint( GLFW_SAMPLES, antiAliasingSamples );
    else
        glfwWindowHint(GLFW_SAMPLES, 0);

    glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);
    glfwWindowHint(GLFW_FLOATING, mFloating ? GL_TRUE : GL_FALSE);
    glfwWindowHint(GLFW_DOUBLEBUFFER, mDoubleBuffered ? GL_TRUE : GL_FALSE);
    if(!mVisible)
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

    setUseQuadbuffer( mStereoMode == Active_Stereo );

    if( mFullScreen )
    {
        int count;
        GLFWmonitor** monitors = glfwGetMonitors(&count);

        //get video modes
        /*for (int i = 0; i<count; i++)
        {
            int numberOfVideoModes;
            const GLFWvidmode * videoModes = glfwGetVideoModes(monitors[i], &numberOfVideoModes);

            MessageHandler::instance()->print("\nMonitor %d '%s' video modes\n========================\n",
                i, glfwGetMonitorName( monitors[i] ) );

            for (int j = 0; j < numberOfVideoModes; j++)
            {
                MessageHandler::instance()->print("%d x %d @ %d | R%d G%d B%d\n",
                    videoModes[j].width, videoModes[j].height, videoModes[j].refreshRate,
                    videoModes[j].redBits, videoModes[j].greenBits, videoModes[j].blueBits);
            }
            MessageHandler::instance()->print("\n");
        }*/
        
        if( SGCTSettings::instance()->getRefreshRateHint() > 0 )
            glfwWindowHint(GLFW_REFRESH_RATE, SGCTSettings::instance()->getRefreshRateHint());
        
        if (mMonitorIndex > 0 && mMonitorIndex < count)
        {
            mMonitor = monitors[mMonitorIndex];
        }
        else
        {
            mMonitor = glfwGetPrimaryMonitor();
            if( mMonitorIndex >= count )
                MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO,
                    "SGCTWindow(%d): Invalid monitor index (%d). This computer has %d monitors.\n",
                    mId, mMonitorIndex, count);
        }

        const GLFWvidmode* currentMode = glfwGetVideoMode(mMonitor);
        mWindowRes[0] = currentMode->width;
        mWindowRes[1] = currentMode->height;
    }

    mWindowHandle = glfwCreateWindow(mWindowRes[0], mWindowRes[1], "SGCT", mMonitor, share);

    if( mWindowHandle != NULL )
    {
        if( share != NULL )
            mSharedHandle = share;
        else
            mSharedHandle = mWindowHandle;

        glfwMakeContextCurrent( mWindowHandle );

        /*
            Mac for example scales the window size != frame buffer size
        */
        int buffer_width, buffer_height;
        glfwGetFramebufferSize(mWindowHandle, &buffer_width, &buffer_height);

        mWindowInitialRes[0] = mWindowRes[0];
        mWindowInitialRes[1] = mWindowRes[1];
        mScale[0] = static_cast<float>(buffer_width) / static_cast<float>(mWindowRes[0]);
        mScale[1] = static_cast<float>(buffer_height) / static_cast<float>(mWindowRes[1]);
        if (!mUseFixResolution)
        {
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
        glfwSwapInterval( SGCTSettings::instance()->getSwapInterval() );

        updateTransferCurve();

        //if slave disable mouse pointer
        if( !Engine::instance()->isMaster() )
            glfwSetInputMode( mWindowHandle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN );

        mFocused = (glfwGetWindowAttrib(mWindowHandle, GLFW_FOCUSED) == GL_TRUE ? true : false);
        mIconified = (glfwGetWindowAttrib(mWindowHandle, GLFW_ICONIFIED) == GL_TRUE ? true : false);
        
        //clear directly otherwise junk will be displayed on some OSs (OS X Yosemite)
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwMakeContextCurrent(mSharedHandle);

        if (SGCTSettings::instance()->useFBO())
        {
            mScreenCapture[0] = new sgct_core::ScreenCapture();

            if (mUseRightEyeTexture)
                mScreenCapture[1] = new sgct_core::ScreenCapture();
        }

        mFinalFBO_Ptr = new sgct_core::OffScreenBuffer();

        return true;
    }
    else
        return false;
}

/*!
Init Nvidia swap groups if they are supported by hardware. Supported hardware is Nvidia Quadro graphics card + sync card or AMD/ATI FireGL graphics card + sync card.
*/
void sgct::SGCTWindow::initNvidiaSwapGroups()
{    

#ifdef __WIN32__ //Windows uses wglew.h
    if (glfwExtensionSupported("WGL_NV_swap_group"))
    {
        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "SGCTWindow: Joining Nvidia swap group.\n");

        hDC = wglGetCurrentDC();

        unsigned int maxBarrier = 0;
        unsigned int maxGroup = 0;
        wglQueryMaxSwapGroupsNV( hDC, &maxGroup, &maxBarrier );
        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "WGL_NV_swap_group extension is supported.\n\tMax number of groups: %d\n\tMax number of barriers: %d\n",
            maxGroup, maxBarrier);

        /*
        wglJoinSwapGroupNV adds <hDC> to the swap group specified by <group>.
        If <hDC> is already a member of a different group, it is
        implicitly removed from that group first. A swap group is specified as
        an integer value between 0 and the value returned in <maxGroups> by
        wglQueryMaxSwapGroupsNV. If <group> is zero, the hDC is unbound from its
        current group, if any. If <group> is larger than <maxGroups>,
        wglJoinSwapGroupNV fails.

        */
        if( wglJoinSwapGroupNV(hDC, 1) )
        {
            MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "SGCTWindow: Joining swapgroup 1 [ok].\n");
            mUseSwapGroups = true;
        }
        else
        {
            MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "SGCTWindow: Joining swapgroup 1 [failed].\n");
            mUseSwapGroups = false;
        }
    }
    else
        mUseSwapGroups = false;
#else //Apple and Linux uses glext.h
    #ifndef __APPLE__

    if (glfwExtensionSupported("GLX_NV_swap_group"))
    {
        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "SGCTWindow: Joining Nvidia swap group.\n");

        hDC = glXGetCurrentDrawable();
        disp = glXGetCurrentDisplay();

        unsigned int maxBarrier = 0;
        unsigned int maxGroup = 0;
        glXQueryMaxSwapGroupsNV( disp, hDC, &maxGroup, &maxBarrier );
        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "GLX_NV_swap_group extension is supported.\n\tMax number of groups: %d\n\tMax number of barriers: %d\n",
            maxGroup, maxBarrier);

        if( glXJoinSwapGroupNV(disp, hDC, 1) )
        {
            MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "SGCTWindow: Joining swapgroup 1 [ok].\n");
            mUseSwapGroups = true;
        }
        else
        {
            MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "SGCTWindow: Joining swapgroup 1 [failed].\n");
            mUseSwapGroups = false;
        }
    }
    else
        mUseSwapGroups = false;

    #endif
#endif
}

void sgct::SGCTWindow::windowResizeCallback( GLFWwindow * window, int width, int height )
{
    sgct_core::SGCTNode * thisNode = sgct_core::ClusterManager::instance()->getThisNodePtr();

    if( thisNode != NULL )
    {
        //find the correct window to update
        for(std::size_t i=0; i<thisNode->getNumberOfWindows(); i++)
            if( thisNode->getWindowPtr(i)->getWindowHandle() == window )
                thisNode->getWindowPtr(i)->setWindowResolution(width > 0 ? width : 1, height > 0 ? height : 1);
    }
}

void sgct::SGCTWindow::frameBufferResizeCallback( GLFWwindow * window, int width, int height )
{
    sgct_core::SGCTNode * thisNode = sgct_core::ClusterManager::instance()->getThisNodePtr();
    
    if( thisNode != NULL )
    {
        //find the correct window to update
        for(std::size_t i=0; i<thisNode->getNumberOfWindows(); i++)
            if( thisNode->getWindowPtr(i)->getWindowHandle() == window )
                thisNode->getWindowPtr(i)->setFramebufferResolution(width > 0 ? width : 1, height > 0 ? height : 1);
    }
}

void sgct::SGCTWindow::windowFocusCallback( GLFWwindow * window, int state )
{
    sgct_core::SGCTNode * thisNode = sgct_core::ClusterManager::instance()->getThisNodePtr();

    if( thisNode != NULL )
    {
        //find the correct window to update
        for(std::size_t i=0; i<thisNode->getNumberOfWindows(); i++)
        if (thisNode->getWindowPtr(i)->getWindowHandle() == window)
            thisNode->getWindowPtr(i)->setFocused(state == GL_TRUE ? true : false);
    }
}

void sgct::SGCTWindow::windowIconifyCallback( GLFWwindow * window, int state )
{
    sgct_core::SGCTNode * thisNode = sgct_core::ClusterManager::instance()->getThisNodePtr();

    if( thisNode != NULL )
    {
        //find the correct window to update
        for(std::size_t i=0; i<thisNode->getNumberOfWindows(); i++)
            if( thisNode->getWindowPtr(i)->getWindowHandle() == window )
                thisNode->getWindowPtr(i)->setIconified( state == GL_TRUE ? true : false );
    }
}

void sgct::SGCTWindow::initScreenCapture()
{
    for (int i = 0; i < 2; i++)
    if (mScreenCapture[i] != NULL)
    {
        //init PBO in screen capture
        if (i == 0)
        {
            if (mUseRightEyeTexture)
                mScreenCapture[i]->init(mId, sgct_core::ScreenCapture::STEREO_LEFT);
            else
                mScreenCapture[i]->init(mId, sgct_core::ScreenCapture::MONO);
        }
        else
            mScreenCapture[i]->init(mId, sgct_core::ScreenCapture::STEREO_RIGHT);

        //a workaround for devices that are supporting pbos but not showing it, like OS X (Intel)
        if (Engine::instance()->isOGLPipelineFixed())
        {
            mScreenCapture[i]->setUsePBO(glfwExtensionSupported("GL_ARB_pixel_buffer_object") == GL_TRUE &&
                SGCTSettings::instance()->getUsePBO()); //if supported then use them
        }
        else //in modern openGL pbos must be supported
        {
            mScreenCapture[i]->setUsePBO(SGCTSettings::instance()->getUsePBO());
        }

        int numberOfCaputeChannels = mAlpha ? 4 : 3;
        if (sgct::SGCTSettings::instance()->getCaptureFromBackBuffer()) //capute from buffer supports only 8-bit per color component capture (unsigned byte)
        {
            mScreenCapture[i]->setTextureTransferProperties(GL_UNSIGNED_BYTE, mPreferBGR);
            mScreenCapture[i]->initOrResize(getXResolution(), getYResolution(), numberOfCaputeChannels, 1);
        }
        else //default: capture from texture (supports HDR)
        {
            mScreenCapture[i]->setTextureTransferProperties(mColorDataType, mPreferBGR);
            mScreenCapture[i]->initOrResize(getXFramebufferResolution(), getYFramebufferResolution(), numberOfCaputeChannels, mBytesPerColor);
        }

        if (SGCTSettings::instance()->getCaptureFormat() != sgct_core::ScreenCapture::NOT_SET)
            mScreenCapture[i]->setCaptureFormat(static_cast<sgct_core::ScreenCapture::CaptureFormat>(SGCTSettings::instance()->getCaptureFormat()));

        if (!sgct::Engine::checkForOGLErrors())
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "SGCTWindow %d: OpenGL error occured in screen capture %d init!\n", mId, i);
    }
}

void sgct::SGCTWindow::getSwapGroupFrameNumber(unsigned int &frameNumber)
{
    frameNumber = 0;

//#ifdef __WITHSWAPBARRIERS__

    if (mBarrier)
    {

    #ifdef __WIN32__ //Windows uses wglew.h
        if (glfwExtensionSupported("WGL_NV_swap_group"))
            wglQueryFrameCountNV(hDC, &frameNumber);
    #else //Apple and Linux uses glext.h
        #ifndef __APPLE__
        if( glfwExtensionSupported("GLX_NV_swap_group") )
            glXQueryFrameCountNV(disp, hDC, &frameNumber);
        #endif
    #endif
    }
//#endif
}

void sgct::SGCTWindow::resetSwapGroupFrameNumber()
{

//#ifdef __WITHSWAPBARRIERS__

    if (mBarrier)
    {
#ifdef __WIN32__
        if ( glfwExtensionSupported("WGL_NV_swap_group") && wglResetFrameCountNV(hDC) )
#else
    #ifdef __APPLE__
        if(false)
    #else //linux
        if( glfwExtensionSupported("GLX_NV_swap_group") && glXResetFrameCountNV(disp,hDC) )
    #endif
#endif
        {
            mSwapGroupMaster = true;
            MessageHandler::instance()->print( MessageHandler::NOTIFY_INFO, "Resetting frame counter. This computer is the master.\n");
        }
        else
        {
            mSwapGroupMaster = false;
            MessageHandler::instance()->print( MessageHandler::NOTIFY_INFO, "Resetting frame counter failed. This computer is the slave.\n");
        }
    }

//#endif
}

/*!
    This function creates textures that will act as FBO targets.
*/
void sgct::SGCTWindow::createTextures()
{
    //no target textures needed if not using FBO
    if( !SGCTSettings::instance()->useFBO() )
        return;

    if( Engine::instance()->getRunMode() <= Engine::OpenGL_Compablity_Profile )
    {
        glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT);
        glEnable(GL_TEXTURE_2D);
    }

    GLint maxTexSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
    if (mFramebufferResolution[0] > maxTexSize || mFramebufferResolution[1] > maxTexSize)
    {
        MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "SGCTWindow %d: Requested framebuffer is to big (Max: %dx%d)!\n",
            mId, maxTexSize, maxTexSize);
        return;
    }

    /*
        Create left and right color & depth textures.
    */
    //don't allocate the right eye image if stereo is not used
    //create a postFX texture for effects
    for( int i=0; i<NUMBER_OF_TEXTURES; i++ )
    {
        switch( i )
        {
        case Engine::RightEye:
            if( mUseRightEyeTexture )
                generateTexture(i, mFramebufferResolution[0], mFramebufferResolution[1], ColorTexture, true);
            break;

        case Engine::Depth:
            if( SGCTSettings::instance()->useDepthTexture() )
                generateTexture(i, mFramebufferResolution[0], mFramebufferResolution[1], DepthTexture, true );
            break;

        case Engine::FX1:
            if( !mPostFXPasses.empty() )
                generateTexture(i, mFramebufferResolution[0], mFramebufferResolution[1], ColorTexture, true);
            break;

        case Engine::FX2:
            if( mPostFXPasses.size() > 1 )
                generateTexture(i, mFramebufferResolution[0], mFramebufferResolution[1], ColorTexture, true);

        case Engine::Intermediate:
            if( mUsePostFX )
                generateTexture(i, mFramebufferResolution[0], mFramebufferResolution[1], ColorTexture, true);
            break;

        case Engine::Normals:
            if (SGCTSettings::instance()->useNormalTexture())
                generateTexture(i, mFramebufferResolution[0], mFramebufferResolution[1], NormalTexture, true);
            break;

        case Engine::Positions:
            if (SGCTSettings::instance()->usePositionTexture())
                generateTexture(i, mFramebufferResolution[0], mFramebufferResolution[1], PositionTexture, true);
            break;

        default:
            generateTexture(i, mFramebufferResolution[0], mFramebufferResolution[1], ColorTexture, true);
            break;
        }
    }

    if( Engine::instance()->getRunMode() <= Engine::OpenGL_Compablity_Profile )
        glPopAttrib();

    if( Engine::checkForOGLErrors() )
        MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "Texture targets initiated successfully for window %d!\n", mId);
    else
        MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Texture targets failed to initialize for window %d!\n", mId);
}

void sgct::SGCTWindow::generateTexture(unsigned int id, const int xSize, const int ySize, const sgct::SGCTWindow::TextureType type, const bool interpolate)
{
    //clean up if needed
    if( mFrameBufferTextures[id] != GL_FALSE )
    {
        glDeleteTextures(1, &mFrameBufferTextures[ id ]);
        mFrameBufferTextures[id] = GL_FALSE;
    }

    glGenTextures(1, &mFrameBufferTextures[id]);
    glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[id]);
    
    //---------------------
    // Disable mipmaps
    //---------------------
    if( Engine::instance()->isOGLPipelineFixed() )
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    }

    if (type == DepthTexture)
    {
        if (Engine::instance()->isOGLPipelineFixed() || SGCTSettings::instance()->getForceGlTexImage2D())
        {
            glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, xSize, ySize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }
        else
        {
            glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32, xSize, ySize);
        }
        
        MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "%dx%d depth texture (id: %d, type %d) generated for window %d!\n",
            xSize, ySize, mFrameBufferTextures[id], id, mId);
    }
    else if (type == NormalTexture)
    {
        if (Engine::instance()->isOGLPipelineFixed() || SGCTSettings::instance()->getForceGlTexImage2D())
        {
            glTexImage2D(GL_TEXTURE_2D, 0, sgct::SGCTSettings::instance()->getBufferFloatPrecisionAsGLint(), xSize, ySize, 0, GL_RGB, GL_FLOAT, NULL);
        }
        else
        {
            glTexStorage2D(GL_TEXTURE_2D, 1, sgct::SGCTSettings::instance()->getBufferFloatPrecisionAsGLint(), xSize, ySize);
        }
        
        MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "%dx%d normal texture (id: %d, type %d) generated for window %d!\n",
            xSize, ySize, mFrameBufferTextures[id], id, mId);
    }
    else if (type == PositionTexture)
    {
        if (Engine::instance()->isOGLPipelineFixed() || SGCTSettings::instance()->getForceGlTexImage2D())
        {
            glTexImage2D(GL_TEXTURE_2D, 0, sgct::SGCTSettings::instance()->getBufferFloatPrecisionAsGLint(), xSize, ySize, 0, GL_RGB, GL_FLOAT, NULL);
        }
        else
        {
            glTexStorage2D(GL_TEXTURE_2D, 1, sgct::SGCTSettings::instance()->getBufferFloatPrecisionAsGLint(), xSize, ySize);
        }
        
        MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "%dx%d position texture (id: %d, type %d) generated for window %d!\n",
            xSize, ySize, mFrameBufferTextures[id], id, mId);
    }
    else
    {
        if (Engine::instance()->isOGLPipelineFixed() || SGCTSettings::instance()->getForceGlTexImage2D())
        {
            glTexImage2D(GL_TEXTURE_2D, 0, mInternalColorFormat, xSize, ySize, 0, mColorFormat, mColorDataType, NULL);
        }
        else
        {
            glTexStorage2D(GL_TEXTURE_2D, 1, mInternalColorFormat, xSize, ySize);
        }
        
        MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "%dx%d BGRA texture (id: %d, type %d) generated for window %d!\n",
            xSize, ySize, mFrameBufferTextures[id], id, mId);
    }
    
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolate ? GL_LINEAR : GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolate ? GL_LINEAR : GL_NEAREST );
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

/*!
    This function creates FBOs if they are supported.
    This is done in the initOGL function.
*/
void sgct::SGCTWindow::createFBOs()
{
    if( !SGCTSettings::instance()->useFBO() )
    {
        //disable anaglyph & checkerboard stereo if FBOs are not used
        if( mStereoMode > Active_Stereo )
            mStereoMode = No_Stereo;
        MessageHandler::instance()->print(MessageHandler::NOTIFY_WARNING, "Warning! FBO rendering is not supported or enabled!\nPostFX, fisheye and some stereo modes are disabled.\n");
    }
    else
    {
        mFinalFBO_Ptr->setInternalColorFormat(mInternalColorFormat);
        mFinalFBO_Ptr->createFBO(
            mFramebufferResolution[0],
            mFramebufferResolution[1],
            mNumberOfAASamples);
            
        if( mFinalFBO_Ptr->checkForErrors() )
            MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "Window %d: FBO initiated successfully. Number of samples: %d\n", mId, mFinalFBO_Ptr->isMultiSampled() ? mNumberOfAASamples : 1);
        else
            MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Window %d: FBO initiated with errors! Number of samples: %d\n", mId, mFinalFBO_Ptr->isMultiSampled() ? mNumberOfAASamples : 1);
    }
}

/*!
    Create vertex buffer objects used to render framebuffer quad
*/
void sgct::SGCTWindow::createVBOs()
{
    if( !Engine::instance()->isOGLPipelineFixed() )
    {
        glGenVertexArrays(1, &mVAO);
        MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "SGCTWindow: Generating VAO: %d\n", mVAO);
    }

    glGenBuffers(1, &mVBO);
    MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "SGCTWindow: Generating VBO: %d\n", mVBO);

    if( !Engine::instance()->isOGLPipelineFixed() )
        glBindVertexArray( mVAO );
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), mQuadVerts, GL_STATIC_DRAW); //2TF + 3VF = 2*4 + 3*4 = 20
    if( !Engine::instance()->isOGLPipelineFixed() )
    {
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
            2,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            5*sizeof(float),    // stride
            reinterpret_cast<void*>(0) // array buffer offset
        );

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            5*sizeof(float),    // stride
            reinterpret_cast<void*>(8) // array buffer offset
        );
    }

    //unbind
    if( !Engine::instance()->isOGLPipelineFixed() )
        glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void sgct::SGCTWindow::loadShaders()
{
    //load shaders
    if( mStereoMode > Active_Stereo && mStereoMode < Side_By_Side_Stereo)
    {
        std::string stereo_frag_shader;
        std::string stereo_vert_shader;
        
        //reload shader program if it exists
        if( mStereoShader.isLinked() )
            mStereoShader.deleteProgram();

        if (Engine::instance()->isOGLPipelineFixed())
            stereo_vert_shader = sgct_core::shaders::Anaglyph_Vert_Shader;
        else
            stereo_vert_shader = sgct_core::shaders_modern::Anaglyph_Vert_Shader;

        if( mStereoMode == Anaglyph_Red_Cyan_Stereo )
        {
            Engine::instance()->isOGLPipelineFixed() ?
                stereo_frag_shader = sgct_core::shaders::Anaglyph_Red_Cyan_Stereo_Frag_Shader :
                stereo_frag_shader = sgct_core::shaders_modern::Anaglyph_Red_Cyan_Stereo_Frag_Shader;

        }
        else if( mStereoMode == Anaglyph_Amber_Blue_Stereo )
        {
            Engine::instance()->isOGLPipelineFixed() ?
                stereo_frag_shader = sgct_core::shaders::Anaglyph_Amber_Blue_Stereo_Frag_Shader :
                stereo_frag_shader = sgct_core::shaders_modern::Anaglyph_Amber_Blue_Stereo_Frag_Shader;
        }
        else if( mStereoMode == Anaglyph_Red_Cyan_Wimmer_Stereo )
        {
            Engine::instance()->isOGLPipelineFixed() ?
                stereo_frag_shader = sgct_core::shaders::Anaglyph_Red_Cyan_Stereo_Frag_Shader_Wimmer :
                stereo_frag_shader = sgct_core::shaders_modern::Anaglyph_Red_Cyan_Stereo_Frag_Shader_Wimmer;
        }
        else if( mStereoMode == Checkerboard_Stereo )
        {
            Engine::instance()->isOGLPipelineFixed() ?
                stereo_frag_shader = sgct_core::shaders::CheckerBoard_Frag_Shader :
                stereo_frag_shader = sgct_core::shaders_modern::CheckerBoard_Frag_Shader;
        }
        else if( mStereoMode == Checkerboard_Inverted_Stereo )
        {
            Engine::instance()->isOGLPipelineFixed() ?
                stereo_frag_shader = sgct_core::shaders::CheckerBoard_Inverted_Frag_Shader :
                stereo_frag_shader = sgct_core::shaders_modern::CheckerBoard_Inverted_Frag_Shader;
        }
        else if( mStereoMode == Vertical_Interlaced_Stereo )
        {
            Engine::instance()->isOGLPipelineFixed() ?
                stereo_frag_shader = sgct_core::shaders::Vertical_Interlaced_Stereo_Frag_Shader :
                stereo_frag_shader = sgct_core::shaders_modern::Vertical_Interlaced_Stereo_Frag_Shader;
        }
        else if( mStereoMode == Vertical_Interlaced_Inverted_Stereo )
        {
            Engine::instance()->isOGLPipelineFixed() ?
                stereo_frag_shader = sgct_core::shaders::Vertical_Interlaced_Inverted_Stereo_Frag_Shader :
                stereo_frag_shader = sgct_core::shaders_modern::Vertical_Interlaced_Inverted_Stereo_Frag_Shader;
        }
        else
        {
            Engine::instance()->isOGLPipelineFixed() ?
                stereo_frag_shader = sgct_core::shaders::Dummy_Stereo_Frag_Shader :
                stereo_frag_shader = sgct_core::shaders_modern::Dummy_Stereo_Frag_Shader;
        }

        //replace glsl version
        sgct_helpers::findAndReplace(stereo_frag_shader, "**glsl_version**", Engine::instance()->getGLSLVersion());
        sgct_helpers::findAndReplace(stereo_vert_shader, "**glsl_version**", Engine::instance()->getGLSLVersion());

        if(!mStereoShader.addShaderSrc(stereo_vert_shader, GL_VERTEX_SHADER, ShaderProgram::SHADER_SRC_STRING))
            MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Failed to load stereo vertex shader\n");
        if(!mStereoShader.addShaderSrc(stereo_frag_shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING))
            MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Failed to load stereo fragment shader\n");

        mStereoShader.setName("StereoShader");
        mStereoShader.createAndLinkProgram();
        mStereoShader.bind();
        if( !Engine::instance()->isOGLPipelineFixed() )
            StereoMVP = mStereoShader.getUniformLocation( "MVP" );
        StereoLeftTex = mStereoShader.getUniformLocation( "LeftTex" );
        StereoRightTex = mStereoShader.getUniformLocation( "RightTex" );
        glUniform1i( StereoLeftTex, 0 );
        glUniform1i( StereoRightTex, 1 );
        ShaderProgram::unbind();

        if (!sgct::Engine::checkForOGLErrors())
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "SGCTWindow %d: OpenGL error occured while loading shaders!\n", mId);
    }
}

void sgct::SGCTWindow::bindVAO() const
{
    glBindVertexArray( mVAO );
}

void sgct::SGCTWindow::bindVBO() const
{
    glBindBuffer(GL_ARRAY_BUFFER, mVBO );
}

void sgct::SGCTWindow::unbindVBO() const
{
    glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);
}

void sgct::SGCTWindow::unbindVAO() const
{
    glBindVertexArray( GL_FALSE );
}

/*!
    Returns pointer to FBO container
*/
sgct_core::OffScreenBuffer * sgct::SGCTWindow::getFBOPtr() const
{
    return mFinalFBO_Ptr;
}

/*!
    \returns pointer to GLFW monitor
*/
GLFWmonitor * sgct::SGCTWindow::getMonitor() const
{
    return mMonitor;
}

/*!
    \returns pointer to GLFW window
*/
GLFWwindow * sgct::SGCTWindow::getWindowHandle() const
{
    return mWindowHandle;
}

/*!
    Get the dimensions of the final FBO. Regular viewport rendering renders directly to this FBO but a fisheye renders first a cubemap and then to the final FBO.
    Post effects are rendered using these dimensions.
*/
void sgct::SGCTWindow::getFinalFBODimensions(int & width, int & height) const
{
    width = mFramebufferResolution[0];
    height = mFramebufferResolution[1];
}

/*!
    Add a post effect for this window
*/
void sgct::SGCTWindow::addPostFX( sgct::PostFX & fx )
{
    mPostFXPasses.push_back( fx );
}

/*!
    This function resizes the FBOs when the window is resized to achive 1:1 pixel-texel mapping.
*/
void sgct::SGCTWindow::resizeFBOs()
{
    if(!mUseFixResolution && SGCTSettings::instance()->useFBO())
    {
        makeOpenGLContextCurrent( Shared_Context );
        for(unsigned int i=0; i<NUMBER_OF_TEXTURES; i++)
        {
            if( mFrameBufferTextures[i] != GL_FALSE )
            {
                glDeleteTextures(1, &mFrameBufferTextures[i]);
                mFrameBufferTextures[i] = GL_FALSE;
            }
        }
        createTextures();

        mFinalFBO_Ptr->resizeFBO(mFramebufferResolution[0],
            mFramebufferResolution[1],
            mNumberOfAASamples);
        
        if( !mFinalFBO_Ptr->isMultiSampled() ) //attatch color buffer to prevent GL errors
        {
            mFinalFBO_Ptr->bind();
            mFinalFBO_Ptr->attachColorTexture( mFrameBufferTextures[Engine::LeftEye] );
            mFinalFBO_Ptr->unBind();
        }

        if( mFinalFBO_Ptr->checkForErrors() )
            MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "Window %d: FBOs resized successfully.\n", mId);
        else
            MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Window %d: FBOs resized with GL errors!\n", mId);
    }
}

/*!
    Returns the stereo mode. The value can be compared to the sgct_core::ClusterManager::StereoMode enum
*/
const sgct::SGCTWindow::StereoMode & sgct::SGCTWindow::getStereoMode() const
{
    return mStereoMode;
}

void sgct::SGCTWindow::addViewport(float left, float right, float bottom, float top)
{
    sgct_core::Viewport * vpPtr = new sgct_core::Viewport(left, right, bottom, top);
    mViewports.push_back(vpPtr);
    MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "Adding viewport (total %d)\n", mViewports.size());
}

void sgct::SGCTWindow::addViewport(sgct_core::Viewport * vpPtr)
{
    mViewports.push_back(vpPtr);
    MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "Adding viewport (total %d)\n", mViewports.size());
}

/*!
    Clears the vector containing all viewport data.
*/
void sgct::SGCTWindow::deleteAllViewports()
{
    mCurrentViewport = NULL;

    for (unsigned int i = 0; i < mViewports.size(); i++)
        delete mViewports[i];
    mViewports.clear();
}

/*!
\returns a pointer to the viewport that is beeing rendered to at the moment
*/
sgct_core::BaseViewport * sgct::SGCTWindow::getCurrentViewport() const
{
    if(mCurrentViewport == NULL)
        MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Window %d error: Current viewport is NULL!\n", mId);
    return mCurrentViewport;
}

/*!
\returns a pointer to a specific viewport
*/
sgct_core::Viewport * sgct::SGCTWindow::getViewport(std::size_t index) const
{
    return mViewports[index];
}

/*!
Get the current viewport data in pixels.
*/
void sgct::SGCTWindow::getCurrentViewportPixelCoords(int &x, int &y, int &xSize, int &ySize) const
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
std::size_t sgct::SGCTWindow::getNumberOfViewports() const
{
    return mViewports.size();
}

/*!
    Set the number of samples used in multisampled anti-aliasing
*/
void sgct::SGCTWindow::setNumberOfAASamples(int samples)
{
    mNumberOfAASamples = samples;
}

/*!
    \returns the number of samples used in multisampled anti-aliasing
*/
const int & sgct::SGCTWindow::getNumberOfAASamples() const
{
    return mNumberOfAASamples;
}

/*!
    Set the stereo mode. Set this mode in your init callback or during runtime in the post-sync-pre-draw callback.
    GLSL shaders will be recompliled if needed.
*/
void sgct::SGCTWindow::setStereoMode( StereoMode sm )
{
    mStereoMode = sm;

    MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "SGCTWindow: Setting stereo mode to '%s' for window %d.\n",
        getStereoModeStr().c_str(), mId);

    if (mStereoMode != No_Stereo && mStereoMode < Side_By_Side_Stereo)
        mUseRightEyeTexture = true;
    else
        mUseRightEyeTexture = false;

    if( mWindowHandle )
        loadShaders();
}

/*!
    This function returns the screen capture pointer if it's set otherwise NULL.

    @param eye can either be 0 (left) or 1 (right)
    
    Returns pointer to screen capture ptr
*/
sgct_core::ScreenCapture * sgct::SGCTWindow::getScreenCapturePointer(unsigned int eye) const
{
    return eye < 2 ? mScreenCapture[eye] : NULL;
}

/*!
    Set the which viewport that is the current. This is done from the Engine and end users shouldn't change this
*/
void sgct::SGCTWindow::setCurrentViewport(std::size_t index)
{
    mCurrentViewport = mViewports[index];
}

/*!
Set the which viewport that is the current. This is done internally from SGCT and end users shouldn't change this
*/
void sgct::SGCTWindow::setCurrentViewport(sgct_core::BaseViewport * vp)
{
    mCurrentViewport = vp;
}

std::string sgct::SGCTWindow::getStereoModeStr() const
{
    std::string mode;

    switch( mStereoMode )
    {
    case Active_Stereo:
        mode.assign("active");
        break;

    case Anaglyph_Red_Cyan_Stereo:
        mode.assign("anaglyph_red_cyan");
        break;

    case Anaglyph_Amber_Blue_Stereo:
        mode.assign("anaglyph_amber_blue");
        break;

    case Anaglyph_Red_Cyan_Wimmer_Stereo:
        mode.assign("anaglyph_wimmer");
        break;

    case Checkerboard_Stereo:
        mode.assign("checkerboard");
        break;

    case Checkerboard_Inverted_Stereo:
        mode.assign("checkerboard_inverted");
        break;

    case Vertical_Interlaced_Stereo:
        mode.assign("vertical_interlaced");
        break;

    case Vertical_Interlaced_Inverted_Stereo:
        mode.assign("vertical_interlaced_inverted");
        break;

    case Dummy_Stereo:
        mode.assign("dummy");
        break;

    case Side_By_Side_Stereo:
        mode.assign("side_by_side");
        break;

    case Side_By_Side_Inverted_Stereo:
        mode.assign("side_by_side_inverted");
        break;

    case Top_Bottom_Stereo:
        mode.assign("top_bottom");
        break;

    case Top_Bottom_Inverted_Stereo:
        mode.assign("top_bottom_inverted");
        break;

    default:
        mode.assign("none");
        break;
    }

    return mode;
}

void sgct::SGCTWindow::updateTransferCurve()
{
    if (!mMonitor)
        return;

    GLFWgammaramp ramp;
    unsigned short red[256], green[256], blue[256];
    
    ramp.size = 256;
    ramp.red = red;
    ramp.green = green;
    ramp.blue = blue;

    float gamma_exp = 1.0f / mGamma;

    for (unsigned int i = 0; i < ramp.size; i++)
    {
        float c = ((static_cast<float>(i)/255.0f) - 0.5f) * mContrast + 0.5f;
        float b = c + (mBrightness - 1.0f);
        float g = powf(b, gamma_exp);

        //transform back
        //unsigned short t = static_cast<unsigned short>(roundf(256.0f * g));

        unsigned short t = static_cast<unsigned short>(
            std::max(0.0f, std::min(65535.0f * g, 65535.0f)) //clamp to range
            + 0.5f); //round to closest integer

        ramp.red[i] = t;
        ramp.green[i] = t;
        ramp.blue[i] = t;
    }

    glfwSetGammaRamp(mMonitor, &ramp);
}

void sgct::SGCTWindow::updateColorBufferData()
{
    mColorFormat = GL_BGRA;
    
    switch (mBufferColorBitDepth)
    {
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
Set if fisheye alpha state. Should only be set using XML config of before calling Engine::init.
*/
void sgct::SGCTWindow::setAlpha(bool state)
{
    mAlpha = state;
}

/*!
Enable alpha clear color and 4-component screenshots
*/
const bool & sgct::SGCTWindow::getAlpha() const
{
    return mAlpha;
}

/*!
Set monitor gamma (works only if fullscreen)
*/
void sgct::SGCTWindow::setGamma(float gamma)
{
    mGamma = gamma;
    updateTransferCurve();
}

/*!
Get monitor gamma value (works only if fullscreen)
*/
const float & sgct::SGCTWindow::getGamma() const
{
    return mGamma;
}

/*!
Set monitor contrast in range [0.5, 1.5] (works only if fullscreen)
*/
void sgct::SGCTWindow::setContrast(float contrast)
{
    mContrast = contrast;
    updateTransferCurve();
}

/*!
Get monitor contrast value (works only if fullscreen)
*/
const float & sgct::SGCTWindow::getContrast() const
{
    return mContrast;
}

/*!
Set monitor brightness in range [0.5, 1.5] (works only if fullscreen)
*/
void sgct::SGCTWindow::setBrightness(float brightness)
{
    mBrightness = brightness;
    updateTransferCurve();
}

/*!
Set the color bit depth of the FBO and Screencapture.
*/
void sgct::SGCTWindow::setColorBitDepth(ColorBitDepth cbd)
{
    mBufferColorBitDepth = cbd;
}

/*!
Get the color bit depth of the FBO and Screencapture.
*/
sgct::SGCTWindow::ColorBitDepth sgct::SGCTWindow::getColorBitDepth() const
{
    return mBufferColorBitDepth;
}

/*!
Set if BGR(A) or RGB(A) rendering should be used. Default is BGR(A), which is usually the native order on GPU hardware.
This setting affects the screencapture which will return the prefered color order.
*/
void sgct::SGCTWindow::setPreferBGR(bool state)
{
    mPreferBGR = state;
}

/*!
Set if screen capturing is allowed.
*/
void sgct::SGCTWindow::setAllowCapture(bool state)
{
    mAllowCapture = state;
}

/*!
Get if buffer is rendered using BGR(A) or RGB(A).
*/
bool sgct::SGCTWindow::isBGRPrefered() const
{
    return mPreferBGR;
}

/*!
Get if (screen) capturing is allowed.
*/
bool sgct::SGCTWindow::isCapturingAllowed() const
{
    return mAllowCapture;
}

/*!
Get monitor brightness value (works only if fullscreen)
*/
const float & sgct::SGCTWindow::getBrightness() const
{
    return mBrightness;
}