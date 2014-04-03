/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifdef __WIN32__
	#define WIN32_LEAN_AND_MEAN
	//prevent conflict between max() in limits.h and max script in windef.h
	#define NOMINMAX
#endif

#include "../include/sgct/Engine.h"
#include "../include/sgct/freetype.h"
#include "../include/sgct/FontManager.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/TextureManager.h"
#include "../include/sgct/SharedData.h"
#include "../include/sgct/shaders/SGCTInternalShaders.h"
#include "../include/sgct/shaders/SGCTInternalShaders_modern.h"
#include "../include/sgct/shaders/SGCTInternalFisheyeShaders.h"
#include "../include/sgct/shaders/SGCTInternalFisheyeShaders_modern.h"
#include "../include/sgct/SGCTVersion.h"
#include "../include/sgct/SGCTSettings.h"
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/ShaderManager.h"
#include "../include/sgct/helpers/SGCTStringFunctions.h"
#include "../include/external/tinythread.h"
#include <glm/gtc/constants.hpp>
#include <math.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <deque>

//#define __SGCT_RENDER_LOOP_DEBUG__

using namespace sgct_core;

sgct::Engine * sgct::Engine::mInstance = NULL;

//Callback wrappers for GLFW
void (*gKeyboardCallbackFn)(int, int) = NULL;
void (*gCharCallbackFn)(unsigned int) = NULL;
void (*gMouseButtonCallbackFn)(int, int) = NULL;
void (*gMousePosCallbackFn)(double, double) = NULL;
void (*gMouseScrollCallbackFn)(double, double) = NULL;

void updateFrameLockLoop(void * arg);
static bool sRunUpdateFrameLockLoop = true;

#ifdef GLEW_MX
GLEWContext * glewGetContext();
#endif

#define USE_SLEEP_TO_WAIT_FOR_NODES 0
#define MAX_SGCT_PATH_LENGTH 512
#define FRAME_LOCK_TIMEOUT 100 //ms
#define RUN_FRAME_LOCK_CHECK_THREAD 1

/*!
This is the only valid constructor that also initiates [GLFW](http://www.glfw.org/). Command line parameters are used to load a configuration file and settings.
Note that parameter with one '\-' are followed by arguments but parameters with '\-\-' are just options without arguments.

Parameter     | Description
------------- | -------------
-config <filename> | set xml confiuration file
-logPath <filepath> | set log file path
--help | display help message and exit
-local <integer> | set which node in configuration that is the localhost (index starts at 0)
--client | run the application as client (only available when running as local)
--slave | run the application as client (only available when running as local)
--debug | set the notify level of messagehandler to debug
--Firm-Sync | enable firm frame sync
--Loose-Sync | disable firm frame sync
--Ignore-Sync | disable frame sync
-notify <integer> | set the notify level used in the MessageHandler (0 = highest priority)
--No-FBO | disable frame buffer objects (some stereo modes, Multi-Window rendering, FXAA and fisheye rendering will be disabled)
--Capture-PNG | use png images for screen capture (default)
--Capture-TGA | use tga images for screen capture
-MSAA <integer> | Enable MSAA as default (argument must be a power of two)
--FXAA | Enable FXAA as default
--gDebugger | Force textures to be genareted using glTexImage2D instead of glTexStorage2D
-numberOfCaptureThreads <integer> | set the maximum amount of threads that should be used during framecapture (default 8)

*/
sgct::Engine::Engine( int& argc, char**& argv )
{
	//init pointers
	mInstance = this;
	mNetworkConnections = NULL;
	mConfig = NULL;
	mRunMode = Default_Mode;
	mStatistics = NULL;
	mThisNode = NULL;

	//init function pointers
	mDrawFn = NULL;
	mDraw2DFn = NULL;
	mPreSyncFn = NULL;
	mPostSyncPreDrawFn = NULL;
	mPostDrawFn = NULL;
	mPreWindowFn = NULL;
	mInitOGLFn = NULL;
	mClearBufferFn = NULL;
	mCleanUpFn = NULL;
	mInternalDrawFn = NULL;
	mInternalRenderFBOFn = NULL;
	mInternalDrawOverlaysFn = NULL;
	mInternalRenderPostFXFn = NULL;
	mInternalRenderFisheyeFn = NULL;
	mNetworkMessageCallbackFn = NULL;
	mNetworkStatusCallbackFn = NULL;
    mScreenShotFn = NULL;
	mThreadPtr = NULL;

	mTerminate = false;
	mIgnoreSync = false;
	mRenderingOffScreen = false;
	mFixedOGLPipeline = true;
	mHelpMode = false;

	localRunningMode = NetworkManager::Remote;

	currentViewportCoords[0] = 0;
	currentViewportCoords[1] = 0;
	currentViewportCoords[2] = 640;
	currentViewportCoords[3] = 480;

	for(unsigned int i=0; i<MAX_UNIFORM_LOCATIONS; i++)
		mShaderLocs[i] = -1;

	setClearBufferFunction( clearBuffer );
	mNearClippingPlaneDist = 0.1f;
	mFarClippingPlaneDist = 100.0f;
	mClearColor[0] = 0.0f;
	mClearColor[1] = 0.0f;
	mClearColor[2] = 0.0f;
	mClearColor[3] = 1.0f;

	mFisheyeClearColor[0] = 0.3f;
	mFisheyeClearColor[1] = 0.3f;
	mFisheyeClearColor[2] = 0.3f;
	mFisheyeClearColor[3] = 1.0f;

	mShowInfo = false;
	mShowGraph = false;
	mShowWireframe = false;
	mTakeScreenshot = false;
	mActiveFrustumMode = Frustum::Mono;
	mFrameCounter = 0;
    mShotCounter = 0;
    mTimerID = 0;
	mExitKey = GLFW_KEY_ESCAPE;

	//parse needs to be before read config since the path to the XML is parsed here
	parseArguments( argc, argv );

	if(!mHelpMode)
	{
		// Initialize GLFW
		glfwSetErrorCallback( internal_glfw_error_callback );
		if( !glfwInit() )
		{
			mTerminate = true;
		}
	}
}

/*!
Engine destructor destructs GLFW and releases resources/memory.
*/
sgct::Engine::~Engine()
{
	clean();
}

/*!
Engine initiation that:
 1. Parse the configuration file
 2. Set up the network communication
 3. Create window(s)
 4. Set up OpenGL
	4.1 Create textures
	4.2 Init FBOs
	4.3 Init VBOs
	4.4 Init PBOs

 @param rm rm is the optional run mode.
*/
bool sgct::Engine::init(RunMode rm)
{
	mRunMode = rm;

	MessageHandler::instance()->print(MessageHandler::NOTIFY_VERSION_INFO, "%s\n", getSGCTVersion().c_str() );

	if(mHelpMode)
		return false;

	if(mTerminate)
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Failed to init GLFW! Application will close in 5 seconds.\n");
		sleep( 5.0 );
		return false;
	}

	mConfig = new ReadConfig( configFilename );
	if( !mConfig->isValid() ) //fatal error
	{
		outputHelpMessage();
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Error in xml config file parsing. Application will close in 5 seconds.\n");
		sleep( 5.0 );

		return false;
	}

	if( !initNetwork() )
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Network init error. Application will close in 5 seconds.\n");
		sleep( 5.0 );
		return false;
	}

	if( !initWindows() )
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Window init error. Application will close in 5 seconds.\n");
		sleep( 5.0 );
		return false;
	}

	//if a single node, skip syncing
	if(ClusterManager::instance()->getNumberOfNodes() == 1)
		mIgnoreSync = true;

	for(std::size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
	{
		if( gKeyboardCallbackFn != NULL )
			glfwSetKeyCallback( getWindowPtr(i)->getWindowHandle(), internal_key_callback );
		if( gMouseButtonCallbackFn != NULL )
			glfwSetMouseButtonCallback( getWindowPtr(i)->getWindowHandle(), internal_mouse_button_callback );
		if( gMousePosCallbackFn != NULL )
			glfwSetCursorPosCallback( getWindowPtr(i)->getWindowHandle(), internal_mouse_pos_callback );
		if( gCharCallbackFn != NULL )
			glfwSetCharCallback( getWindowPtr(i)->getWindowHandle(), internal_key_char_callback );
		if( gMouseScrollCallbackFn != NULL )
			glfwSetScrollCallback( getWindowPtr(i)->getWindowHandle(), internal_mouse_scroll_callback );
	}

	initOGL();

	//start sampling tracking data
	if(isMaster())
		getTrackingManager()->startSampling();

	return true;
}

/*!
	Terminates SGCT.
*/
void sgct::Engine::terminate()
{
	mTerminate = true;
}

/*!
Initiates network communication.
*/
bool sgct::Engine::initNetwork()
{
	try
	{
		mNetworkConnections = new NetworkManager(localRunningMode);

	}
	catch(const char * err)
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Initiating network connections failed! Error: '%s'\n", err);
		return false;
	}

	//check in cluster configuration which it is
	if( localRunningMode == NetworkManager::Remote )
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "Matching ip address to find node in configuration...\n");
		mNetworkConnections->retrieveNodeId();
	}
	else
		MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "Running locally as node %d\n", ClusterManager::instance()->getThisNodeId());

    //If the user has provided the node id as an incorrect cmd argument then make the mThisNode invalid
	if(ClusterManager::instance()->getThisNodeId() >= static_cast<int>(ClusterManager::instance()->getNumberOfNodes()) ||
       ClusterManager::instance()->getThisNodeId() < 0)
        mThisNode = NULL;
    else
        mThisNode = ClusterManager::instance()->getThisNodePtr(); //Set node pointer

	//if any error occured
	if( mThisNode == NULL ) //fatal error
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "This computer is not a part of the cluster configuration!\n");
		mNetworkConnections->close();
		return false;
	}
    
    //set logfile path
    if( !mLogfilePath.empty() )
    {
        MessageHandler::instance()->setLogPath( mLogfilePath.c_str(), ClusterManager::instance()->getThisNodeId() );
        MessageHandler::instance()->setLogToFile(true);
    }

	//Set message handler to send messages or not
	//MessageHandler::instance()->setSendFeedbackToServer( !mNetworkConnections->isComputerServer() );

	if(!mNetworkConnections->init())
		return false;

    return true;
}

/*!
Create and initiate a window.
*/
bool sgct::Engine::initWindows()
{
	if( mThisNode->getNumberOfWindows() == 0 )
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "No windows exist in configuration!\n");
		return false;
	}

	int tmpGlfwVer[3];
    glfwGetVersion( &tmpGlfwVer[0], &tmpGlfwVer[1], &tmpGlfwVer[2] );
	MessageHandler::instance()->print(MessageHandler::NOTIFY_VERSION_INFO, "Using GLFW version %d.%d.%d.\n",
                                         tmpGlfwVer[0],
                                         tmpGlfwVer[1],
                                         tmpGlfwVer[2]);

	switch( mRunMode )
	{
	case OpenGL_3_3_Core_Profile:
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#if __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glewExperimental = true; // Needed for core profile
			mGLSLVersion.assign("#version 330 core");
		}
		break;

	case OpenGL_4_0_Core_Profile:
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#if __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glewExperimental = true; // Needed for core profile
			mGLSLVersion.assign("#version 400 core");
		}
		break;

	case OpenGL_4_1_Core_Profile:
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#if __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glewExperimental = true; // Needed for core profile
			mGLSLVersion.assign("#version 410 core");
		}
		break;

	case OpenGL_4_2_Core_Profile:
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
#if __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glewExperimental = true; // Needed for core profile
			mGLSLVersion.assign("#version 420 core");
		}
		break;

	case OpenGL_4_3_Core_Profile:
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#if __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glewExperimental = true; // Needed for core profile
			mGLSLVersion.assign("#version 430 core");
		}
		break;

	case OpenGL_4_4_Core_Profile:
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
#if __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glewExperimental = true; // Needed for core profile
			mGLSLVersion.assign("#version 440 core");
		}
		break;

    default:
		mGLSLVersion.assign("#version 120");
        break;
	}

	if( mPreWindowFn != NULL )
		mPreWindowFn();

	mStatistics = new Statistics();
	GLFWwindow* share = NULL;
	for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
	{
		if( i > 0 )
			share = mThisNode->getWindowPtr(0)->getWindowHandle();

		if( !mThisNode->getWindowPtr(i)->openWindow( share ) )
		{
			MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Failed to open window %d!\n", i);
			return false;
		}
	}

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
	  //Problem: glewInit failed, something is seriously wrong.
	  MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "GLEW error: %s!\n", glewGetErrorString(err));
	  return false;
	}
	MessageHandler::instance()->print(MessageHandler::NOTIFY_VERSION_INFO, "Using GLEW %s.\n", glewGetString(GLEW_VERSION));

	if( !checkForOGLErrors() )
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "GLEW init triggered an OpenGL error.\n");

	for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
	{
		mThisNode->setCurrentWindowIndex(i);
		getActiveWindowPtr()->init();
		updateAAInfo(i);
	}

	waitForAllWindowsInSwapGroupToOpen();

	if( RUN_FRAME_LOCK_CHECK_THREAD )
	{
		if(ClusterManager::instance()->getNumberOfNodes() > 1)
			mThreadPtr = new tthread::thread( updateFrameLockLoop, 0 );
	}

	//init swap group if enabled
	if( mThisNode->isUsingSwapGroups() )
		SGCTWindow::initNvidiaSwapGroups();

	return true;
}

/*!
Initiates OpenGL.
*/
void sgct::Engine::initOGL()
{
	/*
		Set up function pointers etc. depending on if fixed or programmable pipeline is used
	*/
	if( mRunMode > OpenGL_Compablity_Profile )
	{
		mInternalDrawFn = &Engine::draw;
		mInternalRenderFBOFn = &Engine::renderFBOTexture;
		mInternalDrawOverlaysFn = &Engine::drawOverlays;
		mInternalRenderPostFXFn = &Engine::renderPostFX;
		mInternalRenderFisheyeFn = &Engine::renderFisheye;

		//force buffer objects since display lists are not supported in core opengl 3.3+
		ClusterManager::instance()->setMeshImplementation( ClusterManager::BUFFER_OBJECTS );
		mFixedOGLPipeline = false;
	}
	else
	{
		mInternalDrawFn = &Engine::drawFixedPipeline;
		mInternalRenderFBOFn = &Engine::renderFBOTextureFixedPipeline;
		mInternalDrawOverlaysFn = &Engine::drawOverlaysFixedPipeline;
		mInternalRenderPostFXFn = &Engine::renderPostFXFixedPipeline;
		mInternalRenderFisheyeFn = &Engine::renderFisheyeFixedPipeline;

		mFixedOGLPipeline = true;
	}

	//Get OpenGL version
	int mOpenGL_Version[3];
	mOpenGL_Version[0] = glfwGetWindowAttrib( getActiveWindowPtr()->getWindowHandle(), GLFW_CONTEXT_VERSION_MAJOR);
	mOpenGL_Version[1] = glfwGetWindowAttrib( getActiveWindowPtr()->getWindowHandle(), GLFW_CONTEXT_VERSION_MINOR );
	mOpenGL_Version[2] = glfwGetWindowAttrib( getActiveWindowPtr()->getWindowHandle(), GLFW_CONTEXT_REVISION);

	MessageHandler::instance()->print(MessageHandler::NOTIFY_VERSION_INFO, "OpenGL version %d.%d.%d %s\n", mOpenGL_Version[0], mOpenGL_Version[1], mOpenGL_Version[2],
		mFixedOGLPipeline ? "comp. profile" : "core profile");

	MessageHandler::instance()->print(MessageHandler::NOTIFY_VERSION_INFO, "Vendor: %s\n",
		glGetString(GL_VENDOR));

	MessageHandler::instance()->print(MessageHandler::NOTIFY_VERSION_INFO, "Renderer: %s\n",
		glGetString(GL_RENDERER));	

	if (!glfwExtensionSupported("GL_EXT_framebuffer_object") && mOpenGL_Version[0] < 2)
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_WARNING, "Warning! Frame buffer objects are not supported! A lot of features in SGCT will not work!\n");
		SGCTSettings::instance()->setUseFBO( false );
	}
	else if (!glfwExtensionSupported("GL_EXT_framebuffer_multisample") && mOpenGL_Version[0] < 2)
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_WARNING, "Warning! FBO multisampling is not supported!\n");
		SGCTSettings::instance()->setUseFBO( true );

		for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
			mThisNode->getWindowPtr(i)->setNumberOfAASamples(1);
	}

	if (ClusterManager::instance()->getNumberOfNodes() > 1)
	{
		char nodeName[MAX_SGCT_PATH_LENGTH];
#if (_MSC_VER >= 1400) //visual studio 2005 or later
		sprintf_s(nodeName, MAX_SGCT_PATH_LENGTH, "_node%d",
			ClusterManager::instance()->getThisNodeId());
#else
		sprintf( nodeName, "_node%d",
			ClusterManager::instance()->getThisNodeId());
#endif

		SGCTSettings::instance()->appendCapturePath(std::string(nodeName), SGCTSettings::Mono);
		SGCTSettings::instance()->appendCapturePath(std::string(nodeName), SGCTSettings::LeftStereo);
		SGCTSettings::instance()->appendCapturePath(std::string(nodeName), SGCTSettings::RightStereo);
	}

	//init window opengl data
	getActiveWindowPtr()->makeOpenGLContextCurrent( SGCTWindow::Shared_Context );

	loadShaders();
	mStatistics->initVBO(mFixedOGLPipeline);

	if( mInitOGLFn != NULL )
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_IMPORTANT, "\n---- Calling init callback ----\n");
		mInitOGLFn();
		MessageHandler::instance()->print(MessageHandler::NOTIFY_IMPORTANT, "-------------------------------\n");
	}

	//create all textures, etc
	for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
	{
		mThisNode->setCurrentWindowIndex(i);
		getActiveWindowPtr()->initOGL(); //sets context to shared
        
        if(mScreenShotFn != NULL)
        {
            //set callback
            sgct_cppxeleven::function< void(Image *, std::size_t, ScreenCapture::EyeIndex) > callback;
            callback = sgct_cppxeleven::bind(&Engine::invokeScreenShotCallback, this,
                                             sgct_cppxeleven::placeholders::_1,
                                             sgct_cppxeleven::placeholders::_2,
                                             sgct_cppxeleven::placeholders::_3);
            
            //left channel (Mono and Stereo_Left)
            if( getActiveWindowPtr()->getScreenCapturePointer(0) != NULL )
                getActiveWindowPtr()->getScreenCapturePointer(0)->setCaptureCallback(callback);
            //right channel (Stereo_Right)
            if( getActiveWindowPtr()->getScreenCapturePointer(1) != NULL )
                getActiveWindowPtr()->getScreenCapturePointer(1)->setCaptureCallback(callback);
        }
	}

	updateFrustums();

	//
	// Add fonts
	//
	if( SGCTSettings::instance()->getOSDTextFontPath().empty() )
	{
	    if( !sgct_text::FontManager::instance()->addFont( "SGCTFont", SGCTSettings::instance()->getOSDTextFontName() ) )
            sgct_text::FontManager::instance()->getFont( "SGCTFont", SGCTSettings::instance()->getOSDTextFontSize() );
    }
    else
    {
	    std::string tmpPath = SGCTSettings::instance()->getOSDTextFontPath() + SGCTSettings::instance()->getOSDTextFontName();
	    if( !sgct_text::FontManager::instance()->addFont( "SGCTFont", tmpPath, sgct_text::FontManager::FontPath_Local ) )
            sgct_text::FontManager::instance()->getFont( "SGCTFont", SGCTSettings::instance()->getOSDTextFontSize() );
    }

	//init swap barrier is swap groups are active
	SGCTWindow::setBarrier(true);
	SGCTWindow::resetSwapGroupFrameNumber();

	for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
	{
		mThisNode->setCurrentWindowIndex(i);

		//generate mesh (VAO and VBO)
		getActiveWindowPtr()->initContextSpecificOGL();
	}

	//check for errors
	checkForOGLErrors();

	MessageHandler::instance()->print(MessageHandler::NOTIFY_IMPORTANT, "\nReady to render!\n");
}

