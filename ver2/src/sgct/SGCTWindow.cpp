/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include "../include/sgct/SGCTWindow.h"
#include "../include/sgct/Engine.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/ClusterManager.h"
#include "../include/sgct/SGCTSettings.h"
#include "../include/sgct/SGCTInternalShaders.h"
#include "../include/sgct/SGCTInternalShaders_modern.h"
#include <glm/gtc/matrix_transform.hpp>
#include <stdio.h>

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

sgct::SGCTWindow::SGCTWindow(int id)
{
	mId = id;
	mUseFixResolution = false;
	mUseQuadBuffer = false;
	mFullScreen = false;
	mSetWindowPos = false;
	mDecorated = true;
	mFisheyeMode = false;
	mFisheyeAlpha = false;
	mVisible = false;
	mUseFXAA = false;
	mUsePostFX = false;
	mFullRes = true;
	mFocused = false;
	mIconified = false;
	mAreCubeMapViewPortsGenerated = false;

	mWindowRes[0] = 640;
	mWindowRes[1] = 480;
	mWindowResOld[0] = mWindowRes[0];
	mWindowResOld[1] = mWindowRes[1];
	mWindowInitialRes[0] = mWindowRes[0];
	mWindowInitialRes[1] = mWindowRes[1];
	mWindowPos[0] = 0;
	mWindowPos[1] = 0;
	mMonitorIndex = 0;
	mFramebufferResolution[0] = 512;
	mFramebufferResolution[1] = 256;
	mAspectRatio = 1.0f;
	mShotCounter = 0;

	FisheyeMVP		= -1;
	Cubemap			= -1;
	DepthCubemap	= -1;
	NormalCubemap	= -1;
	FishEyeHalfFov	= -1;
	FishEyeBGColor	= -1;
	FisheyeOffset	= -1;
	FishEyeSwapMVP	= -1;
	FishEyeSwapColor = -1;
	FishEyeSwapDepth = -1;
	FishEyeSwapNear	= -1;
	FishEyeSwapFar	= -1;
	StereoMVP		= -1;
	StereoLeftTex	= -1;
	StereoRightTex	= -1;

	mCubeMapResolution = 256; //low
	mCubeMapSize = 14.8f; //dome diamter
	mFisheyeTilt = 0.0f;
	mFieldOfView = 180.0f;

	mCropFactors[0] = 0.0f;
	mCropFactors[1] = 0.0f;
	mCropFactors[2] = 0.0f;
	mCropFactors[3] = 0.0f;

	mFisheyeOffset[0] = 0.0f;
	mFisheyeOffset[1] = 0.0f;
	mFisheyeOffset[2] = 0.0f;
	mFisheyeBaseOffset[0] = 0.0f;
	mFisheyeBaseOffset[1] = 0.0f;
	mFisheyeBaseOffset[2] = 0.0f;
	mFisheyeOffaxis = false;

	mQuadVerts[0] = 0.0f;
	mQuadVerts[1] = 0.0f;
	mQuadVerts[2] = 0.0f;
	mQuadVerts[3] = 0.0f;
	mQuadVerts[4] = -1.0f;

	mQuadVerts[5] = 1.0f;
	mQuadVerts[6] = 0.0f;
	mQuadVerts[7] = 1.0f;
	mQuadVerts[8] = 0.0f;
	mQuadVerts[9] = -1.0f;

	mQuadVerts[10] = 0.0f;
	mQuadVerts[11] = 1.0f;
	mQuadVerts[12] = 0.0f;
	mQuadVerts[13] = 1.0f;
	mQuadVerts[14] = -1.0f;

	mQuadVerts[15] = 1.0f;
	mQuadVerts[16] = 1.0f;
	mQuadVerts[17] = 1.0f;
	mQuadVerts[18] = 1.0f;
	mQuadVerts[19] = -1.0f;

	mVBO[RenderQuad]	= GL_FALSE; //default to openGL false
	mVBO[FishEyeQuad]	= GL_FALSE; //default to openGL false
	mVAO[RenderQuad]	= GL_FALSE;
	mVAO[FishEyeQuad]	= GL_FALSE;

	mStereoMode = No_Stereo;
	mNumberOfAASamples = 1;

	//FBO targets init
	for(int i=0; i<NUMBER_OF_TEXTURES; i++)
		mFrameBufferTextures[i] = GL_FALSE;

	//pointers
	mMonitor = NULL;
	mWindowHandle = NULL;
	mSharedHandle = NULL;
	mFinalFBO_Ptr = NULL;
	mCubeMapFBO_Ptr = NULL;
	mScreenCapture = NULL;

	mCurrentViewportIndex = 0;
}

/*!
Name this window
*/
void sgct::SGCTWindow::setName(const std::string & name)
{
	mName = name;
}

/*!
\returns the name of this window
*/
std::string sgct::SGCTWindow::getName()
{
	return mName;
}

/*!
	\returns this window's id
*/
int sgct::SGCTWindow::getId()
{
	return mId;
}

/*!
	\returns this window's focused flag
*/
bool sgct::SGCTWindow::isFocused()
{
	return mFocused;
}

/*!
	\returns this window's inconify flag 
*/
bool sgct::SGCTWindow::isIconified()
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

	if( mScreenCapture )
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Deleting screen capture data for window %d...\n", mId);
		delete mScreenCapture;
		mScreenCapture = NULL;
	}

	//delete FBO stuff
	if(mFinalFBO_Ptr != NULL &&
		mCubeMapFBO_Ptr != NULL &&
		SGCTSettings::instance()->useFBO() )
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Releasing OpenGL buffers for window %d...\n", mId);
		mFinalFBO_Ptr->destroy();
		if( mFisheyeMode )
			mCubeMapFBO_Ptr->destroy();

		delete mFinalFBO_Ptr;
		mFinalFBO_Ptr = NULL;
		delete mCubeMapFBO_Ptr;
		mCubeMapFBO_Ptr = NULL;

		for(unsigned int i=0; i<NUMBER_OF_TEXTURES; i++)
		{
			if( mFrameBufferTextures[i] != GL_FALSE )
			{
				glDeleteTextures(1, &mFrameBufferTextures[i]);
				mFrameBufferTextures[i] = GL_FALSE;
			}
		}
	}

	if( mVBO[RenderQuad] )
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Deleting VBOs for window %d...\n", mId);
		glDeleteBuffers(NUMBER_OF_VBOS, &mVBO[0]);
	}

	if( mVAO[RenderQuad] )
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Deleting VAOs for window %d...\n", mId);
		glDeleteVertexArrays(NUMBER_OF_VBOS, &mVAO[0]);
	}

	//delete shaders
	mFisheyeShader.deleteProgram();
	mFisheyeDepthCorrectionShader.deleteProgram();
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
		if( wglewIsSupported("WGL_NV_swap_group") )
		{
			//un-bind
			wglBindSwapBarrierNV(1,0);
			//un-join
			wglJoinSwapGroupNV(hDC,0);
		}
#else
    #ifndef __APPLE__
		if( glewIsSupported("GLX_NV_swap_group") )
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

void sgct::SGCTWindow::init(int id)
{
	if(!mFullScreen)
	{
		if(mSetWindowPos)
			glfwSetWindowPos( mWindowHandle, mWindowPos[0], mWindowPos[1] );
		glfwSetFramebufferSizeCallback( mWindowHandle, windowResizeCallback );
		glfwSetWindowFocusCallback( mWindowHandle, windowFocusCallback );
		glfwSetWindowIconifyCallback( mWindowHandle, windowIconifyCallback );
	}

	//swap the buffers and update the window
	glfwSwapBuffers( mWindowHandle );

	//initNvidiaSwapGroups();
}

