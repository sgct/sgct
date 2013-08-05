/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
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
#include "../include/sgct/SGCTInternalShaders.h"
#include "../include/sgct/SGCTInternalShaders_modern.h"
#include "../include/sgct/SGCTVersion.h"
#include "../include/sgct/SGCTSettings.h"
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/ShaderManager.h"
#include "../include/external/tinythread.h"
#include <glm/gtc/constants.hpp>
#include <math.h>
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

#ifdef GLEW_MX
GLEWContext * glewGetContext();
#endif

#define USE_SLEEP_TO_WAIT_FOR_NODES 0

/*!
This is the only valid constructor that also initiates [GLFW](http://www.glfw.org/). Command line parameters are used to load a configuration file and settings.
Note that parameter with one '\-' are followed by arguments but parameters with '\-\-' are just options without arguments.

Parameter     | Description
------------- | -------------
-config <filename> | set xml confiuration file
-local <integer> | set which node in configuration that is the localhost (index starts at 0)
--client | run the application as client (only available when running as local)
--slave | run the application as client (only available when running as local)
--Firm-Sync | enable firm frame sync
--Loose-Sync | disable firm frame sync
--Ignore-Sync | disable frame sync
-notify <integer> | the notify level used in the MessageHandler (0 = highest priority)
--No-FBO | don't use frame buffer objects (some stereo modes, FXAA and fisheye rendering will be disabled)
--FXAA | use fast approximate anti-aliasing shader
--Capture-PNG | use png images for screen capture (default)
--Capture-TGA | use tga images for screen capture
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
	mPreSyncFn = NULL;
	mPostSyncPreDrawFn = NULL;
	mPostDrawFn = NULL;
	mInitOGLFn = NULL;
	mClearBufferFn = NULL;
	mCleanUpFn = NULL;
	mInternalDrawFn = NULL;
	mInternalRenderFBOFn = NULL;
	mInternalDrawOverlaysFn = NULL;
	mInternalRenderPostFXFn = NULL;
	mInternalRenderFisheyeFn = NULL;
	mNetworkCallbackFn = NULL;

	mTerminate = false;
	mIgnoreSync = false;
	mRenderingOffScreen = false;
	mFixedOGLPipeline = true;

	localRunningMode = NetworkManager::NotLocal;

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
	mClearColor[3] = 0.0f;

	mFisheyeClearColor[0] = 0.3f;
	mFisheyeClearColor[1] = 0.3f;
	mFisheyeClearColor[2] = 0.3f;
	mFisheyeClearColor[3] = 1.0f;

	mShowInfo = false;
	mShowGraph = false;
	mShowWireframe = false;
	mTakeScreenshot = false;
	mActiveFrustum = Frustum::Mono;
	mFrameCounter = 0;
    mTimerID = 0;

	//parse needs to be before read config since the path to the XML is parsed here
	parseArguments( argc, argv );

	mExitKey = GLFW_KEY_ESCAPE;

	// Initialize GLFW
	glfwSetErrorCallback( internal_glfw_error_callback );
	if( !glfwInit() )
	{
		mTerminate = true;
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
 3. Create a window
 4. Set up OpenGL
	4.1 Create textures
	4.2 Init FBOs
	4.3 Init VBOs
	4.4 Init PBOs

 @param rm rm is the optional run mode. If any problems are experienced with Open Scene Graph then use the OSG_Encapsulation_Mode.
*/
bool sgct::Engine::init(RunMode rm)
{
	mRunMode = rm;

	MessageHandler::Instance()->print(MessageHandler::NOTIFY_VERSION_INFO, "%s\n", getSGCTVersion().c_str() );

	if(mTerminate)
	{
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "Failed to init GLFW! Exiting...\n");
		return false;
	}

	mConfig = new ReadConfig( configFilename );
	if( !mConfig->isValid() ) //fatal error
	{
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "Error in xml config file parsing.\n");
		return false;
	}

	if( !initNetwork() )
	{
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "Network init error.\n");
		return false;
	}

	if( !initWindows() )
	{
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "Window init error.\n");
		return false;
	}

	//if a single node, skip syncing
	if(ClusterManager::Instance()->getNumberOfNodes() == 1)
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
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "Initiating network connections failed! Error: '%s'\n", err);
		return false;
	}

	//check in cluster configuration which it is
	if( localRunningMode == NetworkManager::NotLocal )
		for(unsigned int i=0; i<ClusterManager::Instance()->getNumberOfNodes(); i++)
			if( mNetworkConnections->matchAddress( ClusterManager::Instance()->getNodePtr(i)->ip ) )
			{
				ClusterManager::Instance()->setThisNodeId(i);
				mThisNode = ClusterManager::Instance()->getNodePtr(i);
				break;
			}

	if( ClusterManager::Instance()->getThisNodeId() == -1 ||
		ClusterManager::Instance()->getThisNodeId() >= static_cast<int>( ClusterManager::Instance()->getNumberOfNodes() )) //fatal error
	{
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "This computer is not a part of the cluster configuration!\n");
		mNetworkConnections->close();
		return false;
	}
	else
	{
		printNodeInfo( static_cast<unsigned int>(ClusterManager::Instance()->getThisNodeId()) );
	}

	//Set message handler to send messages or not
	MessageHandler::Instance()->setSendFeedbackToServer( !mNetworkConnections->isComputerServer() );

	if(!mNetworkConnections->init())
		return false;

    MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Done\n");
	return true;
}

/*!
Create and initiate a window.
*/
bool sgct::Engine::initWindows()
{
	if( mThisNode->getNumberOfWindows() == 0 )
	{
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "No windows exist in configuration!\n");
		return false;
	}
	
	int tmpGlfwVer[3];
    glfwGetVersion( &tmpGlfwVer[0], &tmpGlfwVer[1], &tmpGlfwVer[2] );
	MessageHandler::Instance()->print(MessageHandler::NOTIFY_VERSION_INFO, "Using GLFW version %d.%d.%d.\n",
                                         tmpGlfwVer[0],
                                         tmpGlfwVer[1],
                                         tmpGlfwVer[2]);

	switch( mRunMode )
	{
	case OpenGL_3_3_Core_Profile:
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glewExperimental = true; // Needed for core profile
		}
		break;

	case OpenGL_4_0_Core_Profile:
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glewExperimental = true; // Needed for core profile
		}
		break;

	case OpenGL_4_1_Core_Profile:
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glewExperimental = true; // Needed for core profile
		}
		break;

	case OpenGL_4_2_Core_Profile:
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glewExperimental = true; // Needed for core profile
		}
		break;

	case OpenGL_4_3_Core_Profile:
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glewExperimental = true; // Needed for core profile
		}
		break;

    /*case OpenGL_3_2_Core_Profile:
		{
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glewExperimental = true; // Needed for core profile
		}
		break;*/

    default:

        break;
	}

	/*
	//OSX ogl 3.2 code
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	*/

	/*
	//win & linux code
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE); //We don't want the old OpenGL
	*/

	mStatistics = new Statistics();
	GLFWwindow* share = NULL;
	for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
	{
		if( i > 0 )
			share = mThisNode->getWindowPtr(0)->getWindowHandle();
		
		if( !mThisNode->getWindowPtr(i)->openWindow( share ) )
		{
			MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "Failed to open window %d!\n", i);
			return false;
		}
	}

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
	  //Problem: glewInit failed, something is seriously wrong.
	  MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "GLEW error: %s!\n", glewGetErrorString(err));
	  return false;
	}
	MessageHandler::Instance()->print(MessageHandler::NOTIFY_VERSION_INFO, "Using GLEW %s.\n", glewGetString(GLEW_VERSION));

	if( !checkForOGLErrors() )
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "GLEW init triggered an OpenGL error.\n");

	for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
	{
		mThisNode->setCurrentWindowIndex(i);
		getCurrentWindowPtr()->init( static_cast<int>(i) );
		getCurrentWindowPtr()->setWindowTitle( getBasicInfo(i) );
	}

	waitForAllWindowsInSwapGroupToOpen();

	//init swap group if enabled
	for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
	{
		mThisNode->getWindowPtr(i)->makeOpenGLContextCurrent();
		mThisNode->getWindowPtr(i)->initNvidiaSwapGroups();
	}

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
		mInternalRenderPostFXFn = &Engine::renderPostFx;
		mInternalRenderFisheyeFn = &Engine::renderFisheye;

		ClusterManager::Instance()->setMeshImplementation( sgct_core::ClusterManager::VAO );
		mFixedOGLPipeline = false;
	}
	else
	{
		mInternalDrawFn = &Engine::drawFixedPipeline;
		mInternalRenderFBOFn = &Engine::renderFBOTextureFixedPipeline;
		mInternalDrawOverlaysFn = &Engine::drawOverlaysFixedPipeline;
		mInternalRenderPostFXFn = &Engine::renderPostFxFixedPipeline;
		mInternalRenderFisheyeFn = &Engine::renderFisheyeFixedPipeline;

		mFixedOGLPipeline = true;
	}

	//Get OpenGL version
	int mOpenGL_Version[3];
	mOpenGL_Version[0] = glfwGetWindowAttrib( getActiveWindowPtr()->getWindowHandle(), GLFW_CONTEXT_VERSION_MAJOR);
	mOpenGL_Version[1] = glfwGetWindowAttrib( getActiveWindowPtr()->getWindowHandle(), GLFW_CONTEXT_VERSION_MINOR );
	mOpenGL_Version[2] = glfwGetWindowAttrib( getActiveWindowPtr()->getWindowHandle(), GLFW_CONTEXT_REVISION);
	
	MessageHandler::Instance()->print(MessageHandler::NOTIFY_VERSION_INFO, "OpenGL version %d.%d.%d %s\n", mOpenGL_Version[0], mOpenGL_Version[1], mOpenGL_Version[2],
		mFixedOGLPipeline ? "comp. profile" : "core profile");

	if( !GLEW_EXT_framebuffer_object && mOpenGL_Version[0] < 2)
	{
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_WARNING, "Warning! Frame buffer objects are not supported! A lot of features in SGCT will not work!\n");
		SGCTSettings::Instance()->setUseFBO( false );
	}
	else if(!GLEW_EXT_framebuffer_multisample && mOpenGL_Version[0] < 2)
	{
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_WARNING, "Warning! FBO multisampling is not supported!\n");
		SGCTSettings::Instance()->setUseFBO( true );
	}

	//init buffers
	for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
	{
		mThisNode->setCurrentWindowIndex(i);
		getCurrentWindowPtr()->initOGL();
	}

	loadShaders();
	mStatistics->initVBO(mFixedOGLPipeline);

	//load overlays if any
	for(std::size_t i=0; i<mThisNode->getNumberOfWindows(); i++)
	{
		mThisNode->setCurrentWindowIndex(i);
		mThisNode->getWindowPtr(i)->makeOpenGLContextCurrent();

		for(std::size_t j=0; j<getWindowPtr(i)->getNumberOfViewports(); j++)
			getWindowPtr(i)->getViewport(j)->loadData();
	}

	//Make "main" context current to make life easier for the end user
	getCurrentWindowPtr()->makeSharedOpenGLContextCurrent();
	if( mInitOGLFn != NULL )
		mInitOGLFn();

	calculateFrustums();

	//
	// Add fonts
	//
	if( mConfig->getFontPath().empty() )
	{
	    if( !sgct_text::FontManager::Instance()->AddFont( "SGCTFont", mConfig->getFontName() ) )
            sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() );
    }
    else
    {
	    std::string tmpPath = mConfig->getFontPath() + mConfig->getFontName();
	    if( !sgct_text::FontManager::Instance()->AddFont( "SGCTFont", tmpPath, sgct_text::FontManager::FontPath_Local ) )
            sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() );
    }

	//check for errors
	checkForOGLErrors();

	//init swap barrier is swap groups are active
	MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Joining swap barrier if enabled and reseting counter...\n");
	
	for(std::size_t i=0; i<mThisNode->getNumberOfWindows(); i++)
	{
		mThisNode->setCurrentWindowIndex(i);
		mThisNode->getWindowPtr(i)->makeOpenGLContextCurrent();
		mThisNode->getWindowPtr(i)->setBarrier(true);
		mThisNode->getWindowPtr(i)->resetSwapGroupFrameNumber();
	}

	MessageHandler::Instance()->print(MessageHandler::NOTIFY_IMPORTANT, "\nReady to render!\n");
}