/*!
Clean up all resources and release memory.
*/
void sgct::Engine::clean()
{
    MessageHandler::instance()->print(MessageHandler::NOTIFY_IMPORTANT, "Cleaning up...\n");

	if( mCleanUpFn != NULL )
	{
		if( mThisNode != NULL && mThisNode->getNumberOfWindows() > 0 )
			mThisNode->getWindowPtr(0)->makeOpenGLContextCurrent( SGCTWindow::Shared_Context );
		mCleanUpFn();
	}

	MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Clearing all callbacks...\n");
	clearAllCallbacks();

	//kill thread
	if( mThreadPtr )
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "Waiting for frameLock thread to finish...\n");

		sgct::SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::FrameSyncMutex );
		sRunUpdateFrameLockLoop = false;
		sgct::SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::FrameSyncMutex );

		mThreadPtr->join();
		delete mThreadPtr;
		mThreadPtr = NULL;
		MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "Done.\n");
	}

	//de-init window and unbind swapgroups...
	if(ClusterManager::instance()->getNumberOfNodes() > 0)
	{
		if(mThisNode != NULL)
			for(std::size_t i=0; i<mThisNode->getNumberOfWindows(); i++)
			{
				mThisNode->getWindowPtr(i)->close();
			}
	}

	//close TCP connections
	if( mNetworkConnections != NULL )
	{
		delete mNetworkConnections;
		mNetworkConnections = NULL;
	}

	if( mConfig != NULL )
	{
		delete mConfig;
		mConfig = NULL;
	}

	// Destroy explicitly to avoid memory leak messages
	//Shared contex -------------------------------------------------------------------------------->
	if( mThisNode != NULL && mThisNode->getNumberOfWindows() > 0 )
		mThisNode->getWindowPtr(0)->makeOpenGLContextCurrent( SGCTWindow::Shared_Context );
	if( mStatistics != NULL )
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Deleting stats data...\n");
		delete mStatistics;
		mStatistics = NULL;
	}

	MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Destroying shader manager and internal shaders...\n");
	ShaderManager::destroy();
	for(size_t i = 0; i < NUMBER_OF_SHADERS; i++)
		mShaders[i].deleteProgram();

	MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Destroying texture manager...\n");
	TextureManager::destroy();

	MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Destroying font manager...\n");
	sgct_text::FontManager::destroy();

	//Window specific context ------------------------------------------------------------------->
	if( mThisNode != NULL && mThisNode->getNumberOfWindows() > 0 )
		mThisNode->getWindowPtr(0)->makeOpenGLContextCurrent( SGCTWindow::Window_Context );
	MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Destroying shared data...\n");
	SharedData::destroy();
	MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Destroying cluster manager...\n");
	ClusterManager::destroy();
	MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Destroying settings...\n");
	SGCTSettings::destroy();

	MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Destroying message handler...\n");
	MessageHandler::destroy();

	std::cout << "Destroying mutexes...\n" << std::endl;
	SGCTMutexManager::destroy();

	// Close window and terminate GLFW
	std::cout << std::endl << "Terminating glfw...";
	glfwTerminate();
	std::cout << " Done." << std::endl;
}

/*!
Un-binds all callbacks.
*/
void sgct::Engine::clearAllCallbacks()
{
	mDrawFn = NULL;
	mDraw2DFn = NULL;
	mPreSyncFn = NULL;
	mPostSyncPreDrawFn = NULL;
	mPostDrawFn = NULL;
	mInitOGLFn = NULL;
	mPreWindowFn = NULL;
	mClearBufferFn = NULL;
	mCleanUpFn = NULL;
	mInternalDrawFn = NULL;
	mInternalRenderFBOFn = NULL;
	mInternalDrawOverlaysFn = NULL;
	mInternalRenderPostFXFn = NULL;
	mInternalRenderFisheyeFn = NULL;
	mNetworkMessageCallbackFn = NULL;
	mNetworkStatusCallbackFn = NULL;
    mScreenShotFn = NULL;

	//global
	gKeyboardCallbackFn = NULL;
	gCharCallbackFn = NULL;
	gMouseButtonCallbackFn = NULL;
	gMousePosCallbackFn = NULL;
	gMouseScrollCallbackFn = NULL;

	for(unsigned int i=0; i < mTimers.size(); i++)
	{
		mTimers[i].mCallback = NULL;
	}
}

/*!
Locks the rendering thread for synchronization. The two stages are:

1. PreStage, locks the slaves until data is successfully received
2. PostStage, locks master until slaves are ready to swap buffers

Sync time from statistics is the time each computer waits for sync.
*/
bool sgct::Engine::frameLock(sgct::Engine::SyncStage stage)
{
	if( stage == PreStage )
	{
        double t0 = glfwGetTime();
		mNetworkConnections->sync(NetworkManager::SendDataToClients, mStatistics); //from server to clients
		mStatistics->setSyncTime( static_cast<float>(glfwGetTime() - t0) );

		//run only on clients/slaves
		if( !mIgnoreSync && !mNetworkConnections->isComputerServer() ) //not server
		{
			t0 = glfwGetTime();
            while(mNetworkConnections->isRunning() && mRunning)
			{
                if( mNetworkConnections->isSyncComplete() )
						break;

                if(USE_SLEEP_TO_WAIT_FOR_NODES)
					tthread::this_thread::sleep_for(tthread::chrono::milliseconds(1));
				else
				{
					SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::FrameSyncMutex );
					NetworkManager::gCond.wait( (*SGCTMutexManager::instance()->getMutexPtr( SGCTMutexManager::FrameSyncMutex )) );
					SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::FrameSyncMutex );
				}
                
                //for debuging
                SGCTNetwork * conn;
				if( glfwGetTime() - t0 > 1.0 ) //more than a second
				{
					conn = mNetworkConnections->getSyncConnection(0);
                    if( !conn->isUpdated() )
                    {
                        unsigned int lFrameNumber = 0;
                        getActiveWindowPtr()->getSwapGroupFrameNumber(lFrameNumber);

                        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Slave: waiting for master... send frame %d != previous recv frame %d\n\tNvidia swap groups: %s\n\tNvidia swap barrier: %s\n\tNvidia universal frame number: %u\n\tSGCT frame number: %u\n",
                            conn->getSendFrame(),
                            conn->getRecvFrame(SGCTNetwork::Previous),
                            getActiveWindowPtr()->isUsingSwapGroups() ? "enabled" : "disabled",
                            getActiveWindowPtr()->isBarrierActive() ? "enabled" : "disabled",
                            lFrameNumber,
                            mFrameCounter);
                    }
                    
                    if( glfwGetTime() - t0 > 60.0 ) //more than a minute
                    {
                        MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Slave: no sync signal from master after 60 seconds! Exiting...");
                        
                        return false;
                    }
				}
			}//end while wait loop

			/*
				A this point all data needed for rendering a frame is received.
				Let's signal that back to the master/server.
			*/
			mNetworkConnections->sync(NetworkManager::AcknowledgeData, mStatistics);

			mStatistics->addSyncTime(static_cast<float>(glfwGetTime() - t0));
		}//end if client
	}
	else //post stage
	{
		if( !mIgnoreSync && mNetworkConnections->isComputerServer() )//&&
			//mConfig->isMasterSyncLocked() &&
			/*localRunningMode == NetworkManager::Remote &&*/
			//!getActiveWindowPtr()->isBarrierActive() )//post stage
		{
			double t0 = glfwGetTime();
            while(mNetworkConnections->isRunning() &&
				mRunning &&
				mNetworkConnections->getActiveConnectionsCount() > 0)
			{
				if( mNetworkConnections->isSyncComplete() )
						break;

				if(USE_SLEEP_TO_WAIT_FOR_NODES)
					tthread::this_thread::sleep_for(tthread::chrono::milliseconds(1));
				else
				{
					SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::FrameSyncMutex );
					NetworkManager::gCond.wait( (*SGCTMutexManager::instance()->getMutexPtr( SGCTMutexManager::FrameSyncMutex )) );
					SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::FrameSyncMutex );
				}

				//for debuging
                SGCTNetwork * conn;
				if( glfwGetTime() - t0 > 1.0 ) //more than a second
				{
                    for(unsigned int i=0; i<mNetworkConnections->getSyncConnectionsCount(); i++)
					{
						conn = mNetworkConnections->getConnection(i);
                        if( !conn->isUpdated() )
                        {
							unsigned int lFrameNumber = 0;
							getActiveWindowPtr()->getSwapGroupFrameNumber(lFrameNumber);

							MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Waiting for slave%d: send frame %d != recv frame %d\n\tNvidia swap groups: %s\n\tNvidia swap barrier: %s\n\tNvidia universal frame number: %u\n\tSGCT frame number: %u\n",
								i,
								mNetworkConnections->getConnection(i)->getSendFrame(),
								mNetworkConnections->getConnection(i)->getRecvFrame(SGCTNetwork::Current),
								getActiveWindowPtr()->isUsingSwapGroups() ? "enabled" : "disabled",
								getActiveWindowPtr()->isBarrierActive() ? "enabled" : "disabled",
								lFrameNumber,
                                mFrameCounter);
						}
					}
                    
                    if( glfwGetTime() - t0 > 60.0 ) //more than a minute
                    {
                        MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Master: no sync signal from all slaves after 60 seconds! Exiting...");
                        
                        return false;
                    }
				}
			}//end while
			mStatistics->addSyncTime(static_cast<float>(glfwGetTime() - t0));
		}//end if server
	}
    
    return true;
}

/*!
	This is SGCT's renderloop where rendeing & synchronization takes place.
*/
void sgct::Engine::render()
{
	mRunning = GL_TRUE;

	while( mRunning )
	{
		mRenderingOffScreen = false;

#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Render-Loop: Updating tracking devices.\n");
#endif

		//update tracking data
		if( isMaster() )
			ClusterManager::instance()->getTrackingManagerPtr()->updateTrackingDevices();

#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Render-Loop: Running pre-sync.\n");
#endif
		if( mPreSyncFn != NULL )
			mPreSyncFn();

		if( mNetworkConnections->isComputerServer() )
		{
#ifdef __SGCT_RENDER_LOOP_DEBUG__
            MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Render-Loop: Encoding data.\n");
#endif
			SharedData::instance()->encode();
		}
		else
		{
            if( !mNetworkConnections->isRunning() ) //exit if not running
			{
				MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Network disconnected! Exiting...\n");
				break;
			}
		}

#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Render-Loop: Sync/framelock\n");
#endif
		if( !frameLock(PreStage) )
            break;

#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Render-Loop: running post-sync-pre-draw\n");
#endif

		//check if re-size needed of VBO and PBO
		//context switching may occur if multiple windows are used
		for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
		{
			mThisNode->getWindowPtr(i)->update();
		}
	
		mRenderingOffScreen = SGCTSettings::instance()->useFBO();
		if( mRenderingOffScreen )
			getActiveWindowPtr()->makeOpenGLContextCurrent( SGCTWindow::Shared_Context );

		//Make sure correct context is current
		if( mPostSyncPreDrawFn != NULL )
			mPostSyncPreDrawFn();

		double startFrameTime = glfwGetTime();
		calculateFPS(startFrameTime); //measures time between calls

		//--------------------------------------------------------------
		//     RENDER VIEWPORTS / DRAW
		//--------------------------------------------------------------
		for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
		if( mThisNode->getWindowPtr(i)->isVisible() )
		{
			mThisNode->setCurrentWindowIndex(i);

			if( !mRenderingOffScreen )
				getActiveWindowPtr()->makeOpenGLContextCurrent( SGCTWindow::Window_Context );

			SGCTWindow::StereoMode sm = getActiveWindowPtr()->getStereoMode();

			//--------------------------------------------------------------
			//     RENDER FISHEYE/CUBEMAP VIEWPORTS TO FBO
			//--------------------------------------------------------------
			if( getActiveWindowPtr()->isUsingFisheyeRendering() )
			{
	#ifdef __SGCT_RENDER_LOOP_DEBUG__
                MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Render-Loop: Rendering fisheye\n");
	#endif
				//set alpha value
				mFisheyeClearColor[3] = getActiveWindowPtr()->getAlpha() ? 0.0f : 1.0f;

                if( sm == static_cast<int>(SGCTWindow::No_Stereo) )
                {
                    mActiveFrustumMode = Frustum::Mono;
                    (this->*mInternalRenderFisheyeFn)(LeftEye);
                }
                else
                {
                    mActiveFrustumMode = Frustum::StereoLeftEye;
                    (this->*mInternalRenderFisheyeFn)(LeftEye);
                    
                    mActiveFrustumMode = Frustum::StereoRightEye;
                    
                    //use a single texture for side-by-side and top-bottom stereo modes
                    sm >= SGCTWindow::Side_By_Side_Stereo ?
                        (this->*mInternalRenderFisheyeFn)(LeftEye) :
                        (this->*mInternalRenderFisheyeFn)(RightEye);
                }
			}
			//--------------------------------------------------------------
			//     RENDER REGULAR VIEWPORTS TO FBO
			//--------------------------------------------------------------
			else
			{
	#ifdef __SGCT_RENDER_LOOP_DEBUG__
                MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Render-Loop: Rendering\n");
	#endif
				//if any stereo type (except passive) then set frustum mode to left eye
				if( sm == static_cast<int>(SGCTWindow::No_Stereo))
                {
                    mActiveFrustumMode = Frustum::Mono;
                    renderViewports(LeftEye);
                }
                else
                {
                    mActiveFrustumMode = Frustum::StereoLeftEye;
                    renderViewports(LeftEye);
                    
                    mActiveFrustumMode = Frustum::StereoRightEye;
                    
					//use a single texture for side-by-side and top-bottom stereo modes
					sm >= SGCTWindow::Side_By_Side_Stereo ?
                        renderViewports(LeftEye):
                        renderViewports(RightEye);
                }
			}
            //--------------------------------------------------------------
			//           DONE RENDERING VIEWPORTS TO FBO
			//--------------------------------------------------------------

#ifdef __SGCT_RENDER_LOOP_DEBUG__
            MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Render-Loop: Rendering FBO quad\n");
#endif
		}//end window loop

        //--------------------------------------------------------------
        //           RENDER TO SCREEN
        //--------------------------------------------------------------
		for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
			if( mThisNode->getWindowPtr(i)->isVisible() )
			{
				mThisNode->setCurrentWindowIndex(i);

				mRenderingOffScreen = false;
				if( SGCTSettings::instance()->useFBO() )
					(this->*mInternalRenderFBOFn)();
			}

#ifdef __SGCT_RENDER_LOOP_DEBUG__
		MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Render-Loop: Running post-sync\n");
#endif
		/*
			For single threaded rendering glFinish should be fine to use for frame sync.
			For multitheded usage a glFenceSync fence should be used to synchronize all GPU threads.

			example: GLsync mFence = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
			Then on each thread: glWaitSync(mFence);
		*/
		//glFinish(); //wait for all rendering to finish /* ATI doesn't like this.. the framerate is halfed if it's used. */

		getActiveWindowPtr()->makeOpenGLContextCurrent(SGCTWindow::Shared_Context);

#ifdef __SGCT_DEBUG__
		//check for errors
		checkForOGLErrors();
#endif

#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Render-Loop: swap and update data\n");
#endif
        double endFrameTime = glfwGetTime();
		mStatistics->setDrawTime(static_cast<float>(endFrameTime - startFrameTime));
        updateTimers( endFrameTime );

		//run post frame actions
		if (mPostDrawFn != NULL)
			mPostDrawFn();

		if (mShowGraph)
        {
#ifdef __SGCT_RENDER_LOOP_DEBUG__
            MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Render-Loop: update stats VBOs\n");
#endif
			mStatistics->update();
        }
        
#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Render-Loop: lock\n");
#endif
		//master will wait for nodes render before swapping
		if( !frameLock(PostStage) )
            break;

#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Render-Loop: Swap buffers\n");
#endif
		// Swap front and back rendering buffers
		for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
		{
			mThisNode->setCurrentWindowIndex(i);
			getActiveWindowPtr()->swap(mTakeScreenshot);
		}

		glfwPollEvents();

		// Check if ESC key was pressed or window was closed
        mRunning = !(mThisNode->getKeyPressed( mExitKey ) ||
			mThisNode->shouldAllWindowsClose() ||
			mTerminate ||
			!mNetworkConnections->isRunning());

		//for all windows
		mFrameCounter++;
        if(mTakeScreenshot)
            mShotCounter++;
		mTakeScreenshot = false;

#ifdef __SGCT_RENDER_LOOP_DEBUG__
        MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Render-Loop: End iteration\n");
#endif
	}
}

