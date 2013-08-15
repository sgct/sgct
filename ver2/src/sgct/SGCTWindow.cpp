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

#ifdef __WIN32__
HDC hDC;
#else // APPLE || LINUX
GLXDrawable hDC;
Display * disp;
#ifdef GLEW_MX
GLXEWContext * glxewGetContext();
#endif
#endif

sgct_core::SGCTWindow::SGCTWindow()
{
	mUseFixResolution = false;
	mUseSwapGroups = false;
	mSwapGroupMaster = false;
	mUseQuadBuffer = false;
	mBarrier = false;
	mFullScreen = false;
	mSetWindowPos = false;
	mDecorated = true;
	mFisheyeMode = false;
	mFisheyeAlpha = false;
	mVisible = true;
	mUseFXAA = false;
	mUsePostFX = false;

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
	mId = -1;
	mShotCounter = 0;

	FisheyeMVP		= -1;
	Cubemap			= -1;
	DepthCubemap	= -1;
	NormalCubemap	= -1;
	FishEyeHalfFov	= -1;
	FisheyeOffset	= -1;
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

	mStereoMode = NoStereo;
	mSwapInterval = 1;
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

void sgct_core::SGCTWindow::close()
{
	makeOpenGLContextCurrent( Shared_Context );

	//delete postFX
	for(std::size_t i=0; i<getNumberOfPostFXs(); i++)
		mPostFXPasses[i].destroy();
	mPostFXPasses.clear();

	if( mScreenCapture )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Deleting screen capture data for window %d...\n", mId);
		delete mScreenCapture;
		mScreenCapture = NULL;
	}

	//delete FBO stuff
	if(mFinalFBO_Ptr != NULL &&
		mCubeMapFBO_Ptr != NULL &&
		sgct::SGCTSettings::Instance()->useFBO() )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Releasing OpenGL buffers for window %d...\n", mId);
		mFinalFBO_Ptr->destroy();
		if( mFisheyeMode )
			mCubeMapFBO_Ptr->destroy();

		delete mFinalFBO_Ptr;
		mFinalFBO_Ptr = NULL;
		delete mCubeMapFBO_Ptr;
		mCubeMapFBO_Ptr = NULL;

		glDeleteTextures(NUMBER_OF_TEXTURES, &mFrameBufferTextures[0]);
	}

	if( mVBO[RenderQuad] )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Deleting VBOs for window %d...\n", mId);
		glDeleteBuffers(NUMBER_OF_VBOS, &mVBO[0]);
	}

	if( mVAO[RenderQuad] )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Deleting VAOs for window %d...\n", mId);
		glDeleteVertexArrays(NUMBER_OF_VBOS, &mVAO[0]);
	}

	//delete shaders
	mFisheyeShader.deleteProgram();
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

void sgct_core::SGCTWindow::init(int id)
{
	mId = id;
	
	if(!mFullScreen)
	{
		if(mSetWindowPos)
			glfwSetWindowPos( mWindowHandle, mWindowPos[0], mWindowPos[1] );
		glfwSetWindowSizeCallback( mWindowHandle, windowResizeCallback );
	}

	//swap the buffers and update the window
	glfwSwapBuffers( mWindowHandle );

	//initNvidiaSwapGroups();
}

/*!
	Init window buffers such as textures, FBOs, VAOs, VBOs and PBOs. 
*/
void sgct_core::SGCTWindow::initOGL()
{
	createTextures();
	createVBOs(); //must be created before FBO
	createFBOs();
	initScreenCapture();
	loadShaders();
}

/*!
	Set the visibility state of this window. If a window is hidden the rendering for that window will be paused.
*/
void sgct_core::SGCTWindow::setVisibility(bool state)
{
	if( state != mVisible )
	{
		state ? glfwShowWindow( mWindowHandle ) : glfwHideWindow( mWindowHandle );
		mVisible = state;
	}
}

/*!
	Set the window title
	@param title The title of the window.
*/
void sgct_core::SGCTWindow::setWindowTitle(const char * title)
{
	glfwSetWindowTitle( mWindowHandle, title );
}

/*!
	Sets the window resolution.
	
	@param x The width of the window in pixels.
	@param y The height of the window in pixels.
*/
void sgct_core::SGCTWindow::setWindowResolution(const int x, const int y)
{
	mWindowRes[0] = x;
	mWindowRes[1] = y;
	mAspectRatio = static_cast<float>( x ) /
			static_cast<float>( y );

	if( !mUseFixResolution )
	{
		mFramebufferResolution[0] = x;
		mFramebufferResolution[1] = y;
	}

	sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "SGCTWindow: Resolution changed to %dx%d for window %d...\n", mWindowRes[0], mWindowRes[1], mId);
}

/*!
	Sets the framebuffer resolution. Theese parameters will only be used if a fixed resolution is used that is different from the window resolution.
	This might be useful in fullscreen mode on Apples retina displays to force 1080p resolution or similar.
	
	@param x The width of the frame buffer in pixels.
	@param y The height of the frame buffer in pixels.
*/
void sgct_core::SGCTWindow::setFramebufferResolution(const int x, const int y)
{
	mFramebufferResolution[0] = x;
	mFramebufferResolution[1] = y;
}