/*!
Clean up all resources and release memory.
*/
void sgct::Engine::clean()
{
	MessageHandler::Instance()->print(MessageHandler::NOTIFY_IMPORTANT, "Cleaning up...\n");

	if( mCleanUpFn != NULL )
		mCleanUpFn();

	MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Clearing all callbacks...\n");
	clearAllCallbacks();

	if( mStatistics != NULL )
	{
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Deleting stats data...\n");
		delete mStatistics;
		mStatistics = NULL;
	}

	//de-init window and unbind swapgroups...
	if(ClusterManager::Instance()->getNumberOfNodes() > 0)
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
	MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Destroying font manager...\n");
	sgct_text::FontManager::Destroy();
	MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Destroying shader manager and internal shaders...\n");
	ShaderManager::Destroy();
	for(size_t i = 0; i < NUMBER_OF_SHADERS; i++)
		mShaders[i].deleteProgram();
	MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Destroying shared data...\n");
	SharedData::Destroy();
	MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Destroying texture manager...\n");
	TextureManager::Destroy();
	MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Destroying cluster manager...\n");
	ClusterManager::Destroy();
	MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Destroying settings...\n");
	SGCTSettings::Destroy();

	MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Destroying message handler...\n");
	MessageHandler::Destroy();

	MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Destroying mutexes...\n");
	SGCTMutexManager::Destroy();

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
	mPreSyncFn = NULL;
	mPostSyncPreDrawFn = NULL;
	mPostDrawFn = NULL;
	mInitOGLFn = NULL;
	mClearBufferFn = NULL;
	mCleanUpFn = NULL;
	mInternalDrawFn = NULL;
	mInternalRenderFBOFn = NULL;
	mInternalDrawOverlaysFn = NULL;
	mInternalRenderPostFXFn = NULL;
	mInternalRenderFisheyeFn = NULL;
	mNetworkCallbackFn = NULL;
	
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
void sgct::Engine::frameSyncAndLock(sgct::Engine::SyncStage stage)
{
	if( stage == PreStage )
	{
		double t0 = glfwGetTime();
		mNetworkConnections->sync(NetworkManager::SendDataToClients, mStatistics); //from server to clients
		mStatistics->setSyncTime(glfwGetTime() - t0);

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
					SGCTMutexManager::Instance()->lockMutex( SGCTMutexManager::SyncMutex );
					NetworkManager::gCond.wait( (*SGCTMutexManager::Instance()->getMutexPtr( SGCTMutexManager::SyncMutex )) );
					SGCTMutexManager::Instance()->unlockMutex( SGCTMutexManager::SyncMutex );
				}

				//for debuging
				if( glfwGetTime() - t0 > 1.0 )
				{
					for(unsigned int i=0; i<mNetworkConnections->getConnectionsCount(); i++)
					{
						if( !mNetworkConnections->getConnection(i)->isUpdated() )
						{
							MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Waiting for master... send frame %d, recv frame %d\n",
								mNetworkConnections->getConnection(i)->getSendFrame(),
								mNetworkConnections->getConnection(i)->getRecvFrame(SGCTNetwork::Current));
						}
					}
				}
			}//end while wait loop

			/*
				A this point all data needed for rendering a frame is received.
				Let's signal that back to the master/server.
			*/
			mNetworkConnections->sync(NetworkManager::AcknowledgeData, mStatistics);

			mStatistics->addSyncTime(glfwGetTime() - t0);
		}//end if client
	}
	else //post stage
	{
		if( !mIgnoreSync && mNetworkConnections->isComputerServer() )//&&
			//mConfig->isMasterSyncLocked() &&
			/*localRunningMode == NetworkManager::NotLocal &&*/
			//!getActiveWindowPtr()->isBarrierActive() )//post stage
		{
			double t0 = glfwGetTime();
			while(mNetworkConnections->isRunning() &&
				mRunning &&
				mNetworkConnections->getConnectionsCount() > 0)
			{
				if( mNetworkConnections->isSyncComplete() )
						break;

				if(USE_SLEEP_TO_WAIT_FOR_NODES)
					tthread::this_thread::sleep_for(tthread::chrono::milliseconds(1));
				else
				{
					SGCTMutexManager::Instance()->lockMutex( SGCTMutexManager::SyncMutex );
					NetworkManager::gCond.wait( (*SGCTMutexManager::Instance()->getMutexPtr( SGCTMutexManager::SyncMutex )) );
					SGCTMutexManager::Instance()->unlockMutex( SGCTMutexManager::SyncMutex );
				}

				//for debuging
				if( glfwGetTime() - t0 > 1.0 )
				{
					for(unsigned int i=0; i<mNetworkConnections->getConnectionsCount(); i++)
					{
						if( !mNetworkConnections->getConnection(i)->isUpdated() )
						{
							MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Waiting for slave at connection %d: send frame %d != recv frame %d\n", i,
								mNetworkConnections->getConnection(i)->getSendFrame(),
								mNetworkConnections->getConnection(i)->getRecvFrame(SGCTNetwork::Current));
						}
					}
				}
			}//end while
			mStatistics->addSyncTime(glfwGetTime() - t0);
		}//end if server
	}
}