/*!
	Init window buffers such as textures, FBOs, VAOs, VBOs and PBOs.
*/
void sgct::SGCTWindow::initOGL()
{
	createTextures();
	createVBOs(); //must be created before FBO
	createFBOs();
	initScreenCapture();
	loadShaders();
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
				generateTexture(index, mFramebufferResolution[0], mFramebufferResolution[1], false, false, true);
				break;

			case Engine::Intermediate:
			case Engine::FX1:
			case Engine::FX2:
				generateTexture(index, mFramebufferResolution[0], mFramebufferResolution[1], true, false, true);
				break;

			case Engine::Depth:
				generateTexture(index, mFramebufferResolution[0], mFramebufferResolution[1], false, true, true);
				break;

			case Engine::CubeMap:
				generateCubeMap(index, false);
				break;

			case Engine::CubeMapDepth:
				generateCubeMap(index, true);
				break;

			case Engine::FisheyeColorSwap:
				generateTexture(index, mCubeMapResolution, mCubeMapResolution, false, false, false);
				break;

			case Engine::FisheyeDepthSwap:
				generateTexture(index, mCubeMapResolution, mCubeMapResolution, false, true, false);
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
	Set the visibility state of this window. If a window is hidden the rendering for that window will be paused.
*/
void sgct::SGCTWindow::setVisibility(bool state)
{
	if( state != mVisible )
	{
		state ? glfwShowWindow( mWindowHandle ) : glfwHideWindow( mWindowHandle );
		mVisible = state;
	}
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

	if( !mUseFixResolution )
	{
		if( mFullRes )
		{
			mFramebufferResolution[0] = x;
			mFramebufferResolution[1] = y;
		}
		else
			glfwGetWindowSize( mWindowHandle, &mFramebufferResolution[0], &mFramebufferResolution[1] );
	}

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
	mFramebufferResolution[0] = x;
	mFramebufferResolution[1] = y;
}

/*!
	Swap previus data and current data. This is done at the end of the render loop.
*/
void sgct::SGCTWindow::swap()
{
	if( mVisible )
	{
		mWindowResOld[0] = mWindowRes[0];
		mWindowResOld[1] = mWindowRes[1];

		glfwSwapBuffers( mWindowHandle );
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

void sgct::SGCTWindow::update()
{
	if( mVisible && isWindowResized() )
	{
		//resize FBOs
		resizeFBOs();

		//resize PBOs
		(mFisheyeMode && !mFisheyeAlpha) ?
				mScreenCapture->initOrResize( mFramebufferResolution[0], mFramebufferResolution[1], 3 ) :
				mScreenCapture->initOrResize( mFramebufferResolution[0], mFramebufferResolution[1], 4 );
	}
}

/*!
	Set the this window's OpenGL context as current
*/
void sgct::SGCTWindow::makeOpenGLContextCurrent(OGL_Context context)
{
	if( sgct_core::ClusterManager::instance()->getThisNodePtr()->getNumberOfWindows() < 2 )
		return;

	if( context == Window_Context )
		glfwMakeContextCurrent( mWindowHandle );
	else
		glfwMakeContextCurrent( mSharedHandle );
}

/*!
	\returns true if this window is resized
*/
bool sgct::SGCTWindow::isWindowResized()
{
	return (mWindowRes[0] != mWindowResOld[0] || mWindowRes[1] != mWindowResOld[1]);
}

/*!
	\returns true if full screen rendering is enabled
*/
bool sgct::SGCTWindow::isFullScreen()
{
	return mFullScreen;
}

/*!
	\returns if the window is visible or not
*/
bool sgct::SGCTWindow::isVisible()
{
	return mVisible;
}

/*!
	\returns If the frame buffer has a fix resolution this function returns true.
*/
bool sgct::SGCTWindow::isFixResolution()
{
	return mUseFixResolution;
}

/*!
	Returns true if any kind of stereo is enabled
*/
bool sgct::SGCTWindow::isStereo()
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
Set if window borders should be visible
*/
void sgct::SGCTWindow::setWindowDecoration(bool state)
{
	mDecorated = state;
}

/*!
Set if full resolution should be used on displays which have different point resoultion than pixel resolution like Apple's retina displays
*/
void sgct::SGCTWindow::setFullResolutionMode(bool state)
{
	mFullRes = state;
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
	Use nvidia swap groups. This freature is only supported on quadro cards together with a compatible sync card.
	This function can only be used before the window is created.
*/
void sgct::SGCTWindow::setUseSwapGroups(const bool state)
{
	mUseSwapGroups = state;
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

	//disable MSAA if FXAA is in use and fisheye is not enabled
	/*
		Fisheye can use MSAA for cubemap rendering and FXAA for screen space rendering
		It's a bit overkill but usefull for rendering fisheye movies
	*/
	if( mUseFXAA && !isUsingFisheyeRendering())
	{
		setNumberOfAASamples(1);
	}

	//if using fisheye rendering for a dome display
	if( isUsingFisheyeRendering() )
	{
		if( !SGCTSettings::instance()->useFBO() )
		{
			MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "SGCTWindow(%d): Forcing FBOs for fisheye mode!\n", mId);
			SGCTSettings::instance()->setUseFBO( true );
		}

		if (!mAreCubeMapViewPortsGenerated)
			generateCubeMapViewports();
	}

	int antiAliasingSamples = getNumberOfAASamples();
	if( antiAliasingSamples > 1 && !SGCTSettings::instance()->useFBO() ) //if multisample is used
		 glfwWindowHint( GLFW_SAMPLES, antiAliasingSamples );

	int count;
	GLFWmonitor** monitors = glfwGetMonitors(&count);

	//get video modes and print them
	
	/*for(int i=0; i<count; i++)
	{
		int numberOfVideoModes;
		const GLFWvidmode * videoModes = glfwGetVideoModes( monitors[i], &numberOfVideoModes);

		MessageHandler::instance()->print("\nMonitor %d '%s' video modes\n========================\n",
			i, glfwGetMonitorName( monitors[i] ) );

		for(int j=0; j<numberOfVideoModes; j++)
			MessageHandler::instance()->print("%d x %d @ %d | R%d G%d B%d\n",
			videoModes[j].width, videoModes[j].height, videoModes[j].refreshRate,
			videoModes[j].redBits, videoModes[j].greenBits, videoModes[j].blueBits);

		MessageHandler::instance()->print("\n");
	}*/

	setUseQuadbuffer( mStereoMode == Active_Stereo );

	if( mFullScreen )
	{
		if( SGCTSettings::instance()->getRefreshRateHint() > 0 )
			glfwWindowHint(GLFW_REFRESH_RATE, SGCTSettings::instance()->getRefreshRateHint());
		
		if( mMonitorIndex > 0 && mMonitorIndex < count )
			mMonitor = monitors[ mMonitorIndex ];
		else
		{
			mMonitor = glfwGetPrimaryMonitor();
			if( mMonitorIndex >= count )
				MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO,
					"SGCTWindow(%d): Invalid monitor index (%d). This computer has %d monitors.\n",
					mId, mMonitorIndex, count);
		}
	}

	mWindowHandle = glfwCreateWindow(mWindowRes[0], mWindowRes[1], "SGCT", mMonitor, share);
	mVisible = true;

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
        glfwGetFramebufferSize(mWindowHandle, &mWindowRes[0], &mWindowRes[1]);

        mWindowInitialRes[0] = mWindowRes[0];
        mWindowInitialRes[1] = mWindowRes[1];
        if( !mUseFixResolution )
        {
            if( mFullRes )
			{
				mFramebufferResolution[0] = mWindowRes[0];
				mFramebufferResolution[1] = mWindowRes[1];
			}
			else
				glfwGetWindowSize( mWindowHandle, &mFramebufferResolution[0], &mFramebufferResolution[1] );
        }


		/*
			Swap inerval:
			-1 = adaptive sync
			0  = vertical sync off
			1  = wait for vertical sync
			2  = fix when using swapgroups in xp and running half the framerate
		*/
		glfwSwapInterval( SGCTSettings::instance()->getSwapInterval() );

		//if slave disable mouse pointer
		if( !Engine::instance()->isMaster() )
			glfwSetInputMode( mWindowHandle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN );

		mFocused = (glfwGetWindowAttrib(mWindowHandle, GLFW_FOCUSED) == GL_TRUE ? true : false);
		mIconified = (glfwGetWindowAttrib(mWindowHandle, GLFW_ICONIFIED) == GL_TRUE ? true : false);

		glfwMakeContextCurrent( mSharedHandle );

		mScreenCapture = new sgct_core::ScreenCapture();
		mFinalFBO_Ptr = new sgct_core::OffScreenBuffer();
		mCubeMapFBO_Ptr = new sgct_core::OffScreenBuffer();

		return true;
	}
	else
		return false;
}

void sgct::SGCTWindow::initNvidiaSwapGroups()
{
//#ifdef __WITHSWAPBARRIERS__
	setUseSwapGroups(true);

#ifdef __WIN32__ //Windows uses wglew.h
	if (wglewIsSupported("WGL_NV_swap_group") && mUseSwapGroups)
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
			MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "SGCTWindow: Joining swapgroup 1 [ok].\n");
		else
		{
			MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "SGCTWindow: Joining swapgroup 1 [failed].\n");
			mUseSwapGroups = false;
			return;
		}
	}
	else
		mUseSwapGroups = false;