/*!
	Swap previus data and current data. This is done at the end of the render loop.
*/
void sgct_core::SGCTWindow::swap()
{
	if( mVisible )
	{
		mWindowResOld[0] = mWindowRes[0];
		mWindowResOld[1] = mWindowRes[1];

		makeOpenGLContextCurrent( Window_Context );
		glfwSwapBuffers( mWindowHandle );
	}
}

/*!
	Don't use this function if you want to set the window resolution. Use setWindowResolution(const int x, const int y) instead.
	This function is called within sgct when the window is created.
*/
void sgct_core::SGCTWindow::initWindowResolution(const int x, const int y)
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

void sgct_core::SGCTWindow::update()
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
void sgct_core::SGCTWindow::makeOpenGLContextCurrent(OGL_Context context)
{
	if( ClusterManager::Instance()->getThisNodePtr()->getNumberOfWindows() < 2 )
		return;
	
	if( context == Window_Context )
		glfwMakeContextCurrent( mWindowHandle );
	else
		glfwMakeContextCurrent( mSharedHandle );
}

/*!
	\returns true if this window is resized 
*/
bool sgct_core::SGCTWindow::isWindowResized()
{
	return (mWindowRes[0] != mWindowResOld[0] || mWindowRes[1] != mWindowResOld[1]);
}

/*!
	Set this window's position in screen coordinates
	\param x horisontal position in pixels
	\param y vertical position in pixels
*/
void sgct_core::SGCTWindow::setWindowPosition(const int x, const int y)
{
	mWindowPos[0] = x;
	mWindowPos[1] = y;
	mSetWindowPos = true;
}

void sgct_core::SGCTWindow::setWindowMode(bool fullscreen)
{
	mFullScreen = fullscreen;
}

void sgct_core::SGCTWindow::setWindowDecoration(bool state)
{
	mDecorated = state;
}

void sgct_core::SGCTWindow::setFullScreenMonitorIndex( int index )
{
	mMonitorIndex = index;
}

void sgct_core::SGCTWindow::setBarrier(const bool state)
{
//#ifdef __WITHSWAPBARRIERS__

	if( mUseSwapGroups && state != mBarrier)
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "SGCTWindow: Enabling Nvidia swap barrier for window %d...\n", mId);

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
void sgct_core::SGCTWindow::setFixResolution(const bool state)
{
	mUseFixResolution = state;
}

/*!
Set if post effects should be used.
*/
void sgct_core::SGCTWindow::setUsePostFX(bool state)
{
	mUsePostFX = state;
	if( !state )
		mUseFXAA = false;
}

/*!
Set if FXAA should be used.
*/
void sgct_core::SGCTWindow::setUseFXAA(bool state)
{
	mUseFXAA = state;
	mUsePostFX = state;
	//sgct::MessageHandler::Instance()->print("FXAA status: %s\n", state ? "enabled" : "disabled");
}

/*!
	Use nvidia swap groups. This freature is only supported on quadro cards together with a compatible sync card.
	This function can only be used before the window is created.
*/
void sgct_core::SGCTWindow::setUseSwapGroups(const bool state)
{
	mUseSwapGroups = state;
}

/*!
	Use quad buffer (hardware stereoscopic rendering).
	This function can only be used before the window is created. 
	The quad buffer feature is only supported on professional CAD graphics cards such as
	Nvidia Quadro or AMD/ATI FireGL.
*/
void sgct_core::SGCTWindow::setUseQuadbuffer(const bool state)
{
	mUseQuadBuffer = state;
	if( mUseQuadBuffer )
		glfwWindowHint(GLFW_STEREO, GL_TRUE);
}

/*!
	This function is used internally within sgct to open the window.

	/returns True if window was created successfully.
*/
bool sgct_core::SGCTWindow::openWindow(GLFWwindow* share)
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
		if( !sgct::SGCTSettings::Instance()->useFBO() )
		{
			sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "SGCTWindow(%d): Forcing FBOs for fisheye mode!\n", mId);
			sgct::SGCTSettings::Instance()->setUseFBO( true );
		}

		//create the cube mapped viewports
		generateCubeMapViewports();
	}

	int antiAliasingSamples = getNumberOfAASamples();
	if( antiAliasingSamples > 1 && !sgct::SGCTSettings::Instance()->useFBO() ) //if multisample is used
		 glfwWindowHint( GLFW_SAMPLES, antiAliasingSamples );

	int count;
	GLFWmonitor** monitors = glfwGetMonitors(&count);

	/*for(int i=0; i<count; i++)
	{
		int numberOfVideoModes;
		const GLFWvidmode * videoModes = glfwGetVideoModes( monitors[i], &numberOfVideoModes);

		sgct::MessageHandler::Instance()->print("\nMonitor %d '%s' video modes\n========================\n",
			i, glfwGetMonitorName( monitors[i] ) );

		for(int j=0; j<numberOfVideoModes; j++)
			sgct::MessageHandler::Instance()->print("%d x %d @ %d\n", videoModes[j].width, videoModes[j].height, videoModes[j].refreshRate );

		sgct::MessageHandler::Instance()->print("\n");
	}*/

	setUseQuadbuffer( mStereoMode == Active );

	if( mFullScreen )
	{
		if( mMonitorIndex > 0 && mMonitorIndex < count )
			mMonitor = monitors[ mMonitorIndex ];
		else
		{
			mMonitor = glfwGetPrimaryMonitor();
			if( mMonitorIndex >= count )
				sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO,
					"SGCTWindow(%d): Invalid monitor index (%d). This computer has %d monitors.\n",
					mId, mMonitorIndex, count);
		}
	}

	mWindowHandle = glfwCreateWindow(mWindowRes[0], mWindowRes[1], "SGCT", mMonitor, share);
	mWindowInitialRes[0] = mWindowRes[0];
	mWindowInitialRes[1] = mWindowRes[1];
	mVisible = true;
	
	if( mWindowHandle != NULL )
	{
		if( share != NULL )
			mSharedHandle = share;
		else
			mSharedHandle = mWindowHandle;

		glfwMakeContextCurrent( mWindowHandle );
		/*
			Swap inerval:
			-1 = adaptive sync
			0  = vertical sync off
			1  = wait for vertical sync
			2  = fix when using swapgroups in xp and running half the framerate
		*/
		glfwSwapInterval( mSwapInterval );
		
		glfwMakeContextCurrent( mSharedHandle );

		mScreenCapture = new ScreenCapture();
		mFinalFBO_Ptr = new OffScreenBuffer();
		mCubeMapFBO_Ptr = new OffScreenBuffer();

		return true;
	}
	else
		return false;
}