/*!
	This function renders basic text info and statistics on screen.
*/
void sgct::Engine::renderDisplayInfo()
{
	unsigned int lFrameNumber = 0;
	getActiveWindowPtr()->getSwapGroupFrameNumber(lFrameNumber);

	glm::vec4 strokeColor = sgct_text::FontManager::instance()->getStrokeColor();
	signed long strokeSize = sgct_text::FontManager::instance()->getStrokeSize();
	sgct_text::FontManager::instance()->setStrokeColor( glm::vec4( 0.0f, 0.0f, 0.0f, 0.8f ) );
	sgct_text::FontManager::instance()->setStrokeSize( 1 );

	const sgct_text::Font * font = sgct_text::FontManager::instance()->getFont( "SGCTFont", SGCTSettings::instance()->getOSDTextFontSize() );

	if( font != NULL )
	{
		float lineHeight = font->getHeight() * 1.59f;

		sgct_text::print(font,
			static_cast<float>( getActiveWindowPtr()->getXResolution() ) * SGCTSettings::instance()->getOSDTextXOffset(),
			lineHeight * 7.0f + static_cast<float>( getActiveWindowPtr()->getYResolution() ) * SGCTSettings::instance()->getOSDTextYOffset(),
			glm::vec4(0.8f,0.8f,0.8f,1.0f),
			"Node ip: %s (%s)",
			mThisNode->getAddress().c_str(),
			mNetworkConnections->isComputerServer() ? "master" : "slave");

		sgct_text::print(font,
			static_cast<float>( getActiveWindowPtr()->getXResolution() ) * SGCTSettings::instance()->getOSDTextXOffset(),
			lineHeight * 6.0f + static_cast<float>( getActiveWindowPtr()->getYResolution() ) * SGCTSettings::instance()->getOSDTextYOffset(),
			glm::vec4(0.8f,0.8f,0.0f,1.0f),
			"Frame rate: %.2f Hz, frame: %u",
			mStatistics->getAvgFPS(),
			mFrameCounter);

		sgct_text::print(font,
			static_cast<float>( getActiveWindowPtr()->getXResolution() ) * SGCTSettings::instance()->getOSDTextXOffset(),
			lineHeight * 5.0f + static_cast<float>( getActiveWindowPtr()->getYResolution() ) * SGCTSettings::instance()->getOSDTextYOffset(),
			glm::vec4(0.8f,0.0f,0.8f,1.0f),
			"Avg. draw time: %.2f ms",
			mStatistics->getAvgDrawTime()*1000.0);

		sgct_text::print(font,
			static_cast<float>( getActiveWindowPtr()->getXResolution() ) * SGCTSettings::instance()->getOSDTextXOffset(),
			lineHeight * 4.0f + static_cast<float>( getActiveWindowPtr()->getYResolution() ) * SGCTSettings::instance()->getOSDTextYOffset(),
			glm::vec4(0.0f,0.8f,0.8f,1.0f),
			"Avg. sync time: %.2f ms",
			mStatistics->getAvgSyncTime()*1000.0);

		bool usingSwapGroups = getActiveWindowPtr()->isUsingSwapGroups();
		if(usingSwapGroups)
		{
			sgct_text::print(font,
				static_cast<float>( getActiveWindowPtr()->getXResolution() ) * SGCTSettings::instance()->getOSDTextXOffset(),
				lineHeight * 3.0f + static_cast<float>( getActiveWindowPtr()->getYResolution() ) * SGCTSettings::instance()->getOSDTextYOffset(),
				glm::vec4(0.8f,0.8f,0.8f,1.0f),
				"Swap groups: %s and barrier is %s (%s) | Frame: %d",
				getActiveWindowPtr()->isUsingSwapGroups() ? "Enabled" : "Disabled",
				getActiveWindowPtr()->isBarrierActive() ? "active" : "inactive",
				getActiveWindowPtr()->isSwapGroupMaster() ? "master" : "slave",
				lFrameNumber);
		}
		else
		{
			sgct_text::print(font,
				static_cast<float>( getActiveWindowPtr()->getXResolution() ) * SGCTSettings::instance()->getOSDTextXOffset(),
				lineHeight * 3.0f + static_cast<float>( getActiveWindowPtr()->getYResolution() ) * SGCTSettings::instance()->getOSDTextYOffset(),
				glm::vec4(0.8f,0.8f,0.8f,1.0f),
				"Swap groups: Disabled");
		}

		sgct_text::print(font,
			static_cast<float>(getActiveWindowPtr()->getXResolution()) * SGCTSettings::instance()->getOSDTextXOffset(),
			lineHeight * 2.0f + static_cast<float>(getActiveWindowPtr()->getYResolution()) * SGCTSettings::instance()->getOSDTextYOffset(),
			glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
			"Frame buffer resolution: %d x %d",
			getActiveWindowPtr()->getXFramebufferResolution(), getActiveWindowPtr()->getYFramebufferResolution());

		sgct_text::print(font,
			static_cast<float>( getActiveWindowPtr()->getXResolution() ) * SGCTSettings::instance()->getOSDTextXOffset(),
			lineHeight * 1.0f + static_cast<float>( getActiveWindowPtr()->getYResolution() ) * SGCTSettings::instance()->getOSDTextYOffset(),
			glm::vec4(0.8f,0.8f,0.8f,1.0f),
			"Anti-Aliasing: %s",
			aaInfo);

		if (getActiveWindowPtr()->isUsingFisheyeRendering())
		{
			sgct_text::print(font,
				static_cast<float>(getActiveWindowPtr()->getXResolution()) * SGCTSettings::instance()->getOSDTextXOffset(),
				lineHeight * 0.0f + static_cast<float>(getActiveWindowPtr()->getYResolution()) * SGCTSettings::instance()->getOSDTextYOffset(),
				glm::vec4(0.8f, 0.8f, 0.8f, 1.0f),
				"Interpolation: %s",
				getActiveWindowPtr()->getFisheyeUseCubicInterpolation() ? "Cubic" : "Linear");
		}

		//if active stereoscopic rendering
		if( mActiveFrustumMode == Frustum::StereoLeftEye )
		{
			sgct_text::print(font,
				static_cast<float>( getActiveWindowPtr()->getXResolution() ) * SGCTSettings::instance()->getOSDTextXOffset(),
				lineHeight * 9.0f + static_cast<float>( getActiveWindowPtr()->getYResolution() ) * SGCTSettings::instance()->getOSDTextYOffset(),
				glm::vec4(0.8f,0.8f,0.8f,1.0f),
				"Stereo type: %s\nActive eye: Left", getActiveWindowPtr()->getStereoModeStr().c_str() );
		}
		else if( mActiveFrustumMode == Frustum::StereoRightEye )
		{
			sgct_text::print(font,
				static_cast<float>( getActiveWindowPtr()->getXResolution() ) * SGCTSettings::instance()->getOSDTextXOffset(),
				lineHeight * 9.0f + static_cast<float>( getActiveWindowPtr()->getYResolution() ) * SGCTSettings::instance()->getOSDTextYOffset(),
				glm::vec4(0.8f,0.8f,0.8f,1.0f),
				"Stereo type: %s\nActive eye:          Right", getActiveWindowPtr()->getStereoModeStr().c_str() );
		}
	}

	//reset
	sgct_text::FontManager::instance()->setStrokeColor( strokeColor );
	sgct_text::FontManager::instance()->setStrokeSize( strokeSize );
}

/*!
	This function enters the correct viewport, frustum, stereo mode and calls the draw callback.
*/
void sgct::Engine::draw()
{
	//run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);
    
    enterCurrentViewport();
    
    //clear buffers
    SGCTSettings::instance()->useFBO() ? setAndClearBuffer(RenderToTexture) : setAndClearBuffer(BackBuffer);
    
    glDisable(GL_SCISSOR_TEST);

	if( mDrawFn != NULL )
	{
		glLineWidth(1.0);
		mShowWireframe ? glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ) : glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

		mDrawFn();

		//restore polygon mode
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}
}

/*!
	This function enters the correct viewport, frustum, stereo mode and calls the draw callback.
*/
void sgct::Engine::drawFixedPipeline()
{
	//run scissor test to prevent clearing of entire buffer
    glEnable(GL_SCISSOR_TEST);
    
    enterCurrentViewport(); //set glViewport & glScissor
    
    //clear buffers
    SGCTSettings::instance()->useFBO() ? setAndClearBuffer(RenderToTexture) : setAndClearBuffer(BackBuffer);
    
    glDisable(GL_SCISSOR_TEST);


	glMatrixMode(GL_PROJECTION);

	Viewport * tmpVP = getActiveWindowPtr()->getCurrentViewport();
	glLoadMatrixf( glm::value_ptr(tmpVP->getProjectionMatrix(mActiveFrustumMode)) );

	glMatrixMode(GL_MODELVIEW);

	glLoadMatrixf( glm::value_ptr( tmpVP->getViewMatrix(mActiveFrustumMode) * getModelMatrix() ) );

	if( mDrawFn != NULL )
	{
		glLineWidth(1.0);
		mShowWireframe ? glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ) : glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

		mDrawFn();

		//restore polygon mode
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}
}

/*!
	Draw viewport overlays if there are any.
*/
void sgct::Engine::drawOverlays()
{
	std::size_t numberOfIterations = ( getActiveWindowPtr()->isUsingFisheyeRendering() ? 1 : getActiveWindowPtr()->getNumberOfViewports() );
	for(std::size_t i=0; i < numberOfIterations; i++)
	{
		getActiveWindowPtr()->setCurrentViewport(i);

		//if viewport has overlay
		Viewport * tmpVP = getActiveWindowPtr()->getCurrentViewport();
		
        if( tmpVP->hasOverlayTexture() && tmpVP->isEnabled() )
		{
			/*
				Some code (using OpenSceneGraph) can mess up the viewport settings.
				To ensure correct mapping enter the current viewport.
			*/

			if( getActiveWindowPtr()->isUsingFisheyeRendering() )
			{
				enterFisheyeViewport();
			}
			else
			{
				enterCurrentViewport();
			}

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tmpVP->getOverlayTextureIndex() );

			mShaders[OverlayShader].bind();

			glUniform1i( mShaderLocs[OverlayTex], 0);

			getActiveWindowPtr()->isUsingFisheyeRendering() ?
				getActiveWindowPtr()->bindVAO(SGCTWindow::FishEyeQuad):
				getActiveWindowPtr()->bindVAO(SGCTWindow::RenderQuad);

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			//unbind
			getActiveWindowPtr()->unbindVAO();
			ShaderProgram::unbind();
		}
	}
}

/*!
	Draw viewport overlays if there are any.
*/
void sgct::Engine::drawOverlaysFixedPipeline()
{
	std::size_t numberOfIterations = ( getActiveWindowPtr()->isUsingFisheyeRendering() ? 1 : getActiveWindowPtr()->getNumberOfViewports() );
	for(std::size_t i=0; i < numberOfIterations; i++)
	{
		getActiveWindowPtr()->setCurrentViewport(i);

		//if viewport has overlay
		Viewport * tmpVP = getActiveWindowPtr()->getCurrentViewport();
		if( tmpVP->hasOverlayTexture() && tmpVP->isEnabled() )
		{
			//enter ortho mode
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glPushMatrix();
			
			/*
				Some code (using OpenSceneGraph) can mess up the viewport settings.
				To ensure correct mapping enter the current viewport.
			*/
			if( getActiveWindowPtr()->isUsingFisheyeRendering() )
			{
				enterFisheyeViewport();
			}
			else
			{
				enterCurrentViewport();
			}

			glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
			glMatrixMode(GL_MODELVIEW);

			glPushAttrib( GL_ALL_ATTRIB_BITS );
			glDisable(GL_LIGHTING);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glDisable(GL_CULL_FACE);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glLoadIdentity();
			glColor4f(1.0f,1.0f,1.0f,1.0f);

            //Open Scene Graph or the user may have changed the active texture
			glActiveTexture(GL_TEXTURE0);
            //glMatrixMode(GL_TEXTURE);
            //glLoadIdentity();
            
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, tmpVP->getOverlayTextureIndex() );

			glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
            
            getActiveWindowPtr()->isUsingFisheyeRendering() ?
				getActiveWindowPtr()->bindVBO(SGCTWindow::FishEyeQuad):
				getActiveWindowPtr()->bindVBO(SGCTWindow::RenderQuad);

			glClientActiveTexture(GL_TEXTURE0);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(0));

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(8));

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			getActiveWindowPtr()->unbindVBO();

			glPopClientAttrib();
			glPopAttrib();

			//exit ortho mode
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
		}
	}
}

/*!
	This function attaches targets to FBO if FBO is in use
*/
void sgct::Engine::prepareBuffer(TextureIndexes ti)
{
	if( SGCTSettings::instance()->useFBO() )
	{
		if( getActiveWindowPtr()->usePostFX() )
			ti = Intermediate;

		OffScreenBuffer * fbo = getActiveWindowPtr()->mFinalFBO_Ptr;

		fbo->bind();
		if( !fbo->isMultiSampled() )
		{
			//update attachments
			fbo->attachColorTexture( getActiveWindowPtr()->getFrameBufferTexture( ti ) );

			if( SGCTSettings::instance()->useDepthTexture() )
				fbo->attachDepthTexture( getActiveWindowPtr()->getFrameBufferTexture( Depth ) );

			if (SGCTSettings::instance()->useNormalTexture())
				fbo->attachColorTexture(getActiveWindowPtr()->getFrameBufferTexture(Normals), GL_COLOR_ATTACHMENT1);

			if (SGCTSettings::instance()->usePositionTexture())
				fbo->attachColorTexture(getActiveWindowPtr()->getFrameBufferTexture(Positions), GL_COLOR_ATTACHMENT2);
		}
    }
}

/*!
	Draw geometry and bind FBO as texture in screenspace (ortho mode).
	The geometry can be a simple quad or a geometry correction and blending mesh.
*/
void sgct::Engine::renderFBOTexture()
{
	//unbind framebuffer
	OffScreenBuffer::unBind();

	getActiveWindowPtr()->makeOpenGLContextCurrent( SGCTWindow::Window_Context );

	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //needed for shaders

	//clear buffers
	mActiveFrustumMode = getActiveWindowPtr()->getStereoMode() == SGCTWindow::Active_Stereo ? Frustum::StereoLeftEye : Frustum::Mono;

	glViewport (0, 0, getActiveWindowPtr()->getXResolution(), getActiveWindowPtr()->getYResolution());
    setAndClearBuffer(BackBufferBlack);
    
    std::size_t numberOfIterations = (getActiveWindowPtr()->isUsingFisheyeRendering() ? 1 : getActiveWindowPtr()->getNumberOfViewports());

	sgct_core::CorrectionMesh::MeshType mt = SGCTSettings::instance()->getUseWarping() ?
		sgct_core::CorrectionMesh::WARP_MESH : sgct_core::CorrectionMesh::QUAD_MESH;

	SGCTWindow::StereoMode sm = getActiveWindowPtr()->getStereoMode();
	if( sm > SGCTWindow::Active_Stereo && sm < SGCTWindow::Side_By_Side_Stereo )
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, getActiveWindowPtr()->getFrameBufferTexture(LeftEye));

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, getActiveWindowPtr()->getFrameBufferTexture(RightEye));

		getActiveWindowPtr()->bindStereoShaderProgram();

		glUniform1i( getActiveWindowPtr()->getStereoShaderLeftTexLoc(), 0);
		glUniform1i( getActiveWindowPtr()->getStereoShaderRightTexLoc(), 1);

		for(std::size_t i=0; i<numberOfIterations; i++)
			getActiveWindowPtr()->getViewport(i)->renderMesh( mt );

		//render mask (mono)
		if (getActiveWindowPtr()->hasAnyMasks())
		{
			glDrawBuffer(GL_BACK);
			glReadBuffer(GL_BACK);

			mShaders[FBOQuadShader].bind(); //bind
			glUniform1i(mShaderLocs[MonoTex], 0);

			glActiveTexture(GL_TEXTURE0);
			glEnable(GL_BLEND);
			glBlendFunc(GL_DST_COLOR, GL_ZERO);
			for (std::size_t i = 0; i < numberOfIterations; i++)
			{
				Viewport * vpPtr = getActiveWindowPtr()->getViewport(i);
				if (vpPtr->hasMaskTexture() && vpPtr->isEnabled())
				{
					glBindTexture(GL_TEXTURE_2D, vpPtr->getMaskTextureIndex());
					vpPtr->renderMesh(sgct_core::CorrectionMesh::MASK_MESH);
				}
			}
		}

		ShaderProgram::unbind();
	}
	else
	{
		glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
		glBindTexture(GL_TEXTURE_2D, getActiveWindowPtr()->getFrameBufferTexture(LeftEye));

		mShaders[FBOQuadShader].bind(); //bind
		glUniform1i( mShaderLocs[MonoTex], 0);

		for (std::size_t i = 0; i < numberOfIterations; i++)
			getActiveWindowPtr()->getViewport(i)->renderMesh( mt );

		//render right eye in active stereo mode
		if( getActiveWindowPtr()->getStereoMode() == SGCTWindow::Active_Stereo )
		{
			glViewport (0, 0, getActiveWindowPtr()->getXResolution(), getActiveWindowPtr()->getYResolution());
            
            //clear buffers
			mActiveFrustumMode = Frustum::StereoRightEye;
			setAndClearBuffer(BackBufferBlack);

			glBindTexture(GL_TEXTURE_2D, getActiveWindowPtr()->getFrameBufferTexture(RightEye));
			glUniform1i( mShaderLocs[MonoTex], 0);

			for(std::size_t i=0; i<numberOfIterations; i++)
				getActiveWindowPtr()->getViewport(i)->renderMesh( mt );
		}

		//render mask (mono)
		if (getActiveWindowPtr()->hasAnyMasks())
		{
			glDrawBuffer(GL_BACK);
			glReadBuffer(GL_BACK);
			glActiveTexture(GL_TEXTURE0);
			glEnable(GL_BLEND);
			glBlendFunc(GL_DST_COLOR, GL_ZERO);
			for (std::size_t i = 0; i < numberOfIterations; i++)
			{
				Viewport * vpPtr = getActiveWindowPtr()->getViewport(i);
				if (vpPtr->hasMaskTexture() && vpPtr->isEnabled())
				{
					glBindTexture(GL_TEXTURE_2D, vpPtr->getMaskTextureIndex());
					vpPtr->renderMesh(sgct_core::CorrectionMesh::MASK_MESH);
				}
			}
		}

		ShaderProgram::unbind();
	}

	glDisable(GL_BLEND);
}


/*!
	Draw geometry and bind FBO as texture in screenspace (ortho mode).
	The geometry can be a simple quad or a geometry correction and blending mesh.
*/
void sgct::Engine::renderFBOTextureFixedPipeline()
{
	//unbind framebuffer
	OffScreenBuffer::unBind();

	getActiveWindowPtr()->makeOpenGLContextCurrent( SGCTWindow::Window_Context );
    
    mActiveFrustumMode = getActiveWindowPtr()->getStereoMode() == SGCTWindow::Active_Stereo ? Frustum::StereoLeftEye : Frustum::Mono;
	glViewport (0, 0, getActiveWindowPtr()->getXResolution(), getActiveWindowPtr()->getYResolution());
    setAndClearBuffer(BackBufferBlack);

	//enter ortho mode
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glPushMatrix();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);

	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //needed for shaders

	glLoadIdentity();
    
    std::size_t numberOfIterations = (getActiveWindowPtr()->isUsingFisheyeRendering() ? 1 : getActiveWindowPtr()->getNumberOfViewports());

	SGCTWindow::StereoMode sm = getActiveWindowPtr()->getStereoMode();

	sgct_core::CorrectionMesh::MeshType mt = SGCTSettings::instance()->getUseWarping() ?
		sgct_core::CorrectionMesh::WARP_MESH : sgct_core::CorrectionMesh::QUAD_MESH;

	if( sm > SGCTWindow::Active_Stereo && sm < SGCTWindow::Side_By_Side_Stereo )
	{
		getActiveWindowPtr()->bindStereoShaderProgram();

		glUniform1i( getActiveWindowPtr()->getStereoShaderLeftTexLoc(), 0);
		glUniform1i( getActiveWindowPtr()->getStereoShaderRightTexLoc(), 1);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, getActiveWindowPtr()->getFrameBufferTexture(LeftEye));
		glEnable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, getActiveWindowPtr()->getFrameBufferTexture(RightEye));
		glEnable(GL_TEXTURE_2D);

		for(std::size_t i=0; i<numberOfIterations; i++)
			getActiveWindowPtr()->getViewport(i)->renderMesh( mt );
		ShaderProgram::unbind();
	}
	else
	{
		glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
		glBindTexture(GL_TEXTURE_2D, getActiveWindowPtr()->getFrameBufferTexture(LeftEye));
		glEnable(GL_TEXTURE_2D);

		for (std::size_t i = 0; i < numberOfIterations; i++)
			getActiveWindowPtr()->getViewport(i)->renderMesh( mt );

		//render right eye in active stereo mode
		if( getActiveWindowPtr()->getStereoMode() == SGCTWindow::Active_Stereo )
		{
			glViewport(0, 0, getActiveWindowPtr()->getXResolution(), getActiveWindowPtr()->getYResolution());
            
            //clear buffers
			mActiveFrustumMode = Frustum::StereoRightEye;
			setAndClearBuffer(BackBufferBlack);

			glBindTexture(GL_TEXTURE_2D, getActiveWindowPtr()->getFrameBufferTexture(RightEye));

			for(std::size_t i=0; i<numberOfIterations; i++)
				getActiveWindowPtr()->getViewport(i)->renderMesh( mt );
		}
	}

	//render mask (mono)
	if (getActiveWindowPtr()->hasAnyMasks())
	{
		glDrawBuffer(GL_BACK);
		glReadBuffer(GL_BACK);

		//if stereo != active stereo
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_BLEND);
		glBlendFunc(GL_DST_COLOR, GL_ZERO);
		for (std::size_t i = 0; i < numberOfIterations; i++)
		{
			Viewport * vpPtr = getActiveWindowPtr()->getViewport(i);
			if (vpPtr->hasMaskTexture() && vpPtr->isEnabled())
			{
				glBindTexture(GL_TEXTURE_2D, vpPtr->getMaskTextureIndex());
				vpPtr->renderMesh(sgct_core::CorrectionMesh::MASK_MESH);
			}
		}
	}
	
	glPopAttrib();

	//exit ortho mode
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