#else //Apple and Linux uses glext.h
    #ifndef __APPLE__

    if (glewIsSupported("GLX_NV_swap_group") && mUseSwapGroups)
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
			MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "SGCTWindow: Joining swapgroup 1 [ok].\n");
		else
		{
			MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "SGCTWindow: Joining swapgroup 1 [failed].\n");
			mUseSwapGroups = false;
			return;
		}
	}
	else
		mUseSwapGroups = false;

    #endif
#endif

//#else
//        mUseSwapGroups = false;
//#endif
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

void sgct::SGCTWindow::windowFocusCallback( GLFWwindow * window, int state )
{
	sgct_core::SGCTNode * thisNode = sgct_core::ClusterManager::instance()->getThisNodePtr();

	if( thisNode != NULL )
	{
		//find the correct window to update
		for(std::size_t i=0; i<thisNode->getNumberOfWindows(); i++)
			if( thisNode->getWindowPtr(i)->getWindowHandle() == window )
				thisNode->getWindowPtr(i)->setFocused( state == GL_TRUE ? true : false );
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
	//init PBO in screen capture
	mScreenCapture->init( mId );
	mScreenCapture->setUsePBO( GLEW_EXT_pixel_buffer_object && glfwGetWindowAttrib( mWindowHandle, GLFW_CONTEXT_VERSION_MAJOR) > 1 ); //if supported then use them
	if( mFisheyeMode && !mFisheyeAlpha )
		mScreenCapture->initOrResize( getXFramebufferResolution(), getYFramebufferResolution(), 3 );
	else
		mScreenCapture->initOrResize( getXFramebufferResolution(), getYFramebufferResolution(), 4 );
	if( SGCTSettings::instance()->getCaptureFormat() != sgct_core::ScreenCapture::NOT_SET )
		mScreenCapture->setFormat( static_cast<sgct_core::ScreenCapture::CaptureFormat>( SGCTSettings::instance()->getCaptureFormat() ) );
}

void sgct::SGCTWindow::getSwapGroupFrameNumber(unsigned int &frameNumber)
{
	frameNumber = 0;

//#ifdef __WITHSWAPBARRIERS__

	if (mBarrier)
	{

    #ifdef __WIN32__ //Windows uses wglew.h
		if( wglewIsSupported("WGL_NV_swap_group") )
			wglQueryFrameCountNV(hDC, &frameNumber);
    #else //Apple and Linux uses glext.h
        #ifndef __APPLE__
		if( glewIsSupported("GLX_NV_swap_group") )
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
		if( wglewIsSupported("WGL_NV_swap_group") && wglResetFrameCountNV(hDC) )
#else
    #ifdef __APPLE__
        if(false)
    #else //linux
		if( glewIsSupported("GLX_NV_swap_group") && glXResetFrameCountNV(disp,hDC) )
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
	Returns if fisheye rendering is active in this window
*/
bool sgct::SGCTWindow::isUsingFisheyeRendering()
{
	return mFisheyeMode;
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

	/*
		Create left and right color & depth textures.
	*/
	//don't allocate the right eye image if stereo is not used
	//create a postFX texture for effects
	for( int i=0; i<(NUMBER_OF_TEXTURES-4); i++ ) //all textures except fisheye cubemap(s)
	{
		switch( i )
		{
		case Engine::RightEye:
			if( mStereoMode != No_Stereo && mStereoMode < Side_By_Side_Stereo )
				generateTexture(i, mFramebufferResolution[0], mFramebufferResolution[1], false, false, true );
			break;

		case Engine::Depth:
			if( SGCTSettings::instance()->useDepthTexture() )
				generateTexture(i, mFramebufferResolution[0], mFramebufferResolution[1], false, true, true );
			break;

		case Engine::FX1:
			if( !mPostFXPasses.empty() )
				generateTexture(i, mFramebufferResolution[0], mFramebufferResolution[1], true, false, true );
			break;

		case Engine::FX2:
			if( mPostFXPasses.size() > 1 )
				generateTexture(i, mFramebufferResolution[0], mFramebufferResolution[1], true, false, true );

		case Engine::Intermediate:
			if( mUsePostFX )
				generateTexture(i, mFramebufferResolution[0], mFramebufferResolution[1], true, false, true );
			break;

		default:
			generateTexture(i, mFramebufferResolution[0], mFramebufferResolution[1], false, false, true );
			break;
		}
	}
	/*
		Create cubemap texture for fisheye rendering if enabled.
	*/
	if( mFisheyeMode )
	{
		generateCubeMap(Engine::CubeMap, false);

		if( SGCTSettings::instance()->useDepthTexture() )
		{
			//set up texture target
			generateCubeMap(Engine::CubeMapDepth, true);

			//create swap textures
			//Color
			generateTexture(Engine::FisheyeColorSwap, mCubeMapResolution, mCubeMapResolution, false, false, false);
			//Depth
			generateTexture(Engine::FisheyeColorSwap, mCubeMapResolution, mCubeMapResolution, false, true, false);
		}
	}

	if( Engine::instance()->getRunMode() <= Engine::OpenGL_Compablity_Profile )
		glPopAttrib();

	if( Engine::checkForOGLErrors() )
		MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "Texture targets initiated successfully for window %d!\n", mId);
	else
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Texture targets failed to initialize for window %d!\n", mId);
}

void sgct::SGCTWindow::generateTexture(unsigned int id, int xSize, int ySize, bool anisotropicFiltering, bool depth, bool interpolate)
{
	//clean up if needed
	if( mFrameBufferTextures[id] != GL_FALSE )
	{
		glDeleteTextures(1, &mFrameBufferTextures[ id ]);
		mFrameBufferTextures[id] = GL_FALSE;
	}

	/*
		Anisotropic box filtering needed by FXAA
	*/
	GLfloat maxAni;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAni);
	float anisotropy_filter_size = (maxAni < 4.0f) ? maxAni : 4.0f;

	glGenTextures(1, &mFrameBufferTextures[id]);
	glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[id]);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolate ? GL_LINEAR : GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolate ? GL_LINEAR : GL_NEAREST );
	if( anisotropicFiltering )
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy_filter_size);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	if( depth )
	{
		//glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

		glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, xSize, ySize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "%dx%d depth texture (id: %d, type %d) generated for window %d!\n",
			xSize, ySize, mFrameBufferTextures[id], id, mId);
	}
	else
	{
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, xSize, ySize, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "%dx%d RGBA texture (id: %d, type %d) generated for window %d!\n",
			xSize, ySize, mFrameBufferTextures[id], id, mId);
	}
}