void sgct_core::SGCTWindow::initNvidiaSwapGroups()
{
//#ifdef __WITHSWAPBARRIERS__

#ifdef __WIN32__ //Windows uses wglew.h
	if (wglewIsSupported("WGL_NV_swap_group") && mUseSwapGroups)
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "SGCTWindow(%d): Joining Nvidia swap group.\n", mId);
		
		hDC = wglGetCurrentDC();

		unsigned int maxBarrier = 0;
		unsigned int maxGroup = 0;
		wglQueryMaxSwapGroupsNV( hDC, &maxGroup, &maxBarrier );
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "WGL_NV_swap_group extension is supported.\n\tMax number of groups: %u\n\tMax number of barriers: %u\n",
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
			sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "SGCTWindow(%d): Joining swapgroup 1 [ok].\n", mId);
		else
		{
			sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "SGCTWindow(%d): Joining swapgroup 1 [failed].\n", mId);
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
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "SGCTWindow(%d): Joining Nvidia swap group.\n", mId);
		
		hDC = glXGetCurrentDrawable();
		disp = glXGetCurrentDisplay();

		unsigned int maxBarrier = 0;
		unsigned int maxGroup = 0;
		glXQueryMaxSwapGroupsNV( disp, hDC, &maxGroup, &maxBarrier );
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "GLX_NV_swap_group extension is supported.\n\tMax number of groups: %u\n\tMax number of barriers: %u\n",
			maxGroup, maxBarrier);

		if( glXJoinSwapGroupNV(disp, hDC, 1) )
			sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "SGCTWindow(%d): Joining swapgroup 1 [ok].\n", mId);
		else
		{
			sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "SGCTWindow(%d): Joining swapgroup 1 [failed].\n", mId);
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

void sgct_core::SGCTWindow::windowResizeCallback( GLFWwindow * window, int width, int height )
{
	SGCTNode * thisNode = ClusterManager::Instance()->getThisNodePtr();
	
	//find the correct window to update
	for(std::size_t i=0; i<thisNode->getNumberOfWindows(); i++)
		if( thisNode->getWindowPtr(i)->getWindowHandle() == window )
			thisNode->getWindowPtr(i)->setWindowResolution(width > 0 ? width : 1, height > 0 ? height : 1);
}

void sgct_core::SGCTWindow::initScreenCapture()
{	
	//init PBO in screen capture
	mScreenCapture->init( mId );
	mScreenCapture->setUsePBO( GLEW_EXT_pixel_buffer_object && glfwGetWindowAttrib( mWindowHandle, GLFW_CONTEXT_VERSION_MAJOR) > 1 ); //if supported then use them
	if( mFisheyeMode && !mFisheyeAlpha )
		mScreenCapture->initOrResize( getXFramebufferResolution(), getYFramebufferResolution(), 3 );
	else
		mScreenCapture->initOrResize( getXFramebufferResolution(), getYFramebufferResolution(), 4 );
	if( sgct::SGCTSettings::Instance()->getCaptureFormat() != ScreenCapture::NOT_SET )
		mScreenCapture->setFormat( static_cast<ScreenCapture::CaptureFormat>( sgct::SGCTSettings::Instance()->getCaptureFormat() ) );
}

void sgct_core::SGCTWindow::getSwapGroupFrameNumber(unsigned int &frameNumber)
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

void sgct_core::SGCTWindow::resetSwapGroupFrameNumber()
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
			sgct::MessageHandler::Instance()->print( sgct::MessageHandler::NOTIFY_INFO, "Resetting frame counter. This computer is the master.\n");
		}
		else
		{
			mSwapGroupMaster = false;
			sgct::MessageHandler::Instance()->print( sgct::MessageHandler::NOTIFY_INFO, "Resetting frame counter failed. This computer is the slave.\n");
		}
	}

//#endif
}

/*!
	Returns if fisheye rendering is active in this window
*/
bool sgct_core::SGCTWindow::isUsingFisheyeRendering()
{
	return mFisheyeMode;
}