/*!
	This functions works in two steps:
	1. Render a cubemap
	2. Render to a fisheye using a GLSL shader
*/
void sgct::Engine::renderFisheye(TextureIndexes ti)
{
	bool statesSet = false;

	if( mActiveFrustumMode == Frustum::StereoLeftEye )
		getActiveWindowPtr()->setFisheyeOffset( -getUserPtr()->getEyeSeparation() / getActiveWindowPtr()->getDomeDiameter(), 0.0f);
	else if( mActiveFrustumMode == Frustum::StereoRightEye )
		getActiveWindowPtr()->setFisheyeOffset( getUserPtr()->getEyeSeparation() / getActiveWindowPtr()->getDomeDiameter(), 0.0f);

	//iterate the cube sides
	OffScreenBuffer * CubeMapFBO = getActiveWindowPtr()->mCubeMapFBO_Ptr;
	for(std::size_t i=0; i<getActiveWindowPtr()->getNumberOfViewports(); i++)
	{
		getActiveWindowPtr()->setCurrentViewport(i);

		if( getActiveWindowPtr()->getCurrentViewport()->isEnabled() )
		{
			mShowWireframe ? glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ) : glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

			//bind & attach buffer
			CubeMapFBO->bind(); //osg seems to unbind FBO when rendering with osg FBO cameras
			if( !CubeMapFBO->isMultiSampled() )
			{
				if (SGCTSettings::instance()->useDepthTexture())
				{
					CubeMapFBO->attachDepthTexture(getActiveWindowPtr()->getFrameBufferTexture(FisheyeDepthSwap));
					CubeMapFBO->attachColorTexture(getActiveWindowPtr()->getFrameBufferTexture(FisheyeColorSwap));
				}
				else
					CubeMapFBO->attachCubeMapTexture(getActiveWindowPtr()->getFrameBufferTexture(CubeMap), static_cast<unsigned int>(i));

				if (SGCTSettings::instance()->useNormalTexture())
					CubeMapFBO->attachCubeMapTexture(getActiveWindowPtr()->getFrameBufferTexture(CubeMapNormals), static_cast<unsigned int>(i), GL_COLOR_ATTACHMENT1);

				if (SGCTSettings::instance()->usePositionTexture())
					CubeMapFBO->attachCubeMapTexture(getActiveWindowPtr()->getFrameBufferTexture(CubeMapPositions), static_cast<unsigned int>(i), GL_COLOR_ATTACHMENT2);
			}

            //reset depth function
			glDepthFunc( GL_LESS );

			//render
			(this->*mInternalDrawFn)();

			//copy AA-buffer to "regular"/non-AA buffer
			if( CubeMapFBO->isMultiSampled() )
			{
				CubeMapFBO->bindBlit(); //bind separate read and draw buffers to prepare blit operation

				//update attachments
				if (SGCTSettings::instance()->useDepthTexture())
				{
					CubeMapFBO->attachDepthTexture(getActiveWindowPtr()->getFrameBufferTexture(FisheyeDepthSwap));
					CubeMapFBO->attachColorTexture(getActiveWindowPtr()->getFrameBufferTexture(FisheyeColorSwap));
				}
				else
					CubeMapFBO->attachCubeMapTexture(getActiveWindowPtr()->getFrameBufferTexture(CubeMap), static_cast<unsigned int>(i));

				if (SGCTSettings::instance()->useNormalTexture())
					CubeMapFBO->attachCubeMapTexture(getActiveWindowPtr()->getFrameBufferTexture(CubeMapNormals), static_cast<unsigned int>(i), GL_COLOR_ATTACHMENT1);

				if (SGCTSettings::instance()->usePositionTexture())
					CubeMapFBO->attachCubeMapTexture(getActiveWindowPtr()->getFrameBufferTexture(CubeMapPositions), static_cast<unsigned int>(i), GL_COLOR_ATTACHMENT2);

				CubeMapFBO->blit();
			}

			//restore polygon mode
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

			//re-calculate depth values from a cube to spherical model
			if( SGCTSettings::instance()->useDepthTexture() )
			{
				GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
				CubeMapFBO->bind(false, 1, buffers); //bind no multi-sampled
				
				CubeMapFBO->attachCubeMapTexture( getActiveWindowPtr()->getFrameBufferTexture(CubeMap), static_cast<unsigned int>(i) );
				CubeMapFBO->attachCubeMapDepthTexture( getActiveWindowPtr()->getFrameBufferTexture(CubeMapDepth), static_cast<unsigned int>(i) );

				int cmRes = static_cast<int>(getActiveWindowPtr()->getCubeMapResolution());
				glViewport(0, 0, cmRes, cmRes);
				glScissor(currentViewportCoords[0],
					currentViewportCoords[1],
					currentViewportCoords[2],
					currentViewportCoords[3]);

				glEnable(GL_SCISSOR_TEST);

				mClearBufferFn();

				glDisable( GL_CULL_FACE );
				if( getActiveWindowPtr()->getAlpha() )
				{
					glEnable(GL_BLEND);
					glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
				}
				else
					glDisable(GL_BLEND);
				glEnable( GL_DEPTH_TEST );
				glDepthFunc( GL_ALWAYS );
				statesSet = true;

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, getActiveWindowPtr()->getFrameBufferTexture(FisheyeColorSwap));

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, getActiveWindowPtr()->getFrameBufferTexture(FisheyeDepthSwap));

				//bind shader
				getActiveWindowPtr()->bindFisheyeDepthCorrectionShaderProgram();
				glUniform1i( getActiveWindowPtr()->getFisheyeSwapShaderColorLoc(), 0);
				glUniform1i( getActiveWindowPtr()->getFisheyeSwapShaderDepthLoc(), 1);
				glUniform1f( getActiveWindowPtr()->getFisheyeSwapShaderNearLoc(), mNearClippingPlaneDist);
				glUniform1f( getActiveWindowPtr()->getFisheyeSwapShaderFarLoc(), mFarClippingPlaneDist);

				getActiveWindowPtr()->bindVAO( SGCTWindow::RenderQuad );
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				getActiveWindowPtr()->unbindVAO();

				//unbind shader
				ShaderProgram::unbind();

				glDisable(GL_SCISSOR_TEST);
			}//end if depthmap
		}//end if viewport is enabled
	}//end for

	//bind fisheye target FBO
	OffScreenBuffer * finalFBO = getActiveWindowPtr()->mFinalFBO_Ptr;
	finalFBO->bind();

	getActiveWindowPtr()->usePostFX() ?
		finalFBO->attachColorTexture( getActiveWindowPtr()->getFrameBufferTexture(Intermediate) ) :
		finalFBO->attachColorTexture( getActiveWindowPtr()->getFrameBufferTexture(ti) );

	if (SGCTSettings::instance()->useDepthTexture())
		finalFBO->attachDepthTexture(getActiveWindowPtr()->getFrameBufferTexture(Depth));

	if (SGCTSettings::instance()->useNormalTexture())
		finalFBO->attachColorTexture(getActiveWindowPtr()->getFrameBufferTexture(Normals), GL_COLOR_ATTACHMENT1);

	if (SGCTSettings::instance()->usePositionTexture())
		finalFBO->attachColorTexture(getActiveWindowPtr()->getFrameBufferTexture(Positions), GL_COLOR_ATTACHMENT2);

	enterFisheyeViewport();
    sgct::SGCTWindow::StereoMode sm = getActiveWindowPtr()->getStereoMode();
	if( !(sm >= SGCTWindow::Side_By_Side_Stereo && mActiveFrustumMode == Frustum::StereoRightEye) )
	{
		glClearColor(mFisheyeClearColor[0], mFisheyeClearColor[1], mFisheyeClearColor[2], mFisheyeClearColor[3]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	getActiveWindowPtr()->bindFisheyeShaderProgram();

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	//if for some reson the active texture has been reset
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, getActiveWindowPtr()->getFrameBufferTexture(CubeMap));
	
	if( SGCTSettings::instance()->useDepthTexture() )
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, getActiveWindowPtr()->getFrameBufferTexture(CubeMapDepth));
		glUniform1i(getActiveWindowPtr()->getFisheyeShaderCubemapDepthLoc(), 1);
	}

	if (SGCTSettings::instance()->useNormalTexture())
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, getActiveWindowPtr()->getFrameBufferTexture(CubeMapNormals));
		glUniform1i(getActiveWindowPtr()->getFisheyeShaderCubemapNormalsLoc(), 2);
	}

	if (SGCTSettings::instance()->usePositionTexture())
	{
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_CUBE_MAP, getActiveWindowPtr()->getFrameBufferTexture(CubeMapPositions));
		glUniform1i(getActiveWindowPtr()->getFisheyeShaderCubemapPositionsLoc(), 3);
	}

	if( !statesSet )
	{
		glDisable( GL_CULL_FACE );
		if( getActiveWindowPtr()->getAlpha() )
		{
			glEnable(GL_BLEND);
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		}
		else
			glDisable(GL_BLEND);
		glEnable( GL_DEPTH_TEST );
		glDepthFunc( GL_ALWAYS );
	}

	glUniform1i( getActiveWindowPtr()->getFisheyeShaderCubemapLoc(), 0);

	glUniform1f( getActiveWindowPtr()->getFisheyeShaderHalfFOVLoc(), glm::radians<float>(getActiveWindowPtr()->getFisheyeFOV()/2.0f) );
	
    if( getActiveWindowPtr()->isFisheyeOffaxis() )
	{
		glUniform3f( getActiveWindowPtr()->getFisheyeShaderOffsetLoc(),
			getActiveWindowPtr()->getFisheyeOffset(0),
			getActiveWindowPtr()->getFisheyeOffset(1),
			getActiveWindowPtr()->getFisheyeOffset(2) );
	}

	getActiveWindowPtr()->bindVAO( SGCTWindow::FishEyeQuad );
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	getActiveWindowPtr()->unbindVAO();

	ShaderProgram::unbind();
	glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	if(mTakeScreenshot)
	{
		int size = getActiveWindowPtr()->getXFramebufferResolution();
		unsigned int fontSize = 0;

		if( size > 512 )
			fontSize = static_cast<unsigned int>(size)/96;
		else if( size == 512 )
			fontSize = 8;

		if( fontSize != 0 )
		{
			float x = static_cast<float>( size - size/4 );
			float y = static_cast<float>( fontSize );

			//draw in pixel space / FBO space
			sgct_text::FontManager::instance()->setDrawInScreenSpace(false);
			sgct_text::print(sgct_text::FontManager::instance()->getFont( "SGCTFont", fontSize ), x, 2.0f * y + y/5.0f, "Frame#: %u", mShotCounter);

			if( mActiveFrustumMode == Frustum::Mono )
				sgct_text::print(sgct_text::FontManager::instance()->getFont( "SGCTFont", fontSize ), x, y, "Mono");
			else if( mActiveFrustumMode == Frustum::StereoLeftEye )
				sgct_text::print(sgct_text::FontManager::instance()->getFont( "SGCTFont", fontSize ), x, y, "Left");
			else
				sgct_text::print(sgct_text::FontManager::instance()->getFont( "SGCTFont", fontSize ), x, y, "Right");

			//restore: draw in point space / screen space
			sgct_text::FontManager::instance()->setDrawInScreenSpace(true);
		}
	}

	if (!getActiveWindowPtr()->getAlpha())
		glEnable(GL_BLEND);

	glDisable(GL_DEPTH_TEST);

	//if side-by-side and top-bottom mode only do post fx and blit only after rendered right eye
	bool split_screen_stereo = (sm >= sgct::SGCTWindow::Side_By_Side_Stereo);
	if( !( split_screen_stereo && mActiveFrustumMode == Frustum::StereoLeftEye) )
	{
		if( getActiveWindowPtr()->usePostFX() )
		{
			//blit buffers
			updateRenderingTargets(ti); //only used if multisampled FBOs

			(this->*mInternalRenderPostFXFn)(ti);

			render2D();
			if(split_screen_stereo)
			{
				//render left eye info and graph so that all 2D items are rendered after post fx
				mActiveFrustumMode = Frustum::StereoLeftEye;
				render2D();
			}
		}
		else
		{
			render2D();
			if(split_screen_stereo)
			{
				//render left eye info and graph so that all 2D items are rendered after post fx
				mActiveFrustumMode = Frustum::StereoLeftEye;
				render2D();
			}

			updateRenderingTargets(ti); //only used if multisampled FBOs
		}
	}

	glDisable(GL_BLEND);
}