/*!
	This is SGCT's renderloop where rendeing & synchronization takes place.
*/
void sgct::Engine::render()
{
	mRunning = GL_TRUE;

	SGCTUser * usrPtr = ClusterManager::Instance()->getUserPtr();

	while( mRunning )
	{
		mRenderingOffScreen = false;

#ifdef __SGCT_RENDER_LOOP_DEBUG__
    fprintf(stderr, "Render-Loop: Updating tracking devices.\n");
#endif

		//update tracking data
		if( isMaster() )
			ClusterManager::Instance()->getTrackingManagerPtr()->updateTrackingDevices();

#ifdef __SGCT_RENDER_LOOP_DEBUG__
    fprintf(stderr, "Render-Loop: Running pre-sync.\n");
#endif
		if( mPreSyncFn != NULL )
			mPreSyncFn();

		if( mNetworkConnections->isComputerServer() )
		{
#ifdef __SGCT_RENDER_LOOP_DEBUG__
    fprintf(stderr, "Render-Loop: Encoding data.\n");
#endif
			SharedData::Instance()->encode();
		}
		else
		{
			if( !mNetworkConnections->isRunning() ) //exit if not running
			{
				MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "Network disconnected! Exiting...\n");
				break;
			}
		}

#ifdef __SGCT_RENDER_LOOP_DEBUG__
    fprintf(stderr, "Render-Loop: Sync/framelock\n");
#endif
		frameSyncAndLock(PreStage);

#ifdef __SGCT_RENDER_LOOP_DEBUG__
    fprintf(stderr, "Render-Loop: running post-sync-pre-draw\n");
#endif
		if( mPostSyncPreDrawFn != NULL )
			mPostSyncPreDrawFn();

		double startFrameTime = glfwGetTime();
		calculateFPS(startFrameTime); //measures time between calls

		//check if re-size needed
		for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
		{
			mThisNode->getWindowPtr(i)->update();
		}

		//rendering offscreen if using FBOs
		mRenderingOffScreen = SGCTSettings::Instance()->useFBO();

		for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
		{
			mThisNode->setCurrentWindowIndex(i);

			getCurrentWindowPtr()->makeOpenGLContextCurrent();

			//if fisheye rendering is used then render the cubemap
			if( getCurrentWindowPtr()->isUsingFisheyeRendering() )
			{
	#ifdef __SGCT_RENDER_LOOP_DEBUG__
		fprintf(stderr, "Render-Loop: Rendering fisheye\n");
	#endif
				//set alpha value
				mFisheyeClearColor[3] = getCurrentWindowPtr()->useFisheyeAlpha() ? 0.0f : 1.0f;
		
				mActiveFrustum = getCurrentWindowPtr()->getStereoMode() != static_cast<int>(SGCTWindow::NoStereo) ? Frustum::StereoLeftEye : Frustum::Mono;
				(this->*mInternalRenderFisheyeFn)(LeftEye);

				if( SGCTSettings::Instance()->usePostFX() )
					(this->*mInternalRenderPostFXFn)(LeftEye);

				if( getCurrentWindowPtr()->getStereoMode() != SGCTWindow::NoStereo )
				{
					mActiveFrustum = Frustum::StereoRightEye;
					(this->*mInternalRenderFisheyeFn)(RightEye);

					if( SGCTSettings::Instance()->usePostFX() )
						(this->*mInternalRenderPostFXFn)(RightEye);
				}
			}
			else //regular viewport rendering
			{
	#ifdef __SGCT_RENDER_LOOP_DEBUG__
		fprintf(stderr, "Render-Loop: Rendering\n");
	#endif
				//if any stereo type (except passive) then set frustum mode to left eye
				mActiveFrustum = getCurrentWindowPtr()->getStereoMode() != static_cast<int>(SGCTWindow::NoStereo) ? Frustum::StereoLeftEye : Frustum::Mono;
				setRenderTarget(LeftEye); //Set correct render target (Backbuffer, FBO etc..)

				//render all viewports for mono or left eye
				for(unsigned int i=0; i<getCurrentWindowPtr()->getNumberOfViewports(); i++)
				{
					getCurrentWindowPtr()->setCurrentViewport(i);

					if( getCurrentWindowPtr()->getCurrentViewport()->isEnabled() )
					{
						//if passive stereo or mono
						if( getCurrentWindowPtr()->getStereoMode() == SGCTWindow::NoStereo )
							mActiveFrustum = getCurrentWindowPtr()->getCurrentViewport()->getEye();

						if( getCurrentWindowPtr()->getCurrentViewport()->isTracked() )
						{
							getCurrentWindowPtr()->getCurrentViewport()->calculateFrustum(
								mActiveFrustum,
								usrPtr->getPosPtr(mActiveFrustum),
								mNearClippingPlaneDist,
								mFarClippingPlaneDist);
						}
						(this->*mInternalDrawFn)();
					}
				}

				updateRenderingTargets(LeftEye); //only used if multisampled FBOs
				if( SGCTSettings::Instance()->usePostFX() )
					(this->*mInternalRenderPostFXFn)(LeftEye);

				//render right eye view port(s)
				if( getCurrentWindowPtr()->getStereoMode() != SGCTWindow::NoStereo )
				{
					mActiveFrustum = Frustum::StereoRightEye;
					setRenderTarget(RightEye); //Set correct render target (Backbuffer, FBO etc..)

					//render all viewports for right eye
					for(unsigned int i=0; i<getCurrentWindowPtr()->getNumberOfViewports(); i++)
					{
						getCurrentWindowPtr()->setCurrentViewport(i);

						if( getCurrentWindowPtr()->getCurrentViewport()->isEnabled() )
						{
							if( getCurrentWindowPtr()->getCurrentViewport()->isTracked() )
							{
								getCurrentWindowPtr()->getCurrentViewport()->calculateFrustum(
									mActiveFrustum,
									usrPtr->getPosPtr(mActiveFrustum),
									mNearClippingPlaneDist,
									mFarClippingPlaneDist);
							}
							(this->*mInternalDrawFn)();
						}
					}

					updateRenderingTargets(RightEye);
					if( SGCTSettings::Instance()->usePostFX() )
						(this->*mInternalRenderPostFXFn)(RightEye);
				}
			}

#ifdef __SGCT_RENDER_LOOP_DEBUG__
		fprintf(stderr, "Render-Loop: Rendering FBO quad\n");
#endif
			mRenderingOffScreen = false;
			if( SGCTSettings::Instance()->useFBO() )
				(this->*mInternalRenderFBOFn)();

#ifdef __SGCT_RENDER_LOOP_DEBUG__
		fprintf(stderr, "Render-Loop: Rendering overlay\n");
#endif
			//draw viewport overlays if any
			(this->*mInternalDrawOverlaysFn)();

#ifdef __SGCT_RENDER_LOOP_DEBUG__
		fprintf(stderr, "Render-Loop: Running post-sync\n");
#endif
			
		}//end window loop

		//run post frame actions
		if( mPostDrawFn != NULL )
			mPostDrawFn();

		/*
			For single threaded rendering glFinish should be fine to use for frame sync.
			For multitheded usage a glFenceSync fence should be used to synchronize all GPU threads.

			example: GLsync mFence = glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
			Then on each thread: glWaitSync(mFence);
		*/
		//glFinish(); //wait for all rendering to finish /* ATI doesn't like this.. the framerate is halfed if it's used. */

		for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
		{
			mThisNode->setCurrentWindowIndex(i);
			getCurrentWindowPtr()->makeOpenGLContextCurrent();

			if( mTakeScreenshot )
				getCurrentWindowPtr()->captureBuffer();

#ifdef __SGCT_DEBUG__
			//check for errors
			checkForOGLErrors();
#endif

#ifdef __SGCT_RENDER_LOOP_DEBUG__
    fprintf(stderr, "Render-Loop: Rendering info & stats\n");
#endif
			//draw info & stats
			//the cubemap viewports are all the same so it makes no sense to render everything several times
			//therefore just loop one iteration in that case.
			std::size_t numberOfIterations = ( getCurrentWindowPtr()->isUsingFisheyeRendering() ? 1 : getCurrentWindowPtr()->getNumberOfViewports() );
			for(std::size_t i=0; i < numberOfIterations; i++)
			{
				getCurrentWindowPtr()->setCurrentViewport(i);
				enterCurrentViewport(ScreenSpace);

				if( mShowGraph )
					mStatistics->draw(mFrameCounter);
				/*
					The text renderer enters automatically the correct viewport
				*/
				if( mShowInfo )
					renderDisplayInfo();
			}
		}//end window loop

#ifdef __SGCT_RENDER_LOOP_DEBUG__
    fprintf(stderr, "Render-Loop: swap and update data\n");
#endif
        double endFrameTime = glfwGetTime();
		mStatistics->setDrawTime(endFrameTime - startFrameTime);
        updateTimers( endFrameTime );

#ifdef __SGCT_RENDER_LOOP_DEBUG__
    fprintf(stderr, "Render-Loop: lock\n");
#endif
		//master will wait for nodes render before swapping
		frameSyncAndLock(PostStage);

#ifdef __SGCT_RENDER_LOOP_DEBUG__
    fprintf(stderr, "Render-Loop: Swap buffers\n");
#endif
		// Swap front and back rendering buffers
		for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
		{
			mThisNode->setCurrentWindowIndex(i);
			getCurrentWindowPtr()->swap();
		}
		glfwPollEvents();

		// Check if ESC key was pressed or window was closed
		mRunning = !mThisNode->getKeyPressed( mExitKey ) && 
			!mThisNode->shouldAllWindowsClose() &&
			!mTerminate;

		//for all windows
		mFrameCounter++;
		mTakeScreenshot = false;

#ifdef __SGCT_RENDER_LOOP_DEBUG__
    fprintf(stderr, "Render-Loop: End iteration\n");
#endif
	}
}

/*!
	This function renders basic text info and statistics on screen.
*/
void sgct::Engine::renderDisplayInfo()
{
	if(mFixedOGLPipeline)
		glPushAttrib(GL_CURRENT_BIT);

	unsigned int lFrameNumber = 0;
	getCurrentWindowPtr()->getSwapGroupFrameNumber(lFrameNumber);

	glDrawBuffer(GL_BACK); //draw into both back buffers
	glReadBuffer(GL_BACK);
	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 95,
		glm::vec4(0.8f,0.8f,0.8f,1.0f),
		"Node ip: %s (%s)",
		mThisNode->ip.c_str(),
		mNetworkConnections->isComputerServer() ? "master" : "slave");

	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 80,
		glm::vec4(0.8f,0.8f,0.0f,1.0f),
		"Frame rate: %.3f Hz, frame: %u",
		mStatistics->getAvgFPS(),
		mFrameCounter);

	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 65,
		glm::vec4(0.8f,0.0f,0.8f,1.0f),
		"Avg. draw time: %.2f ms",
		mStatistics->getAvgDrawTime()*1000.0);

	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 50,
		glm::vec4(0.0f,0.8f,0.8f,1.0f),
		"Avg. sync time (size: %d, comp. ratio: %.3f): %.2f ms",
		SharedData::Instance()->getUserDataSize(),
		SharedData::Instance()->getCompressionRatio(),
		mStatistics->getAvgSyncTime()*1000.0);

	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 35,
		glm::vec4(0.8f,0.8f,0.8f,1.0f),
		"Swap groups: %s and %s (%s) | Frame: %d",
		getActiveWindowPtr()->isUsingSwapGroups() ? "Enabled" : "Disabled",
		getActiveWindowPtr()->isBarrierActive() ? "active" : "not active",
		getActiveWindowPtr()->isSwapGroupMaster() ? "master" : "slave",
		lFrameNumber);

	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 20,
		glm::vec4(0.8f,0.8f,0.8f,1.0f),
		"Tracked: %s | User position: %.3f %.3f %.3f",
		getCurrentWindowPtr()->getCurrentViewport()->isTracked() ? "true" : "false",
		getUserPtr()->getXPos(),
		getUserPtr()->getYPos(),
		getUserPtr()->getZPos());

	//if active stereoscopic rendering
	if( getCurrentWindowPtr()->getStereoMode() == SGCTWindow::Active )
	{
		glDrawBuffer(GL_BACK_LEFT);
		glReadBuffer(GL_BACK_LEFT);
		sgct_text::print( sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 110,
			glm::vec4(0.8f,0.8f,0.8f,1.0f),
			"Active eye: Left");

		glDrawBuffer(GL_BACK_RIGHT);
		glReadBuffer(GL_BACK_RIGHT);
		sgct_text::print( sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 110,
			glm::vec4(0.8f,0.8f,0.8f,1.0f),
			"Active eye:          Right");

		glDrawBuffer(GL_BACK);
		glReadBuffer(GL_BACK);
	}
	else //if passive stereo
	{
		if( getCurrentWindowPtr()->getCurrentViewport()->getEye() == Frustum::StereoLeftEye )
		{
			sgct_text::print( sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 110,
				glm::vec4(0.8f,0.8f,0.8f,1.0f),
				"Active eye: Left");
		}
		else if( getCurrentWindowPtr()->getCurrentViewport()->getEye() == Frustum::StereoRightEye )
		{
			sgct_text::print( sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 110,
				glm::vec4(0.8f,0.8f,0.8f,1.0f),
				"Active eye:          Right");
		}
	}

	if(mFixedOGLPipeline)
		glPopAttrib();
}