/*!
	This function creates textures that will act as FBO targets.
*/
void sgct_core::SGCTWindow::createTextures()
{
	//no target textures needed if not using FBO
	if( !sgct::SGCTSettings::Instance()->useFBO() )
		return;

	if( sgct::Engine::Instance()->getRunMode() <= sgct::Engine::OpenGL_Compablity_Profile )
	{
		glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT);
		glEnable(GL_TEXTURE_2D);
	}

	//allocate
	glGenTextures(NUMBER_OF_TEXTURES, &mFrameBufferTextures[0]);

	/*
		Create left and right color & depth textures.
	*/
	//don't allocate the right eye image if stereo is not used
	//create a postFX texture for effects
	for( int i=0; i<(NUMBER_OF_TEXTURES-3); i++ ) //all textures except fisheye cubemap(s)
	{
		bool create = true;
		
		switch( i )
		{
		case sgct::Engine::RightEye:
			if( mStereoMode == NoStereo )
				create = false;
			break;

		case sgct::Engine::Depth:
			if( !sgct::SGCTSettings::Instance()->useDepthTexture() )
				create = false;
			break;

		case sgct::Engine::FX1:
			if( mPostFXPasses.empty() )
				create = false;
			break;

		case sgct::Engine::FX2:
			if( mPostFXPasses.size() < 2 )
				create = false;
			break;
		}
		
		if( create )
		{
			glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[i]);
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); //must be linear if warping, blending or fix resolution is used
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			
			if( i == sgct::Engine::Depth )
			{
				//glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
				//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
				
				glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, mFramebufferResolution[0], mFramebufferResolution[1], 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
				sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "%dx%d depth texture (id: %d) generated for window %d!\n",
					mFramebufferResolution[0], mFramebufferResolution[1], mFrameBufferTextures[i], mId);
			}
			else
			{
				glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, mFramebufferResolution[0], mFramebufferResolution[1], 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
				sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "%dx%d RGBA texture (id: %d) generated for window %d!\n",
					mFramebufferResolution[0], mFramebufferResolution[1], mFrameBufferTextures[i], mId);
			}
		}
	}
	/*
		Create cubemap texture for fisheye rendering if enabled.
	*/
	if( mFisheyeMode )
	{
		GLint MaxCubeMapRes;
		glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &MaxCubeMapRes);
		if(mCubeMapResolution > MaxCubeMapRes)
		{
			mCubeMapResolution = MaxCubeMapRes;
			sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Info: Cubemap size set to max size: %d\n", MaxCubeMapRes);
		}

		//set up texture target
		glBindTexture(GL_TEXTURE_CUBE_MAP, mFrameBufferTextures[ sgct::Engine::CubeMap ]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		for (int i = 0; i < 6; ++i)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, mCubeMapResolution, mCubeMapResolution, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "%dx%d cube map texture (id: %d) generated for window %d!\n",
			mCubeMapResolution, mCubeMapResolution, mFrameBufferTextures[ sgct::Engine::CubeMap ], mId);

		if( sgct::SGCTSettings::Instance()->useDepthTexture() )
		{
			//set up texture target
			glBindTexture(GL_TEXTURE_CUBE_MAP, mFrameBufferTextures[ sgct::Engine::CubeMapDepth ]);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			for (int i = 0; i < 6; ++i)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT32, mCubeMapResolution, mCubeMapResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "%dx%d depth cube map texture (id: %d) generated for window %d!\n",
				mCubeMapResolution, mCubeMapResolution, mFrameBufferTextures[ sgct::Engine::CubeMapDepth ], mId);
		}
	}

	if( sgct::Engine::Instance()->getRunMode() <= sgct::Engine::OpenGL_Compablity_Profile )
		glPopAttrib();

	if( sgct::Engine::checkForOGLErrors() )
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Texture targets initiated successfully for window %d!\n", mId);
	else
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Texture targets failed to initialize for window %d!\n", mId);
}

/*!
	This function creates FBOs if they are supported.
	This is done in the initOGL function.
*/
void sgct_core::SGCTWindow::createFBOs()
{
	if( !sgct::SGCTSettings::Instance()->useFBO() )
	{
		//disable anaglyph & checkerboard stereo if FBOs are not used
		if( mStereoMode > Active )
			mStereoMode = NoStereo;
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Warning! FBO rendering is not supported or enabled!\nPostFX, fisheye and some stereo modes are disabled.\n");
	}
	else
	{
		if( mFisheyeMode )
		{
			mFinalFBO_Ptr->createFBO( mFramebufferResolution[0],
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
					mCubeMapFBO_Ptr->attachCubeMapTexture( mFrameBufferTextures[sgct::Engine::CubeMap], i );
				}

				mFisheyeAlpha ? glClearColor(0.0f, 0.0f, 0.0f, 0.0f) : glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

				//copy AA-buffer to "regular"/non-AA buffer
				if( mCubeMapFBO_Ptr->isMultiSampled() )
				{
					mCubeMapFBO_Ptr->bindBlit(); //bind separate read and draw buffers to prepare blit operation

					//update attachments
					mCubeMapFBO_Ptr->attachCubeMapTexture( mFrameBufferTextures[sgct::Engine::CubeMap], i );

					mCubeMapFBO_Ptr->blit();
				}
			}

			OffScreenBuffer::unBind();

			sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Window %d: Initial cube map rendered.\n", mId);

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

		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Window %d: FBO initiated successfully! Number of samples: %d\n", mId, mNumberOfAASamples);
	}
}