/*!
	This functions works in two steps:
	1. Render a cubemap
	2. Render to a fisheye using a GLSL shader
*/
void sgct::Engine::renderFisheyeFixedPipeline(TextureIndexes ti)
{
	if( mActiveFrustumMode == Frustum::StereoLeftEye )
		getActiveWindowPtr()->setFisheyeOffset( -getUserPtr()->getEyeSeparation() / getActiveWindowPtr()->getDomeDiameter(), 0.0f);
	else if( mActiveFrustumMode == Frustum::StereoRightEye )
		getActiveWindowPtr()->setFisheyeOffset( getUserPtr()->getEyeSeparation() / getActiveWindowPtr()->getDomeDiameter(), 0.0f);

	//iterate the cube sides
	OffScreenBuffer * CubeMapFBO = getActiveWindowPtr()->mCubeMapFBO_Ptr;
	for(std::size_t i=0; i<getActiveWindowPtr()->getNumberOfViewports(); i++)
	{
		getActiveWindowPtr()->setCurrentViewport(i);

		if( getActiveWindowPtr()->getCurrentViewport()->isEnabled() )
		{
			mShowWireframe ? glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ) : glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

			//bind & attach buffer
			CubeMapFBO->bind(); //osg seems to unbind FBO when rendering with osg FBO cameras
			if( !CubeMapFBO->isMultiSampled() )
			{
				if( SGCTSettings::instance()->useDepthTexture() )
				{
					CubeMapFBO->attachDepthTexture( getActiveWindowPtr()->getFrameBufferTexture(FisheyeDepthSwap) );
					CubeMapFBO->attachColorTexture( getActiveWindowPtr()->getFrameBufferTexture(FisheyeColorSwap) );
				}
				else
					CubeMapFBO->attachCubeMapTexture( getActiveWindowPtr()->getFrameBufferTexture(CubeMap), static_cast<unsigned int>(i) );

				if (SGCTSettings::instance()->useNormalTexture())
					CubeMapFBO->attachCubeMapTexture(getActiveWindowPtr()->getFrameBufferTexture(CubeMapNormals), static_cast<unsigned int>(i), GL_COLOR_ATTACHMENT1);

				if (SGCTSettings::instance()->usePositionTexture())
					CubeMapFBO->attachCubeMapTexture(getActiveWindowPtr()->getFrameBufferTexture(CubeMapPositions), static_cast<unsigned int>(i), GL_COLOR_ATTACHMENT2);
			}
            
            //render
			(this->*mInternalDrawFn)();


			//copy AA-buffer to "regular"/non-AA buffer
			if( CubeMapFBO->isMultiSampled() )
			{
				CubeMapFBO->bindBlit(); //bind separate read and draw buffers to prepare blit operation

				//update attachments
				if( SGCTSettings::instance()->useDepthTexture() )
				{
					CubeMapFBO->attachDepthTexture( getActiveWindowPtr()->getFrameBufferTexture(FisheyeDepthSwap) );
					CubeMapFBO->attachColorTexture( getActiveWindowPtr()->getFrameBufferTexture(FisheyeColorSwap) );
				}
				else
					CubeMapFBO->attachCubeMapTexture( getActiveWindowPtr()->getFrameBufferTexture(CubeMap), static_cast<unsigned int>(i) );

				if (SGCTSettings::instance()->useNormalTexture())
					CubeMapFBO->attachCubeMapTexture(getActiveWindowPtr()->getFrameBufferTexture(CubeMapNormals), static_cast<unsigned int>(i), GL_COLOR_ATTACHMENT1);

				if (SGCTSettings::instance()->usePositionTexture())
					CubeMapFBO->attachCubeMapTexture(getActiveWindowPtr()->getFrameBufferTexture(CubeMapPositions), static_cast<unsigned int>(i), GL_COLOR_ATTACHMENT2);

				CubeMapFBO->blit();
			}

			//restore polygon mode
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

			//re-calculate depth values from a cube to spherical model
			if( SGCTSettings::instance()->useDepthTexture() )
			{
				GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
				CubeMapFBO->bind(false, 1, buffers); //bind no multi-sampled

				CubeMapFBO->attachCubeMapTexture( getActiveWindowPtr()->getFrameBufferTexture(CubeMap), static_cast<unsigned int>(i) );
				CubeMapFBO->attachCubeMapDepthTexture( getActiveWindowPtr()->getFrameBufferTexture(CubeMapDepth), static_cast<unsigned int>(i) );

				int cmRes = static_cast<int>(getActiveWindowPtr()->getCubeMapResolution());
				glViewport(0, 0, cmRes, cmRes);
				glScissor(currentViewportCoords[0],
					currentViewportCoords[1],
					currentViewportCoords[2],
					currentViewportCoords[3]);

				glPushAttrib(GL_ALL_ATTRIB_BITS);
				glEnable(GL_SCISSOR_TEST);

				mClearBufferFn();

				glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
				glMatrixMode(GL_TEXTURE);
				glLoadIdentity();

				glMatrixMode(GL_MODELVIEW); //restore

				//bind shader
				getActiveWindowPtr()->bindFisheyeDepthCorrectionShaderProgram();
				glUniform1i( getActiveWindowPtr()->getFisheyeSwapShaderColorLoc(), 0);
				glUniform1i( getActiveWindowPtr()->getFisheyeSwapShaderDepthLoc(), 1);
				glUniform1f( getActiveWindowPtr()->getFisheyeSwapShaderNearLoc(), mNearClippingPlaneDist);
				glUniform1f( getActiveWindowPtr()->getFisheyeSwapShaderFarLoc(), mFarClippingPlaneDist);

				glDisable( GL_CULL_FACE );
				if (getActiveWindowPtr()->getAlpha())
				{
					glEnable(GL_BLEND);
					glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
				}
				else
					glDisable(GL_BLEND);
				glEnable( GL_DEPTH_TEST );
				glDepthFunc( GL_ALWAYS );

				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, getActiveWindowPtr()->getFrameBufferTexture(FisheyeColorSwap));

				glActiveTexture(GL_TEXTURE1);
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, getActiveWindowPtr()->getFrameBufferTexture(FisheyeDepthSwap));

				glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
				//make sure that VBO:s are unbinded, to not mess up the vertex array
				getActiveWindowPtr()->bindVBO( SGCTWindow::RenderQuad );
				glClientActiveTexture(GL_TEXTURE0);

				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(0));

				glEnableClientState(GL_VERTEX_ARRAY);
				glVertexPointer(3, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(8));
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

				getActiveWindowPtr()->unbindVBO();

				glPopClientAttrib();
				ShaderProgram::unbind();
				glPopAttrib();
			}//end if depthmap
		}
	}//end for

	//restore polygon mode
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	//bind fisheye target FBO
	OffScreenBuffer * finalFBO = getActiveWindowPtr()->mFinalFBO_Ptr;
	finalFBO->bind();
	getActiveWindowPtr()->usePostFX() ?
			finalFBO->attachColorTexture( getActiveWindowPtr()->getFrameBufferTexture(Intermediate) ) :
			finalFBO->attachColorTexture( getActiveWindowPtr()->getFrameBufferTexture(ti) );

	if( SGCTSettings::instance()->useDepthTexture() )
		finalFBO->attachDepthTexture( getActiveWindowPtr()->getFrameBufferTexture(Depth) );

	if (SGCTSettings::instance()->useNormalTexture())
		finalFBO->attachColorTexture(getActiveWindowPtr()->getFrameBufferTexture(Normals), GL_COLOR_ATTACHMENT1);

	if (SGCTSettings::instance()->usePositionTexture())
		finalFBO->attachColorTexture(getActiveWindowPtr()->getFrameBufferTexture(Positions), GL_COLOR_ATTACHMENT2);

    enterFisheyeViewport();
	sgct::SGCTWindow::StereoMode sm = getActiveWindowPtr()->getStereoMode();
	if( !(sm >= SGCTWindow::Side_By_Side_Stereo && mActiveFrustumMode == Frustum::StereoRightEye) )
	{
		glClearColor(mFisheyeClearColor[0], mFisheyeClearColor[1], mFisheyeClearColor[2], mFisheyeClearColor[3]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW); //restore

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	//if for some reson the active texture has been reset
	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, getActiveWindowPtr()->getFrameBufferTexture(CubeMap));

	glDisable(GL_CULL_FACE);
	if (getActiveWindowPtr()->getAlpha())
	{
		glEnable(GL_BLEND);
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}
	else
		glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_ALWAYS );

	getActiveWindowPtr()->bindFisheyeShaderProgram();
	glUniform1i( getActiveWindowPtr()->getFisheyeShaderCubemapLoc(), 0);

	if( SGCTSettings::instance()->useDepthTexture() )
	{
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_CUBE_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, getActiveWindowPtr()->getFrameBufferTexture(CubeMapDepth));
		glUniform1i( getActiveWindowPtr()->getFisheyeShaderCubemapDepthLoc(), 1);
	}

	if (SGCTSettings::instance()->useNormalTexture())
	{
		glActiveTexture(GL_TEXTURE2);
		glEnable(GL_TEXTURE_CUBE_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, getActiveWindowPtr()->getFrameBufferTexture(CubeMapNormals));
		glUniform1i(getActiveWindowPtr()->getFisheyeShaderCubemapNormalsLoc(), 2);
	}

	if (SGCTSettings::instance()->usePositionTexture())
	{
		glActiveTexture(GL_TEXTURE3);
		glEnable(GL_TEXTURE_CUBE_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, getActiveWindowPtr()->getFrameBufferTexture(CubeMapPositions));
		glUniform1i(getActiveWindowPtr()->getFisheyeShaderCubemapPositionsLoc(), 3);
	}

	glUniform1f( getActiveWindowPtr()->getFisheyeShaderHalfFOVLoc(), glm::radians<float>(getActiveWindowPtr()->getFisheyeFOV()/2.0f) );

	if( getActiveWindowPtr()->isFisheyeOffaxis() )
	{
		glUniform3f( getActiveWindowPtr()->getFisheyeShaderOffsetLoc(),
			getActiveWindowPtr()->getFisheyeOffset(0),
			getActiveWindowPtr()->getFisheyeOffset(1),
			getActiveWindowPtr()->getFisheyeOffset(2) );
	}

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	//make sure that VBO:s are unbinded, to not mess up the vertex array
	getActiveWindowPtr()->bindVBO( SGCTWindow::FishEyeQuad );
	glClientActiveTexture(GL_TEXTURE0);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(0));

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(8));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	getActiveWindowPtr()->unbindVBO();

	ShaderProgram::unbind();

	glPopClientAttrib();

	glActiveTexture(GL_TEXTURE3);
	glDisable(GL_TEXTURE_CUBE_MAP);
	glActiveTexture(GL_TEXTURE2);
	glDisable(GL_TEXTURE_CUBE_MAP);
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_CUBE_MAP);
	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_CUBE_MAP);

	glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	if(mTakeScreenshot)
	{
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		int size = getActiveWindowPtr()->getXFramebufferResolution();
		unsigned int fontSize = 0;

		if( size > 512 )
			fontSize = static_cast<unsigned int>(size)/96;
		else if( size == 512 )
			fontSize = 8;

		if( fontSize != 0 )
		{
			float x = static_cast<float>( size - size/4 );
			float y = static_cast<float>( fontSize );

			//draw in pixel space / FBO space
			sgct_text::FontManager::instance()->setDrawInScreenSpace(false);
			sgct_text::print(sgct_text::FontManager::instance()->getFont( "SGCTFont", fontSize ), x, 2.0f * y + y/5.0f, "Frame#: %u", mShotCounter);

			if( mActiveFrustumMode == Frustum::Mono )
				sgct_text::print(sgct_text::FontManager::instance()->getFont( "SGCTFont", fontSize ), x, y, "Mono");
			else if( mActiveFrustumMode == Frustum::StereoLeftEye )
				sgct_text::print(sgct_text::FontManager::instance()->getFont( "SGCTFont", fontSize ), x, y, "Left");
			else
				sgct_text::print(sgct_text::FontManager::instance()->getFont( "SGCTFont", fontSize ), x, y, "Right");

			//restore: draw in point space / screen space
			sgct_text::FontManager::instance()->setDrawInScreenSpace(true);
		}
	}

	if (!getActiveWindowPtr()->getAlpha())
		glEnable(GL_BLEND);

	glDisable(GL_DEPTH_TEST);
	//if side-by-side and top-bottom mode only do post fx and blit only after rendered right eye
	bool split_screen_stereo = (sm >= sgct::SGCTWindow::Side_By_Side_Stereo);
	if( !( split_screen_stereo && mActiveFrustumMode == Frustum::StereoLeftEye) )
	{
		if( getActiveWindowPtr()->usePostFX() )
		{
			//blit buffers
			updateRenderingTargets(ti); //only used if multisampled FBOs

			(this->*mInternalRenderPostFXFn)(ti);

			render2D();
			if(split_screen_stereo)
			{
				//render left eye info and graph so that all 2D items are rendered after post fx
				mActiveFrustumMode = Frustum::StereoLeftEye;
				render2D();
			}
		}
		else
		{
			render2D();
			if(split_screen_stereo)
			{
				//render left eye info and graph so that all 2D items are rendered after post fx
				mActiveFrustumMode = Frustum::StereoLeftEye;
				render2D();
			}

			updateRenderingTargets(ti); //only used if multisampled FBOs
		}
	}

	glPopAttrib();
}

/*
	Works for fixed and programable pipeline
*/
void sgct::Engine::renderViewports(TextureIndexes ti)
{
	prepareBuffer( ti ); //attach FBO
	SGCTUser * usrPtr = ClusterManager::instance()->getUserPtr();

	sgct::SGCTWindow::StereoMode sm = getActiveWindowPtr()->getStereoMode();

	//render all viewports for selected eye
	for(unsigned int i=0; i<getActiveWindowPtr()->getNumberOfViewports(); i++)
	{
		getActiveWindowPtr()->setCurrentViewport(i);

		if( getActiveWindowPtr()->getCurrentViewport()->isEnabled() )
		{
			//if passive stereo or mono
			if( sm == SGCTWindow::No_Stereo )
				mActiveFrustumMode = getActiveWindowPtr()->getCurrentViewport()->getEye();

			if( getActiveWindowPtr()->getCurrentViewport()->isTracked() )
			{
				getActiveWindowPtr()->getCurrentViewport()->calculateFrustum(
					mActiveFrustumMode,
					usrPtr->getPosPtr(mActiveFrustumMode),
					mNearClippingPlaneDist,
					mFarClippingPlaneDist);
			}
			(this->*mInternalDrawFn)();
		}
	}

	if( mFixedOGLPipeline )
		glPushAttrib( GL_ALL_ATTRIB_BITS );
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	//if side-by-side and top-bottom mode only do post fx and blit only after rendered right eye
	bool split_screen_stereo = (sm >= sgct::SGCTWindow::Side_By_Side_Stereo);
	if( !( split_screen_stereo && mActiveFrustumMode == Frustum::StereoLeftEye) )
	{
		if( getActiveWindowPtr()->usePostFX() )
		{
			//blit buffers
			updateRenderingTargets(ti); //only used if multisampled FBOs

			(this->*mInternalRenderPostFXFn)(ti);

			render2D();
			if(split_screen_stereo)
			{
				//render left eye info and graph so that all 2D items are rendered after post fx
				mActiveFrustumMode = Frustum::StereoLeftEye;
				render2D();
			}
		}
		else
		{
			render2D();
			if(split_screen_stereo)
			{
				//render left eye info and graph so that all 2D items are rendered after post fx
				mActiveFrustumMode = Frustum::StereoLeftEye;
				render2D();
			}

			updateRenderingTargets(ti); //only used if multisampled FBOs
		}
	}

	glDisable(GL_BLEND);
	if( mFixedOGLPipeline )
		glPopAttrib();
}

/*!
	This function renders stats, OSD and overlays
*/
void sgct::Engine::render2D()
{
	//draw viewport overlays if any
	(this->*mInternalDrawOverlaysFn)();

	//draw info & stats
	//the cubemap viewports are all the same so it makes no sense to render everything several times
	//therefore just loop one iteration in that case.
	if( mShowGraph || mShowInfo || mDraw2DFn != NULL )
	{
		std::size_t numberOfIterations = (getActiveWindowPtr()->isUsingFisheyeRendering() ? 1 : getActiveWindowPtr()->getNumberOfViewports());
		for(std::size_t i=0; i < numberOfIterations; i++)
		{
			getActiveWindowPtr()->setCurrentViewport(i);
            
            if( getActiveWindowPtr()->getCurrentViewport()->isEnabled() )
            {
                getActiveWindowPtr()->isUsingFisheyeRendering() ? enterFisheyeViewport() : enterCurrentViewport();

                if( mShowGraph )
                    mStatistics->draw(
                        static_cast<float>(getActiveWindowPtr()->getYFramebufferResolution()) / static_cast<float>(getActiveWindowPtr()->getYResolution()));

                //The text renderer enters automatically the correct viewport
                if( mShowInfo )
                {
                    //choose specified eye from config
                    if( getActiveWindowPtr()->getStereoMode() == SGCTWindow::No_Stereo )
                        mActiveFrustumMode = getActiveWindowPtr()->getCurrentViewport()->getEye();
                    renderDisplayInfo();
                }

                if( mDraw2DFn != NULL )
                    mDraw2DFn();
            }
		}
	}
}

/*!
	This function combines a texture and a shader into a new texture
*/
void sgct::Engine::renderPostFX(TextureIndexes finalTargetIndex)
{
	glDrawBuffer( GL_COLOR_ATTACHMENT0 );

	PostFX * fx = NULL;
	PostFX * fxPrevious = NULL;

	std::size_t numberOfPasses = getActiveWindowPtr()->getNumberOfPostFXs();
	for( std::size_t i = 0; i<numberOfPasses; i++ )
	{
		fx = getActiveWindowPtr()->getPostFXPtr( i );

		//set output
		if( i == (numberOfPasses-1) && !getActiveWindowPtr()->useFXAA() ) //if last
			fx->setOutputTexture( getActiveWindowPtr()->getFrameBufferTexture( finalTargetIndex ) );
		else
			fx->setOutputTexture( getActiveWindowPtr()->getFrameBufferTexture( (i%2 == 0) ? FX1 : FX2 ) ); //ping pong between the two FX buffers

		//set input (dependent on output)
		if( i == 0 )
			fx->setInputTexture( getActiveWindowPtr()->getFrameBufferTexture( Intermediate ) );
		else
		{
			fxPrevious = getActiveWindowPtr()->getPostFXPtr( i-1 );
			fx->setInputTexture( fxPrevious->getOutputTexture() );
		}

		fx->render();
	}
	if( getActiveWindowPtr()->useFXAA() )
	{
		//bind target FBO
		getActiveWindowPtr()->mFinalFBO_Ptr->attachColorTexture( getActiveWindowPtr()->getFrameBufferTexture( finalTargetIndex ) );

		//if for some reson the active texture has been reset
		glViewport(0, 0, getActiveWindowPtr()->getXFramebufferResolution(), getActiveWindowPtr()->getYFramebufferResolution());
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);

		if( fx != NULL )
			glBindTexture(GL_TEXTURE_2D, fx->getOutputTexture() );
		else
			glBindTexture(GL_TEXTURE_2D, getActiveWindowPtr()->getFrameBufferTexture( Intermediate ) );

		mShaders[FXAAShader].bind();
		glUniform1f( mShaderLocs[SizeX], static_cast<float>(getActiveWindowPtr()->getXFramebufferResolution()) );
		glUniform1f( mShaderLocs[SizeY], static_cast<float>(getActiveWindowPtr()->getYFramebufferResolution()) );
		glUniform1i( mShaderLocs[FXAA_Texture], 0 );
		glUniform1f( mShaderLocs[FXAA_SUBPIX_TRIM], SGCTSettings::instance()->getFXAASubPixTrim() );
		glUniform1f( mShaderLocs[FXAA_SUBPIX_OFFSET], SGCTSettings::instance()->getFXAASubPixOffset() );

		getActiveWindowPtr()->bindVAO( SGCTWindow::RenderQuad );
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		getActiveWindowPtr()->unbindVAO();

		//unbind FXAA
		ShaderProgram::unbind();
	}
}

/*!
	This function combines a texture and a shader into a new texture
*/
void sgct::Engine::renderPostFXFixedPipeline(TextureIndexes finalTargetIndex)
{
	glDrawBuffer( GL_COLOR_ATTACHMENT0 );

	PostFX * fx = NULL;
	PostFX * fxPrevious = NULL;

	std::size_t numberOfPasses = getActiveWindowPtr()->getNumberOfPostFXs();
	if( numberOfPasses > 0 )
	{
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
	}

	for( std::size_t i = 0; i<numberOfPasses; i++ )
	{
		fx = getActiveWindowPtr()->getPostFXPtr( i );

		//set output
		if( i == (numberOfPasses-1) && !getActiveWindowPtr()->useFXAA() ) //if last
			fx->setOutputTexture( getActiveWindowPtr()->getFrameBufferTexture( finalTargetIndex ) );
		else
			fx->setOutputTexture( getActiveWindowPtr()->getFrameBufferTexture( (i%2 == 0) ? FX1 : FX2 ) ); //ping pong between the two FX buffers

		//set input (dependent on output)
		if( i == 0 )
			fx->setInputTexture( getActiveWindowPtr()->getFrameBufferTexture( Intermediate ) );
		else
		{
			fxPrevious = getActiveWindowPtr()->getPostFXPtr( i-1 );
			fx->setInputTexture( fxPrevious->getOutputTexture() );
		}

		fx->render();
	}

	if( numberOfPasses > 0 )
		glPopAttrib();

	if( getActiveWindowPtr()->useFXAA() )
	{
		//bind target FBO
		getActiveWindowPtr()->mFinalFBO_Ptr->attachColorTexture( getActiveWindowPtr()->getFrameBufferTexture( finalTargetIndex ) );

		//if for some reson the active texture has been reset
		glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();

		glMatrixMode(GL_MODELVIEW); //restore
		glViewport(0, 0, getActiveWindowPtr()->getXFramebufferResolution(), getActiveWindowPtr()->getYFramebufferResolution());
        
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glPushAttrib(GL_ALL_ATTRIB_BITS);

		glEnable(GL_TEXTURE_2D);
		if( fx != NULL )
			glBindTexture(GL_TEXTURE_2D, fx->getOutputTexture() );
		else
			glBindTexture(GL_TEXTURE_2D, getActiveWindowPtr()->getFrameBufferTexture( Intermediate ) );

		glDisable(GL_CULL_FACE);
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);

		mShaders[FXAAShader].bind();
		glUniform1f( mShaderLocs[SizeX], static_cast<float>(getActiveWindowPtr()->getXFramebufferResolution()) );
		glUniform1f( mShaderLocs[SizeY], static_cast<float>(getActiveWindowPtr()->getYFramebufferResolution()) );
		glUniform1i( mShaderLocs[FXAA_Texture], 0 );
		glUniform1f( mShaderLocs[FXAA_SUBPIX_TRIM], SGCTSettings::instance()->getFXAASubPixTrim() );
		glUniform1f( mShaderLocs[FXAA_SUBPIX_OFFSET], SGCTSettings::instance()->getFXAASubPixOffset() );

		glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

		getActiveWindowPtr()->bindVBO( SGCTWindow::RenderQuad );
		glClientActiveTexture(GL_TEXTURE0);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(0));

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(8));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		getActiveWindowPtr()->unbindVBO();

		ShaderProgram::unbind();

		glPopClientAttrib();
		glPopAttrib();
	}
}

/*!
	This function updates the renderingtargets.
*/
void sgct::Engine::updateRenderingTargets(TextureIndexes ti)
{
	//copy AA-buffer to "regular"/non-AA buffer
	OffScreenBuffer * fbo = getActiveWindowPtr()->mFinalFBO_Ptr;
	if( fbo->isMultiSampled() )
	{
		if( getActiveWindowPtr()->usePostFX() )
			ti = Intermediate;

		fbo->bindBlit(); //bind separate read and draw buffers to prepare blit operation

		//update attachments
		fbo->attachColorTexture( getActiveWindowPtr()->getFrameBufferTexture(ti) );

		if( SGCTSettings::instance()->useDepthTexture() )
			fbo->attachDepthTexture( getActiveWindowPtr()->getFrameBufferTexture( Depth ) );

		if (SGCTSettings::instance()->useNormalTexture())
			fbo->attachColorTexture(getActiveWindowPtr()->getFrameBufferTexture(Normals), GL_COLOR_ATTACHMENT1);

		if (SGCTSettings::instance()->usePositionTexture())
			fbo->attachColorTexture(getActiveWindowPtr()->getFrameBufferTexture(Positions), GL_COLOR_ATTACHMENT2);

		fbo->blit();
	}
}

/*!
	This function updates the timers.
*/
void sgct::Engine::updateTimers(double timeStamp)
{
	// check all timers if one of them has expired
    if ( isMaster() )
    {
        for( size_t i = 0; i < mTimers.size(); ++i )
        {
            TimerInformation& currentTimer = mTimers[i];
            const double timeSinceLastFiring = timeStamp - currentTimer.mLastFired;
            if( timeSinceLastFiring > currentTimer.mInterval )
            {
                currentTimer.mLastFired = timeStamp;
                currentTimer.mCallback(currentTimer.mId);
            }
        }
    }
}