/*!
	This function enters the correct viewport, frustum, stereo mode and calls the draw callback.
*/
void sgct::Engine::draw()
{
	enterCurrentViewport(FBOSpace);

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
	enterCurrentViewport(FBOSpace);

	glMatrixMode(GL_PROJECTION);

	Viewport * tmpVP = getActiveWindowPtr()->getCurrentViewport();
	glLoadMatrixf( glm::value_ptr(tmpVP->getProjectionMatrix(mActiveFrustum)) );

	glMatrixMode(GL_MODELVIEW);

	glLoadMatrixf( glm::value_ptr( tmpVP->getViewMatrix(mActiveFrustum) * getModelMatrix() ) );

	if( mDrawFn != NULL )
	{
		glLineWidth(1.0);
		mShowWireframe ? glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ) : glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

		if( mRunMode == OSG_Encapsulation_Mode)
		{
			//glPushAttrib(GL_ALL_ATTRIB_BITS);
			glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
			//glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
			mDrawFn();
			glPopClientAttrib();
			//glPopAttrib();
		}
		else
		{
			mDrawFn();
		}

		//restore polygon mode
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	}
}

/*!
	Draw viewport overlays if there are any.
*/
void sgct::Engine::drawOverlays()
{
	glDrawBuffer(GL_BACK); //draw into both back buffers
	glReadBuffer(GL_BACK);

	std::size_t numberOfIterations = ( getCurrentWindowPtr()->isUsingFisheyeRendering() ? 1 : getCurrentWindowPtr()->getNumberOfViewports() );
	for(std::size_t i=0; i < numberOfIterations; i++)
	{
		getCurrentWindowPtr()->setCurrentViewport(i);

		//if viewport has overlay
		sgct_core::Viewport * tmpVP = getCurrentWindowPtr()->getCurrentViewport();
		if( tmpVP->hasOverlayTexture() )
		{
			/*
				Some code (using OpenSceneGraph) can mess up the viewport settings.
				To ensure correct mapping enter the current viewport.
			*/
			enterCurrentViewport(ScreenSpace);

			//enter ortho mode
			glm::mat4 orthoMat;
			if( getCurrentWindowPtr()->isUsingFisheyeRendering() )
				orthoMat = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
			else
				orthoMat = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);

			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glDisable(GL_CULL_FACE);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, TextureManager::Instance()->getTextureByHandle( tmpVP->getOverlayTextureIndex() ) );

			mShaders[OverlayShader].bind();

			glUniform1i( mShaderLocs[OverlayTex], 0);
			glUniformMatrix4fv( mShaderLocs[OverlayMVP], 1, GL_FALSE, &orthoMat[0][0]);

			getCurrentWindowPtr()->bindVAO();
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
	glDrawBuffer(GL_BACK); //draw into both back buffers
	glReadBuffer(GL_BACK);

	std::size_t numberOfIterations = ( getCurrentWindowPtr()->isUsingFisheyeRendering() ? 1 : getCurrentWindowPtr()->getNumberOfViewports() );
	for(std::size_t i=0; i < numberOfIterations; i++)
	{
		getCurrentWindowPtr()->setCurrentViewport(i);

		//if viewport has overlay
		sgct_core::Viewport * tmpVP = getCurrentWindowPtr()->getCurrentViewport();
		if( tmpVP->hasOverlayTexture() )
		{
			//enter ortho mode
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glPushMatrix();
			/*
				Some code (using OpenSceneGraph) can mess up the viewport settings.
				To ensure correct mapping enter the current viewport.
			*/
			enterCurrentViewport(ScreenSpace);

			if( getCurrentWindowPtr()->isUsingFisheyeRendering() )
				gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
			else
				gluOrtho2D(0.0, 1.0, 0.0, 1.0);

			glMatrixMode(GL_MODELVIEW);

			glPushAttrib( GL_ALL_ATTRIB_BITS );
			glDisable(GL_LIGHTING);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glDisable(GL_CULL_FACE);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glLoadIdentity();

			glColor4f(1.0f,1.0f,1.0f,1.0f);

			//glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, TextureManager::Instance()->getTextureByHandle( tmpVP->getOverlayTextureIndex() ) );

			glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

			getCurrentWindowPtr()->bindVBO();
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
	This function clears and sets the correct buffer like:

	- Backbuffer
	- FBO
	- Multisampled FBO
*/
void sgct::Engine::setRenderTarget(TextureIndexes ti)
{
	if( SGCTSettings::Instance()->useFBO() )
	{
		if( SGCTSettings::Instance()->usePostFX() )
			ti = PostFX;

		//un-bind texture
		glBindTexture(GL_TEXTURE_2D, 0);

		sgct_core::OffScreenBuffer * fbo = getCurrentWindowPtr()->mFinalFBO_Ptr;

		fbo->bind();
		if( !fbo->isMultiSampled() )
		{
			//update attachments
			fbo->attachColorTexture( getCurrentWindowPtr()->getFrameBufferTexture(ti) );
		}

		setAndClearBuffer(RenderToTexture);
	}
	else
		setAndClearBuffer(BackBuffer);
}

/*!
	Draw geometry and bind FBO as texture in screenspace (ortho mode).
	The geometry can be a simple quad or a geometry correction and blending mesh.
*/
void sgct::Engine::renderFBOTexture()
{
	//unbind framebuffer
	OffScreenBuffer::unBind();

	//enter ortho mode
	glm::mat4 orthoMat = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	//clear buffers
	mActiveFrustum = getCurrentWindowPtr()->getStereoMode() == SGCTWindow::Active ? Frustum::StereoLeftEye : Frustum::Mono;
	setAndClearBuffer(BackBufferBlack);

	glViewport (0, 0, getCurrentWindowPtr()->getXResolution(), getCurrentWindowPtr()->getYResolution());

	if( getCurrentWindowPtr()->getStereoMode() > SGCTWindow::Active )
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, getCurrentWindowPtr()->getFrameBufferTexture(LeftEye));

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, getCurrentWindowPtr()->getFrameBufferTexture(RightEye));

		getCurrentWindowPtr()->bindStereoShader();

		glUniform1i( getCurrentWindowPtr()->getStereoShaderLeftTexLoc(), 0);
		glUniform1i( getCurrentWindowPtr()->getStereoShaderRightTexLoc(), 1);
		glUniformMatrix4fv( getCurrentWindowPtr()->getStereoShaderMVPLoc(), 1, GL_FALSE, &orthoMat[0][0]);

		for(std::size_t i=0; i<getCurrentWindowPtr()->getNumberOfViewports(); i++)
			getCurrentWindowPtr()->getViewport(i)->renderMesh();
		ShaderManager::Instance()->unBindShader();
	}
	else
	{
		glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
		glBindTexture(GL_TEXTURE_2D, getCurrentWindowPtr()->getFrameBufferTexture(LeftEye));

		mShaders[FBOQuadShader].bind(); //bind
		glUniform1i( mShaderLocs[MonoTex], 0);
		glUniformMatrix4fv( mShaderLocs[MonoMVP], 1, GL_FALSE, &orthoMat[0][0]);

		for(std::size_t i=0; i<getCurrentWindowPtr()->getNumberOfViewports(); i++)
			getCurrentWindowPtr()->getViewport(i)->renderMesh();

		//render right eye in active stereo mode
		if( getCurrentWindowPtr()->getStereoMode() == SGCTWindow::Active )
		{
			//clear buffers
			mActiveFrustum = Frustum::StereoRightEye;
			setAndClearBuffer(BackBufferBlack);

			glViewport (0, 0, getCurrentWindowPtr()->getXResolution(), getCurrentWindowPtr()->getYResolution());

			glBindTexture(GL_TEXTURE_2D, getCurrentWindowPtr()->getFrameBufferTexture(RightEye));
			glUniform1i( mShaderLocs[MonoTex], 0);

			for(std::size_t i=0; i<getCurrentWindowPtr()->getNumberOfViewports(); i++)
				getCurrentWindowPtr()->getViewport(i)->renderMesh();
		}

		ShaderManager::Instance()->unBindShader();
	}
}