void sgct::SGCTWindow::generateCubeMap(unsigned int id, bool depth)
{
	if( mFrameBufferTextures[id] != GL_FALSE )
	{
		glDeleteTextures(1, &mFrameBufferTextures[ id ]);
		mFrameBufferTextures[id] = GL_FALSE;
	}

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	GLint MaxCubeMapRes;
	glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &MaxCubeMapRes);
	if(mCubeMapResolution > MaxCubeMapRes)
	{
		mCubeMapResolution = MaxCubeMapRes;
		MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "Info: Cubemap size set to max size: %d\n", MaxCubeMapRes);
	}

	//set up texture target
	glGenTextures(1, &mFrameBufferTextures[ id ]);
	glBindTexture(GL_TEXTURE_CUBE_MAP, mFrameBufferTextures[ id ]);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if( depth )
	{
		for (int side = 0; side < 6; ++side)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, 0, GL_DEPTH_COMPONENT32, mCubeMapResolution, mCubeMapResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "%dx%d depth cube map texture (id: %d) generated for window %d!\n",
			mCubeMapResolution, mCubeMapResolution, mFrameBufferTextures[ id ], mId);
	}
	else
	{
		for (int i = 0; i < 6; ++i)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, mCubeMapResolution, mCubeMapResolution, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "%dx%d cube map texture (id: %d) generated for window %d!\n",
			mCubeMapResolution, mCubeMapResolution, mFrameBufferTextures[ id ], mId);
	}

	glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
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
		if( mFisheyeMode )
		{
			mFinalFBO_Ptr->createFBO(
                mFramebufferResolution[0],
                mFramebufferResolution[1],
                1);

			mCubeMapFBO_Ptr->createFBO( mCubeMapResolution,
				mCubeMapResolution,
				mNumberOfAASamples);

			mCubeMapFBO_Ptr->bind();
			for(int i=0; i<6; i++)
			{
				if(!mCubeMapFBO_Ptr->isMultiSampled())
				{
					mCubeMapFBO_Ptr->attachCubeMapTexture( mFrameBufferTextures[Engine::CubeMap], i );
				}

				mFisheyeAlpha ? glClearColor(0.0f, 0.0f, 0.0f, 0.0f) : glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

				//copy AA-buffer to "regular"/non-AA buffer
				if( mCubeMapFBO_Ptr->isMultiSampled() )
				{
					mCubeMapFBO_Ptr->bindBlit(); //bind separate read and draw buffers to prepare blit operation

					//update attachments
					mCubeMapFBO_Ptr->attachCubeMapTexture( mFrameBufferTextures[Engine::CubeMap], i );

					mCubeMapFBO_Ptr->blit();
				}
			}

			if( mCubeMapFBO_Ptr->checkForErrors() )
                MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "Window %d: Cube map FBO created.\n", mId);
            else
                MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Window %d: Cube map FBO created with errors!\n", mId);

			sgct_core::OffScreenBuffer::unBind();

			//set ut the fisheye geometry etc.
			initFisheye();
		}
		else //regular viewport rendering
		{
			mFinalFBO_Ptr->createFBO(
				mFramebufferResolution[0],
				mFramebufferResolution[1],
				mNumberOfAASamples);
		}

        if( !mFinalFBO_Ptr->isMultiSampled() ) //attatch color buffer to prevent GL errors
        {
            mFinalFBO_Ptr->bind();
            mFinalFBO_Ptr->attachColorTexture( mFrameBufferTextures[Engine::LeftEye] );
            mFinalFBO_Ptr->unBind();
        }

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
		glGenVertexArrays(NUMBER_OF_VBOS, &mVAO[0]);

		MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "SGCTWindow: Generating VAOs:\n");
		for( unsigned int i=0; i<NUMBER_OF_VBOS; i++ )
			MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "\t%d\n", mVAO[i]);
		MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "\n");
	}

	glGenBuffers(NUMBER_OF_VBOS, &mVBO[0]);

	MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "SGCTWindow: Generating VBOs:\n");
	for( unsigned int i=0; i<NUMBER_OF_VBOS; i++ )
		MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "\t%d\n", mVBO[i]);
	MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "\n");

	if( !Engine::instance()->isOGLPipelineFixed() )
		glBindVertexArray( mVAO[RenderQuad] );
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[RenderQuad]);
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

	if( !Engine::instance()->isOGLPipelineFixed() )
		glBindVertexArray( mVAO[FishEyeQuad] );
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[FishEyeQuad]);
	glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), mFisheyeQuadVerts, GL_STREAM_DRAW); //2TF + 3VF = 2*4 + 3*4 = 20
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
	else
		glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void sgct::SGCTWindow::loadShaders()
{
	//load shaders
	if( mFisheyeMode )
	{
		float total_x = mFisheyeBaseOffset[0] + mFisheyeOffset[0];
		float total_y = mFisheyeBaseOffset[1] + mFisheyeOffset[1];
		float total_z = mFisheyeBaseOffset[2] + mFisheyeOffset[2];

		if( mStereoMode != No_Stereo ) // if any stereo
			mFisheyeOffaxis = true;
		else if( total_x == 0.0f && total_y == 0.0f && total_z == 0.0f )
			mFisheyeOffaxis = false;

		//reload shader program if it exists
		if( mFisheyeShader.isLinked() )
			mFisheyeShader.deleteProgram();

		if( Engine::instance()->isOGLPipelineFixed() )
		{
			mFisheyeShader.addShaderSrc( sgct_core::shaders::Fisheye_Vert_Shader, GL_VERTEX_SHADER, ShaderProgram::SHADER_SRC_STRING );

			if(mFisheyeOffaxis)
			{
				if( SGCTSettings::instance()->useDepthTexture() )
					mFisheyeShader.addShaderSrc( sgct_core::shaders::Fisheye_Frag_Shader_OffAxis_Depth, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
				else
					mFisheyeShader.addShaderSrc( sgct_core::shaders::Fisheye_Frag_Shader_OffAxis, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
			}
			else
			{
				if( SGCTSettings::instance()->useDepthTexture() )
					mFisheyeShader.addShaderSrc( sgct_core::shaders::Fisheye_Frag_Shader_Depth, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
				else
					mFisheyeShader.addShaderSrc( sgct_core::shaders::Fisheye_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
			}

			if( SGCTSettings::instance()->useDepthTexture() )
			{
				mFisheyeDepthCorrectionShader.addShaderSrc( sgct_core::shaders::Base_Vert_Shader, GL_VERTEX_SHADER, ShaderProgram::SHADER_SRC_STRING );
				mFisheyeDepthCorrectionShader.addShaderSrc( sgct_core::shaders::Fisheye_Depth_Correction_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
			}
		}
		else //modern pipeline
		{
			mFisheyeShader.addShaderSrc( sgct_core::shaders_modern::Fisheye_Vert_Shader, GL_VERTEX_SHADER, ShaderProgram::SHADER_SRC_STRING );

			if(mFisheyeOffaxis)
			{
				if( SGCTSettings::instance()->useDepthTexture() )
				{
					mFisheyeShader.addShaderSrc( sgct_core::shaders_modern::Fisheye_Frag_Shader_OffAxis_Depth, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
				}
				else
				{
					mFisheyeShader.addShaderSrc( sgct_core::shaders_modern::Fisheye_Frag_Shader_OffAxis, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
				}
			}
			else//not off axis
			{
				if( SGCTSettings::instance()->useDepthTexture() )
				{
					mFisheyeShader.addShaderSrc( sgct_core::shaders_modern::Fisheye_Frag_Shader_Depth, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
				}
				else //no depth
				{
					mFisheyeShader.addShaderSrc( sgct_core::shaders_modern::Fisheye_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
				}
			}

			if( SGCTSettings::instance()->useDepthTexture() )
			{
				mFisheyeDepthCorrectionShader.addShaderSrc( sgct_core::shaders_modern::Overlay_Vert_Shader, GL_VERTEX_SHADER, ShaderProgram::SHADER_SRC_STRING );
				mFisheyeDepthCorrectionShader.addShaderSrc( sgct_core::shaders_modern::Fisheye_Depth_Correction_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
			}
		}

		mFisheyeShader.setName("FisheyeShader");
		mFisheyeShader.createAndLinkProgram();
		mFisheyeShader.bind();

		if( !Engine::instance()->isOGLPipelineFixed() )
		{
			FisheyeMVP = mFisheyeShader.getUniformLocation( "MVP" );
		}

		Cubemap = mFisheyeShader.getUniformLocation( "cubemap" );
		glUniform1i( Cubemap, 0 );

		if( SGCTSettings::instance()->useDepthTexture() )
		{
			DepthCubemap = mFisheyeShader.getUniformLocation( "depthmap" );
			glUniform1i( DepthCubemap, 1 );
		}

		FishEyeHalfFov = mFisheyeShader.getUniformLocation( "halfFov" );
		glUniform1f( FishEyeHalfFov, glm::half_pi<float>() );

		const float * bgColor = Engine::instance()->getFisheyeClearColor();
		FishEyeBGColor = mFisheyeShader.getUniformLocation( "bgColor" );
		glUniform4f( FishEyeBGColor, bgColor[0], bgColor[1], bgColor[2], bgColor[3] );

		if( mFisheyeOffaxis )
		{
			FisheyeOffset = mFisheyeShader.getUniformLocation( "offset" );
			glUniform3f( FisheyeOffset,
				mFisheyeBaseOffset[0] + mFisheyeOffset[0],
				mFisheyeBaseOffset[1] + mFisheyeOffset[1],
				mFisheyeBaseOffset[2] + mFisheyeOffset[2] );
		}
		ShaderProgram::unbind();

		if( SGCTSettings::instance()->useDepthTexture() )
		{
			mFisheyeDepthCorrectionShader.setName("FisheyeDepthCorrectionShader");
			mFisheyeDepthCorrectionShader.createAndLinkProgram();
			mFisheyeDepthCorrectionShader.bind();

			if( !Engine::instance()->isOGLPipelineFixed() )
				FishEyeSwapMVP = mFisheyeDepthCorrectionShader.getUniformLocation( "MVP" );
			FishEyeSwapColor = mFisheyeDepthCorrectionShader.getUniformLocation( "cTex" );
			glUniform1i( FishEyeSwapColor, 0 );
			FishEyeSwapDepth = mFisheyeDepthCorrectionShader.getUniformLocation( "dTex" );
			glUniform1i( FishEyeSwapDepth, 1 );
			FishEyeSwapNear = mFisheyeDepthCorrectionShader.getUniformLocation( "near" );
			FishEyeSwapFar = mFisheyeDepthCorrectionShader.getUniformLocation( "far" );

			ShaderProgram::unbind();
		}
	}

	//-------------- end fisheye shader ----------->

	if( mStereoMode > Active_Stereo && mStereoMode < Side_By_Side_Stereo)
	{
		//reload shader program if it exists
		if( mStereoShader.isLinked() )
			mStereoShader.deleteProgram();

		if( Engine::instance()->isOGLPipelineFixed() )
			mStereoShader.addShaderSrc( sgct_core::shaders::Anaglyph_Vert_Shader, GL_VERTEX_SHADER, ShaderProgram::SHADER_SRC_STRING );
		else
			mStereoShader.addShaderSrc( sgct_core::shaders_modern::Anaglyph_Vert_Shader, GL_VERTEX_SHADER, ShaderProgram::SHADER_SRC_STRING );

		if( mStereoMode == Anaglyph_Red_Cyan_Stereo )
		{
			Engine::instance()->isOGLPipelineFixed() ?
				mStereoShader.addShaderSrc(sgct_core::shaders::Anaglyph_Red_Cyan_Stereo_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.addShaderSrc(sgct_core::shaders_modern::Anaglyph_Red_Cyan_Stereo_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );

		}
		else if( mStereoMode == Anaglyph_Amber_Blue_Stereo )
		{
			Engine::instance()->isOGLPipelineFixed() ?
				mStereoShader.addShaderSrc(sgct_core::shaders::Anaglyph_Amber_Blue_Stereo_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.addShaderSrc(sgct_core::shaders_modern::Anaglyph_Amber_Blue_Stereo_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
		}
		else if( mStereoMode == Anaglyph_Red_Cyan_Wimmer_Stereo )
		{
			Engine::instance()->isOGLPipelineFixed() ?
				mStereoShader.addShaderSrc(sgct_core::shaders::Anaglyph_Red_Cyan_Stereo_Frag_Shader_Wimmer, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.addShaderSrc(sgct_core::shaders_modern::Anaglyph_Red_Cyan_Stereo_Frag_Shader_Wimmer, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
		}
		else if( mStereoMode == Checkerboard_Stereo )
		{
			Engine::instance()->isOGLPipelineFixed() ?
				mStereoShader.addShaderSrc(sgct_core::shaders::CheckerBoard_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.addShaderSrc(sgct_core::shaders_modern::CheckerBoard_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
		}
		else if( mStereoMode == Checkerboard_Inverted_Stereo )
		{
			Engine::instance()->isOGLPipelineFixed() ?
				mStereoShader.addShaderSrc(sgct_core::shaders::CheckerBoard_Inverted_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.addShaderSrc(sgct_core::shaders_modern::CheckerBoard_Inverted_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
		}
		else if( mStereoMode == Vertical_Interlaced_Stereo )
		{
			Engine::instance()->isOGLPipelineFixed() ?
				mStereoShader.addShaderSrc(sgct_core::shaders::Vertical_Interlaced_Stereo_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.addShaderSrc(sgct_core::shaders_modern::Vertical_Interlaced_Stereo_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
		}
		else if( mStereoMode == Vertical_Interlaced_Inverted_Stereo )
		{
			Engine::instance()->isOGLPipelineFixed() ?
				mStereoShader.addShaderSrc(sgct_core::shaders::Vertical_Interlaced_Inverted_Stereo_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.addShaderSrc(sgct_core::shaders_modern::Vertical_Interlaced_Inverted_Stereo_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
		}
		/*else if( mStereoMode == Side_By_Side_Stereo )
		{
			Engine::instance()->isOGLPipelineFixed() ?
				mStereoShader.addShaderSrc(sgct_core::shaders::SBS_Stereo_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.addShaderSrc(sgct_core::shaders_modern::SBS_Stereo_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
		}
		else if( mStereoMode == Side_By_Side_Inverted_Stereo )
		{
			Engine::instance()->isOGLPipelineFixed() ?
				mStereoShader.addShaderSrc(sgct_core::shaders::SBS_Stereo_Inverted_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.addShaderSrc(sgct_core::shaders_modern::SBS_Stereo_Inverted_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
		}
		else if( mStereoMode == Top_Bottom_Stereo )
		{
			Engine::instance()->isOGLPipelineFixed() ?
				mStereoShader.addShaderSrc(sgct_core::shaders::TB_Stereo_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.addShaderSrc(sgct_core::shaders_modern::TB_Stereo_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
		}
		else if( mStereoMode == Top_Bottom_Inverted_Stereo )
		{
			Engine::instance()->isOGLPipelineFixed() ?
				mStereoShader.addShaderSrc(sgct_core::shaders::TB_Stereo_Inverted_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.addShaderSrc(sgct_core::shaders_modern::TB_Stereo_Inverted_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
		}*/
		else
		{
			Engine::instance()->isOGLPipelineFixed() ?
				mStereoShader.addShaderSrc(sgct_core::shaders::Dummy_Stereo_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.addShaderSrc(sgct_core::shaders_modern::Dummy_Stereo_Frag_Shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING );
		}

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
	}
}

void sgct::SGCTWindow::bindVAO()
{
	mFisheyeMode ? glBindVertexArray( mVAO[FishEyeQuad] ) : glBindVertexArray( mVAO[RenderQuad] );
}

void sgct::SGCTWindow::bindVAO( VBOIndex index )
{
	glBindVertexArray( mVAO[ index ] );
}

void sgct::SGCTWindow::bindVBO()
{
	mFisheyeMode ? glBindBuffer(GL_ARRAY_BUFFER, mVBO[FishEyeQuad]) : glBindBuffer(GL_ARRAY_BUFFER, mVBO[RenderQuad]);
}

void sgct::SGCTWindow::bindVBO( VBOIndex index )
{
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[index]);
}

void sgct::SGCTWindow::unbindVBO()
{
	glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);
}

void sgct::SGCTWindow::unbindVAO()
{
	glBindVertexArray( GL_FALSE );
}

/*!
	This functions saves a png screenshot or a stereoscopic pair in the current working directory.
	All screenshots are numbered so this function can be called several times whitout overwriting previous screenshots.

	The PNG images are saved as RGBA images with transparancy. Alpha is taken from the clear color alpha.
*/
void sgct::SGCTWindow::captureBuffer()
{
	if(mStereoMode == No_Stereo)
	{
		if( SGCTSettings::instance()->useFBO() )
			mScreenCapture->SaveScreenCapture( mFrameBufferTextures[Engine::LeftEye], mShotCounter, sgct_core::ScreenCapture::FBO_Texture );
		else
			mScreenCapture->SaveScreenCapture( 0, mShotCounter, sgct_core::ScreenCapture::Front_Buffer );
	}
	else
	{
		if( SGCTSettings::instance()->useFBO() )
		{
			mScreenCapture->SaveScreenCapture( mFrameBufferTextures[Engine::LeftEye], mShotCounter, sgct_core::ScreenCapture::FBO_Left_Texture );
			mScreenCapture->SaveScreenCapture( mFrameBufferTextures[Engine::RightEye], mShotCounter, sgct_core::ScreenCapture::FBO_Right_Texture );
		}
		else
		{
			mScreenCapture->SaveScreenCapture( 0, mShotCounter, sgct_core::ScreenCapture::Left_Front_Buffer );
			mScreenCapture->SaveScreenCapture( 0, mShotCounter, sgct_core::ScreenCapture::Right_Front_Buffer );
		}
	}

	mShotCounter++;
}

/*!
	Returns pointer to FBO container
*/
sgct_core::OffScreenBuffer * sgct::SGCTWindow::getFBOPtr()
{
	return mFisheyeMode ? mCubeMapFBO_Ptr : mFinalFBO_Ptr;
}

/*!
	\returns pointer to GLFW monitor
*/
GLFWmonitor * sgct::SGCTWindow::getMonitor()
{
	return mMonitor;
}

/*!
	\returns pointer to GLFW window
*/
GLFWwindow * sgct::SGCTWindow::getWindowHandle()
{
	return mWindowHandle;
}

/*!
	Get the dimensions of the FBO that the Engine::draw function renders to.
*/
void sgct::SGCTWindow::getDrawFBODimensions( int & width, int & height )
{
	if( mFisheyeMode )
	{
		width = mCubeMapResolution;
		height = mCubeMapResolution;
	}
	else
	{
		width = mFramebufferResolution[0];
		height = mFramebufferResolution[1];
	}
}

/*!
	Get the dimensions of the final FBO. Regular viewport rendering renders directly to this FBO but a fisheye renders first a cubemap and then to the final FBO.
	Post effects are rendered using these dimensions.
*/
void sgct::SGCTWindow::getFinalFBODimensions( int & width, int & height )
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

		if( mFisheyeMode )
		{
			mFinalFBO_Ptr->resizeFBO( mFramebufferResolution[0],
				mFramebufferResolution[1],
				1);

			//Cube map FBO is constant in size so we don't need to resize that one

			//set ut the fisheye geometry etc.
			initFisheye();
		}
		else //regular viewport rendering
		{
			mFinalFBO_Ptr->resizeFBO(mFramebufferResolution[0],
				mFramebufferResolution[1],
				mNumberOfAASamples);
		}

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
	Init the fisheye data by creating geometry and precalculate textures
*/
void sgct::SGCTWindow::initFisheye()
{
	//create proxy geometry
	float leftcrop		= mCropFactors[CropLeft];
	float rightcrop		= mCropFactors[CropRight];
	float bottomcrop	= mCropFactors[CropBottom];
	float topcrop		= mCropFactors[CropTop];

	float cropAspect = ((1.0f-2.0f * bottomcrop) + (1.0f-2.0f*topcrop)) / ((1.0f-2.0f*leftcrop) + (1.0f-2.0f*rightcrop));

	float x = 1.0f;
	float y = 1.0f;
	float frameBufferAspect = static_cast<float>( mFramebufferResolution[0] ) /
		static_cast<float>( mFramebufferResolution[1] );

	float aspect = frameBufferAspect * cropAspect;
	( aspect >= 1.0f ) ? x = 1.0f / aspect : y = aspect;

	mFisheyeQuadVerts[0] = leftcrop;
	mFisheyeQuadVerts[1] = bottomcrop;
	mFisheyeQuadVerts[2] = -x;
	mFisheyeQuadVerts[3] = -y;
	mFisheyeQuadVerts[4] = -1.0f;

	mFisheyeQuadVerts[5] = leftcrop;
	mFisheyeQuadVerts[6] = 1.0f - topcrop;
	mFisheyeQuadVerts[7] = -x;
	mFisheyeQuadVerts[8] = y;
	mFisheyeQuadVerts[9] = -1.0f;

	mFisheyeQuadVerts[10] = 1.0f - rightcrop;
	mFisheyeQuadVerts[11] = bottomcrop;
	mFisheyeQuadVerts[12] = x;
	mFisheyeQuadVerts[13] = -y;
	mFisheyeQuadVerts[14] = -1.0f;

	mFisheyeQuadVerts[15] = 1.0f - rightcrop;
	mFisheyeQuadVerts[16] = 1.0f - topcrop;
	mFisheyeQuadVerts[17] = x;
	mFisheyeQuadVerts[18] = y;
	mFisheyeQuadVerts[19] = -1.0f;

	//update VBO
	if( !Engine::instance()->isOGLPipelineFixed() )
		glBindVertexArray( mVAO[FishEyeQuad] );
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[FishEyeQuad]);

	GLvoid* PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	memcpy(PositionBuffer, mFisheyeQuadVerts, 20 * sizeof(float));
	glUnmapBuffer(GL_ARRAY_BUFFER);

	if( !Engine::instance()->isOGLPipelineFixed() )
		glBindVertexArray( 0 );
	else
		glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*!
	Set if fisheye rendering is used in this window
*/
void sgct::SGCTWindow::setFisheyeRendering(bool state)
{
	mFisheyeMode = state;
}

/*!
	Returns the stereo mode. The value can be compared to the sgct_core::ClusterManager::StereoMode enum
*/
sgct::SGCTWindow::StereoMode sgct::SGCTWindow::getStereoMode()
{
	return mStereoMode;
}

/*!
	\returns true if full resoultion is used for displays where point resolution is different from pixel resolution like Apple's retina displays
*/
bool sgct::SGCTWindow::getFullResolutionMode()
{
	return mFullRes;
}

void sgct::SGCTWindow::addViewport(float left, float right, float bottom, float top)
{
	sgct_core::Viewport tmpVP(left, right, bottom, top);
	mViewports.push_back(tmpVP);
	MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "Adding viewport (total %d)\n", mViewports.size());
}

void sgct::SGCTWindow::addViewport(sgct_core::Viewport &vp)
{
	mViewports.push_back(vp);
	MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "Adding viewport (total %d)\n", mViewports.size());
}

/*!
	Clears the vector containing all viewport data.
*/
void sgct::SGCTWindow::deleteAllViewports()
{
	mCurrentViewportIndex = 0;
	mViewports.clear();
}

/*!
	Generates six viewports that renders the inside of a cube. The method used in SGCT is to only use four faces by rotating the cube 45 degrees.
*/
void sgct::SGCTWindow::generateCubeMapViewports()
{
	//clear the viewports since they will be replaced
	deleteAllViewports();
	glm::vec4 lowerLeft, upperLeft, upperRight;

	float radius = getDomeDiameter() / 2.0f;

	//+Z face
	lowerLeft.x = -1.0f * radius;
	lowerLeft.y = -1.0f * radius;
	lowerLeft.z = 1.0f * radius;
	lowerLeft.w = 1.0f;

	upperLeft.x = -1.0f * radius;
	upperLeft.y = 1.0f * radius;
	upperLeft.z = 1.0f * radius;
	upperLeft.w = 1.0f;

	upperRight.x = 1.0f * radius;
	upperRight.y = 1.0f * radius;
	upperRight.z = 1.0f * radius;
	upperRight.w = 1.0f;

	//tilt
	glm::mat4 tiltMat = glm::rotate(glm::mat4(1.0f), 90.0f-mFisheyeTilt, glm::vec3(1.0f, 0.0f, 0.0f));
	//glm::mat4 tiltMat(1.0f);

	//pan 45 deg
	glm::mat4 panRot = glm::rotate(tiltMat, 45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	//glm::mat4 panRot(1.0f);

	//add viewports
	for(unsigned int i=0; i<6; i++)
	{
		sgct_core::Viewport tmpVP;
		tmpVP.setName("Fisheye");

		glm::mat4 rotMat(1.0f);

		switch(i)
		{
		case 0: //+X face
			rotMat = glm::rotate(panRot, -90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
			break;

		case 1: //-X face
			tmpVP.setEnabled( false );
			rotMat = glm::rotate(panRot, 90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
			break;

		case 2: //+Y face
			rotMat = glm::rotate(panRot, -90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
			break;

		case 3: //-Y face
			rotMat = glm::rotate(panRot, 90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
			break;

		case 4: //+Z face
			rotMat = panRot;
			break;

		case 5: //-Z face
			tmpVP.setEnabled( false );
			rotMat = glm::rotate(panRot, 180.0f, glm::vec3(0.0f, 1.0f, 0.0f));
			break;
		}

		//Compensate for users pos
		glm::vec4 userVec = glm::vec4(
			sgct_core::ClusterManager::instance()->getUserPtr()->getXPos(),
			sgct_core::ClusterManager::instance()->getUserPtr()->getYPos(),
			sgct_core::ClusterManager::instance()->getUserPtr()->getZPos(),
			1.0f );

		//add viewplane vertices
		tmpVP.setViewPlaneCoords(0, rotMat * lowerLeft + userVec);
		tmpVP.setViewPlaneCoords(1, rotMat * upperLeft + userVec);
		tmpVP.setViewPlaneCoords(2, rotMat * upperRight + userVec);

		/*
			Each viewport contains frustums for mono, left stereo and right stereo
		*/
		addViewport( tmpVP );

		/*
		fprintf(stderr, "View #%d:\n", i);
		fprintf(stderr, "LowerLeft: %f %f %f\n", tmpVP.getViewPlaneCoords( Viewport::LowerLeft ).x, tmpVP.getViewPlaneCoords( Viewport::LowerLeft ).y, tmpVP.getViewPlaneCoords( Viewport::LowerLeft ).z);
		fprintf(stderr, "UpperLeft: %f %f %f\n", tmpVP.getViewPlaneCoords( Viewport::UpperLeft ).x, tmpVP.getViewPlaneCoords( Viewport::UpperLeft ).y, tmpVP.getViewPlaneCoords( Viewport::UpperLeft ).z);
		fprintf(stderr, "UpperRight: %f %f %f\n\n", tmpVP.getViewPlaneCoords( Viewport::UpperRight ).x, tmpVP.getViewPlaneCoords( Viewport::UpperRight ).y, tmpVP.getViewPlaneCoords( Viewport::UpperRight ).z);
		*/
	}

	if( getFisheyeOverlay() != NULL )
	{
		mViewports[0].setOverlayTexture( getFisheyeOverlay() );
		//MessageHandler::instance()->print("Setting fisheye overlay to '%s'\n", SGCTSettings::instance()->getFisheyeOverlay());
	}

	mAreCubeMapViewPortsGenerated = true;
}

/*!
\returns a pointer to the viewport that is beeing rendered to at the moment
*/
sgct_core::Viewport * sgct::SGCTWindow::getCurrentViewport()
{
	return &mViewports[mCurrentViewportIndex];
}

/*!
\returns a pointer to a specific viewport
*/
sgct_core::Viewport * sgct::SGCTWindow::getViewport(std::size_t index)
{
	return &mViewports[index];
}

/*!
Get the current viewport data in pixels.
*/
void sgct::SGCTWindow::getCurrentViewportPixelCoords(int &x, int &y, int &xSize, int &ySize)
{
	x = static_cast<int>(getCurrentViewport()->getX() *
		static_cast<double>(mFramebufferResolution[0]));
	y = static_cast<int>(getCurrentViewport()->getY() *
		static_cast<double>(mFramebufferResolution[1]));
	xSize = static_cast<int>(getCurrentViewport()->getXSize() *
		static_cast<double>(mFramebufferResolution[0]));
	ySize = static_cast<int>(getCurrentViewport()->getYSize() *
		static_cast<double>(mFramebufferResolution[1]));
}

/*!
\returns the viewport count for this window
*/
std::size_t sgct::SGCTWindow::getNumberOfViewports()
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
int sgct::SGCTWindow::getNumberOfAASamples()
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

	if( mWindowHandle )
		loadShaders();
}

/*!
	Returns pointer to screen capture ptr
*/
sgct_core::ScreenCapture * sgct::SGCTWindow::getScreenCapturePointer()
{
	return mScreenCapture;
}

/*!
	Set the screenshot number (file index)
*/
void sgct::SGCTWindow::setScreenShotNumber(int number)
{
	mShotCounter = number;
}

/*!
	\returns the current screenshot number (file index)
*/
int sgct::SGCTWindow::getScreenShotNumber()
{
	return mShotCounter;
}

/*!
	Set the which viewport that is the current. This is done from the Engine and end users shouldn't change this
*/
void sgct::SGCTWindow::setCurrentViewport(std::size_t index)
{
	mCurrentViewportIndex = index;
}

/*!
Set the cubemap resolution used in the fisheye renderer

@param res resolution of the cubemap sides (should be a power of two for best performance)
*/
void sgct::SGCTWindow::setCubeMapResolution(int res)
{
	mCubeMapResolution = res;
}

/*!
Set the dome diameter used in the fisheye renderer (used for the viewplane distance calculations)

@param size size of the dome diameter (cube side) in meters
*/
void sgct::SGCTWindow::setDomeDiameter(float size)
{
	mCubeMapSize = size;
	generateCubeMapViewports();
}

/*!
Set if fisheye alpha state. Should only be set using XML config of before calling Engine::init.
*/
void sgct::SGCTWindow::setFisheyeAlpha(bool state)
{
	mFisheyeAlpha = state;
}

/*!
Set the fisheye/dome tilt angle used in the fisheye renderer.
The tilt angle is from the horizontal.

@param angle the tilt angle in degrees
*/
void sgct::SGCTWindow::setFisheyeTilt(float angle)
{
	mFisheyeTilt = angle;
}

/*!
Set the fisheye/dome field-of-view angle used in the fisheye renderer.

@param angle the FOV angle in degrees
*/
void sgct::SGCTWindow::setFisheyeFOV(float angle)
{
	mFieldOfView = angle;
}

/*!
Set the fisheye crop values. Theese values are used when rendering content for a single projector dome.
The elumenati geodome has usually a 4:3 SXGA+ (1400x1050) projector and the fisheye is cropped 25% (350 pixels) at the top.
*/
void sgct::SGCTWindow::setFisheyeCropValues(float left, float right, float bottom, float top)
{
	mCropFactors[ CropLeft ] = (left < 1.0f && left > 0.0f) ? left : 0.0f;
	mCropFactors[ CropRight ] = (right < 1.0f && right > 0.0f) ? right : 0.0f;
	mCropFactors[ CropBottom ] = (bottom < 1.0f && bottom > 0.0f) ? bottom : 0.0f;
	mCropFactors[ CropTop ] = (top < 1.0f && top > 0.0f) ? top : 0.0f;
}

/*!
	Set fisheye base offset to render offaxis. Length of vector must be smaller then 1.
	Base of fisheye is the XY-plane. The base offset will be added to the offset specified by setFisheyeOffset.
	These values are set from the XML config.
*/
void sgct::SGCTWindow::setFisheyeBaseOffset(float x, float y, float z )
{
	mFisheyeBaseOffset[0] = x;
	mFisheyeBaseOffset[1] = y;
	mFisheyeBaseOffset[2] = z;

	float total_x = mFisheyeBaseOffset[0] + mFisheyeOffset[0];
	float total_y = mFisheyeBaseOffset[1] + mFisheyeOffset[1];
	float total_z = mFisheyeBaseOffset[2] + mFisheyeOffset[2];

	if( total_x == 0.0f && total_y == 0.0f && total_z == 0.0f )
		mFisheyeOffaxis = false;
	else
		mFisheyeOffaxis = true;
}

/*!
	Set fisheye offset to render offaxis. Length of vector must be smaller then 1.
	Base of fisheye is the XY-plane. This function is normally used in fisheye stereo rendering.
*/
void sgct::SGCTWindow::setFisheyeOffset(float x, float y, float z)
{
	mFisheyeOffset[0] = x;
	mFisheyeOffset[1] = y;
	mFisheyeOffset[2] = z;

	float total_x = mFisheyeBaseOffset[0] + mFisheyeOffset[0];
	float total_y = mFisheyeBaseOffset[1] + mFisheyeOffset[1];
	float total_z = mFisheyeBaseOffset[2] + mFisheyeOffset[2];

	if( total_x == 0.0f && total_y == 0.0f && total_z == 0.0f )
		mFisheyeOffaxis = false;
	else
		mFisheyeOffaxis = true;
}

/*!
Set the fisheye overlay image.
*/
void sgct::SGCTWindow::setFisheyeOverlay(std::string filename)
{
	mFisheyeOverlayFilename.assign(filename);
}

//! Get the cubemap size in pixels used in the fisheye renderer
int sgct::SGCTWindow::getCubeMapResolution()
{
	return mCubeMapResolution;
}

//! Get the dome diameter in meters used in the fisheye renderer
float sgct::SGCTWindow::getDomeDiameter()
{
	return mCubeMapSize;
}

//! Get the fisheye/dome tilt angle in degrees
float sgct::SGCTWindow::getFisheyeTilt()
{
	return mFisheyeTilt;
}

//! Get the fisheye/dome field-of-view angle in degrees
float sgct::SGCTWindow::getFisheyeFOV()
{
	return mFieldOfView;
}

/*! Get the fisheye crop value for a side:
	- Left
	- Right
	- Bottom
	- Top
*/
float sgct::SGCTWindow::getFisheyeCropValue(FisheyeCropSide side)
{
	return mCropFactors[side];
}

//! Get if fisheye is offaxis (not rendered from centre)
bool sgct::SGCTWindow::isFisheyeOffaxis()
{
	return mFisheyeOffaxis;
}

//! Get the fisheye overlay image filename/path.
const char * sgct::SGCTWindow::getFisheyeOverlay()
{
	return mFisheyeOverlayFilename.empty() ? NULL : mFisheyeOverlayFilename.c_str();
}

std::string sgct::SGCTWindow::getStereoModeStr()
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