/*!
	Create vertex buffer objects used to render framebuffer quad
*/
void sgct_core::SGCTWindow::createVBOs()
{
	if( !sgct::Engine::Instance()->isOGLPipelineFixed() )
	{
		glGenVertexArrays(NUMBER_OF_VBOS, &mVAO[0]);

		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "SGCTWindow: Generating VAOs: ");
		for( unsigned int i=0; i<NUMBER_OF_VBOS; i++ )
			sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "%d ", mVAO[i]);
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "\n");
	}

	glGenBuffers(NUMBER_OF_VBOS, &mVBO[0]);

	sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "SGCTWindow: Generating VBOs: ");
	for( unsigned int i=0; i<NUMBER_OF_VBOS; i++ )
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "%d ", mVBO[i]);
	sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "\n");

	if( !sgct::Engine::Instance()->isOGLPipelineFixed() )
		glBindVertexArray( mVAO[RenderQuad] );
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[RenderQuad]);
	glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), mQuadVerts, GL_STATIC_DRAW); //2TF + 3VF = 2*4 + 3*4 = 20
	if( !sgct::Engine::Instance()->isOGLPipelineFixed() )
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

	if( !sgct::Engine::Instance()->isOGLPipelineFixed() )
		glBindVertexArray( mVAO[FishEyeQuad] );
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[FishEyeQuad]);
	glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), mFisheyeQuadVerts, GL_STREAM_DRAW); //2TF + 3VF = 2*4 + 3*4 = 20
	if( !sgct::Engine::Instance()->isOGLPipelineFixed() )
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
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if( !sgct::Engine::Instance()->isOGLPipelineFixed() )
		glBindVertexArray(0);
}