/*!
	Draw geometry and bind FBO as texture in screenspace (ortho mode).
	The geometry can be a simple quad or a geometry correction and blending mesh.
*/
void sgct::Engine::renderFBOTextureFixedPipeline()
{
	//unbind framebuffer
	OffScreenBuffer::unBind();

	//enter ortho mode
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glPushMatrix();
	gluOrtho2D(0.0, 1.0, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);

	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	//glDisable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//clear buffers
	mActiveFrustum = getCurrentWindowPtr()->getStereoMode() == SGCTWindow::Active ? Frustum::StereoLeftEye : Frustum::Mono;
	setAndClearBuffer(BackBufferBlack);

	glLoadIdentity();

	glViewport (0, 0, getCurrentWindowPtr()->getXResolution(), getCurrentWindowPtr()->getYResolution());

	if( getCurrentWindowPtr()->getStereoMode() > SGCTWindow::Active )
	{
		getCurrentWindowPtr()->bindStereoShader();

		glUniform1i( getCurrentWindowPtr()->getStereoShaderLeftTexLoc(), 0);
		glUniform1i( getCurrentWindowPtr()->getStereoShaderRightTexLoc(), 1);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, getCurrentWindowPtr()->getFrameBufferTexture(LeftEye));
		glEnable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, getCurrentWindowPtr()->getFrameBufferTexture(RightEye));
		glEnable(GL_TEXTURE_2D);

		for(std::size_t i=0; i<getCurrentWindowPtr()->getNumberOfViewports(); i++)
			getCurrentWindowPtr()->getViewport(i)->renderMesh();
		ShaderManager::Instance()->unBindShader();
	}
	else
	{
		glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
		glBindTexture(GL_TEXTURE_2D, getCurrentWindowPtr()->getFrameBufferTexture(LeftEye));
		glEnable(GL_TEXTURE_2D);

		for(std::size_t i=0; i<getCurrentWindowPtr()->getNumberOfViewports(); i++)
			getCurrentWindowPtr()->getViewport(i)->renderMesh();

		//render right eye in active stereo mode
		if( getCurrentWindowPtr()->getStereoMode() == SGCTWindow::Active )
		{
			//clear buffers
			mActiveFrustum = Frustum::StereoRightEye;
			setAndClearBuffer(BackBufferBlack);

			glLoadIdentity();

			glViewport(0, 0, getActiveWindowPtr()->getXResolution(), getActiveWindowPtr()->getYResolution());

			glBindTexture(GL_TEXTURE_2D, getCurrentWindowPtr()->getFrameBufferTexture(RightEye));

			for(std::size_t i=0; i<getCurrentWindowPtr()->getNumberOfViewports(); i++)
				getCurrentWindowPtr()->getViewport(i)->renderMesh();
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
	mShowWireframe ? glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ) : glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	if( mActiveFrustum == Frustum::StereoLeftEye )
		getCurrentWindowPtr()->setFisheyeOffset( -getUserPtr()->getEyeSeparation() / getCurrentWindowPtr()->getDomeDiameter(), 0.0f);
	else if( mActiveFrustum == Frustum::StereoRightEye )
		getCurrentWindowPtr()->setFisheyeOffset( getUserPtr()->getEyeSeparation() / getCurrentWindowPtr()->getDomeDiameter(), 0.0f);

	//iterate the cube sides
	for(std::size_t i=0; i<getCurrentWindowPtr()->getNumberOfViewports(); i++)
	{
		getCurrentWindowPtr()->setCurrentViewport(i);

		if( getCurrentWindowPtr()->getCurrentViewport()->isEnabled() )
		{
			//un-bind texture
			glBindTexture(GL_TEXTURE_2D, 0);

			//bind & attach buffer
			getCurrentWindowPtr()->mCubeMapFBO_Ptr->bind(); //osg seems to unbind FBO when rendering with osg FBO cameras
			if( !getCurrentWindowPtr()->mCubeMapFBO_Ptr->isMultiSampled() )
			{
				getCurrentWindowPtr()->mCubeMapFBO_Ptr->attachCubeMapTexture( getCurrentWindowPtr()->getFrameBufferTexture(FishEye), static_cast<unsigned int>(i) );
			}

			setAndClearBuffer(RenderToTexture);

			//render
			(this->*mInternalDrawFn)();

			//copy AA-buffer to "regular"/non-AA buffer
			if( getCurrentWindowPtr()->mCubeMapFBO_Ptr->isMultiSampled() )
			{
				getCurrentWindowPtr()->mCubeMapFBO_Ptr->bindBlit(); //bind separate read and draw buffers to prepare blit operation

				//update attachments
				getCurrentWindowPtr()->mCubeMapFBO_Ptr->attachCubeMapTexture( getCurrentWindowPtr()->getFrameBufferTexture(FishEye), static_cast<unsigned int>(i) );

				getCurrentWindowPtr()->mCubeMapFBO_Ptr->blit();
			}
		}
	}//end for

	//restore polygon mode
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	//bind fisheye target FBO
	getCurrentWindowPtr()->mFinalFBO_Ptr->bind();
	SGCTSettings::Instance()->usePostFX() ?
			getCurrentWindowPtr()->mFinalFBO_Ptr->attachColorTexture( getCurrentWindowPtr()->getFrameBufferTexture(PostFX) ) :
			getCurrentWindowPtr()->mFinalFBO_Ptr->attachColorTexture( getCurrentWindowPtr()->getFrameBufferTexture(ti) );

	glClearColor(mFisheyeClearColor[0], mFisheyeClearColor[1], mFisheyeClearColor[2], mFisheyeClearColor[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/*
		The code below flips the viewport vertically. Top & bottom coords are flipped.
	*/
	glm::mat4 orthoMat = glm::ortho( -1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 2.0f );

	glViewport(0, 0, getCurrentWindowPtr()->getXFramebufferResolution(), getCurrentWindowPtr()->getYFramebufferResolution());

	//if for some reson the active texture has been reset
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, getCurrentWindowPtr()->getFrameBufferTexture(FishEye));

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	getCurrentWindowPtr()->bindFisheyeShader();

	glUniformMatrix4fv( getCurrentWindowPtr()->getFisheyeShaderMVPLoc(), 1, GL_FALSE, &orthoMat[0][0]);
	glUniform1i( getCurrentWindowPtr()->getFisheyeShaderCubemapLoc(), 0);
	glUniform1f( getCurrentWindowPtr()->getFisheyeShaderHalfFOVLoc(), glm::radians<float>(getCurrentWindowPtr()->getFisheyeFOV()/2.0f) );
	if( getCurrentWindowPtr()->isFisheyeOffaxis() )
	{
		glUniform3f( getCurrentWindowPtr()->getFisheyeShaderOffsetLoc(),
			getCurrentWindowPtr()->getFisheyeOffset(0),
			getCurrentWindowPtr()->getFisheyeOffset(1),
			getCurrentWindowPtr()->getFisheyeOffset(2) );
	}

	getActiveWindowPtr()->bindVAO( SGCTWindow::FishEyeQuad );
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	getActiveWindowPtr()->unbindVAO();

	ShaderManager::Instance()->unBindShader();

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

			sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), x, 2.0f * y + y/5.0f, "Frame#: %d", getCurrentWindowPtr()->getScreenShotNumber());

			if( mActiveFrustum == Frustum::Mono )
				sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), x, y, "Mono");
			else if( mActiveFrustum == Frustum::StereoLeftEye )
				sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), x, y, "Left");
			else
				sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), x, y, "Right");
		}
	}
}

/*!
	This functions works in two steps:
	1. Render a cubemap
	2. Render to a fisheye using a GLSL shader
*/
void sgct::Engine::renderFisheyeFixedPipeline(TextureIndexes ti)
{
	mShowWireframe ? glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ) : glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	if( mActiveFrustum == Frustum::StereoLeftEye )
		getCurrentWindowPtr()->setFisheyeOffset( -getUserPtr()->getEyeSeparation() / getCurrentWindowPtr()->getDomeDiameter(), 0.0f);
	else if( mActiveFrustum == Frustum::StereoRightEye )
		getCurrentWindowPtr()->setFisheyeOffset( getUserPtr()->getEyeSeparation() / getCurrentWindowPtr()->getDomeDiameter(), 0.0f);

	//iterate the cube sides
	for(std::size_t i=0; i<getCurrentWindowPtr()->getNumberOfViewports(); i++)
	{
		getCurrentWindowPtr()->setCurrentViewport(i);

		if( getCurrentWindowPtr()->getCurrentViewport()->isEnabled() )
		{
			//un-bind texture
			glBindTexture(GL_TEXTURE_2D, 0);

			//bind & attach buffer
			getCurrentWindowPtr()->mCubeMapFBO_Ptr->bind(); //osg seems to unbind FBO when rendering with osg FBO cameras
			if( !getCurrentWindowPtr()->mCubeMapFBO_Ptr->isMultiSampled() )
			{
				getCurrentWindowPtr()->mCubeMapFBO_Ptr->attachCubeMapTexture( getCurrentWindowPtr()->getFrameBufferTexture(FishEye), static_cast<unsigned int>(i) );
			}

			setAndClearBuffer(RenderToTexture);

			//render
			(this->*mInternalDrawFn)();

			//copy AA-buffer to "regular"/non-AA buffer
			if( getCurrentWindowPtr()->mCubeMapFBO_Ptr->isMultiSampled() )
			{
				getCurrentWindowPtr()->mCubeMapFBO_Ptr->bindBlit(); //bind separate read and draw buffers to prepare blit operation

				//update attachments
				getCurrentWindowPtr()->mCubeMapFBO_Ptr->attachCubeMapTexture( getCurrentWindowPtr()->getFrameBufferTexture(FishEye), static_cast<unsigned int>(i) );

				getCurrentWindowPtr()->mCubeMapFBO_Ptr->blit();
			}
		}
	}//end for

	//restore polygon mode
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	//un-bind texture
	glBindTexture(GL_TEXTURE_2D, 0);

	//bind fisheye target FBO
	getCurrentWindowPtr()->mFinalFBO_Ptr->bind();
	SGCTSettings::Instance()->usePostFX() ?
			getCurrentWindowPtr()->mFinalFBO_Ptr->attachColorTexture( getCurrentWindowPtr()->getFrameBufferTexture(PostFX) ) :
			getCurrentWindowPtr()->mFinalFBO_Ptr->attachColorTexture( getCurrentWindowPtr()->getFrameBufferTexture(ti) );

	glClearColor(mFisheyeClearColor[0], mFisheyeClearColor[1], mFisheyeClearColor[2], mFisheyeClearColor[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	/*
		The code below flips the viewport vertically. Top & bottom coords are flipped.
	*/

	glOrtho( -1.0,
		1.0,
		-1.0,
		1.0,
		0.1,
		2.0);

	glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glViewport(0, 0, getActiveWindowPtr()->getXFramebufferResolution(), getActiveWindowPtr()->getYFramebufferResolution());

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	//if for some reson the active texture has been reset
	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, getCurrentWindowPtr()->getFrameBufferTexture(FishEye));

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	getCurrentWindowPtr()->bindFisheyeShader();
	glUniform1i( getCurrentWindowPtr()->getFisheyeShaderCubemapLoc(), 0);
	glUniform1f( getCurrentWindowPtr()->getFisheyeShaderHalfFOVLoc(), glm::radians<float>(getCurrentWindowPtr()->getFisheyeFOV()/2.0f) );
	if( getCurrentWindowPtr()->isFisheyeOffaxis() )
	{
		glUniform3f( getCurrentWindowPtr()->getFisheyeShaderOffsetLoc(),
			getCurrentWindowPtr()->getFisheyeOffset(0),
			getCurrentWindowPtr()->getFisheyeOffset(1),
			getCurrentWindowPtr()->getFisheyeOffset(2) );
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

	ShaderManager::Instance()->unBindShader();

	if(mTakeScreenshot)
	{
		glDisable(GL_TEXTURE_CUBE_MAP);
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

			sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), x, 2.0f * y + y/5.0f, "Frame#: %d", getCurrentWindowPtr()->getScreenShotNumber());

			if( mActiveFrustum == Frustum::Mono )
				sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), x, y, "Mono");
			else if( mActiveFrustum == Frustum::StereoLeftEye )
				sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), x, y, "Left");
			else
				sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), x, y, "Right");
		}
	}

	glPopClientAttrib();
	glPopAttrib();
	glPopMatrix();
}