/*!
	This function loads shaders that handles different 3D modes.
	The shaders are only loaded once in the initOGL function.
*/
void sgct::Engine::loadShaders()
{
	//create FXAA shaders
	mShaders[FXAAShader].setName("FXAAShader");
	std::string fxaa_vert_shader;
	std::string fxaa_frag_shader;
	
	if( mFixedOGLPipeline )
	{
		fxaa_vert_shader = shaders::FXAA_Vert_Shader;
		fxaa_frag_shader = shaders::FXAA_Frag_Shader;
	}
	else
	{
		fxaa_vert_shader = shaders_modern::FXAA_Vert_Shader;
		fxaa_frag_shader = shaders_modern::FXAA_Frag_Shader;
	}

	//replace glsl version
	sgct_helpers::findAndReplace(fxaa_vert_shader, "**glsl_version**", Engine::instance()->getGLSLVersion());
	sgct_helpers::findAndReplace(fxaa_frag_shader, "**glsl_version**", Engine::instance()->getGLSLVersion());

	mShaders[FXAAShader].addShaderSrc(fxaa_vert_shader, GL_VERTEX_SHADER, ShaderProgram::SHADER_SRC_STRING);
	mShaders[FXAAShader].addShaderSrc(fxaa_frag_shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING);
	mShaders[FXAAShader].createAndLinkProgram();
	mShaders[FXAAShader].bind();

	mShaderLocs[SizeX] = mShaders[FXAAShader].getUniformLocation( "rt_w" );
	glUniform1f( mShaderLocs[SizeX], static_cast<float>( getActiveWindowPtr()->getXFramebufferResolution()) );

	mShaderLocs[SizeY] = mShaders[FXAAShader].getUniformLocation( "rt_h" );
	glUniform1f( mShaderLocs[SizeY], static_cast<float>( getActiveWindowPtr()->getYFramebufferResolution()) );

	mShaderLocs[FXAA_SUBPIX_TRIM] = mShaders[FXAAShader].getUniformLocation( "FXAA_SUBPIX_TRIM" );
	glUniform1f( mShaderLocs[FXAA_SUBPIX_TRIM], SGCTSettings::instance()->getFXAASubPixTrim() );

	mShaderLocs[FXAA_SUBPIX_OFFSET] = mShaders[FXAAShader].getUniformLocation( "FXAA_SUBPIX_OFFSET" );
	glUniform1f( mShaderLocs[FXAA_SUBPIX_OFFSET], SGCTSettings::instance()->getFXAASubPixOffset() );

	mShaderLocs[FXAA_Texture] = mShaders[FXAAShader].getUniformLocation( "tex" );
	glUniform1i( mShaderLocs[FXAA_Texture], 0 );

	ShaderProgram::unbind();

	/*!
		Used for overlays & mono.
	*/
	if( !mFixedOGLPipeline )
	{
		std::string FBO_quad_vert_shader;
		std::string FBO_quad_frag_shader;
		FBO_quad_vert_shader = shaders_modern::Base_Vert_Shader;
		FBO_quad_frag_shader = shaders_modern::Base_Frag_Shader;
		
		//replace glsl version
		sgct_helpers::findAndReplace(FBO_quad_vert_shader, "**glsl_version**", Engine::instance()->getGLSLVersion());
		sgct_helpers::findAndReplace(FBO_quad_frag_shader, "**glsl_version**", Engine::instance()->getGLSLVersion());
		
		mShaders[FBOQuadShader].setName("FBOQuadShader");
		mShaders[FBOQuadShader].addShaderSrc(FBO_quad_vert_shader, GL_VERTEX_SHADER, ShaderProgram::SHADER_SRC_STRING);
		mShaders[FBOQuadShader].addShaderSrc(FBO_quad_frag_shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING);
		mShaders[FBOQuadShader].createAndLinkProgram();
		mShaders[FBOQuadShader].bind();
		mShaderLocs[MonoTex] = mShaders[FBOQuadShader].getUniformLocation( "Tex" );
		glUniform1i( mShaderLocs[MonoTex], 0 );
		ShaderProgram::unbind();
        
        std::string Overlay_vert_shader;
        std::string Overlay_frag_shader;
        Overlay_vert_shader = shaders_modern::Overlay_Vert_Shader;
        Overlay_frag_shader = shaders_modern::Overlay_Frag_Shader;
        
        //replace glsl version
        sgct_helpers::findAndReplace(Overlay_vert_shader, "**glsl_version**", Engine::instance()->getGLSLVersion());
        sgct_helpers::findAndReplace(Overlay_frag_shader, "**glsl_version**", Engine::instance()->getGLSLVersion());
        
        mShaders[OverlayShader].setName("OverlayShader");
        mShaders[OverlayShader].addShaderSrc(Overlay_vert_shader, GL_VERTEX_SHADER, ShaderProgram::SHADER_SRC_STRING);
        mShaders[OverlayShader].addShaderSrc(Overlay_frag_shader, GL_FRAGMENT_SHADER, ShaderProgram::SHADER_SRC_STRING);
        mShaders[OverlayShader].createAndLinkProgram();
        mShaders[OverlayShader].bind();
        mShaderLocs[OverlayTex] = mShaders[OverlayShader].getUniformLocation( "Tex" );
        glUniform1i( mShaderLocs[OverlayTex], 0 );
        ShaderProgram::unbind();
	}
}

/*!
	\param mode is the one of the following:

	- Backbuffer (transparent)
	- Backbuffer (black)
	- RenderToTexture

	This function clears and sets the appropriate buffer from:

	- Back buffer
	- Left back buffer
	- Right back buffer
*/
void sgct::Engine::setAndClearBuffer(sgct::Engine::BufferMode mode)
{
	if(mode < RenderToTexture)
	{
		//Set buffer
		if( getActiveWindowPtr()->getStereoMode() != SGCTWindow::Active_Stereo )
		{
			glDrawBuffer(GL_BACK);
			glReadBuffer(GL_BACK);
		}
		else if( mActiveFrustumMode == Frustum::StereoLeftEye ) //if active left
		{
			glDrawBuffer(GL_BACK_LEFT);
			glReadBuffer(GL_BACK_LEFT);
		}
		else if( mActiveFrustumMode == Frustum::StereoRightEye ) //if active right
		{
			glDrawBuffer(GL_BACK_RIGHT);
			glReadBuffer(GL_BACK_RIGHT);
		}
	}

	//clear
	if( mode != BackBufferBlack && mClearBufferFn != NULL )
	{
		mClearBufferFn();
	}
	else //when rendering textures to backbuffer (using fbo)
	{
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}
}

/*!
	This functions checks for OpenGL errors and prints them using the MessageHandler (to commandline).
	Avoid this function in the render loop for release code since it can reduce performance.

	\returns true if no errors occured
*/
bool sgct::Engine::checkForOGLErrors()
{
	GLenum oglError = glGetError();

	switch( oglError )
	{
	case GL_INVALID_ENUM:
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "OpenGL error: GL_INVALID_ENUM\n");
		break;

	case GL_INVALID_VALUE:
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "OpenGL error: GL_INVALID_VALUE\n");
		break;

	case GL_INVALID_OPERATION:
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "OpenGL error: GL_INVALID_OPERATION\n");
		break;

	case GL_INVALID_FRAMEBUFFER_OPERATION:
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "OpenGL error: GL_INVALID_FRAMEBUFFER_OPERATION\n");
		break;

	case GL_STACK_OVERFLOW:
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "OpenGL error: GL_STACK_OVERFLOW\n");
		break;

	case GL_STACK_UNDERFLOW:
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "OpenGL error: GL_STACK_UNDERFLOW\n");
		break;

	case GL_OUT_OF_MEMORY:
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "OpenGL error: GL_OUT_OF_MEMORY\n");
		break;

	case GL_TABLE_TOO_LARGE:
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "OpenGL error: GL_TABLE_TOO_LARGE\n");
		break;
	}

	return oglError == GL_NO_ERROR; //returns true if no errors
}

/*!
	This function waits for all windows to be created on the whole cluster in order to set the barrier (hardware swap-lock).
	Under some Nvida drivers the stability is improved by first join a swapgroup and then set the barrier then all windows in a swapgroup are created.
*/
void sgct::Engine::waitForAllWindowsInSwapGroupToOpen()
{
	//clear the buffers initially
	for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
	{
		mThisNode->getWindowPtr(i)->makeOpenGLContextCurrent( SGCTWindow::Window_Context );
		glDrawBuffer(GL_BACK);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glfwSwapBuffers( mThisNode->getWindowPtr(i)->getWindowHandle() );
	}
	glfwPollEvents();
	
	//Must wait until all nodes are running if using swap barrier
	if( !mIgnoreSync && ClusterManager::instance()->getNumberOfNodes() > 1)
	{
		//check if swapgroups are supported
		#ifdef __WIN32__
		if (glfwExtensionSupported("WGL_NV_swap_group"))
			MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Swap groups are supported by hardware.\n");
		#else
		if( glfwExtensionSupported("GLX_NV_swap_group") )
			MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Swap groups are supported by hardware.\n");
		#endif
		else
			MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Swap groups are not supported by hardware.\n");

		MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Waiting for all nodes to connect.\n");
        MessageHandler::instance()->setShowTime(false);
        
		while(mNetworkConnections->isRunning() &&
			!mThisNode->getKeyPressed( mExitKey ) &&
			!mThisNode->shouldAllWindowsClose() &&
			!mTerminate)
		{
			MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, ".");

			// Swap front and back rendering buffers
			for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
				glfwSwapBuffers( mThisNode->getWindowPtr(i)->getWindowHandle() );
			glfwPollEvents();

			if(mNetworkConnections->areAllNodesConnected())
				break;

			sleep( 0.1 );
		}
		MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "\n");

		//wait for user to release exit key
		while( mThisNode->getKeyPressed( mExitKey ) )
		{
			// Swap front and back rendering buffers
			// key buffers also swapped
			for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
				glfwSwapBuffers( mThisNode->getWindowPtr(i)->getWindowHandle() );
			glfwPollEvents();
            
            MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, ".");

			tthread::this_thread::sleep_for(tthread::chrono::milliseconds(50));
		}

		MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "\n");
        MessageHandler::instance()->setShowTime(true);
	}
}

/*!
	This functions updates the frustum of all viewports on demand. However if the viewport is tracked this is done on the fly.
*/
void sgct::Engine::updateFrustums()
{
	for(size_t w=0; w < mThisNode->getNumberOfWindows(); w++)
	{
		SGCTWindow * winPtr = mThisNode->getWindowPtr(w);

		for(unsigned int i=0; i < winPtr->getNumberOfViewports(); i++)
			if( !winPtr->getViewport(i)->isTracked() ) //if not tracked update, otherwise this is done on the fly
			{
				SGCTUser * usrPtr = ClusterManager::instance()->getUserPtr();

				if( !winPtr->isUsingFisheyeRendering() )
				{
					winPtr->getViewport(i)->calculateFrustum(
						Frustum::Mono,
						usrPtr->getPosPtr(),
						mNearClippingPlaneDist,
						mFarClippingPlaneDist);

					winPtr->getViewport(i)->calculateFrustum(
						Frustum::StereoLeftEye,
						usrPtr->getPosPtr(Frustum::StereoLeftEye),
						mNearClippingPlaneDist,
						mFarClippingPlaneDist);

					winPtr->getViewport(i)->calculateFrustum(
						Frustum::StereoRightEye,
						usrPtr->getPosPtr(Frustum::StereoRightEye),
						mNearClippingPlaneDist,
						mFarClippingPlaneDist);
				}
				else
				{
					winPtr->getViewport(i)->calculateFisheyeFrustum(
						Frustum::Mono,
						usrPtr->getPosPtr(),
						usrPtr->getPosPtr(),
						mNearClippingPlaneDist,
						mFarClippingPlaneDist);

					winPtr->getViewport(i)->calculateFisheyeFrustum(
						Frustum::StereoLeftEye,
						usrPtr->getPosPtr(),
						usrPtr->getPosPtr(Frustum::StereoLeftEye),
						mNearClippingPlaneDist,
						mFarClippingPlaneDist);

					winPtr->getViewport(i)->calculateFisheyeFrustum(
						Frustum::StereoRightEye,
						usrPtr->getPosPtr(),
						usrPtr->getPosPtr(Frustum::StereoRightEye),
						mNearClippingPlaneDist,
						mFarClippingPlaneDist);
				}
			}
	}
}