void sgct_core::SGCTWindow::loadShaders()
{
	//load shaders
	if( mFisheyeMode )
	{
		if( sgct::Engine::Instance()->isOGLPipelineFixed() )
		{
			mFisheyeShader.setVertexShaderSrc( sgct_core::shaders::Fisheye_Vert_Shader, sgct::ShaderProgram::SHADER_SRC_STRING );
			
			if(mFisheyeOffaxis)
			{
				if( sgct::SGCTSettings::Instance()->useDepthTexture() )
					mFisheyeShader.setFragmentShaderSrc( sgct_core::shaders::Fisheye_Frag_Shader_OffAxis_Depth, sgct::ShaderProgram::SHADER_SRC_STRING );
				else
					mFisheyeShader.setFragmentShaderSrc( sgct_core::shaders::Fisheye_Frag_Shader_OffAxis, sgct::ShaderProgram::SHADER_SRC_STRING );
			}
			else
			{
				if( sgct::SGCTSettings::Instance()->useDepthTexture() )
					mFisheyeShader.setFragmentShaderSrc( sgct_core::shaders::Fisheye_Frag_Shader_Depth, sgct::ShaderProgram::SHADER_SRC_STRING );
				else
					mFisheyeShader.setFragmentShaderSrc( sgct_core::shaders::Fisheye_Frag_Shader, sgct::ShaderProgram::SHADER_SRC_STRING );
			}
		}
		else //modern pipeline
		{
			mFisheyeShader.setVertexShaderSrc( sgct_core::shaders_modern::Fisheye_Vert_Shader, sgct::ShaderProgram::SHADER_SRC_STRING );
			
			if(mFisheyeOffaxis)
			{
				if( sgct::SGCTSettings::Instance()->useDepthTexture() )
				{
					mFisheyeShader.setFragmentShaderSrc( sgct_core::shaders_modern::Fisheye_Frag_Shader_OffAxis_Depth, sgct::ShaderProgram::SHADER_SRC_STRING );
				}
				else
				{
					mFisheyeShader.setFragmentShaderSrc( sgct_core::shaders_modern::Fisheye_Frag_Shader_OffAxis, sgct::ShaderProgram::SHADER_SRC_STRING );
				}
			}
			else//not off axis
			{
				if( sgct::SGCTSettings::Instance()->useDepthTexture() )
				{
					mFisheyeShader.setFragmentShaderSrc( sgct_core::shaders_modern::Fisheye_Frag_Shader_Depth, sgct::ShaderProgram::SHADER_SRC_STRING );
				}
				else //no depth
				{
					mFisheyeShader.setFragmentShaderSrc( sgct_core::shaders_modern::Fisheye_Frag_Shader, sgct::ShaderProgram::SHADER_SRC_STRING );
				}
			}
		}

		mFisheyeShader.setName("FisheyeShader");
		mFisheyeShader.createAndLinkProgram();
		mFisheyeShader.bind();

		if( !sgct::Engine::Instance()->isOGLPipelineFixed() )
		{
			FisheyeMVP = mFisheyeShader.getUniformLocation( "MVP" );
		}

		Cubemap = mFisheyeShader.getUniformLocation( "cubemap" );
		glUniform1i( Cubemap, 0 );

		if( sgct::SGCTSettings::Instance()->useDepthTexture() )
		{
			DepthCubemap = mFisheyeShader.getUniformLocation( "depthmap" );
			glUniform1i( DepthCubemap, 1 );
		}

		FishEyeHalfFov = mFisheyeShader.getUniformLocation( "halfFov" );
		glUniform1f( FishEyeHalfFov, glm::half_pi<float>() );

		if( mFisheyeOffaxis )
		{
			FisheyeOffset = mFisheyeShader.getUniformLocation( "offset" );
			glUniform3f( FisheyeOffset,
				mFisheyeBaseOffset[0] + mFisheyeOffset[0],
				mFisheyeBaseOffset[1] + mFisheyeOffset[1],
				mFisheyeBaseOffset[2] + mFisheyeOffset[2] );
		}
		sgct::ShaderProgram::unbind();
	}

	//-------------- end fisheye shader ----------->

	if( mStereoMode > Active )
	{
		if( sgct::Engine::Instance()->isOGLPipelineFixed() )
			mStereoShader.setVertexShaderSrc( sgct_core::shaders::Anaglyph_Vert_Shader, sgct::ShaderProgram::SHADER_SRC_STRING );
		else
			mStereoShader.setVertexShaderSrc( sgct_core::shaders_modern::Anaglyph_Vert_Shader, sgct::ShaderProgram::SHADER_SRC_STRING );
			
		if( mStereoMode == Anaglyph_Red_Cyan )
		{
			sgct::Engine::Instance()->isOGLPipelineFixed() ?
				mStereoShader.setFragmentShaderSrc(sgct_core::shaders::Anaglyph_Red_Cyan_Frag_Shader, sgct::ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.setFragmentShaderSrc(sgct_core::shaders_modern::Anaglyph_Red_Cyan_Frag_Shader, sgct::ShaderProgram::SHADER_SRC_STRING );
					
		}
		else if( mStereoMode == Anaglyph_Amber_Blue )
		{
			sgct::Engine::Instance()->isOGLPipelineFixed() ?
				mStereoShader.setFragmentShaderSrc(sgct_core::shaders::Anaglyph_Amber_Blue_Frag_Shader, sgct::ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.setFragmentShaderSrc(sgct_core::shaders_modern::Anaglyph_Amber_Blue_Frag_Shader, sgct::ShaderProgram::SHADER_SRC_STRING );
		}
		else if( mStereoMode == Anaglyph_Red_Cyan_Wimmer )
		{
			sgct::Engine::Instance()->isOGLPipelineFixed() ?
				mStereoShader.setFragmentShaderSrc(sgct_core::shaders::Anaglyph_Red_Cyan_Frag_Shader_Wimmer, sgct::ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.setFragmentShaderSrc(sgct_core::shaders_modern::Anaglyph_Red_Cyan_Frag_Shader_Wimmer, sgct::ShaderProgram::SHADER_SRC_STRING );
		}
		else if( mStereoMode == Checkerboard )
		{
			sgct::Engine::Instance()->isOGLPipelineFixed() ?
				mStereoShader.setFragmentShaderSrc(sgct_core::shaders::CheckerBoard_Frag_Shader, sgct::ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.setFragmentShaderSrc(sgct_core::shaders_modern::CheckerBoard_Frag_Shader, sgct::ShaderProgram::SHADER_SRC_STRING );
		}
		else if( mStereoMode == Checkerboard_Inverted )
		{
			sgct::Engine::Instance()->isOGLPipelineFixed() ?
				mStereoShader.setFragmentShaderSrc(sgct_core::shaders::CheckerBoard_Inverted_Frag_Shader, sgct::ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.setFragmentShaderSrc(sgct_core::shaders_modern::CheckerBoard_Inverted_Frag_Shader, sgct::ShaderProgram::SHADER_SRC_STRING );
		}
		else if( mStereoMode == Vertical_Interlaced )
		{
			sgct::Engine::Instance()->isOGLPipelineFixed() ?
				mStereoShader.setFragmentShaderSrc(sgct_core::shaders::Vertical_Interlaced_Frag_Shader, sgct::ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.setFragmentShaderSrc(sgct_core::shaders_modern::Vertical_Interlaced_Frag_Shader, sgct::ShaderProgram::SHADER_SRC_STRING );
		}
		else if( mStereoMode == Vertical_Interlaced_Inverted )
		{
			sgct::Engine::Instance()->isOGLPipelineFixed() ?
				mStereoShader.setFragmentShaderSrc(sgct_core::shaders::Vertical_Interlaced_Inverted_Frag_Shader, sgct::ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.setFragmentShaderSrc(sgct_core::shaders_modern::Vertical_Interlaced_Inverted_Frag_Shader, sgct::ShaderProgram::SHADER_SRC_STRING );
		}
		else
		{
			sgct::Engine::Instance()->isOGLPipelineFixed() ?
				mStereoShader.setFragmentShaderSrc(sgct_core::shaders::Dummy_Stereo_Frag_Shader, sgct::ShaderProgram::SHADER_SRC_STRING ) :
				mStereoShader.setFragmentShaderSrc(sgct_core::shaders_modern::Dummy_Stereo_Frag_Shader, sgct::ShaderProgram::SHADER_SRC_STRING );
		}

		mStereoShader.setName("StereoShader");
		mStereoShader.createAndLinkProgram();
		mStereoShader.bind();
		if( !sgct::Engine::Instance()->isOGLPipelineFixed() )
			StereoMVP = mStereoShader.getUniformLocation( "MVP" );
		StereoLeftTex = mStereoShader.getUniformLocation( "LeftTex" );
		StereoRightTex = mStereoShader.getUniformLocation( "RightTex" );
		glUniform1i( StereoLeftTex, 0 );
		glUniform1i( StereoRightTex, 1 );
		sgct::ShaderProgram::unbind();
	}
}

void sgct_core::SGCTWindow::bindVAO()
{
	mFisheyeMode ? glBindVertexArray( mVAO[FishEyeQuad] ) : glBindVertexArray( mVAO[RenderQuad] );
}

void sgct_core::SGCTWindow::bindVAO( VBOIndex index )
{
	glBindVertexArray( mVAO[ index ] );
}

void sgct_core::SGCTWindow::bindVBO()
{
	mFisheyeMode ? glBindBuffer(GL_ARRAY_BUFFER, mVBO[FishEyeQuad]) : glBindBuffer(GL_ARRAY_BUFFER, mVBO[RenderQuad]);
}

void sgct_core::SGCTWindow::bindVBO( VBOIndex index )
{
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[index]);
}

void sgct_core::SGCTWindow::unbindVBO()
{
	glBindBuffer(GL_ARRAY_BUFFER, GL_FALSE);
}

void sgct_core::SGCTWindow::unbindVAO()
{
	glBindVertexArray( GL_FALSE );
}

/*!
	This functions saves a png screenshot or a stereoscopic pair in the current working directory.
	All screenshots are numbered so this function can be called several times whitout overwriting previous screenshots.

	The PNG images are saved as RGBA images with transparancy. Alpha is taken from the clear color alpha.
*/
void sgct_core::SGCTWindow::captureBuffer()
{
	if(mStereoMode == NoStereo)
	{
		if( sgct::SGCTSettings::Instance()->useFBO() )
			mScreenCapture->SaveScreenCapture( mFrameBufferTextures[sgct::Engine::LeftEye], mShotCounter, ScreenCapture::FBO_Texture );
		else
			mScreenCapture->SaveScreenCapture( 0, mShotCounter, ScreenCapture::Front_Buffer );
	}
	else
	{
		if( sgct::SGCTSettings::Instance()->useFBO() )
		{
			mScreenCapture->SaveScreenCapture( mFrameBufferTextures[sgct::Engine::LeftEye], mShotCounter, ScreenCapture::FBO_Left_Texture );
			mScreenCapture->SaveScreenCapture( mFrameBufferTextures[sgct::Engine::RightEye], mShotCounter, ScreenCapture::FBO_Right_Texture );
		}
		else
		{
			mScreenCapture->SaveScreenCapture( 0, mShotCounter, ScreenCapture::Left_Front_Buffer );
			mScreenCapture->SaveScreenCapture( 0, mShotCounter, ScreenCapture::Right_Front_Buffer );
		}
	}

	mShotCounter++;
}

/*!
	Returns pointer to FBO container
*/
sgct_core::OffScreenBuffer * sgct_core::SGCTWindow::getFBOPtr()
{
	return mFisheyeMode ? mCubeMapFBO_Ptr : mFinalFBO_Ptr;
}

/*!
	Get the width and height of FBO in pixels
*/
void sgct_core::SGCTWindow::getFBODimensions( int & width, int & height )
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
	Add a post effect for this window
*/
void sgct_core::SGCTWindow::addPostFX( sgct::PostFX & fx )
{
	mPostFXPasses.push_back( fx );
}

/*!
	This function resizes the FBOs when the window is resized to achive 1:1 pixel-texel mapping.
*/
void sgct_core::SGCTWindow::resizeFBOs()
{
	if(!mUseFixResolution && sgct::SGCTSettings::Instance()->useFBO())
	{
		makeOpenGLContextCurrent( Shared_Context );
		glDeleteTextures(NUMBER_OF_TEXTURES, &mFrameBufferTextures[0]);
		createTextures();

		if( mFisheyeMode )
		{
			mFinalFBO_Ptr->resizeFBO( mFramebufferResolution[0],
			mFramebufferResolution[0],
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

		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_INFO, "FBOs resized successfully!\n");
	}
}

/*!
	Init the fisheye data by creating geometry and precalculate textures
*/
void sgct_core::SGCTWindow::initFisheye()
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
	if( !sgct::Engine::Instance()->isOGLPipelineFixed() )
		glBindVertexArray( mVAO[FishEyeQuad] );
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[FishEyeQuad]);

	GLvoid* PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	memcpy(PositionBuffer, mFisheyeQuadVerts, 20 * sizeof(float));
	glUnmapBuffer(GL_ARRAY_BUFFER);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if( !sgct::Engine::Instance()->isOGLPipelineFixed() )
		glBindVertexArray( 0 );
}

/*!
	Set if fisheye rendering is used in this window
*/
void sgct_core::SGCTWindow::setFisheyeRendering(bool state)
{
	mFisheyeMode = state;
}

sgct_core::SGCTWindow::StereoMode sgct_core::SGCTWindow::getStereoMode()
{
	return mStereoMode;
}

void sgct_core::SGCTWindow::addViewport(float left, float right, float bottom, float top)
{
	Viewport tmpVP(left, right, bottom, top);
	mViewports.push_back(tmpVP);
	sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Adding viewport (total %d)\n", mViewports.size());
}

void sgct_core::SGCTWindow::addViewport(sgct_core::Viewport &vp)
{
	mViewports.push_back(vp);
	sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Adding viewport (total %d)\n", mViewports.size());
}

/*!
	Clears the vector containing all viewport data.
*/
void sgct_core::SGCTWindow::deleteAllViewports()
{
	mCurrentViewportIndex = 0;
	mViewports.clear();
}

/*!
	Generates six viewports that renders the inside of a cube. The method used in SGCT is to only use four faces by rotating the cube 45 degrees.
*/
void sgct_core::SGCTWindow::generateCubeMapViewports()
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
		Viewport tmpVP;
		
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
			ClusterManager::Instance()->getUserPtr()->getXPos(),
			ClusterManager::Instance()->getUserPtr()->getYPos(),
			ClusterManager::Instance()->getUserPtr()->getZPos(),
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
		//sgct::MessageHandler::Instance()->print("Setting fisheye overlay to '%s'\n", SGCTSettings::Instance()->getFisheyeOverlay());
	}
}

sgct_core::Viewport * sgct_core::SGCTWindow::getCurrentViewport()
{
	return &mViewports[mCurrentViewportIndex];
}

sgct_core::Viewport * sgct_core::SGCTWindow::getViewport(std::size_t index)
{
	return &mViewports[index];
}

void sgct_core::SGCTWindow::getCurrentViewportPixelCoords(int &x, int &y, int &xSize, int &ySize)
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

std::size_t sgct_core::SGCTWindow::getNumberOfViewports()
{
	return mViewports.size();
}

void sgct_core::SGCTWindow::setNumberOfAASamples(int samples)
{
	mNumberOfAASamples = samples;
}

int sgct_core::SGCTWindow::getNumberOfAASamples()
{
	return mNumberOfAASamples;
}

/*!
Set the cubemap resolution used in the fisheye renderer

@param res resolution of the cubemap sides (should be a power of two for best performance)
*/
void sgct_core::SGCTWindow::setCubeMapResolution(int res)
{
	mCubeMapResolution = res;
}

/*!
Set the dome diameter used in the fisheye renderer (used for the viewplane distance calculations)

@param size size of the dome diameter (cube side) in meters
*/
void sgct_core::SGCTWindow::setDomeDiameter(float size)
{
	mCubeMapSize = size;
}

/*!
Set if fisheye alpha state. Should only be set using XML config of before calling Engine::init.
*/
void sgct_core::SGCTWindow::setFisheyeAlpha(bool state)
{
	mFisheyeAlpha = state;
}

/*!
Set the fisheye/dome tilt angle used in the fisheye renderer.
The tilt angle is from the horizontal.

@param angle the tilt angle in degrees
*/
void sgct_core::SGCTWindow::setFisheyeTilt(float angle)
{
	mFisheyeTilt = angle;
}

/*!
Set the fisheye/dome field-of-view angle used in the fisheye renderer.

@param angle the FOV angle in degrees
*/
void sgct_core::SGCTWindow::setFisheyeFOV(float angle)
{
	mFieldOfView = angle;
}

/*!
Set the fisheye crop values. Theese values are used when rendering content for a single projector dome.
The elumenati geodome has usually a 4:3 SXGA+ (1400x1050) projector and the fisheye is cropped 25% (350 pixels) at the top.
*/
void sgct_core::SGCTWindow::setFisheyeCropValues(float left, float right, float bottom, float top)
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
void sgct_core::SGCTWindow::setFisheyeBaseOffset(float x, float y, float z )
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
void sgct_core::SGCTWindow::setFisheyeOffset(float x, float y, float z)
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
void sgct_core::SGCTWindow::setFisheyeOverlay(std::string filename)
{
	mFisheyeOverlayFilename.assign(filename);
}

//! Get the cubemap size in pixels used in the fisheye renderer
int sgct_core::SGCTWindow::getCubeMapResolution()
{
	return mCubeMapResolution;
}

//! Get the dome diameter in meters used in the fisheye renderer
float sgct_core::SGCTWindow::getDomeDiameter()
{
	return mCubeMapSize;
}

//! Get the fisheye/dome tilt angle in degrees
float sgct_core::SGCTWindow::getFisheyeTilt()
{
	return mFisheyeTilt;
}

//! Get the fisheye/dome field-of-view angle in degrees
float sgct_core::SGCTWindow::getFisheyeFOV()
{
	return mFieldOfView;
}

/*! Get the fisheye crop value for a side:
	- Left
	- Right
	- Bottom
	- Top
*/
float sgct_core::SGCTWindow::getFisheyeCropValue(FisheyeCropSide side)
{
	return mCropFactors[side];
}

//! Get if fisheye is offaxis (not rendered from centre)
bool sgct_core::SGCTWindow::isFisheyeOffaxis()
{
	return mFisheyeOffaxis;
}

//! Get the fisheye overlay image filename/path.
const char * sgct_core::SGCTWindow::getFisheyeOverlay()
{
	return mFisheyeOverlayFilename.empty() ? NULL : mFisheyeOverlayFilename.c_str();
}