/*!
	This function combines a texture and a shader into a new texture
*/
void sgct::Engine::renderPostFx(TextureIndexes ti)
{
	//bind fisheye target FBO
	getCurrentWindowPtr()->mFinalFBO_Ptr->attachColorTexture( getCurrentWindowPtr()->getFrameBufferTexture( ti ) );

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/*
		The code below flips the viewport vertically. Top & bottom coords are flipped.
	*/
	glm::mat4 orthoMat = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);

	//if for some reson the active texture has been reset
	glViewport(0, 0, getCurrentWindowPtr()->getXFramebufferResolution(), getCurrentWindowPtr()->getYFramebufferResolution());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, getCurrentWindowPtr()->getFrameBufferTexture( PostFX ) );

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	mShaders[FXAAShader].bind();
	glUniformMatrix4fv( mShaderLocs[FXAAMVP], 1, GL_FALSE, &orthoMat[0][0]);
	glUniform1f( mShaderLocs[SizeX], static_cast<float>(getCurrentWindowPtr()->getXFramebufferResolution()) );
	glUniform1f( mShaderLocs[SizeY], static_cast<float>(getCurrentWindowPtr()->getYFramebufferResolution()) );
	glUniform1i( mShaderLocs[FXAATexture], 0 );

	getActiveWindowPtr()->bindVAO( SGCTWindow::RenderQuad );
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	getActiveWindowPtr()->unbindVAO();

	//unbind
	ShaderManager::Instance()->unBindShader();
}

/*!
	This function combines a texture and a shader into a new texture
*/
void sgct::Engine::renderPostFxFixedPipeline(TextureIndexes ti)
{
	//bind fisheye target FBO
	getCurrentWindowPtr()->mFinalFBO_Ptr->attachColorTexture( getCurrentWindowPtr()->getFrameBufferTexture( ti ) );

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	/*
		The code below flips the viewport vertically. Top & bottom coords are flipped.
	*/

	glOrtho( 0.0,
		1.0,
		0.0,
		1.0,
		0.1,
		2.0);

	//if for some reson the active texture has been reset
	glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glViewport(0, 0, getCurrentWindowPtr()->getXFramebufferResolution(), getCurrentWindowPtr()->getYFramebufferResolution());

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, getCurrentWindowPtr()->getFrameBufferTexture( PostFX ) );

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	mShaders[FXAAShader].bind();
	glUniform1f( mShaderLocs[SizeX], static_cast<float>(getCurrentWindowPtr()->getXFramebufferResolution()) );
	glUniform1f( mShaderLocs[SizeY], static_cast<float>(getCurrentWindowPtr()->getYFramebufferResolution()) );
	glUniform1i( mShaderLocs[FXAATexture], 0 );

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

	getActiveWindowPtr()->bindVBO( SGCTWindow::RenderQuad );
	glClientActiveTexture(GL_TEXTURE0);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(0));

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(8));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	getActiveWindowPtr()->unbindVBO();

	ShaderManager::Instance()->unBindShader();

	glPopClientAttrib();
	glPopAttrib();
	glPopMatrix();
}

/*!
	This function updates the renderingtargets.
*/
void sgct::Engine::updateRenderingTargets(TextureIndexes ti)
{
	//copy AA-buffer to "regular"/non-AA buffer
	if( getCurrentWindowPtr()->mFinalFBO_Ptr->isMultiSampled() )
	{
		if( SGCTSettings::Instance()->usePostFX() )
			ti = PostFX;

		getCurrentWindowPtr()->mFinalFBO_Ptr->bindBlit(); //bind separate read and draw buffers to prepare blit operation

		//update attachments
		getCurrentWindowPtr()->mFinalFBO_Ptr->attachColorTexture( getCurrentWindowPtr()->getFrameBufferTexture(ti) );

		getCurrentWindowPtr()->mFinalFBO_Ptr->blit();
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
	if( mFixedOGLPipeline )
	{
		mShaders[FXAAShader].setVertexShaderSrc( sgct_core::shaders::FXAA_Vert_Shader, ShaderProgram::SHADER_SRC_STRING );
		mShaders[FXAAShader].setFragmentShaderSrc( sgct_core::shaders::FXAA_Frag_Shader, ShaderProgram::SHADER_SRC_STRING );
		mShaders[FXAAShader].createAndLinkProgram();
	}
	else
	{
		mShaders[FXAAShader].setVertexShaderSrc( sgct_core::shaders_modern::FXAA_Vert_Shader, ShaderProgram::SHADER_SRC_STRING );
		mShaders[FXAAShader].setFragmentShaderSrc( sgct_core::shaders_modern::FXAA_Frag_Shader, ShaderProgram::SHADER_SRC_STRING );
		mShaders[FXAAShader].createAndLinkProgram();
	}
	mShaders[FXAAShader].bind();

	if( !mFixedOGLPipeline )
		mShaderLocs[FXAAMVP] = mShaders[FXAAShader].getUniformLocation( "MVP" );

	mShaderLocs[SizeX] = mShaders[FXAAShader].getUniformLocation( "rt_w" );
	glUniform1f( mShaderLocs[SizeX], static_cast<float>(getActiveWindowPtr()->getXFramebufferResolution()) );

	mShaderLocs[SizeY] = mShaders[FXAAShader].getUniformLocation( "rt_h" );
	glUniform1f( mShaderLocs[SizeY], static_cast<float>(getActiveWindowPtr()->getYFramebufferResolution()) );

	mShaderLocs[FXAASubPixShift] = mShaders[FXAAShader].getUniformLocation( "FXAA_SUBPIX_SHIFT" );
	//glUniform1f( mShaderLocs[FXAASubPixShift], 0.25f );
	glUniform1f( mShaderLocs[FXAASubPixShift], 0.0f ); //better quality

	mShaderLocs[FXAASpanMax] = mShaders[FXAAShader].getUniformLocation( "FXAA_SPAN_MAX" );
	glUniform1f( mShaderLocs[FXAASpanMax], 8.0f );

	mShaderLocs[FXAARedMul] = mShaders[FXAAShader].getUniformLocation( "FXAA_REDUCE_MUL" );
	//glUniform1f( mShaderLocs[FXAARedMul], 1.0f/8.0f );
	glUniform1f( mShaderLocs[FXAARedMul], 0.0f ); //better quality

	mShaderLocs[FXAAOffset] = mShaders[FXAAShader].getUniformLocation( "vx_offset" );
	glUniform1f( mShaderLocs[FXAAOffset], 0.0f );

	mShaderLocs[FXAATexture] = mShaders[FXAAShader].getUniformLocation( "tex0" );
	glUniform1i( mShaderLocs[FXAATexture], 0 );

	ShaderProgram::unbind();

	/*!
		Used for overlays & mono.
	*/
	if( !mFixedOGLPipeline )
	{
		mShaders[FBOQuadShader].setVertexShaderSrc( sgct_core::shaders_modern::Base_Vert_Shader, ShaderProgram::SHADER_SRC_STRING );
		mShaders[FBOQuadShader].setFragmentShaderSrc( sgct_core::shaders_modern::Base_Frag_Shader, ShaderProgram::SHADER_SRC_STRING );
		mShaders[FBOQuadShader].createAndLinkProgram();
		mShaders[FBOQuadShader].bind();
		mShaderLocs[MonoMVP] = mShaders[FBOQuadShader].getUniformLocation( "MVP" );
		mShaderLocs[MonoTex] = mShaders[FBOQuadShader].getUniformLocation( "Tex" );
		glUniform1i( mShaderLocs[MonoTex], 0 );
		ShaderProgram::unbind();

		mShaders[OverlayShader].setVertexShaderSrc( sgct_core::shaders_modern::Overlay_Vert_Shader, ShaderProgram::SHADER_SRC_STRING );
		mShaders[OverlayShader].setFragmentShaderSrc( sgct_core::shaders_modern::Overlay_Frag_Shader, ShaderProgram::SHADER_SRC_STRING );
		mShaders[OverlayShader].createAndLinkProgram();
		mShaders[OverlayShader].bind();
		mShaderLocs[OverlayMVP] = mShaders[OverlayShader].getUniformLocation( "MVP" );
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
		if( getCurrentWindowPtr()->getStereoMode() != SGCTWindow::Active )
		{
			glDrawBuffer(GL_BACK);
			glReadBuffer(GL_BACK);
		}
		else if( mActiveFrustum == Frustum::StereoLeftEye ) //if active left
		{
			glDrawBuffer(GL_BACK_LEFT);
			glReadBuffer(GL_BACK_LEFT);
		}
		else if( mActiveFrustum == Frustum::StereoRightEye ) //if active right
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
	Avoid this function in the render loop for release code since it can reduse performance.

	Returns true if no errors occured
*/
bool sgct::Engine::checkForOGLErrors()
{
	GLenum oglError = glGetError();

	switch( oglError )
	{
	case GL_INVALID_ENUM:
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "OpenGL error: GL_INVALID_ENUM\n");
		break;

	case GL_INVALID_VALUE:
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "OpenGL error: GL_INVALID_VALUE\n");
		break;

	case GL_INVALID_OPERATION:
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "OpenGL error: GL_INVALID_OPERATION\n");
		break;

	case GL_INVALID_FRAMEBUFFER_OPERATION:
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "OpenGL error: GL_INVALID_FRAMEBUFFER_OPERATION\n");
		break;

	case GL_STACK_OVERFLOW:
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "OpenGL error: GL_STACK_OVERFLOW\n");
		break;

	case GL_STACK_UNDERFLOW:
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "OpenGL error: GL_STACK_UNDERFLOW\n");
		break;

	case GL_OUT_OF_MEMORY:
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "OpenGL error: GL_OUT_OF_MEMORY\n");
		break;

	case GL_TABLE_TOO_LARGE:
		MessageHandler::Instance()->print(MessageHandler::NOTIFY_ERROR, "OpenGL error: GL_TABLE_TOO_LARGE\n");
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
	//Must wait until all nodes are running if using swap barrier
	if( !mIgnoreSync && ClusterManager::Instance()->getNumberOfNodes() > 1)
	{
		//check if swapgroups are supported
		#ifdef __WIN32__
		if( wglewIsSupported("WGL_NV_swap_group") )
			MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Swap groups are supported by hardware.\n");
		#else
		if( glewIsSupported("GLX_NV_swap_group") )
			MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Swap groups are supported by hardware.\n");
		#endif
		else
			MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Swap groups are not supported by hardware.\n");

		MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Waiting for all nodes to connect.");
		for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
		{
			if( !mThisNode->getWindowPtr(i)->isUsingSwapGroups() )
				MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Swapgroups (swap-lock) are disabled for window %d.\n", i);

			glfwSwapBuffers( mThisNode->getWindowPtr(i)->getWindowHandle() );
		}
		glfwPollEvents();

		while(mNetworkConnections->isRunning() &&
			!mThisNode->getKeyPressed( mExitKey ) &&
			!mThisNode->shouldAllWindowsClose() &&
			!mTerminate)
		{
			MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, ".");

			// Swap front and back rendering buffers
			for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
				glfwSwapBuffers( mThisNode->getWindowPtr(i)->getWindowHandle() );
			glfwPollEvents();

			if(mNetworkConnections->areAllNodesConnected())
				break;

			SGCTMutexManager::Instance()->lockMutex( SGCTMutexManager::MainMutex );
			NetworkManager::gCond.wait( (*SGCTMutexManager::Instance()->getMutexPtr(SGCTMutexManager::MainMutex )) );
			SGCTMutexManager::Instance()->unlockMutex( SGCTMutexManager::MainMutex );
		}

		//wait for user to release exit key
		while( mThisNode->getKeyPressed( mExitKey ) )
		{
			// Swap front and back rendering buffers
			// key buffers also swapped
			for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
				glfwSwapBuffers( mThisNode->getWindowPtr(i)->getWindowHandle() );
			glfwPollEvents();

			tthread::this_thread::sleep_for(tthread::chrono::milliseconds(50));
		}

		MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "\n");
	}
}