/*!
	\param argc is the number of arguments separated by whitespace
	\param argv is the string array of arguments

	This function parses all SGCT arguments and removes them from the argument list.
*/
void sgct::Engine::parseArguments( int& argc, char**& argv )
{
	//parse arguments
	MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Parsing arguments...\n");
	int i=0;
    std::deque<int> argumentsToRemove;
	while( i<argc )
	{
		if( strcmp(argv[i],"-config") == 0 && argc > (i+1))
		{
			configFilename.assign(argv[i+1]);
            argumentsToRemove.push_back(i);
            argumentsToRemove.push_back(i+1);
			i+=2;
		}
		else if( strcmp(argv[i],"--client") == 0 )
		{
			localRunningMode = NetworkManager::LocalClient;
            argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--slave") == 0 )
		{
			localRunningMode = NetworkManager::LocalClient;
            argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--debug") == 0 )
		{
			MessageHandler::instance()->setNotifyLevel( MessageHandler::NOTIFY_DEBUG );
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--help") == 0 )
		{
			mHelpMode = true;
			outputHelpMessage();
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"-local") == 0 && argc > (i+1) )
		{
			localRunningMode = NetworkManager::LocalServer;
			int tmpi = -1;
			std::stringstream ss( argv[i+1] );
			ss >> tmpi;
			ClusterManager::instance()->setThisNodeId(tmpi);
            argumentsToRemove.push_back(i);
            argumentsToRemove.push_back(i+1);
			i+=2;
		}
		else if( strcmp(argv[i],"-logPath") == 0 )
		{
			//Remove unwanted chars
			std::string tmpStr( argv[i+1] );
			tmpStr.erase( remove( tmpStr.begin(), tmpStr.end(), '\"' ), tmpStr.end() );
			std::size_t lastPos = tmpStr.length() - 1;
			
            const char last = tmpStr.at( lastPos );
			if( last == '\\' || last == '/' )
				tmpStr.erase( lastPos );

            mLogfilePath.assign( tmpStr );

			argumentsToRemove.push_back(i);
            argumentsToRemove.push_back(i+1);
			i+=2;
		}
		else if( strcmp(argv[i],"-notify") == 0 && argc > (i+1) )
		{
			localRunningMode = NetworkManager::LocalServer;
			int tmpi = -1;
			std::stringstream ss( argv[i+1] );
			ss >> tmpi;
			if( tmpi != -1 )
				MessageHandler::instance()->setNotifyLevel( static_cast<MessageHandler::NotifyLevel>( tmpi ) );
            argumentsToRemove.push_back(i);
            argumentsToRemove.push_back(i+1);
			i+=2;
		}
		else if( strcmp(argv[i],"--Firm-Sync") == 0 )
		{
			ClusterManager::instance()->setFirmFrameLockSyncStatus(true);
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--Loose-Sync") == 0 )
		{
			ClusterManager::instance()->setFirmFrameLockSyncStatus(false);
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--Ignore-Sync") == 0 )
		{
			mIgnoreSync = true;
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--No-Sync") == 0 )
		{
			mIgnoreSync = true;
			argumentsToRemove.push_back(i);
			i++;
		}
		else if (strcmp(argv[i], "--gDebugger") == 0)
		{
			SGCTSettings::instance()->setForceGlTexImage2D(true);
			argumentsToRemove.push_back(i);
			i++;
		}
		else if (strcmp(argv[i], "--FXAA") == 0)
		{
			SGCTSettings::instance()->setDefaultFXAAState(true);
			argumentsToRemove.push_back(i);
			i++;
		}
		else if (strcmp(argv[i], "-MSAA") == 0 && argc > (i + 1))
		{
			int tmpi = -1;
			std::stringstream ss(argv[i + 1]);
			ss >> tmpi;
			SGCTSettings::instance()->setDefaultNumberOfAASamples(tmpi);
			argumentsToRemove.push_back(i);
			argumentsToRemove.push_back(i + 1);
			i += 2;
		}
		else if( strcmp(argv[i],"--No-FBO") == 0 )
		{
			SGCTSettings::instance()->setUseFBO(false);
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--Capture-TGA") == 0 )
		{
			SGCTSettings::instance()->setCaptureFormat("TGA");
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--Capture-PNG") == 0 )
		{
			SGCTSettings::instance()->setCaptureFormat("PNG");
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"-numberOfCaptureThreads") == 0 && argc > (i+1) )
		{
			int tmpi = -1;
			std::stringstream ss( argv[i+1] );
			ss >> tmpi;

			if(tmpi > 0)
				SGCTSettings::instance()->setNumberOfCaptureThreads( tmpi );

            argumentsToRemove.push_back(i);
            argumentsToRemove.push_back(i+1);
			i+=2;
		}
		else
			i++; //iterate
	}

    // remove the arguments that have been processed
    if( argumentsToRemove.size() > 0 )
    {
        int newArgc = argc - static_cast<int>(argumentsToRemove.size());
        char** newArgv = new char*[newArgc];
        int newIterator = 0;
        for( int oldIterator = 0; oldIterator < argc; ++oldIterator )
        {
            if( !argumentsToRemove.empty() && oldIterator == argumentsToRemove.front())
            {
                argumentsToRemove.pop_front();
            }
            else
            {
                newArgv[newIterator] = argv[oldIterator];
                newIterator++;
            }
        }
        argv = newArgv;
        argc = newArgc;
    }

	MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Done\n");
}

/*!
	\param fnPtr is the function pointer to a draw callback

	This function sets the draw callback. It's possible to have several draw functions and change the callback on the fly preferably in a stage before the draw like the post-sync-pre-draw stage or the pre-sync stage.
	The draw callback can be called several times per frame since it's called once for every viewport and once for every eye if stereoscopy is used.
*/
void sgct::Engine::setDrawFunction(void(*fnPtr)(void))
{
	mDrawFn = fnPtr;
}

/*!
	\param fnPtr is the function pointer to a draw 2D callback

	This function sets the draw 2D callback. This callback will be called after overlays and post effects has been drawn.
	This makes it possible to render text and HUDs that will not be filtered and antialiasied.
*/
void sgct::Engine::setDraw2DFunction( void(*fnPtr)(void) )
{
	mDraw2DFn = fnPtr;
}

/*!
	\param fnPtr is the function pointer to a pre-sync callback

	This function sets the pre-sync callback. The Engine will then use the callback before the sync stage.
	In the callback set the variables that will be shared.
*/
void sgct::Engine::setPreSyncFunction(void(*fnPtr)(void))
{
	mPreSyncFn = fnPtr;
}

/*!
	\param fnPtr is the function pointer to a post-sync-pre-draw callback

	This function sets the post-sync-pre-draw callback. The Engine will then use the callback after the sync stage but before the draw stage. Compared to the draw callback the post-sync-pre-draw callback is called only once per frame.
	In this callback synchronized variables can be applied or simulations depending on synchronized input can run.
*/
void sgct::Engine::setPostSyncPreDrawFunction(void(*fnPtr)(void))
{
	mPostSyncPreDrawFn = fnPtr;
}

/*!
	\param fnPtr is the function pointer to a post-draw callback

	This function sets the post-draw callback. The Engine will then use the callback after the draw stage but before the OpenGL buffer swap. Compared to the draw callback the post-draw callback is called only once per frame.
	In this callback data/buffer swaps can be made.
*/
void sgct::Engine::setPostDrawFunction(void(*fnPtr)(void))
{
	mPostDrawFn = fnPtr;
}

/*!
	\param fnPtr is the function pointer to a initiation of OpenGL callback

	This function sets the initOGL callback. The Engine will then use the callback only once before the starting the render loop.
	Textures, Models, Buffers, etc. can be loaded/allocated here.
*/
void sgct::Engine::setInitOGLFunction(void(*fnPtr)(void))
{
	mInitOGLFn = fnPtr;
}

/*!
	This callback is called before the window is created (before OpenGL context is created).
	At this stage the config file has been read and network initialized. Therefore it's suitable for loading master or slave specific data.

	\param fnPtr is the function pointer to a pre window creation callback
*/
void sgct::Engine::setPreWindowFunction( void(*fnPtr)(void) )
{
	mPreWindowFn = fnPtr;
}

/*!
	\param fnPtr is the function pointer to a clear buffer function callback

	This function sets the clear buffer callback which will override the default clear buffer function:

	\code
	void sgct::Engine::clearBuffer(void)
	{
		const float * colorPtr = sgct::Engine::getPtr()->getClearColor();
		glClearColor(colorPtr[0], colorPtr[1], colorPtr[2], colorPtr[3]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	\endcode
*/
void sgct::Engine::setClearBufferFunction(void(*fnPtr)(void))
{
	mClearBufferFn = fnPtr;
}

/*!
	\param fnPtr is the function pointer to a clean up function callback

	This function sets the clean up callback which will be called in the Engine destructor before all sgct components (like window, OpenGL context, network, etc.) will be destroyed.
*/
void sgct::Engine::setCleanUpFunction( void(*fnPtr)(void) )
{
	mCleanUpFn = fnPtr;
}

/*!
	\param fnPtr is the function pointer to an external control message callback

	This function sets the external control message callback which will be called when a TCP message is received. The TCP listner is enabled in the XML configuration file in the Cluster tag by externalControlPort, where the portnumber is an integer preferably above 20000.
	Example:
	\code
	<Cluster masterAddress="127.0.0.1" externalControlPort="20500">
	\endcode

	All TCP messages must be separated by carriage return (CR) followed by a newline (NL). Look at this [tutorial](https://c-student.itn.liu.se/wiki/develop:sgcttutorials:externalguicsharp) for more info.

*/
void sgct::Engine::setExternalControlCallback(void(*fnPtr)(const char *, int, int))
{
	mNetworkMessageCallbackFn = fnPtr;
}

/*!
	\param fnPtr is the function pointer to an external control status callback

	This function sets the external control status callback which will be called when the connection status changes (connect or disconnect).

*/
void sgct::Engine::setExternalControlStatusCallback(void(*fnPtr)(bool, int))
{
	mNetworkStatusCallbackFn = fnPtr;
}

/*!
 \param fnPtr is the function pointer to a screenshot callback for custom frame capture & export
 This callback must be set before Engine::init is called\n
 Parameters to the callback are: Image pointer for image data, window index, eye index
 */
void sgct::Engine::setScreenShotCallback(void(*fnPtr)(Image *, std::size_t, ScreenCapture::EyeIndex))
{
    mScreenShotFn = fnPtr;
}

/*!
	\param fnPtr is the function pointer to a keyboard callback function

	This function sets the keyboard callback (GLFW wrapper) where the two parameters are: int key, int action. Key can be a character (e.g. 'A', 'B', '5' or ',') or a special character defined in the table below. Action can either be SGCT_PRESS or SGCT_RELEASE.
	All windows are connected to this callback.

	Name          | Description
	------------- | -------------
	SGCT_KEY_UNKNOWN  | Unknown
	SGCT_KEY_SPACE  | Space
	SGCT_KEY_APOSTROPHE | Apostrophe
	SGCT_KEY_COMMA | Comma
	SGCT_KEY_MINUS | Minus
	SGCT_KEY_PERIOD | Period
	SGCT_KEY_SLASH | Slash
	SGCT_KEY_0 | 0
	SGCT_KEY_1 | 1
	SGCT_KEY_2 | 2
	SGCT_KEY_3 | 3
	SGCT_KEY_4 | 4
	SGCT_KEY_5 | 5
	SGCT_KEY_6 | 6
	SGCT_KEY_7 | 7
	SGCT_KEY_8 | 8
	SGCT_KEY_9 | 9
	SGCT_KEY_SEMICOLON | Semicolon
	SGCT_KEY_EQUAL | Equal
	SGCT_KEY_A | A
	SGCT_KEY_B | B
	SGCT_KEY_C | C
	SGCT_KEY_D | D
	SGCT_KEY_E | E
	SGCT_KEY_F | F
	SGCT_KEY_G | G
	SGCT_KEY_H | H
	SGCT_KEY_I | I
	SGCT_KEY_J | J
	SGCT_KEY_K | K
	SGCT_KEY_L | L
	SGCT_KEY_M | M
	SGCT_KEY_N | N
	SGCT_KEY_O | O
	SGCT_KEY_P | P
	SGCT_KEY_Q | Q
	SGCT_KEY_R | R
	SGCT_KEY_S | S
	SGCT_KEY_T | T
	SGCT_KEY_U | U
	SGCT_KEY_V | V
	SGCT_KEY_W | W
	SGCT_KEY_X | X
	SGCT_KEY_Y | Y
	SGCT_KEY_Z | Z
	SGCT_KEY_LEFT_BRACKET | Left bracket
	SGCT_KEY_BACKSLASH | backslash
	SGCT_KEY_RIGHT_BRACKET | Right bracket
	SGCT_KEY_GRAVE_ACCENT | Grave accent
	SGCT_KEY_WORLD_1 | World 1
	SGCT_KEY_WORLD_2 | World 2
	SGCT_KEY_ESC | Escape
	SGCT_KEY_ESCAPE | Escape
	SGCT_KEY_ENTER | Enter
	SGCT_KEY_TAB | Tab
	SGCT_KEY_BACKSPACE | Backspace
	SGCT_KEY_INSERT | Insert
	SGCT_KEY_DEL | Delete
	SGCT_KEY_DELETE | Delete
	SGCT_KEY_RIGHT | Right
	SGCT_KEY_LEFT | Left
	SGCT_KEY_DOWN | Down
	SGCT_KEY_UP | Up
	SGCT_KEY_PAGEUP | Page up
	SGCT_KEY_PAGEDOWN | Page down
	SGCT_KEY_PAGE_UP | Page up
	SGCT_KEY_PAGE_DOWN | Page down
	SGCT_KEY_HOME | Home
	SGCT_KEY_END | End
	SGCT_KEY_CAPS_LOCK | Caps lock
	SGCT_KEY_SCROLL_LOCK | Scroll lock
	SGCT_KEY_NUM_LOCK | Num lock
	SGCT_KEY_PRINT_SCREEN | Print screen
	SGCT_KEY_PAUSE | Pause
	SGCT_KEY_F1 | F1
	SGCT_KEY_F2 | F2
	SGCT_KEY_F3 | F3
	SGCT_KEY_F4 | F4
	SGCT_KEY_F5 | F5
	SGCT_KEY_F6 | F6
	SGCT_KEY_F7 | F7
	SGCT_KEY_F8 | F8
	SGCT_KEY_F9 | F9
	SGCT_KEY_F10 | F10
	SGCT_KEY_F11 | F11
	SGCT_KEY_F12 | F12
	SGCT_KEY_F13 | F13
	SGCT_KEY_F14 | F14
	SGCT_KEY_F15 | F15
	SGCT_KEY_F16 | F16
	SGCT_KEY_F17 | F17
	SGCT_KEY_F18 | F18
	SGCT_KEY_F19 | F19
	SGCT_KEY_F20 | F20
	SGCT_KEY_F21 | F21
	SGCT_KEY_F22 | F22
	SGCT_KEY_F23 | F23
	SGCT_KEY_F24 | F24
	SGCT_KEY_F25 | F25
	SGCT_KEY_KP_0 | Keypad 0
	SGCT_KEY_KP_1 | Keypad 1
	SGCT_KEY_KP_2 | Keypad 2
	SGCT_KEY_KP_3 | Keypad 3
	SGCT_KEY_KP_4 | Keypad 4
	SGCT_KEY_KP_5 | Keypad 5
	SGCT_KEY_KP_6 | Keypad 6
	SGCT_KEY_KP_7 | Keypad 7
	SGCT_KEY_KP_8 | Keypad 8
	SGCT_KEY_KP_9 | Keypad 9
	SGCT_KEY_KP_DECIMAL| Keypad decimal
	SGCT_KEY_KP_DIVIDE | Keypad divide
	SGCT_KEY_KP_MULTIPLY | Keypad multiply
	SGCT_KEY_KP_SUBTRACT | Keypad subtract
	SGCT_KEY_KP_ADD | Keypad add
	SGCT_KEY_KP_ENTER | Keypad enter
	SGCT_KEY_KP_EQUAL | Keypad equal
	SGCT_KEY_LSHIFT | Left shift
	SGCT_KEY_LEFT_SHIFT | Left shift
	SGCT_KEY_LCTRL | Left control
	SGCT_KEY_LEFT_CONTROL | Left control
	SGCT_KEY_LALT | Left alt
	SGCT_KEY_LEFT_ALT | Left alt
	SGCT_KEY_LEFT_SUPER | Left super
	SGCT_KEY_RSHIFT | Right shift
	SGCT_KEY_RIGHT_SHIFT | Right shift
	SGCT_KEY_RCTRL | Right control
	SGCT_KEY_RIGHT_CONTROL | Right control
	SGCT_KEY_RALT | Right alt
	SGCT_KEY_RIGHT_ALT | Right alt
	SGCT_KEY_RIGHT_SUPER | Right super
	SGCT_KEY_MENU | Menu
	SGCT_KEY_LAST | Last key index

*/
void sgct::Engine::setKeyboardCallbackFunction( void(*fnPtr)(int,int) )
{
	gKeyboardCallbackFn = fnPtr;
}

/*!
All windows are connected to this callback.
*/
void sgct::Engine::setCharCallbackFunction( void(*fnPtr)(unsigned int) )
{
	gCharCallbackFn = fnPtr;
}
/*!
	\param fnPtr is the function pointer to a mouse button callback function

	This function sets the mouse button callback (GLFW wrapper) where the two parameters are: int button, int action. Button id's are listed in the table below. Action can either be SGCT_PRESS or SGCT_RELEASE.
	All windows are connected to this callback.

	Name          | Description
	------------- | -------------
	SGCT_MOUSE_BUTTON_LEFT | Left button
	SGCT_MOUSE_BUTTON_RIGHT | Right button
	SGCT_MOUSE_BUTTON_MIDDLE | Middle button
	SGCT_MOUSE_BUTTON_1 | Button 1
	SGCT_MOUSE_BUTTON_2 | Button 2
	SGCT_MOUSE_BUTTON_3 | Button 3
	SGCT_MOUSE_BUTTON_4 | Button 4
	SGCT_MOUSE_BUTTON_5 | Button 5
	SGCT_MOUSE_BUTTON_6 | Button 6
	SGCT_MOUSE_BUTTON_7 | Button 7
	SGCT_MOUSE_BUTTON_8 | Button 8
	SGCT_MOUSE_BUTTON_LAST | Last mouse button index

*/
void sgct::Engine::setMouseButtonCallbackFunction( void(*fnPtr)(int, int) )
{
	gMouseButtonCallbackFn = fnPtr;
}

/*!
All windows are connected to this callback.
*/
void sgct::Engine::setMousePosCallbackFunction( void(*fnPtr)(double, double) )
{
	gMousePosCallbackFn = fnPtr;
}

/*!
All windows are connected to this callback.
*/
void sgct::Engine::setMouseScrollCallbackFunction( void(*fnPtr)(double, double) )
{
	gMouseScrollCallbackFn = fnPtr;
}

void sgct::Engine::clearBuffer()
{
	const float * colorPtr = Engine::instance()->getClearColor();

	float alpha = instance()->getActiveWindowPtr()->getAlpha() ? 0.0f : colorPtr[3];

	glClearColor(colorPtr[0], colorPtr[1], colorPtr[2], alpha);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void sgct::Engine::internal_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if( gKeyboardCallbackFn != NULL )
		gKeyboardCallbackFn(key, action);
}

void sgct::Engine::internal_key_char_callback(GLFWwindow* window, unsigned int ch)
{
	if( gCharCallbackFn != NULL )
		gCharCallbackFn(ch);
}

void sgct::Engine::internal_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if( gMouseButtonCallbackFn != NULL )
		gMouseButtonCallbackFn(button, action);
}

void sgct::Engine::internal_mouse_pos_callback(GLFWwindow* window, double xPos, double yPos)
{
	if( gMousePosCallbackFn != NULL )
		gMousePosCallbackFn(xPos, yPos);
}

void sgct::Engine::internal_mouse_scroll_callback(GLFWwindow* window, double xOffset, double yOffset)
{
	if( gMouseScrollCallbackFn != NULL)
		gMouseScrollCallbackFn(xOffset, yOffset);
}

void sgct::Engine::internal_glfw_error_callback(int error, const char* description)
{
	MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "GLFW error: %s\n", description);
}

/*!
	Print the node info to terminal.

	\param nodeId Which node to print
*/
void sgct::Engine::printNodeInfo(unsigned int nodeId)
{
	MessageHandler::instance()->print(MessageHandler::NOTIFY_DEBUG, "This node has index %d.\n", nodeId);
}

/*!
	Set up the current viewport.

	\param vs Which space to set up the viewport in.
*/
void sgct::Engine::enterCurrentViewport()
{
	if( getActiveWindowPtr()->isUsingFisheyeRendering() )
	{
		float cmRes = static_cast<float>(getActiveWindowPtr()->getCubeMapResolution());
		currentViewportCoords[0] =
        static_cast<int>( getActiveWindowPtr()->getCurrentViewport()->getX() * cmRes);
		currentViewportCoords[1] =
        static_cast<int>( getActiveWindowPtr()->getCurrentViewport()->getY() * cmRes);
		currentViewportCoords[2] =
        static_cast<int>( getActiveWindowPtr()->getCurrentViewport()->getXSize() * cmRes);
		currentViewportCoords[3] =
        static_cast<int>( getActiveWindowPtr()->getCurrentViewport()->getYSize() * cmRes);
	}
	else
	{
		currentViewportCoords[0] =
			static_cast<int>( getActiveWindowPtr()->getCurrentViewport()->getX() * static_cast<float>(getActiveWindowPtr()->getXFramebufferResolution()));
		currentViewportCoords[1] =
			static_cast<int>( getActiveWindowPtr()->getCurrentViewport()->getY() * static_cast<float>(getActiveWindowPtr()->getYFramebufferResolution()));
		currentViewportCoords[2] =
			static_cast<int>( getActiveWindowPtr()->getCurrentViewport()->getXSize() * static_cast<float>(getActiveWindowPtr()->getXFramebufferResolution()));
		currentViewportCoords[3] =
			static_cast<int>( getActiveWindowPtr()->getCurrentViewport()->getYSize() * static_cast<float>(getActiveWindowPtr()->getYFramebufferResolution()));

		SGCTWindow::StereoMode sm = getActiveWindowPtr()->getStereoMode();
		if( sm >= SGCTWindow::Side_By_Side_Stereo )
		{
			if( mActiveFrustumMode == Frustum::StereoLeftEye )
			{
				switch(sm)
				{
				case SGCTWindow::Side_By_Side_Stereo:
					currentViewportCoords[0] = currentViewportCoords[0] >> 1; //x offset
					currentViewportCoords[2] = currentViewportCoords[2] >> 1; //x size
					break;

				case SGCTWindow::Side_By_Side_Inverted_Stereo:
					currentViewportCoords[0] = (currentViewportCoords[0] >> 1) + (currentViewportCoords[2] >> 1); //x offset
					currentViewportCoords[2] = currentViewportCoords[2] >> 1; //x size
					break;

				case SGCTWindow::Top_Bottom_Stereo:
					currentViewportCoords[1] = (currentViewportCoords[1] >> 1) + (currentViewportCoords[3] >> 1); //y offset
					currentViewportCoords[3] = currentViewportCoords[3] >> 1; //y size
					break;

				case SGCTWindow::Top_Bottom_Inverted_Stereo:
					currentViewportCoords[1] = currentViewportCoords[1] >> 1; //y offset
					currentViewportCoords[3] = currentViewportCoords[3] >> 1; //y size
					break;

                default:
                    break;
				}
			}
			else
			{
				switch(sm)
				{
				case SGCTWindow::Side_By_Side_Stereo:
					currentViewportCoords[0] = (currentViewportCoords[0] >> 1) + (currentViewportCoords[2] >> 1); //x offset
					currentViewportCoords[2] = currentViewportCoords[2] >> 1; //x size
					break;

				case SGCTWindow::Side_By_Side_Inverted_Stereo:
					currentViewportCoords[0] = currentViewportCoords[0] >> 1; //x offset
					currentViewportCoords[2] = currentViewportCoords[2] >> 1; //x size
					break;

				case SGCTWindow::Top_Bottom_Stereo:
					currentViewportCoords[1] = currentViewportCoords[1] >> 1; //y offset
					currentViewportCoords[3] = currentViewportCoords[3] >> 1; //y size
					break;

				case SGCTWindow::Top_Bottom_Inverted_Stereo:
					currentViewportCoords[1] = (currentViewportCoords[1] >> 1) + (currentViewportCoords[3] >> 1); //y offset
					currentViewportCoords[3] = currentViewportCoords[3] >> 1; //y size
					break;

                default:
                    break;
				}
			}
		}
	}

	glViewport(currentViewportCoords[0],
               currentViewportCoords[1],
               currentViewportCoords[2],
               currentViewportCoords[3]);
    
    glScissor(currentViewportCoords[0],
              currentViewportCoords[1],
              currentViewportCoords[2],
              currentViewportCoords[3]);

	/*fprintf(stderr, "Viewport %d: %d %d %d %d\n",
		getActiveWindowPtr()->getCurrentViewportIndex(),
		currentViewportCoords[0],
        currentViewportCoords[1],
        currentViewportCoords[2],
        currentViewportCoords[3]);*/
}

void sgct::Engine::enterFisheyeViewport()
{
	int x, y, xSize, ySize;
	x = 0;
	y = 0;
	xSize = getActiveWindowPtr()->getXFramebufferResolution();
	ySize = getActiveWindowPtr()->getYFramebufferResolution();

	SGCTWindow::StereoMode sm = getActiveWindowPtr()->getStereoMode();
	if( sm >= SGCTWindow::Side_By_Side_Stereo )
	{
		if( mActiveFrustumMode == Frustum::StereoLeftEye )
		{
			switch(sm)
			{
			case SGCTWindow::Side_By_Side_Stereo:
				x = x >> 1; //x offset
				xSize = xSize >> 1; //x size
				break;

			case SGCTWindow::Side_By_Side_Inverted_Stereo:
				x = (x >> 1) + (xSize >> 1); //x offset
				xSize = xSize >> 1; //x size
				break;

			case SGCTWindow::Top_Bottom_Stereo:
				y = (y >> 1) + (ySize >> 1); //y offset
				ySize = ySize >> 1; //y size
				break;

			case SGCTWindow::Top_Bottom_Inverted_Stereo:
				y = y >> 1; //y offset
				ySize = ySize >> 1; //y size
				break;

            default:
                break;
			}
		}
		else
		{
			switch(sm)
			{
			case SGCTWindow::Side_By_Side_Stereo:
				x = (x >> 1) + (xSize >> 1); //x offset
				xSize = xSize >> 1; //x size
				break;

			case SGCTWindow::Side_By_Side_Inverted_Stereo:
				x = x >> 1; //x offset
				xSize = xSize >> 1; //x size
				break;

			case SGCTWindow::Top_Bottom_Stereo:
				y = y >> 1; //y offset
				ySize = ySize >> 1; //y size
				break;

			case SGCTWindow::Top_Bottom_Inverted_Stereo:
				y = (y >> 1) + (ySize >> 1); //y offset
				ySize = ySize >> 1; //y size
				break;

            default:
                break;
			}
		}
	}

	glViewport( x, y, xSize, ySize );
}

void sgct::Engine::calculateFPS(double timestamp)
{
	static double lastTimestamp = glfwGetTime();
	mStatistics->setFrameTime(static_cast<float>(timestamp - lastTimestamp));
	lastTimestamp = timestamp;
    static float renderedFrames = 0.0f;
	static float tmpTime = 0.0f;
	renderedFrames += 1.0f;
	tmpTime += mStatistics->getFrameTime();
	if( tmpTime >= 1.0f )
	{
		mStatistics->setAvgFPS(renderedFrames / tmpTime);
		renderedFrames = 0.0f;
		tmpTime = 0.0f;

		for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
			if( mThisNode->getWindowPtr(i)->isVisible() )
				updateAAInfo(i);
	}
}

/*!
\returns the frame time (delta time) in seconds
*/
const double sgct::Engine::getDt()
{
	return mStatistics->getFrameTime();
}

/*!
\returns the average frame time (delta time) in seconds
*/
const double sgct::Engine::getAvgDt()
{
	return mStatistics->getAvgFrameTime();
}

/*!
\returns the draw time in seconds
*/
const double sgct::Engine::getDrawTime()
{
	return mStatistics->getDrawTime();
}

/*!
\returns the sync time (time waiting for other nodes and network) in seconds
*/
const double sgct::Engine::getSyncTime()
{
	return mStatistics->getSyncTime();
}

/*!
	Set the near and far clipping planes. This operation recalculates all frustums for all viewports.

	@param nearClippingPlane near clipping plane in meters
	@param farClippingPlane far clipping plane in meters
*/
void sgct::Engine::setNearAndFarClippingPlanes(float nearClippingPlane, float farClippingPlane)
{
	mNearClippingPlaneDist = nearClippingPlane;
	mFarClippingPlaneDist = farClippingPlane;
	updateFrustums();
}

/*!
	Set the eye separation (interocular distance) for the user. This operation recalculates all frustums for all viewports.

	@param eyeSeparation eye separation in meters
*/
void sgct::Engine::setEyeSeparation(float eyeSeparation)
{
	getUserPtr()->setEyeSeparation( eyeSeparation );
	updateFrustums();
}

/*!
Set the clear color (background color).

@param red the red color component
@param green the green color component
@param blue the blue color component
@param alpha the alpha color component
*/
void sgct::Engine::setClearColor(float red, float green, float blue, float alpha)
{
	mClearColor[0] = red;
	mClearColor[1] = green;
	mClearColor[2] = blue;
	mClearColor[3] = alpha;
}

/*!
Set the clear color (background color) for the fisheye renderer. Alpha is not supported in this mode.

@param red the red color component
@param green the green color component
@param blue the blue color component
*/
void sgct::Engine::setFisheyeClearColor(float red, float green, float blue)
{
	mFisheyeClearColor[0] = red;
	mFisheyeClearColor[1] = green;
	mFisheyeClearColor[2] = blue;
	mFisheyeClearColor[3] = 1.0f;
}

/*!
Set the exit key that will kill SGCT or abort certain SGCT functions.
Default value is: SGCT_KEY_ESC. To diable shutdown or escaping SGCT then use: SGCT_KEY_UNKNOWN
\param key can be either an uppercase printable ISO 8859-1 (Latin 1) character (e.g. 'A', '3' or '.'), or
a special key identifier described in \link setKeyboardCallbackFunction \endlink description.
*/
void sgct::Engine::setExitKey(int key)
{
	mExitKey = key;
}

/*!
	Add a post effect to all windows
*/
void sgct::Engine::addPostFX( PostFX & fx )
{
	for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
	{
		mThisNode->getWindowPtr(i)->setUsePostFX(true);
		mThisNode->getWindowPtr(i)->addPostFX( fx );
	}
}

/*!
	\Returns the active draw texture if frame buffer objects are used otherwise GL_FALSE
*/
unsigned int sgct::Engine::getActiveDrawTexture()
{
	if( getActiveWindowPtr()->usePostFX() )
		return getActiveWindowPtr()->getFrameBufferTexture( Intermediate );
	else
		return mActiveFrustumMode == Frustum::StereoRightEye ? getActiveWindowPtr()->getFrameBufferTexture( RightEye ) : getActiveWindowPtr()->getFrameBufferTexture( LeftEye );
}

/*!
	\Returns the active depth texture if depth texture rendering is enabled through SGCTSettings and if frame buffer objects are used otherwise GL_FALSE
*/
unsigned int sgct::Engine::getActiveDepthTexture()
{
	return getActiveWindowPtr()->getFrameBufferTexture( Depth );
}

/*!
\Returns the active normal texture if normal texture rendering is enabled through SGCTSettings and if frame buffer objects are used otherwise GL_FALSE
*/
unsigned int sgct::Engine::getActiveNormalTexture()
{
	return getActiveWindowPtr()->getFrameBufferTexture( Normals );
}

/*!
\Returns the active position texture if position texture rendering is enabled through SGCTSettings and if frame buffer objects are used otherwise GL_FALSE
*/
unsigned int sgct::Engine::getActivePositionTexture()
{
	return getActiveWindowPtr()->getFrameBufferTexture( Positions );
}

/*!
	\Returns the horizontal resolution in pixels for the active window's framebuffer
*/
int sgct::Engine::getActiveXResolution()
{
	return getActiveWindowPtr()->getXFramebufferResolution();
}

/*!
	\Returns the vertical resolution in pixels for the active window's framebuffer
*/
int sgct::Engine::getActiveYResolution()
{
	return getActiveWindowPtr()->getYFramebufferResolution();
}

/*!
	\Returns the index of the window that is focued. If no window is focused 0 is returned.
*/
std::size_t sgct::Engine::getFocusedWindowIndex()
{
	for( std::size_t i=0; i<mThisNode->getNumberOfWindows(); i++ )
		if( mThisNode->getWindowPtr(i)->isFocused() )
			return i;
	return 0; //no window is focued
}

/*!
	Don't use this. This function is called from SGCTNetwork and will invoke the external network callback when messages are received.
*/
void sgct::Engine::decodeExternalControl(const char * receivedData, int receivedlength, int clientIndex)
{
	if(mNetworkMessageCallbackFn != NULL && receivedlength > 0)
		mNetworkMessageCallbackFn(receivedData, receivedlength, clientIndex);
}

/*!
	Don't use this. This function is called from SGCTNetwork and will invoke the external network callback when messages are received.
*/
void sgct::Engine::updateStatusForExternalControl(bool connected, int clientIndex)
{
	if(mNetworkStatusCallbackFn != NULL)
		mNetworkStatusCallbackFn(connected, clientIndex);
}

/*!
 Don't use this. This function is called from ScreenCapture and will invoke the screen shot callback.
*/
void sgct::Engine::invokeScreenShotCallback(Image * imPtr, std::size_t winIndex, ScreenCapture::EyeIndex ei)
{
    if(mScreenShotFn != NULL)
        mScreenShotFn(imPtr, winIndex, ei);
}

/*!
	This function sends a message to the external control interface.
	\param data a pointer to the data buffer
	\param length is the number of bytes of data that will be sent
*/
void sgct::Engine::sendMessageToExternalControl(void * data, int length)
{
	if( mNetworkConnections->getExternalControlPtr() != NULL )
		mNetworkConnections->getExternalControlPtr()->sendData( data, length );
}

/*!
	This function sends a message to the external control interface.
	\param msg the message string that will be sent
*/
void sgct::Engine::sendMessageToExternalControl(const std::string msg)
{
	if( mNetworkConnections->getExternalControlPtr() != NULL )
		mNetworkConnections->getExternalControlPtr()->sendData( (void *)msg.c_str(), static_cast<int>(msg.size()) );
}

/*!
	Check if the external control is connected.
*/
bool sgct::Engine::isExternalControlConnected()
{
	return (mNetworkConnections->getExternalControlPtr() != NULL && mNetworkConnections->getExternalControlPtr()->isConnected());
}

/*!
	Set the buffer size for the communication buffer. This size must be equal or larger than the receive buffer size.
*/
void sgct::Engine::setExternalControlBufferSize(unsigned int newSize)
{
	if( mNetworkConnections->getExternalControlPtr() != NULL )
		mNetworkConnections->getExternalControlPtr()->setBufferSize(newSize);
}

/*!
	This function updates the Anti-Aliasing (AA) settings.
	This function is called once per second.
*/
void sgct::Engine::updateAAInfo(std::size_t winIndex)
{
    if( getWindowPtr(winIndex)->useFXAA() )
	{
		if( getWindowPtr(winIndex)->isUsingFisheyeRendering() && getWindowPtr(winIndex)->getNumberOfAASamples() > 1 )
		{
			#if (_MSC_VER >= 1400) //visual studio 2005 or later
				sprintf_s(aaInfo, sizeof(aaInfo), "FXAA+MSAAx%d", getWindowPtr(winIndex)->getNumberOfAASamples() );
			#else
				sprintf(aaInfo, "FXAA+MSAAx%d", getWindowPtr(winIndex)->getNumberOfAASamples() );
			#endif
		}
		else
		{
			#if (_MSC_VER >= 1400) //visual studio 2005 or later
				strcpy_s(aaInfo, sizeof(aaInfo), "FXAA");
			#else
				strcpy(aaInfo, "FXAA");
			#endif
		}
	}
    else //no FXAA
    {
        if( getWindowPtr(winIndex)->getNumberOfAASamples() > 1 )
        {
            #if (_MSC_VER >= 1400) //visual studio 2005 or later
            sprintf_s( aaInfo, sizeof(aaInfo), "MSAAx%d",
                getWindowPtr(winIndex)->getNumberOfAASamples());
            #else
            sprintf( aaInfo, "MSAAx%d",
                getWindowPtr(winIndex)->getNumberOfAASamples());
            #endif
        }
        else
        #if (_MSC_VER >= 1400) //visual studio 2005 or later
        strcpy_s(aaInfo, sizeof(aaInfo), "none");
        #else
            strcpy(aaInfo, "none");
        #endif
    }
}

/*!
	Checks the keyboard if the specified key has been pressed.
	\param winIndex specifies which window to poll
	\param key specifies which key to check
	\returns SGCT_PRESS or SGCT_RELEASE
*/
int sgct::Engine::getKey( std::size_t winIndex, int key )
{
	return glfwGetKey( mInstance->getWindowPtr( winIndex )->getWindowHandle(), key);
}

/*!
	Checks if specified mouse button has been pressed.
	\param winIndex specifies which window to poll
	\param button specifies which button to check
	\returns SGCT_PRESS or SGCT_RELEASE
*/
int sgct::Engine::getMouseButton( std::size_t winIndex, int button )
{
	return glfwGetMouseButton(mInstance->getWindowPtr(winIndex)->getWindowHandle(), button);
}

/*!
	Get the mouse position.
	\param winIndex specifies which window to poll
	\param xPos x screen coordinate
	\param yPos y screen coordinate
*/
void sgct::Engine::getMousePos( std::size_t winIndex, double * xPos, double * yPos )
{
	glfwGetCursorPos(mInstance->getWindowPtr(winIndex)->getWindowHandle(), xPos, yPos);
}

/*!
	Set the mouse position.
	\param winIndex specifies which window's input to set
	\param xPos x screen coordinate
	\param yPos y screen coordinate
*/
void sgct::Engine::setMousePos( std::size_t winIndex, double xPos, double yPos )
{
	glfwSetCursorPos(mInstance->getWindowPtr(winIndex)->getWindowHandle(), xPos, yPos);
}

/*!
	Set the mouse cursor/pointer visibility
	\param winIndex specifies which window's input to set
	\param state set to true if mouse cursor should be visible
*/
void sgct::Engine::setMouseCursorVisibility( std::size_t winIndex, bool state )
{
	state ?
		glfwSetInputMode(mInstance->getWindowPtr(winIndex)->getWindowHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL ) :
		glfwSetInputMode(mInstance->getWindowPtr(winIndex)->getWindowHandle(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN );
}

/*!
	\param joystick is the joystick id. Availible id's:
	   - SGCT_JOYSTICK_1
	   - SGCT_JOYSTICK_2
	   - SGCT_JOYSTICK_3
	   - SGCT_JOYSTICK_4
	   - SGCT_JOYSTICK_5
	   - SGCT_JOYSTICK_6
	   - SGCT_JOYSTICK_7
	   - SGCT_JOYSTICK_8
	   - SGCT_JOYSTICK_9
	   - SGCT_JOYSTICK_10
	   - SGCT_JOYSTICK_11
	   - SGCT_JOYSTICK_12
	   - SGCT_JOYSTICK_13
	   - SGCT_JOYSTICK_14
	   - SGCT_JOYSTICK_15
	   - SGCT_JOYSTICK_16
	   - SGCT_JOYSTICK_LAST
*/
const char * sgct::Engine::getJoystickName( int joystick )
{
	return glfwGetJoystickName(joystick);
}

/*!
\param joystick the joystick id: Availibe id's are specified here: \link getJoystickName \endlink
\param numOfValues is the number of analog axes
\returns the analog float values (array)
*/
const float * sgct::Engine::getJoystickAxes( int joystick, int * numOfValues)
{
	return glfwGetJoystickAxes( joystick, numOfValues );
}

/*!
\param joystick the joystick id: Availibe id's are specified here: \link getJoystickName \endlink
\param numOfValues is the number of buttons
\returns the button values (array)
*/
const unsigned char * sgct::Engine::getJoystickButtons( int joystick, int * numOfValues)
{
	return glfwGetJoystickButtons( joystick, numOfValues );
}

/*!
This function puts the current thread to sleep during a specified time
\param secs is the time to sleep the thread
*/
void sgct::Engine::sleep(double secs)
{
	tthread::this_thread::sleep_for(tthread::chrono::milliseconds( static_cast<int>(secs * 1000.0) ));
}

/*!
	Create a timer that counts down and call the given callback when finished.
	The timer runs only on the master and is not precies since it is triggered in end of the renderloop.

	\param millisec is the countdown time
	\param fnPtr is the function pointer to a timer callback (the argument will be the timer handle/id).

	\returns Handle/id to the created timer
*/
size_t sgct::Engine::createTimer( double millisec, void(*fnPtr)(std::size_t) )
{
    if ( isMaster() )
    {
        // construct the timer object
        TimerInformation timer;
        timer.mCallback = fnPtr;
        timer.mInterval = millisec / 1000.0; // we want to present timers in millisec, but glfwGetTime uses seconds
        timer.mId = mTimerID++;  // use and post-increase
        timer.mLastFired = getTime();
        mTimers.push_back( timer );
        return timer.mId;
    }
    else
		return std::numeric_limits<size_t>::max();
}

/*!
Stops the specified timer
\param id/handle to a timer
*/
void sgct::Engine::stopTimer( size_t id )
{
    if ( isMaster() )
    {
        // iterate over all timers and search for the id
        for( size_t i = 0; i < mTimers.size(); ++i )
        {
            const TimerInformation& currentTimer = mTimers[i];
            if( currentTimer.mId == id )
            {
                mTimers[i].mCallback = NULL;
				// if the id found, delete this timer and return immediately
                mTimers.erase( mTimers.begin() + i );
                return;
            }
        }

        // if we get this far, the searched ID did not exist
        MessageHandler::instance()->print(MessageHandler::NOTIFY_WARNING, "There was no timer with id: %d", id);
    }
}

/*!
	Get the time from program start in seconds
*/
double sgct::Engine::getTime()
{
	return glfwGetTime();
}

/*!
Get the active viewport size in pixels.
\param x the horizontal size
\param y the vertical size
*/
void sgct::Engine::getActiveViewportSize(int & x, int & y)
{
	x = currentViewportCoords[2];
	y = currentViewportCoords[3];
}

/*!
 Set the screenshot number (file index)
 */
void sgct::Engine::setScreenShotNumber(unsigned int number)
{
	mShotCounter = number;
}

/*!
 \returns the current screenshot number (file index)
 */
unsigned int sgct::Engine::getScreenShotNumber()
{
	return mShotCounter;
}

void sgct::Engine::outputHelpMessage()
{
	fprintf( stderr, "\nRequired parameters:\n------------------------------------\n\
\n-config <filename.xml>           \n\tSet xml confiuration file\n\
\nOptional parameters:\n------------------------------------\n\
\n-logPath <filepath>              \n\tSet log file path\n\
\n--help                           \n\tDisplay help message and exit\n\
\n-local <integer>                 \n\tForce node in configuration to localhost\n\t(index starts at 0)\n\
\n--client                         \n\tRun the application as client\n\t(only available when running as local)\n\
\n--slave                          \n\tRun the application as client\n\t(only available when running as local)\n\
\n--debug                          \n\tSet the notify level of messagehandler to debug\n\
\n--Firm-Sync                      \n\tEnable firm frame sync\n\
\n--Loose-Sync                     \n\tDisable firm frame sync\n\
\n--Ignore-Sync                    \n\tDisable frame sync\n\
\n-MSAA	<integer>                  \n\tEnable MSAA as default (argument must be a power of two)\n\
\n--FXAA	                       \n\tEnable FXAA as default\n\
\n-notify <integer>                \n\tSet the notify level used in the MessageHandler\n\t(0 = highest priority)\n\
\n--gDebugger                      \n\tForce textures to be genareted using glTexImage2D instead of glTexStorage2D\n\
\n--No-FBO                         \n\tDisable frame buffer objects\n\t(some stereo modes, Multi-Window rendering,\n\tFXAA and fisheye rendering will be disabled)\n\
\n--Capture-PNG                    \n\tUse png images for screen capture (default)\n\
\n--Capture-TGA                    \n\tUse tga images for screen capture\n\
\n-numberOfCaptureThreads <integer>\n\tSet the maximum amount of threads\n\tthat should be used during framecapture (default 8)\n------------------------------------\n\n");
}

/*
	For feedback: breaks a frame lock wait condition every time interval (FRAME_LOCK_TIMEOUT)
	in order to print waiting message.
*/
void updateFrameLockLoop(void * arg)
{
	bool run = true;

	while(run)
	{
		sgct::SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::FrameSyncMutex );
		run = sRunUpdateFrameLockLoop;
		sgct::SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::FrameSyncMutex );

		sgct_core::NetworkManager::gCond.notify_all();

		tthread::this_thread::sleep_for(tthread::chrono::milliseconds(FRAME_LOCK_TIMEOUT));
	}
}