/*!
	This functions updates the frustum of all viewports on demand. However if the viewport is tracked this is done on the fly.
*/
void sgct::Engine::calculateFrustums()
{
	for(size_t w=0; w < mThisNode->getNumberOfWindows(); w++)
	{
		mThisNode->setCurrentWindowIndex(w);
		
		for(unsigned int i=0; i < getCurrentWindowPtr()->getNumberOfViewports(); i++)
			if( !getCurrentWindowPtr()->getViewport(i)->isTracked() ) //if not tracked update, otherwise this is done on the fly
			{
				SGCTUser * usrPtr = ClusterManager::Instance()->getUserPtr();

				if( !getCurrentWindowPtr()->isUsingFisheyeRendering() )
				{
					getCurrentWindowPtr()->getViewport(i)->calculateFrustum(
						Frustum::Mono,
						usrPtr->getPosPtr(),
						mNearClippingPlaneDist,
						mFarClippingPlaneDist);

					getCurrentWindowPtr()->getViewport(i)->calculateFrustum(
						Frustum::StereoLeftEye,
						usrPtr->getPosPtr(Frustum::StereoLeftEye),
						mNearClippingPlaneDist,
						mFarClippingPlaneDist);

					getCurrentWindowPtr()->getViewport(i)->calculateFrustum(
						Frustum::StereoRightEye,
						usrPtr->getPosPtr(Frustum::StereoRightEye),
						mNearClippingPlaneDist,
						mFarClippingPlaneDist);
				}
				else
				{
					getCurrentWindowPtr()->getViewport(i)->calculateFisheyeFrustum(
						Frustum::Mono,
						usrPtr->getPosPtr(),
						usrPtr->getPosPtr(),
						mNearClippingPlaneDist,
						mFarClippingPlaneDist);

					getCurrentWindowPtr()->getViewport(i)->calculateFisheyeFrustum(
						Frustum::StereoLeftEye,
						usrPtr->getPosPtr(),
						usrPtr->getPosPtr(Frustum::StereoLeftEye),
						mNearClippingPlaneDist,
						mFarClippingPlaneDist);

					getCurrentWindowPtr()->getViewport(i)->calculateFisheyeFrustum(
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
	MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "Parsing arguments...");
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
		else if( strcmp(argv[i],"-local") == 0 && argc > (i+1) )
		{
			localRunningMode = NetworkManager::LocalServer;
			int tmpi = -1;
			std::stringstream ss( argv[i+1] );
			ss >> tmpi;
			ClusterManager::Instance()->setThisNodeId(tmpi);
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
				MessageHandler::Instance()->setNotifyLevel( static_cast<MessageHandler::NotifyLevel>( tmpi ) );
            argumentsToRemove.push_back(i);
            argumentsToRemove.push_back(i+1);
			i+=2;
		}
		else if( strcmp(argv[i],"--Firm-Sync") == 0 )
		{
			ClusterManager::Instance()->setFirmFrameLockSyncStatus(true);
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--Loose-Sync") == 0 )
		{
			ClusterManager::Instance()->setFirmFrameLockSyncStatus(false);
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
		else if( strcmp(argv[i],"--No-FBO") == 0 )
		{
			SGCTSettings::Instance()->setUseFBO(false);
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--FXAA") == 0 )
		{
			SGCTSettings::Instance()->setFXAA(true);
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--Capture-TGA") == 0 )
		{
			SGCTSettings::Instance()->setCaptureFormat("TGA");
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--Capture-PNG") == 0 )
		{
			SGCTSettings::Instance()->setCaptureFormat("PNG");
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"-numberOfCaptureThreads") == 0 && argc > (i+1) )
		{
			int tmpi = -1;
			std::stringstream ss( argv[i+1] );
			ss >> tmpi;

			if(tmpi > 0)
				SGCTSettings::Instance()->setNumberOfCaptureThreads( tmpi );

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

	MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, " Done\n");
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
	\param fnPtr is the function pointer to an external control function callback

	This function sets the external control callback which will be called when a TCP message is received. The TCP listner is enabled in the XML configuration file in the Cluster tag by externalControlPort, where the portnumber is an integer preferably above 20000.
	Example:
	\code
	<Cluster masterAddress="127.0.0.1" externalControlPort="20500">
	\endcode

	All TCP messages must be separated by carriage return (CR) followed by a newline (NL). Look at this [tutorial](https://c-student.itn.liu.se/wiki/develop:sgcttutorials:externalguicsharp) for more info.

*/
void sgct::Engine::setExternalControlCallback(void(*fnPtr)(const char *, int, int))
{
	mNetworkCallbackFn = fnPtr;
}

/*!
	\param fnPtr is the function pointer to a keyboard function callback

	This function sets the keyboard callback (GLFW wrapper) where the two parameters are: int key, int action. Key can be a character (e.g. 'A', 'B', '5' or ',') or a special character defined in the table below. Action can either be SGCT_PRESS or SGCT_RELEASE.

	Name          | Description
	------------- | -------------
	SGCT_KEY_UNKNOWN  | Unknown
	SGCT_KEY_SPACE  | Space
	SGCT_KEY_SPECIAL | Special
	SGCT_KEY_ESC  | Escape
	SGCT_KEY_Fn  | Function key n (n can be in the range 1..25)
	SGCT_KEY_UP  | Arrow/Cursor up
	SGCT_KEY_DOWN  | Arrow/Cursor down
	SGCT_KEY_LEFT  | Arrow/Cursor left
	SGCT_KEY_RIGHT  | Arrow/Cursor right
	SGCT_KEY_LSHIFT  | Left shift key
	SGCT_KEY_RSHIFT  | Right shift key
	SGCT_KEY_LCTRL  | Left control key
	SGCT_KEY_RCTRL  | Right control key
	SGCT_KEY_LALT  | Left alternate function key
	SGCT_KEY_RALT  | Right alternate function key
	SGCT_KEY_TAB  | Tabulator
	SGCT_KEY_ENTER  | Enter
	SGCT_KEY_BACKSPACE  | Backspace
	SGCT_KEY_INSERT  | Insert
	SGCT_KEY_DEL  | Delete
	SGCT_KEY_PAGEUP  | Page up
	SGCT_KEY_PAGEDOWN  | Page down
	SGCT_KEY_HOME  | Home
	SGCT_KEY_END  | End
	SGCT_KEY_KP_n  | Keypad numeric key n (n can be in the range 0..9)
	SGCT_KEY_KP_DIVIDE  | Keypad divide
	SGCT_KEY_KP_MULTIPLY  | Keypad multiply
	SGCT_KEY_KP_SUBTRACT  | Keypad subtract
	SGCT_KEY_KP_ADD  | Keypad add
	SGCT_KEY_KP_DECIMAL  | Keypad decimal
	SGCT_KEY_KP_EQUAL  | Keypad equal
	SGCT_KEY_KP_ENTER  | Keypad enter
	SGCT_KEY_KP_NUM_LOCK  | Keypad num lock
	SGCT_KEY_CAPS_LOCK  | Caps lock
	SGCT_KEY_SCROLL_LOCK  | Scroll lock
	SGCT_KEY_PAUSE  | Pause key
	SGCT_KEY_LSUPER  | Left super key, WinKey, or command key
	SGCT_KEY_RSUPER  | Right super key, WinKey, or command key
	SGCT_KEY_MENU  | Menu key
	SGCT_KEY_LAST  | Last key index

*/

void sgct::Engine::setKeyboardCallbackFunction( void(*fnPtr)(int,int) )
{
	gKeyboardCallbackFn = fnPtr;
}

void sgct::Engine::setCharCallbackFunction( void(*fnPtr)(unsigned int) )
{
	gCharCallbackFn = fnPtr;
}

void sgct::Engine::setMouseButtonCallbackFunction( void(*fnPtr)(int, int) )
{
	gMouseButtonCallbackFn = fnPtr;
}

void sgct::Engine::setMousePosCallbackFunction( void(*fnPtr)(double, double) )
{
	gMousePosCallbackFn = fnPtr;
}

void sgct::Engine::setMouseScrollCallbackFunction( void(*fnPtr)(double, double) )
{
	gMouseScrollCallbackFn = fnPtr;
}

void sgct::Engine::clearBuffer()
{
	const float * colorPtr = Engine::getPtr()->getClearColor();

	float alpha = (Instance()->getActiveWindowPtr()->useFisheyeAlpha() && Instance()->getActiveWindowPtr()->isUsingFisheyeRendering()) ? 0.0f : colorPtr[3];

	glClearColor(colorPtr[0], colorPtr[1], colorPtr[2], alpha);
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
	MessageHandler::Instance()->print(MessageHandler::NOTIFY_VERSION_INFO, "GLFW error: %s\n", description);
}

/*!
	Print the node info to terminal.

	\param nodeId Which node to print
*/
void sgct::Engine::printNodeInfo(unsigned int nodeId)
{
	MessageHandler::Instance()->print(MessageHandler::NOTIFY_INFO, "This node has index %d.\n", nodeId);
}

/*!
	Set up the current viewport.

	\param vs Which space to set up the viewport in.
*/
void sgct::Engine::enterCurrentViewport(ViewportSpace vs)
{
	if( getCurrentWindowPtr()->isUsingFisheyeRendering() && vs != ScreenSpace )
	{
		int cmRes = getCurrentWindowPtr()->getCubeMapResolution();
		currentViewportCoords[0] = 0;
		currentViewportCoords[1] = 0;
		currentViewportCoords[2] = cmRes;
		currentViewportCoords[3] = cmRes;
	}
	else
	{
		if( vs == ScreenSpace || !SGCTSettings::Instance()->useFBO() )
		{
			currentViewportCoords[0] =
				static_cast<int>( getCurrentWindowPtr()->getCurrentViewport()->getX() * static_cast<double>(getActiveWindowPtr()->getXResolution()));
			currentViewportCoords[1] =
				static_cast<int>( getCurrentWindowPtr()->getCurrentViewport()->getY() * static_cast<double>(getActiveWindowPtr()->getYResolution()));
			currentViewportCoords[2] =
				static_cast<int>( getCurrentWindowPtr()->getCurrentViewport()->getXSize() * static_cast<double>(getActiveWindowPtr()->getXResolution()));
			currentViewportCoords[3] =
				static_cast<int>( getCurrentWindowPtr()->getCurrentViewport()->getYSize() * static_cast<double>(getActiveWindowPtr()->getYResolution()));
		}
		else
		{
			currentViewportCoords[0] =
				static_cast<int>( getCurrentWindowPtr()->getCurrentViewport()->getX() * static_cast<double>(getActiveWindowPtr()->getXFramebufferResolution()));
			currentViewportCoords[1] =
				static_cast<int>( getCurrentWindowPtr()->getCurrentViewport()->getY() * static_cast<double>(getActiveWindowPtr()->getYFramebufferResolution()));
			currentViewportCoords[2] =
				static_cast<int>( getCurrentWindowPtr()->getCurrentViewport()->getXSize() * static_cast<double>(getActiveWindowPtr()->getXFramebufferResolution()));
			currentViewportCoords[3] =
				static_cast<int>( getCurrentWindowPtr()->getCurrentViewport()->getYSize() * static_cast<double>(getActiveWindowPtr()->getYFramebufferResolution()));
		}
	}

	glViewport( currentViewportCoords[0],
		currentViewportCoords[1],
		currentViewportCoords[2],
		currentViewportCoords[3]);
}

void sgct::Engine::calculateFPS(double timestamp)
{
	static double lastTimestamp = glfwGetTime();
	mStatistics->setFrameTime(timestamp - lastTimestamp);
	lastTimestamp = timestamp;
    static double renderedFrames = 0.0;
	static double tmpTime = 0.0;
	renderedFrames += 1.0;
	tmpTime += mStatistics->getFrameTime();
	if( tmpTime >= 1.0 )
	{
		mStatistics->setAvgFPS(renderedFrames / tmpTime);
		renderedFrames = 0.0;
		tmpTime = 0.0;

		for(size_t i=0; i < mThisNode->getNumberOfWindows(); i++)
			if( !mThisNode->getWindowPtr(i)->isFullScreen() )
				mThisNode->getWindowPtr(i)->setWindowTitle( getBasicInfo(i) );
	}
}

/*!
\returns the frame time (delta time) in seconds
*/
const double & sgct::Engine::getDt()
{
	return mStatistics->getFrameTime();
}

/*!
\returns the average frame time (delta time) in seconds
*/
const double & sgct::Engine::getAvgDt()
{
	return mStatistics->getAvgFrameTime();
}

/*!
\returns the draw time in seconds
*/
const double & sgct::Engine::getDrawTime()
{
	return mStatistics->getDrawTime();
}

/*!
\returns the sync time (time waiting for other nodes and network) in seconds
*/
const double & sgct::Engine::getSyncTime()
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
	calculateFrustums();
}

/*!
	Set the eye separation (interocular distance) for the user. This operation recalculates all frustums for all viewports.

	@param eyeSeparation eye separation in meters
*/
void sgct::Engine::setEyeSeparation(float eyeSeparation)
{
	getUserPtr()->setEyeSeparation( eyeSeparation );
	calculateFrustums();
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
	Don't use this. This function is called from SGCTNetwork and will invoke the external network callback when messages are received.
*/
void sgct::Engine::decodeExternalControl(const char * receivedData, int receivedlength, int clientIndex)
{
	if(mNetworkCallbackFn != NULL && receivedlength > 0)
		mNetworkCallbackFn(receivedData, receivedlength, clientIndex);
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
	This function returns the window name string containing info as node ip, if its master or slave, fps and AA settings.
	This function is called once per second.
*/
const char * sgct::Engine::getBasicInfo(std::size_t winIndex)
{
	#if (_MSC_VER >= 1400) //visual studio 2005 or later
	sprintf_s( basicInfo, sizeof(basicInfo), "Node: %s (%s:%d) | fps: %.2f | AA: %s",
		localRunningMode == NetworkManager::NotLocal ? mThisNode->ip.c_str() : "127.0.0.1",
		mNetworkConnections->isComputerServer() ? "master" : "slave",
		winIndex,
		static_cast<float>(mStatistics->getAvgFPS()),
        getAAInfo(winIndex));
    #else
    sprintf( basicInfo, "Node: %s (%s:%d) | fps: %.2f | AA: %s",
		localRunningMode == NetworkManager::NotLocal ? mThisNode->ip.c_str() : "127.0.0.1",
		mNetworkConnections->isComputerServer() ? "master" : "slave",
		winIndex,
		static_cast<float>(mStatistics->getAvgFPS()),
        getAAInfo(winIndex));
    #endif

	return basicInfo;
}

/*!
	This function returns the Anti-Aliasing (AA) settings.
	This function is called once per second.
*/
const char * sgct::Engine::getAAInfo(std::size_t winIndex)
{
    if( SGCTSettings::Instance()->useFXAA() )
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
                getWindowPtr(winIndex)->numberOfSamples);
            #endif
        }
        else
        #if (_MSC_VER >= 1400) //visual studio 2005 or later
        strcpy_s(aaInfo, sizeof(aaInfo), "none");
        #else
            strcpy(aaInfo, "none");
        #endif
    }

	return aaInfo;
}

/*!
	Checks the keyboard is if the specified key has been pressed.
	\returns SGCT_PRESS or SGCT_RELEASE
*/
int sgct::Engine::getKey( int key )
{
	return glfwGetKey(getActiveWindowPtr()->getWindowHandle(), key);
}

/*!
	Checks if specified mouse button has been pressed.
	\returns SGCT_PRESS or SGCT_RELEASE
*/
int sgct::Engine::getMouseButton( int button )
{
	return glfwGetMouseButton(getActiveWindowPtr()->getWindowHandle(), button);
}

void sgct::Engine::getMousePos( double * xPos, double * yPos )
{
	glfwGetCursorPos(getActiveWindowPtr()->getWindowHandle(), xPos, yPos);
}

void sgct::Engine::setMousePos( double xPos, double yPos )
{
	glfwSetCursorPos(getActiveWindowPtr()->getWindowHandle(), xPos, yPos);
}

void sgct::Engine::setMousePointerVisibility( bool state )
{
	state ? glfwSetInputMode( getActiveWindowPtr()->getWindowHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL ) : glfwSetInputMode( getActiveWindowPtr()->getWindowHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL );
}

const char * sgct::Engine::getJoystickName( int joystick )
{
	return glfwGetJoystickName(joystick);
}

const float * sgct::Engine::getJoystickAxes( int joystick, int * numOfValues)
{
	return glfwGetJoystickAxes( joystick, numOfValues );
}

const unsigned char * sgct::Engine::getJoystickButtons( int joystick, int * numOfValues)
{
	return glfwGetJoystickButtons( joystick, numOfValues );
}

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
size_t sgct::Engine::createTimer( double millisec, void(*fnPtr)(size_t) )
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
        MessageHandler::Instance()->print(MessageHandler::NOTIFY_WARNING, "There was no timer with id: %d", id);
    }
}

/*!
	Get the time from program start in seconds
*/
double sgct::Engine::getTime()
{
	return glfwGetTime();
}
