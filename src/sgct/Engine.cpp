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
#include "../include/sgct/ShaderManager.h"
#include "../include/sgct/SGCTInternalShaders.h"
#include "../include/sgct/SGCTInternalShaders_modern.h"
#include "../include/sgct/SGCTVersion.h"
#include "../include/sgct/SGCTSettings.h"
#include "../include/sgct/ogl_headers.h"
#include <glm/gtc/constants.hpp>
#include <math.h>
#include <iostream>
#include <sstream>
#include <deque>

using namespace sgct_core;

sgct::Engine * sgct::Engine::mInstance = NULL;

#ifdef GLEW_MX
GLEWContext * glewGetContext();
#endif

#define MAX_SGCT_PATH_LENGTH 256
#define USE_SLEEP_TO_WAIT_FOR_NODES 0

/*!
This is the only valid constructor that also initiates [GLFW](http://www.glfw.org/). Command line parameters are used to load a configuration file and settings.
Note that parameter with one '\-' are followed by arguments but parameters with '\-\-' are just options without arguments.

Parameter     | Description
------------- | -------------
-config <filename> | set xml confiuration file
-local <integer> | set which node in configuration that is the localhost (index starts at 0)
-numberOfCaptureThreads <integer> | set the maximum amount of threads that should be used during framecapture (default 8)
--client | run the application as client (only available when running as local)
--slave | run the application as client (only available when running as local)
--Firm-Sync | enable firm frame sync
--Loose-Sync | disable firm frame sync
--Ignore-Sync | disable frame sync
--No-FBO | don't use frame buffer objects (some stereo modes, FXAA and fisheye rendering will be disabled)
--Regular-FBO | use regular frame buffer objects without multi sampling
--MultiSampled-FBO | use multisampled frame buffer objects (default)
--FXAA | use fast approximate anti-aliasing shader
--Capture-PNG | use png images for screen capture (default)
--Capture-TGA | use tga images for screen capture

*/
sgct::Engine::Engine( int& argc, char**& argv )
{
	//init pointers
	mInstance = this;
	mNetworkConnections = NULL;
	mConfig = NULL;
	mRunMode = Default_Mode;
	mFinalFBO_Ptr = NULL;
	mCubeMapFBO_Ptr = NULL;

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
	mKeyboardCallbackFn = NULL;
	mCharCallbackFn = NULL;
	mMouseButtonCallbackFn = NULL;
	mMousePosCallbackFn = NULL;
	mMouseWheelCallbackFn = NULL;

	mTerminate = false;
	mIgnoreSync = false;
	mRenderingOffScreen = false;
	mFixedOGLPipeline = true;

	localRunningMode = NetworkManager::NotLocal;

	currentViewportCoords[0] = 0;
	currentViewportCoords[1] = 0;
	currentViewportCoords[2] = 640;
	currentViewportCoords[3] = 480;

	//FBO stuff
	mFrameBufferTextures[0] = 0;
	mFrameBufferTextures[1] = 0;
	mFrameBufferTextures[2] = 0;
	mFrameBufferTextures[3] = 0;

	mPostFxQuadVerts[0] = 0.0f;
	mPostFxQuadVerts[1] = 0.0f;
	mPostFxQuadVerts[2] = 0.0f;
	mPostFxQuadVerts[3] = 0.0f;
	mPostFxQuadVerts[4] = -1.0f;

	mPostFxQuadVerts[5] = 1.0f;
	mPostFxQuadVerts[6] = 0.0f;
	mPostFxQuadVerts[7] = 1.0f;
	mPostFxQuadVerts[8] = 0.0f;
	mPostFxQuadVerts[9] = -1.0f;

	mPostFxQuadVerts[10] = 0.0f;
	mPostFxQuadVerts[11] = 1.0f;
	mPostFxQuadVerts[12] = 0.0f;
	mPostFxQuadVerts[13] = 1.0f;
	mPostFxQuadVerts[14] = -1.0f;

	mPostFxQuadVerts[15] = 1.0f;
	mPostFxQuadVerts[16] = 1.0f;
	mPostFxQuadVerts[17] = 1.0f;
	mPostFxQuadVerts[18] = 1.0f;
	mPostFxQuadVerts[19] = -1.0f;

	mShotCounter = 0;

	for(unsigned int i=0; i<MAX_UNIFORM_LOCATIONS; i++)
		mShaderLocs[i] = -1;

	// Initialize GLFW
	if( !glfwInit() )
	{
		mTerminate = true;
		return;
	}


    NetworkManager::gCond = createCondition();

    if( !SGCTMutexManager::Instance()->isValid() || NetworkManager::gCond == NULL )
    {
		mTerminate = true;
		return;
	}

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

	mExitKey = GLFW_KEY_ESC;

	mVBO[RenderQuad]	= 0; //default to openGL false
	mVBO[FishEyeQuad]	= 0; //default to openGL false
	mVAO[RenderQuad]	= 0;
	mVAO[FishEyeQuad]	= 0;
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

	MessageHandler::Instance()->print("%s\n", getSGCTVersion().c_str() );

	if(mTerminate)
	{
		MessageHandler::Instance()->print("Failed to init GLFW.\n");
		return false;
	}

	mConfig = new ReadConfig( configFilename );
	if( !mConfig->isValid() ) //fatal error
	{
		MessageHandler::Instance()->print("Error in xml config file parsing.\n");
		return false;
	}

	if( !initNetwork() )
	{
		MessageHandler::Instance()->print("Network init error.\n");
		return false;
	}

	if( !initWindow() )
	{
		MessageHandler::Instance()->print("Window init error.\n");
		return false;
	}

	//if a single node, skip syncing
	if(ClusterManager::Instance()->getNumberOfNodes() == 1)
		mIgnoreSync = true;

	if( mKeyboardCallbackFn != NULL )
        glfwSetKeyCallback( mKeyboardCallbackFn );
	if( mMouseButtonCallbackFn != NULL )
		glfwSetMouseButtonCallback( mMouseButtonCallbackFn );
	if( mMousePosCallbackFn != NULL )
		glfwSetMousePosCallback( mMousePosCallbackFn );
	if( mCharCallbackFn != NULL )
		glfwSetCharCallback( mCharCallbackFn );
	if( mMouseWheelCallbackFn != NULL )
		glfwSetMouseWheelCallback( mMouseWheelCallbackFn );

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
		sgct::MessageHandler::Instance()->print("Initiating network connections failed! Error: '%s'\n", err);
		return false;
	}

	//check in cluster configuration which it is
	if( localRunningMode == NetworkManager::NotLocal )
		for(unsigned int i=0; i<ClusterManager::Instance()->getNumberOfNodes(); i++)
			if( mNetworkConnections->matchAddress( ClusterManager::Instance()->getNodePtr(i)->ip ) )
			{
				ClusterManager::Instance()->setThisNodeId(i);
				break;
			}

	if( ClusterManager::Instance()->getThisNodeId() == -1 ||
		ClusterManager::Instance()->getThisNodeId() >= static_cast<int>( ClusterManager::Instance()->getNumberOfNodes() )) //fatal error
	{
		sgct::MessageHandler::Instance()->print("This computer is not a part of the cluster configuration!\n");
		mNetworkConnections->close();
		return false;
	}
	else
	{
		printNodeInfo( static_cast<unsigned int>(ClusterManager::Instance()->getThisNodeId()) );
	}

	//Set message handler to send messages or not
	sgct::MessageHandler::Instance()->setSendFeedbackToServer( !mNetworkConnections->isComputerServer() );

	if(!mNetworkConnections->init())
		return false;

    sgct::MessageHandler::Instance()->print("Done\n");
	return true;
}

/*!
Create and initiate a window.
*/
bool sgct::Engine::initWindow()
{
	int tmpGlfwVer[3];
    glfwGetVersion( &tmpGlfwVer[0], &tmpGlfwVer[1], &tmpGlfwVer[2] );
	sgct::MessageHandler::Instance()->print("Using GLFW version %d.%d.%d.\n",
                                         tmpGlfwVer[0],
                                         tmpGlfwVer[1],
                                         tmpGlfwVer[2]);

	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();
	getWindowPtr()->useQuadbuffer( tmpNode->stereo == ClusterManager::Active );

	//disable MSAA if FXAA is in use and fisheye is not enabled
	/*
		Fisheye can use MSAA for cubemap rendering and FXAA for screen space rendering
		It's a bit overkill but usefull for rendering fisheye movies
	*/
	if( SGCTSettings::Instance()->useFXAA() && !tmpNode->isUsingFisheyeRendering())
	{
		tmpNode->numberOfSamples = 1;
	}

	//if using fisheye rendering for a dome display
	if( tmpNode->isUsingFisheyeRendering() )
	{
		if( SGCTSettings::Instance()->getFBOMode() == SGCTSettings::NoFBO )
		{
			MessageHandler::Instance()->print("Engine: Forcing FBOs for fisheye mode!\n");
			SGCTSettings::Instance()->setFBOMode( SGCTSettings::RegularFBO );
		}
		
		mClearColor[3] = SGCTSettings::Instance()->useFisheyeAlpha() ? 0.0f : 1.0f;

		//create the cube mapped viewports
		tmpNode->generateCubeMapViewports();
	}
	
	int antiAliasingSamples = tmpNode->numberOfSamples;
	if( antiAliasingSamples > 1 && SGCTSettings::Instance()->getFBOMode() == SGCTSettings::NoFBO ) //if multisample is used
		glfwOpenWindowHint( GLFW_FSAA_SAMPLES, antiAliasingSamples );
	else if( antiAliasingSamples < 2 && SGCTSettings::Instance()->getFBOMode() == SGCTSettings::MultiSampledFBO ) //on sample or less => no multisampling
		SGCTSettings::Instance()->setFBOMode( SGCTSettings::RegularFBO );

	switch( mRunMode )
	{
	case OpenGL_3_3_Core_Profile:
		{
			glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
			glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
			glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glewExperimental = true; // Needed for core profile
		}
		break;
	}

	/*
	//OSX ogl 3.2 code
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);
    glfwOpenWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	*/

	/*
	//win & linux code
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3); // We want OpenGL 3.3
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
	glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE); //We don't want the old OpenGL
	*/

    if( !getWindowPtr()->openWindow() )
		return false;

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
	  //Problem: glewInit failed, something is seriously wrong.
	  MessageHandler::Instance()->print("Error: %s.\n", glewGetErrorString(err));
	  return false;
	}
	MessageHandler::Instance()->print("Using GLEW %s.\n", glewGetString(GLEW_VERSION));

	if( !checkForOGLErrors() )
		MessageHandler::Instance()->print("GLEW init triggered an OpenGL error.\n");

    /*
        Swap inerval:
        -1 = adaptive sync
        0  = vertical sync off
        1  = wait for vertical sync
        2  = fix when using swapgroups in xp and running half the framerate
    */

    glfwSwapInterval( tmpNode->swapInterval );

	getWindowPtr()->init();

	getWindowPtr()->setWindowTitle( getBasicInfo() );

	waitForAllWindowsInSwapGroupToOpen();

	//init swap group if enabled
	getWindowPtr()->initNvidiaSwapGroups();

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
	glfwGetGLVersion( &mOpenGL_Version[0], &mOpenGL_Version[1], &mOpenGL_Version[2] );
	sgct::MessageHandler::Instance()->print("OpenGL version %d.%d.%d %s\n", mOpenGL_Version[0], mOpenGL_Version[1], mOpenGL_Version[2],
		mFixedOGLPipeline ? "comp. profile" : "core profile");

	if( !GLEW_EXT_framebuffer_object && mOpenGL_Version[0] < 2)
	{
		sgct::MessageHandler::Instance()->print("Warning! Frame buffer objects are not supported! A lot of features in SGCT will not work!\n");
		SGCTSettings::Instance()->setFBOMode( SGCTSettings::NoFBO );
	}
	else if(!GLEW_EXT_framebuffer_multisample && mOpenGL_Version[0] < 2)
	{
		sgct::MessageHandler::Instance()->print("Warning! FBO multisampling is not supported!\n");
		SGCTSettings::Instance()->setFBOMode( SGCTSettings::RegularFBO );
	}

	createTextures();

	mFinalFBO_Ptr = new OffScreenBuffer();
	mCubeMapFBO_Ptr = new OffScreenBuffer();

	createVBOs(); //must be created before FBO
	createFBOs();
	loadShaders();
	mStatistics.initVBO();

	//load overlays if any
	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();
	for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
		tmpNode->getViewport(i)->loadData();

	//init PBO in screen capture
	mScreenCapture.init();
	char nodeName[MAX_SGCT_PATH_LENGTH];
	#if (_MSC_VER >= 1400) //visual studio 2005 or later
		sprintf_s( nodeName, MAX_SGCT_PATH_LENGTH, "_node%d",
			ClusterManager::Instance()->getThisNodeId());

		sprintf_s( nodeName, MAX_SGCT_PATH_LENGTH, "_node%d",
			ClusterManager::Instance()->getThisNodeId());

		sprintf_s( nodeName, MAX_SGCT_PATH_LENGTH, "_node%d",
			ClusterManager::Instance()->getThisNodeId());
	#else
		sprintf( nodeName, "_node%d",
			ClusterManager::Instance()->getThisNodeId());

		sprintf( nodeName, "_node%d",
			ClusterManager::Instance()->getThisNodeId());

		sprintf( nodeName, "_node%d",
			ClusterManager::Instance()->getThisNodeId());
    #endif

	SGCTSettings::Instance()->appendCapturePath( std::string(nodeName), SGCTSettings::Mono );
	SGCTSettings::Instance()->appendCapturePath( std::string(nodeName), SGCTSettings::LeftStereo );
	SGCTSettings::Instance()->appendCapturePath( std::string(nodeName), SGCTSettings::RightStereo );
	
	mScreenCapture.setUsePBO( GLEW_EXT_pixel_buffer_object && mOpenGL_Version[0] > 1 ); //if supported then use them
	if( isFisheye() && !SGCTSettings::Instance()->useFisheyeAlpha())
		mScreenCapture.initOrResize( getWindowPtr()->getXFramebufferResolution(), getWindowPtr()->getYFramebufferResolution(), 3 );
	else
		mScreenCapture.initOrResize( getWindowPtr()->getXFramebufferResolution(), getWindowPtr()->getYFramebufferResolution(), 4 );
	if( SGCTSettings::Instance()->getCaptureFormat() != ScreenCapture::NOT_SET )
		mScreenCapture.setFormat( static_cast<ScreenCapture::CaptureFormat>(SGCTSettings::Instance()->getCaptureFormat()) );

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
	sgct::MessageHandler::Instance()->print("Joining swap barrier if enabled...\n");
	getWindowPtr()->setBarrier(true);
	sgct::MessageHandler::Instance()->print("Reseting swap group frame number...\n");
	getWindowPtr()->resetSwapGroupFrameNumber();

	sgct::MessageHandler::Instance()->print("\nReady to render!\n");
}

/*!
Clean up all resources and release memory.
*/
void sgct::Engine::clean()
{
	sgct::MessageHandler::Instance()->print("Cleaning up...\n");

	if( mCleanUpFn != NULL )
		mCleanUpFn();

	sgct::MessageHandler::Instance()->print("Clearing all callbacks...\n");
	clearAllCallbacks();

	//delete FBO stuff
	if(mFinalFBO_Ptr != NULL &&
		mCubeMapFBO_Ptr != NULL &&
		SGCTSettings::Instance()->getFBOMode() != SGCTSettings::NoFBO)
	{
		sgct::MessageHandler::Instance()->print("Releasing OpenGL buffers...\n");
		mFinalFBO_Ptr->destroy();
		if( ClusterManager::Instance()->getThisNodePtr()->isUsingFisheyeRendering() )
			mCubeMapFBO_Ptr->destroy();

		delete mFinalFBO_Ptr;
		mFinalFBO_Ptr = NULL;
		delete mCubeMapFBO_Ptr;
		mCubeMapFBO_Ptr = NULL;

		glDeleteTextures(4,	&mFrameBufferTextures[0]);
	}

	sgct::ShaderManager::Destroy();

	//de-init window and unbind swapgroups...
	if(ClusterManager::Instance()->getNumberOfNodes() > 0)
	{
		SGCTNode * nPtr = ClusterManager::Instance()->getThisNodePtr();
		if(nPtr != NULL && nPtr->getWindowPtr() != NULL) //make shure to not use destroyed object
			nPtr->getWindowPtr()->close();
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
	MessageHandler::Instance()->print("Destroying font manager...\n");
	sgct_text::FontManager::Destroy();
	MessageHandler::Instance()->print("Destroying shader manager...\n");
	ShaderManager::Destroy();
	MessageHandler::Instance()->print("Destroying shared data...\n");
	SharedData::Destroy();
	MessageHandler::Instance()->print("Destroying texture manager...\n");
	TextureManager::Destroy();
	MessageHandler::Instance()->print("Destroying cluster manager...\n");
	ClusterManager::Destroy();
	MessageHandler::Instance()->print("Destroying settings...\n");
	SGCTSettings::Destroy();

    MessageHandler::Instance()->print("Destroying mutexes...\n");
	SGCTMutexManager::Destroy();

	sgct::MessageHandler::Instance()->print("Destroying condition...\n");
	if( NetworkManager::gCond != NULL )
	{
		destroyCond( NetworkManager::gCond );
		NetworkManager::gCond = NULL;
	}

	sgct::MessageHandler::Instance()->print("Destroying message handler...\n");
	MessageHandler::Destroy();

	if( mVBO[RenderQuad] )
	{
		sgct::MessageHandler::Instance()->print("Deleting VBOs...\n");
		glDeleteBuffers(NUMBER_OF_VBOS, &mVBO[0]);
	}

	if( mVAO[RenderQuad] )
	{
		sgct::MessageHandler::Instance()->print("Deleting VAOs...\n");
		glDeleteVertexArrays(NUMBER_OF_VBOS, &mVAO[0]);
	}

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
	glfwSetKeyCallback( NULL );
	glfwSetMouseButtonCallback( NULL );
	glfwSetMousePosCallback( NULL );
	glfwSetCharCallback( NULL );
	glfwSetMouseWheelCallback( NULL );

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
	mKeyboardCallbackFn = NULL;
	mCharCallbackFn = NULL;
	mMouseButtonCallbackFn = NULL;
	mMousePosCallbackFn = NULL;
	mMouseWheelCallbackFn = NULL;

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
		mNetworkConnections->sync(NetworkManager::SendDataToClients, &mStatistics); //from server to clients
		mStatistics.setSyncTime(glfwGetTime() - t0);

		//run only on clients/slaves
		if( !mIgnoreSync && !mNetworkConnections->isComputerServer() ) //not server
		{
			t0 = glfwGetTime();
			while(mNetworkConnections->isRunning() && mRunning)
			{
				if( mNetworkConnections->isSyncComplete() )
						break;

				if(USE_SLEEP_TO_WAIT_FOR_NODES)
					glfwSleep(0.001);
				else
				{
					SGCTMutexManager::Instance()->lockMutex( SGCTMutexManager::SyncMutex );
					//release lock once per second
					glfwWaitCond( NetworkManager::gCond,
						SGCTMutexManager::Instance()->getMutex( SGCTMutexManager::SyncMutex ),
						1.0 );
					SGCTMutexManager::Instance()->unlockMutex( SGCTMutexManager::SyncMutex );
				}

				//for debuging
				if( glfwGetTime() - t0 > 1.0 )
				{
					for(unsigned int i=0; i<mNetworkConnections->getConnectionsCount(); i++)
					{
						if( !mNetworkConnections->getConnection(i)->isUpdated() )
						{
							MessageHandler::Instance()->print("Waiting for master... send frame %d, recv frame %d\n",
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
			mNetworkConnections->sync(NetworkManager::AcknowledgeData, &mStatistics);

			mStatistics.addSyncTime(glfwGetTime() - t0);
		}//end if client
	}
	else //post stage
	{
		if( !mIgnoreSync && mNetworkConnections->isComputerServer() )//&&
			//mConfig->isMasterSyncLocked() &&
			/*localRunningMode == NetworkManager::NotLocal &&*/
			//!getWindowPtr()->isBarrierActive() )//post stage
		{
			double t0 = glfwGetTime();
			while(mNetworkConnections->isRunning() &&
				mRunning &&
				mNetworkConnections->getConnectionsCount() > 0)
			{
				if( mNetworkConnections->isSyncComplete() )
						break;

				if(USE_SLEEP_TO_WAIT_FOR_NODES)
					glfwSleep(0.001);
				else
				{
					SGCTMutexManager::Instance()->lockMutex( SGCTMutexManager::SyncMutex );
					//release lock once per second
					glfwWaitCond( NetworkManager::gCond,
						SGCTMutexManager::Instance()->getMutex( SGCTMutexManager::SyncMutex ),
						1.0 );
					SGCTMutexManager::Instance()->unlockMutex( SGCTMutexManager::SyncMutex );
				}

				//for debuging
				if( glfwGetTime() - t0 > 1.0 )
				{
					for(unsigned int i=0; i<mNetworkConnections->getConnectionsCount(); i++)
					{
						if( !mNetworkConnections->getConnection(i)->isUpdated() )
						{
							MessageHandler::Instance()->print("Waiting for slave at connection %d: send frame %d != recv frame %d\n", i,
								mNetworkConnections->getConnection(i)->getSendFrame(),
								mNetworkConnections->getConnection(i)->getRecvFrame(SGCTNetwork::Current));
						}
					}
				}
			}//end while
			mStatistics.addSyncTime(glfwGetTime() - t0);
		}//end if server
	}
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

		//update tracking data
		if( isMaster() )
			ClusterManager::Instance()->getTrackingManagerPtr()->updateTrackingDevices();

		if( mPreSyncFn != NULL )
			mPreSyncFn();

		if( mNetworkConnections->isComputerServer() )
		{
			sgct::SharedData::Instance()->encode();
		}
		else
		{
			if( !mNetworkConnections->isRunning() ) //exit if not running
				break;
		}

		frameSyncAndLock(PreStage);

		if( mPostSyncPreDrawFn != NULL )
			mPostSyncPreDrawFn();

		double startFrameTime = glfwGetTime();
		calculateFPS(startFrameTime); //measures time between calls

		SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();
		SGCTUser * usrPtr = ClusterManager::Instance()->getUserPtr();

		//check if re-size needed
		if( getWindowPtr()->isWindowResized() )
		{
			resizeFBOs();
			(tmpNode->isUsingFisheyeRendering() && !SGCTSettings::Instance()->useFisheyeAlpha()) ?
				mScreenCapture.initOrResize( getWindowPtr()->getXFramebufferResolution(), getWindowPtr()->getYFramebufferResolution(), 3 ) :
				mScreenCapture.initOrResize( getWindowPtr()->getXFramebufferResolution(), getWindowPtr()->getYFramebufferResolution(), 4 );
		}

		//rendering offscreen if using FBOs
		mRenderingOffScreen = (SGCTSettings::Instance()->getFBOMode() != SGCTSettings::NoFBO);

		//if fisheye rendering is used then render the cubemap
		if( tmpNode->isUsingFisheyeRendering() )
		{
			mActiveFrustum = tmpNode->stereo != static_cast<int>(ClusterManager::NoStereo) ? Frustum::StereoLeftEye : Frustum::Mono;
			(this->*mInternalRenderFisheyeFn)(LeftEye);

			if( SGCTSettings::Instance()->usePostFX() )
				renderPostFx(LeftEye);

			if( tmpNode->stereo != ClusterManager::NoStereo )
			{
				mActiveFrustum = Frustum::StereoRightEye;
				(this->*mInternalRenderFisheyeFn)(RightEye);

				if( SGCTSettings::Instance()->usePostFX() )
					renderPostFx(RightEye);
			}
		}
		else //regular viewport rendering
		{
			//if any stereo type (except passive) then set frustum mode to left eye
			mActiveFrustum = tmpNode->stereo != static_cast<int>(ClusterManager::NoStereo) ? Frustum::StereoLeftEye : Frustum::Mono;
			setRenderTarget(LeftEye); //Set correct render target (Backbuffer, FBO etc..)

			//render all viewports for mono or left eye
			for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
			{
				tmpNode->setCurrentViewport(i);

				if( tmpNode->getCurrentViewport()->isEnabled() )
				{
					//if passive stereo or mono
					if( tmpNode->stereo == ClusterManager::NoStereo )
						mActiveFrustum = tmpNode->getCurrentViewport()->getEye();

					if( tmpNode->getCurrentViewport()->isTracked() )
					{
						tmpNode->getCurrentViewport()->calculateFrustum(
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
			if( tmpNode->stereo != ClusterManager::NoStereo )
			{
				mActiveFrustum = Frustum::StereoRightEye;
				setRenderTarget(RightEye); //Set correct render target (Backbuffer, FBO etc..)

				//render all viewports for right eye
				for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
				{
					tmpNode->setCurrentViewport(i);

					if( tmpNode->getCurrentViewport()->isEnabled() )
					{
						if( tmpNode->getCurrentViewport()->isTracked() )
						{
							tmpNode->getCurrentViewport()->calculateFrustum(
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

		mRenderingOffScreen = false;
		if(SGCTSettings::Instance()->getFBOMode() != SGCTSettings::NoFBO)
			(this->*mInternalRenderFBOFn)();

		//draw viewport overlays if any
		(this->*mInternalDrawOverlaysFn)();

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

		//take screenshot
		if( mTakeScreenshot )
			captureBuffer();

#ifdef __SGCT_DEBUG__
		//check for errors
		checkForOGLErrors();
#endif

		//draw info & stats
		//the cubemap viewports are all the same so it makes no sense to render everything several times
		//therefore just loop one iteration in that case.
		std::size_t numberOfIterations = ( tmpNode->isUsingFisheyeRendering() ? 1 : tmpNode->getNumberOfViewports() );
		for(std::size_t i=0; i < numberOfIterations; i++)
		{
			tmpNode->setCurrentViewport(i);
			enterCurrentViewport(ScreenSpace);

			if( mShowGraph )
				mStatistics.draw(mFrameCounter);
			/*
				The text renderer enters automatically the correct viewport
			*/
			if( mShowInfo )
				renderDisplayInfo();
		}

        //glFlush();
        //glFinish();
		double endFrameTime = glfwGetTime();
		mStatistics.setDrawTime(endFrameTime - startFrameTime);
        updateTimers( endFrameTime );

		//swap window size values
		getWindowPtr()->swap();

		//master will wait for nodes render before swapping
		frameSyncAndLock(PostStage);

		// Swap front and back rendering buffers
		glfwSwapBuffers();

		// Check if ESC key was pressed or window was closed
		mRunning = !glfwGetKey( mExitKey ) && glfwGetWindowParam( GLFW_OPENED ) && !mTerminate;

		mFrameCounter++;
	}
}

/*!
	This function renders basic text info and statistics on screen.
*/
void sgct::Engine::renderDisplayInfo()
{
	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

	glPushAttrib(GL_CURRENT_BIT);
	glColor4f(0.8f,0.8f,0.8f,1.0f);
	unsigned int lFrameNumber = 0;
	getWindowPtr()->getSwapGroupFrameNumber(lFrameNumber);

	glDrawBuffer(GL_BACK); //draw into both back buffers
	glReadBuffer(GL_BACK);

	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 110, "Node ip: %s (%s)",
		tmpNode->ip.c_str(),
		mNetworkConnections->isComputerServer() ? "master" : "slave");
	glColor4f(0.8f,0.8f,0.0f,1.0f);
	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 95, "Frame rate: %.3f Hz, frame: %u", mStatistics.getAvgFPS(), mFrameCounter);
	glColor4f(0.8f,0.0f,0.8f,1.0f);
	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 80, "Avg. draw time: %.2f ms", mStatistics.getAvgDrawTime()*1000.0);
	glColor4f(0.0f,0.8f,0.8f,1.0f);
	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 65, "Avg. sync time (size: %d, comp. ratio: %.3f): %.2f ms",
		SharedData::Instance()->getUserDataSize(),
		SharedData::Instance()->getCompressionRatio(),
		mStatistics.getAvgSyncTime()*1000.0);
	glColor4f(0.8f,0.8f,0.8f,1.0f);

	/*
		Get memory info from nvidia and ati/amd cards.
	*/
	static int tot_mem = 0;
	int av_mem = 0;
	if( GLEW_NVX_gpu_memory_info )
	{
		glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &tot_mem);
		glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &av_mem);
	}
	else if( GLEW_ATI_meminfo )
	{
#ifdef __WIN32__
        unsigned int tmpui = 0;
		unsigned int gpu_ids[] = {0, 0, 0, 0, 0, 0, 0, 0};
		if( tot_mem == 0 && wglewIsSupported("WGL_AMD_gpu_association") && wglGetGPUIDsAMD(8, gpu_ids) != 0 )
		{
			MessageHandler::Instance()->print("Polling AMD GPU info...\n");
			if( gpu_ids[0] != 0 && wglGetGPUInfoAMD(gpu_ids[0], WGL_GPU_RAM_AMD, GL_UNSIGNED_INT, sizeof(unsigned int), &tmpui) != -1 )
				tot_mem = static_cast<int>( tmpui ) * 1024;
		}
#elif defined __APPLE__
		//might get availible with GLEW 1.9.1: http://ehc.ac/p/glew/bugs/202/
#else //Linux
        //might get availible with GLEW 1.9.1: http://ehc.ac/p/glew/bugs/202/

		/*unsigned int tmpui = 0;
		unsigned int gpu_ids[] = {0, 0, 0, 0, 0, 0, 0, 0};
		if( tot_mem == 0 && glewIsSupported("GLX_AMD_gpu_association") && glXGetGPUIDsAMD(8, gpu_ids) != 0 )
		{
			MessageHandler::Instance()->print("Polling AMD GPU info...\n");
			if( gpu_ids[0] != 0 && glXGetGPUInfoAMD(gpu_ids[0], GLX_GPU_RAM_AMD, GL_UNSIGNED_INT, sizeof(unsigned int), &tmpui) != -1 )
				tot_mem = static_cast<int>( tmpui ) * 1024;
		}*/
#endif

		int mem[] = {0, 0, 0, 0};
		glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, mem);
		av_mem = mem[0];
	}

	if( av_mem == 0 || tot_mem == 0 )
		sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 50, "Available memory: na\n");
	else
	{
#ifdef __APPLE__
	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 50, "Available memory: %d (MB)",
		av_mem/1024);
#else
	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 50, "Memory usage: %d %%, %d of %d (MB)",
		tot_mem > 0 ? (100*(tot_mem-av_mem))/tot_mem : 0, //if not supported card, prevent div by zero
		(tot_mem-av_mem)/1024,
		tot_mem/1024);
#endif
	}

	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 35, "Swap groups: %s and %s (%s) | Frame: %d",
		getWindowPtr()->isUsingSwapGroups() ? "Enabled" : "Disabled",
		getWindowPtr()->isBarrierActive() ? "active" : "not active",
		getWindowPtr()->isSwapGroupMaster() ? "master" : "slave",
		lFrameNumber);

	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 20, "Tracked: %s | User position: %.3f %.3f %.3f",
		tmpNode->getCurrentViewport()->isTracked() ? "true" : "false",
		getUserPtr()->getXPos(),
		getUserPtr()->getYPos(),
		getUserPtr()->getZPos());

	//if active stereoscopic rendering
	if( tmpNode->stereo == ClusterManager::Active )
	{
		glDrawBuffer(GL_BACK_LEFT);
		glReadBuffer(GL_BACK_LEFT);
		sgct_text::print( sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 125, "Active eye: Left");
		glDrawBuffer(GL_BACK_RIGHT);
		glReadBuffer(GL_BACK_RIGHT);
		sgct_text::print( sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 125, "Active eye:          Right");
		glDrawBuffer(GL_BACK);
		glReadBuffer(GL_BACK);
	}
	else //if passive stereo
	{
		if( tmpNode->getCurrentViewport()->getEye() == Frustum::StereoLeftEye )
		{
			sgct_text::print( sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 125, "Active eye: Left");
		}
		else if( tmpNode->getCurrentViewport()->getEye() == Frustum::StereoRightEye )
		{
			sgct_text::print( sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 125, "Active eye:          Right");
		}
	}
	glPopAttrib();
}

/*!
	This function enters the correct viewport, frustum, stereo mode and calls the draw callback.
*/
void sgct::Engine::draw()
{
	enterCurrentViewport(FBOSpace);

	Viewport * tmpVP = ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport();

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

	Viewport * tmpVP = ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport();
	glLoadMatrixf( glm::value_ptr(tmpVP->getViewProjectionMatrix(mActiveFrustum)) );

	glMatrixMode(GL_MODELVIEW);

	glLoadMatrixf( glm::value_ptr( getModelMatrix() ) );

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

	sgct_core::SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

	std::size_t numberOfIterations = ( tmpNode->isUsingFisheyeRendering() ? 1 : tmpNode->getNumberOfViewports() );
	for(std::size_t i=0; i < numberOfIterations; i++)
	{
		tmpNode->setCurrentViewport(i);

		//if viewport has overlay
		sgct_core::Viewport * tmpVP = ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport();
		if( tmpVP->hasOverlayTexture() )
		{
			/*
				Some code (using OpenSceneGraph) can mess up the viewport settings.
				To ensure correct mapping enter the current viewport.
			*/
			enterCurrentViewport(ScreenSpace);

			//enter ortho mode
			glm::mat4 orthoMat;
			if( tmpNode->isUsingFisheyeRendering() )
				orthoMat = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
			else
				orthoMat = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);

			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glDisable(GL_CULL_FACE);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByHandle( tmpVP->getOverlayTextureIndex() ) );

			mShaders[OverlayShader].use();

			glUniform1i( mShaderLocs[OverlayTex], 0);
			glUniformMatrix4fv( mShaderLocs[OverlayMVP], 1, GL_FALSE, &orthoMat[0][0]);

			if( tmpNode->isUsingFisheyeRendering() )
			{
				glBindVertexArray( mVAO[FishEyeQuad] );
				glBindBuffer(GL_ARRAY_BUFFER, mVBO[FishEyeQuad]);
			}
			else
			{
				glBindVertexArray( mVAO[RenderQuad] );
				glBindBuffer(GL_ARRAY_BUFFER, mVBO[RenderQuad]);
			}

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

			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			 //unbind
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			sgct::ShaderManager::Instance()->unBindShader();
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

	sgct_core::SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

	std::size_t numberOfIterations = ( tmpNode->isUsingFisheyeRendering() ? 1 : tmpNode->getNumberOfViewports() );
	for(std::size_t i=0; i < numberOfIterations; i++)
	{
		tmpNode->setCurrentViewport(i);

		//if viewport has overlay
		sgct_core::Viewport * tmpVP = ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport();
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

			if( tmpNode->isUsingFisheyeRendering() )
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
			glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByHandle( tmpVP->getOverlayTextureIndex() ) );

			glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

			if( tmpNode->isUsingFisheyeRendering() )
				glBindBuffer(GL_ARRAY_BUFFER, mVBO[FishEyeQuad]);
			else
				glBindBuffer(GL_ARRAY_BUFFER, mVBO[RenderQuad]);

			glClientActiveTexture(GL_TEXTURE0);

			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(0));

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(8));
			
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind

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
	if(SGCTSettings::Instance()->getFBOMode() != SGCTSettings::NoFBO)
	{
		if( SGCTSettings::Instance()->usePostFX() )
			ti = PostFX;

		//un-bind texture
		glBindTexture(GL_TEXTURE_2D, 0);

		mFinalFBO_Ptr->bind();
		if( !mFinalFBO_Ptr->isMultiSampled() )
		{
			//update attachments
			mFinalFBO_Ptr->attachColorTexture( mFrameBufferTextures[ti] );
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
	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

	//unbind framebuffer
	OffScreenBuffer::unBind();

	//enter ortho mode
	glm::mat4 orthoMat = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	
	//clear buffers
	mActiveFrustum = tmpNode->stereo == ClusterManager::Active ? Frustum::StereoLeftEye : Frustum::Mono;
	setAndClearBuffer(BackBufferBlack);

	glViewport (0, 0, getWindowPtr()->getXResolution(), getWindowPtr()->getYResolution());

	if( tmpNode->stereo > ClusterManager::Active )
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[LeftEye]);
		
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[RightEye]);

		mShaders[StereoShader].use();

		glUniform1i( mShaderLocs[StereoLeftTex], 0);
		glUniform1i( mShaderLocs[StereoRightTex], 1);
		glUniformMatrix4fv( mShaderLocs[StereoMVP], 1, GL_FALSE, &orthoMat[0][0]);
		
		for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
			tmpNode->getViewport(i)->renderMesh();
		sgct::ShaderManager::Instance()->unBindShader();
	}
	else
	{	
		glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
		glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[LeftEye]);

		mShaders[FBOQuadShader].use(); //bind
		glUniform1i( mShaderLocs[MonoTex], 0);
		glUniformMatrix4fv( mShaderLocs[MonoMVP], 1, GL_FALSE, &orthoMat[0][0]);

		for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
			tmpNode->getViewport(i)->renderMesh();

		//render right eye in active stereo mode
		if( tmpNode->stereo == ClusterManager::Active )
		{
			//clear buffers
			mActiveFrustum = Frustum::StereoRightEye;
			setAndClearBuffer(BackBufferBlack);

			glViewport (0, 0, getWindowPtr()->getXResolution(), getWindowPtr()->getYResolution());

			glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[RightEye]);
			glUniform1i( mShaderLocs[MonoTex], 0);

			for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
				tmpNode->getViewport(i)->renderMesh();
		}

		sgct::ShaderManager::Instance()->unBindShader();
	}
}


/*!
	Draw geometry and bind FBO as texture in screenspace (ortho mode).
	The geometry can be a simple quad or a geometry correction and blending mesh.
*/
void sgct::Engine::renderFBOTextureFixedPipeline()
{
	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

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
	mActiveFrustum = tmpNode->stereo == ClusterManager::Active ? Frustum::StereoLeftEye : Frustum::Mono;
	setAndClearBuffer(BackBufferBlack);

	glLoadIdentity();

	glViewport (0, 0, getWindowPtr()->getXResolution(), getWindowPtr()->getYResolution());

	if( tmpNode->stereo > ClusterManager::Active )
	{
		mShaders[StereoShader].use();

		glUniform1i( mShaderLocs[StereoLeftTex], 0);
		glUniform1i( mShaderLocs[StereoRightTex], 1);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[LeftEye]);
		glEnable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[RightEye]);
		glEnable(GL_TEXTURE_2D);

		for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
			tmpNode->getViewport(i)->renderMesh();
		sgct::ShaderManager::Instance()->unBindShader();
	}
	else
	{
		glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
		glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[LeftEye]);
		glEnable(GL_TEXTURE_2D);

		for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
			tmpNode->getViewport(i)->renderMesh();

		//render right eye in active stereo mode
		if( tmpNode->stereo == ClusterManager::Active )
		{
			//clear buffers
			mActiveFrustum = Frustum::StereoRightEye;
			setAndClearBuffer(BackBufferBlack);

			glLoadIdentity();

			glViewport (0, 0, getWindowPtr()->getXResolution(), getWindowPtr()->getYResolution());

			glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[RightEye]);

			for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
				tmpNode->getViewport(i)->renderMesh();
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

	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

	SGCTSettings * setPtr = SGCTSettings::Instance();

	if( mActiveFrustum == Frustum::StereoLeftEye )
		setPtr->setFisheyeOffset( -getUserPtr()->getEyeSeparation() / setPtr->getDomeDiameter(), 0.0f);
	else if( mActiveFrustum == Frustum::StereoRightEye )
		setPtr->setFisheyeOffset( getUserPtr()->getEyeSeparation() / setPtr->getDomeDiameter(), 0.0f);

	//iterate the cube sides
	for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
	{
		tmpNode->setCurrentViewport(i);

		if( tmpNode->getCurrentViewport()->isEnabled() )
		{
			//un-bind texture
			glBindTexture(GL_TEXTURE_2D, 0);
			
			//bind & attach buffer
			mCubeMapFBO_Ptr->bind(); //osg seems to unbind FBO when rendering with osg FBO cameras
			if( !mCubeMapFBO_Ptr->isMultiSampled() )
			{								
				mCubeMapFBO_Ptr->attachCubeMapTexture( mFrameBufferTextures[FishEye], i );
			}

			setAndClearBuffer(RenderToTexture);

			//render
			(this->*mInternalDrawFn)();

			//copy AA-buffer to "regular"/non-AA buffer
			if( mCubeMapFBO_Ptr->isMultiSampled() )
			{
				mCubeMapFBO_Ptr->bindBlit(); //bind separate read and draw buffers to prepare blit operation

				//update attachments
				mCubeMapFBO_Ptr->attachCubeMapTexture( mFrameBufferTextures[FishEye], i );

				mCubeMapFBO_Ptr->blit();
			}
		}
	}//end for

	//restore polygon mode
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	//bind fisheye target FBO
	mFinalFBO_Ptr->bind();
	SGCTSettings::Instance()->usePostFX() ? 
			mFinalFBO_Ptr->attachColorTexture( mFrameBufferTextures[PostFX] ) :
			mFinalFBO_Ptr->attachColorTexture( mFrameBufferTextures[ti] );

	glClearColor(mFisheyeClearColor[0], mFisheyeClearColor[1], mFisheyeClearColor[2], mFisheyeClearColor[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/*
		The code below flips the viewport vertically. Top & bottom coords are flipped.
	*/
	glm::mat4 orthoMat = glm::ortho( -1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 2.0f );

	glViewport(0, 0, getWindowPtr()->getXFramebufferResolution(), getWindowPtr()->getYFramebufferResolution());

	//if for some reson the active texture has been reset
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, mFrameBufferTextures[FishEye]);

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	mShaders[FisheyeShader].use();

	glUniformMatrix4fv( mShaderLocs[FisheyeMVP], 1, GL_FALSE, &orthoMat[0][0]);
	glUniform1i( mShaderLocs[Cubemap], 0);
	glUniform1f( mShaderLocs[FishEyeHalfFov], glm::radians<float>(SGCTSettings::Instance()->getFisheyeFOV()/2.0f) );
	if( setPtr->isFisheyeOffaxis() )
	{
		glUniform3f( mShaderLocs[FisheyeOffset], setPtr->getFisheyeOffset(0), setPtr->getFisheyeOffset(1), setPtr->getFisheyeOffset(2) );
	}

	glBindVertexArray( mVAO[FishEyeQuad] );
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[FishEyeQuad]);
	
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

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		//unbind
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	sgct::ShaderManager::Instance()->unBindShader();

	if(mTakeScreenshot)
	{
		int size = getWindowPtr()->getXFramebufferResolution();
		unsigned int fontSize = 0;

		if( size > 512 )
			fontSize = static_cast<unsigned int>(size)/96;
		else if( size == 512 )
			fontSize = 8;

		if( fontSize != 0 )
		{
			float x = static_cast<float>( size - size/4 );
			float y = static_cast<float>( fontSize );
			
			sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), x, 2.0f * y + y/5.0f, "Frame#: %d", mShotCounter);

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

	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

	SGCTSettings * setPtr = SGCTSettings::Instance();

	if( mActiveFrustum == Frustum::StereoLeftEye )
		setPtr->setFisheyeOffset( -getUserPtr()->getEyeSeparation() / setPtr->getDomeDiameter(), 0.0f);
	else if( mActiveFrustum == Frustum::StereoRightEye )
		setPtr->setFisheyeOffset( getUserPtr()->getEyeSeparation() / setPtr->getDomeDiameter(), 0.0f);

	//iterate the cube sides
	for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
	{
		tmpNode->setCurrentViewport(i);

		if( tmpNode->getCurrentViewport()->isEnabled() )
		{
			//un-bind texture
			glBindTexture(GL_TEXTURE_2D, 0);
			
			//bind & attach buffer
			mCubeMapFBO_Ptr->bind(); //osg seems to unbind FBO when rendering with osg FBO cameras
			if( !mCubeMapFBO_Ptr->isMultiSampled() )
			{								
				mCubeMapFBO_Ptr->attachCubeMapTexture( mFrameBufferTextures[FishEye], i );
			}

			setAndClearBuffer(RenderToTexture);

			//render
			(this->*mInternalDrawFn)();

			//copy AA-buffer to "regular"/non-AA buffer
			if( mCubeMapFBO_Ptr->isMultiSampled() )
			{
				mCubeMapFBO_Ptr->bindBlit(); //bind separate read and draw buffers to prepare blit operation

				//update attachments
				mCubeMapFBO_Ptr->attachCubeMapTexture( mFrameBufferTextures[FishEye], i );

				mCubeMapFBO_Ptr->blit();
			}
		}
	}//end for

	//restore polygon mode
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	//un-bind texture
	glBindTexture(GL_TEXTURE_2D, 0);

	//bind fisheye target FBO
	mFinalFBO_Ptr->bind();
	SGCTSettings::Instance()->usePostFX() ? 
			mFinalFBO_Ptr->attachColorTexture( mFrameBufferTextures[PostFX] ) :
			mFinalFBO_Ptr->attachColorTexture( mFrameBufferTextures[ti] );

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
	glViewport(0, 0, getWindowPtr()->getXFramebufferResolution(), getWindowPtr()->getYFramebufferResolution());

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	//if for some reson the active texture has been reset
	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, mFrameBufferTextures[FishEye]);

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	mShaders[FisheyeShader].use();
	glUniform1i( mShaderLocs[Cubemap], 0);
	glUniform1f( mShaderLocs[FishEyeHalfFov], glm::radians<float>(SGCTSettings::Instance()->getFisheyeFOV()/2.0f) );
	if( setPtr->isFisheyeOffaxis() )
	{
		glUniform3f( mShaderLocs[FisheyeOffset], setPtr->getFisheyeOffset(0), setPtr->getFisheyeOffset(1), setPtr->getFisheyeOffset(2) );
	}

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	//make sure that VBO:s are unbinded, to not mess up the vertex array
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[FishEyeQuad]);
	glClientActiveTexture(GL_TEXTURE0);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(0));

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(8));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind
	
	sgct::ShaderManager::Instance()->unBindShader();

	if(mTakeScreenshot)
	{
		glDisable(GL_TEXTURE_CUBE_MAP);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		int size = getWindowPtr()->getXFramebufferResolution();
		unsigned int fontSize = 0;

		if( size > 512 )
			fontSize = static_cast<unsigned int>(size)/96;
		else if( size == 512 )
			fontSize = 8;

		if( fontSize != 0 )
		{
			float x = static_cast<float>( size - size/4 );
			float y = static_cast<float>( fontSize );
			
			sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", fontSize ), x, 2.0f * y + y/5.0f, "Frame#: %d", mShotCounter);

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
	mFinalFBO_Ptr->attachColorTexture( mFrameBufferTextures[ ti ] );

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/*
		The code below flips the viewport vertically. Top & bottom coords are flipped.
	*/
	glm::mat4 orthoMat = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);

	//if for some reson the active texture has been reset
	glViewport(0, 0, getWindowPtr()->getXFramebufferResolution(), getWindowPtr()->getYFramebufferResolution());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[ PostFX ] );

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	mShaders[FXAAShader].use();
	glUniformMatrix4fv( mShaderLocs[FXAAMVP], 1, GL_FALSE, &orthoMat[0][0]);
	glUniform1f( mShaderLocs[SizeX], static_cast<float>(getWindowPtr()->getXFramebufferResolution()) );
	glUniform1f( mShaderLocs[SizeY], static_cast<float>(getWindowPtr()->getYFramebufferResolution()) );
	glUniform1i( mShaderLocs[FXAATexture], 0 );

	glBindVertexArray( mVAO[RenderQuad] );
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[RenderQuad]);
	
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

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	//unbind
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	sgct::ShaderManager::Instance()->unBindShader();
}

/*!
	This function combines a texture and a shader into a new texture
*/
void sgct::Engine::renderPostFxFixedPipeline(TextureIndexes ti)
{
	//bind fisheye target FBO
	mFinalFBO_Ptr->attachColorTexture( mFrameBufferTextures[ ti ] );

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
	glViewport(0, 0, getWindowPtr()->getXFramebufferResolution(), getWindowPtr()->getYFramebufferResolution());

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[ PostFX ] );

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	mShaders[FXAAShader].use();
	glUniform1f( mShaderLocs[SizeX], static_cast<float>(getWindowPtr()->getXFramebufferResolution()) );
	glUniform1f( mShaderLocs[SizeY], static_cast<float>(getWindowPtr()->getYFramebufferResolution()) );
	glUniform1i( mShaderLocs[FXAATexture], 0 );

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

	glBindBuffer(GL_ARRAY_BUFFER, mVBO[RenderQuad]);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);
	glClientActiveTexture(GL_TEXTURE0);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(0));

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(8));
	//glInterleavedArrays(GL_T2F_V3F, 0, mPostFxQuadVerts);
	//glInterleavedArrays(GL_T2F_V3F, 0, 0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind

	sgct::ShaderManager::Instance()->unBindShader();

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
	if( mFinalFBO_Ptr->isMultiSampled() )
	{
		if( SGCTSettings::Instance()->usePostFX() )
			ti = PostFX;

		mFinalFBO_Ptr->bindBlit(); //bind separate read and draw buffers to prepare blit operation

		//update attachments
		mFinalFBO_Ptr->attachColorTexture( mFrameBufferTextures[ti] );

		mFinalFBO_Ptr->blit();
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
	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

	//set null shader
	mShaders[NULLShader] = sgct::ShaderManager::Instance()->getShader( "SGCT_NULL" );

	//create FXAA shaders
	if( mFixedOGLPipeline )
	{
		sgct::ShaderManager::Instance()->addShader( mShaders[FXAAShader], "FXAA",
			sgct_core::shaders::FXAA_Vert_Shader,
			sgct_core::shaders::FXAA_FRAG_Shader, ShaderManager::SHADER_SRC_STRING );
	}
	else
	{
		sgct::ShaderManager::Instance()->addShader( mShaders[FXAAShader], "FXAA",
			sgct_core::shaders_modern::FXAA_Vert_Shader,
			sgct_core::shaders_modern::FXAA_FRAG_Shader, ShaderManager::SHADER_SRC_STRING );
	}
	mShaders[FXAAShader].use();

	if( !mFixedOGLPipeline )
		mShaderLocs[FXAAMVP] = mShaders[FXAAShader].getUniformLocation( "MVP" );

	mShaderLocs[SizeX] = mShaders[FXAAShader].getUniformLocation( "rt_w" );
	glUniform1f( mShaderLocs[SizeX], static_cast<float>(getWindowPtr()->getXFramebufferResolution()) );

	mShaderLocs[SizeY] = mShaders[FXAAShader].getUniformLocation( "rt_h" );
	glUniform1f( mShaderLocs[SizeY], static_cast<float>(getWindowPtr()->getYFramebufferResolution()) );

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

	sgct::ShaderManager::Instance()->unBindShader();

	if( tmpNode->isUsingFisheyeRendering() )
	{
		if( mFixedOGLPipeline )
		{
			if( SGCTSettings::Instance()->isFisheyeOffaxis() || tmpNode->stereo != ClusterManager::NoStereo )
				sgct::ShaderManager::Instance()->addShader( mShaders[FisheyeShader], "Fisheye", sgct_core::shaders::Fisheye_Vert_Shader, sgct_core::shaders::Fisheye_Frag_Shader_OffAxis, ShaderManager::SHADER_SRC_STRING );
			else
				sgct::ShaderManager::Instance()->addShader( mShaders[FisheyeShader], "Fisheye", sgct_core::shaders::Fisheye_Vert_Shader, sgct_core::shaders::Fisheye_Frag_Shader, ShaderManager::SHADER_SRC_STRING );
		}
		else
		{
			if( SGCTSettings::Instance()->isFisheyeOffaxis() || tmpNode->stereo != ClusterManager::NoStereo )
				sgct::ShaderManager::Instance()->addShader( mShaders[FisheyeShader], "Fisheye", sgct_core::shaders_modern::Fisheye_Vert_Shader, sgct_core::shaders_modern::Fisheye_Frag_Shader_OffAxis, ShaderManager::SHADER_SRC_STRING );
			else
				sgct::ShaderManager::Instance()->addShader( mShaders[FisheyeShader], "Fisheye", sgct_core::shaders_modern::Fisheye_Vert_Shader, sgct_core::shaders_modern::Fisheye_Frag_Shader, ShaderManager::SHADER_SRC_STRING );
		}
		mShaders[FisheyeShader].use();

		if( !mFixedOGLPipeline )
			mShaderLocs[FisheyeMVP] = mShaders[FisheyeShader].getUniformLocation( "MVP" );
		
		mShaderLocs[Cubemap] = mShaders[FisheyeShader].getUniformLocation( "cubemap" );
		glUniform1i( mShaderLocs[Cubemap], 0 );
		
		mShaderLocs[FishEyeHalfFov] = mShaders[FisheyeShader].getUniformLocation( "halfFov" );
		glUniform1f( mShaderLocs[FishEyeHalfFov], glm::half_pi<float>() );
		
		if( SGCTSettings::Instance()->isFisheyeOffaxis() || tmpNode->stereo != ClusterManager::NoStereo )
		{
			mShaderLocs[FisheyeOffset] = mShaders[FisheyeShader].getUniformLocation( "offset" );
			glUniform3f( mShaderLocs[FisheyeOffset],
				SGCTSettings::Instance()->getFisheyeOffset(0),
				SGCTSettings::Instance()->getFisheyeOffset(1),
				SGCTSettings::Instance()->getFisheyeOffset(2) );
		}
		sgct::ShaderManager::Instance()->unBindShader();
	}

	if( tmpNode->stereo > ClusterManager::Active )
	{
		if( tmpNode->stereo == ClusterManager::Anaglyph_Red_Cyan )
		{
			mFixedOGLPipeline ? sgct::ShaderManager::Instance()->addShader( mShaders[StereoShader], "Anaglyph_Red_Cyan",
					sgct_core::shaders::Anaglyph_Vert_Shader,
					sgct_core::shaders::Anaglyph_Red_Cyan_Frag_Shader,
					ShaderManager::SHADER_SRC_STRING ) : 
				sgct::ShaderManager::Instance()->addShader( mShaders[StereoShader], "Anaglyph_Red_Cyan",
					sgct_core::shaders_modern::Anaglyph_Vert_Shader,
					sgct_core::shaders_modern::Anaglyph_Red_Cyan_Frag_Shader,
					ShaderManager::SHADER_SRC_STRING );
		}
		else if( tmpNode->stereo == ClusterManager::Anaglyph_Amber_Blue )
		{
			mFixedOGLPipeline ? sgct::ShaderManager::Instance()->addShader( mShaders[StereoShader], "Anaglyph_Amber_Blue",
					sgct_core::shaders::Anaglyph_Vert_Shader,
					sgct_core::shaders::Anaglyph_Amber_Blue_Frag_Shader,
					ShaderManager::SHADER_SRC_STRING ) :
				sgct::ShaderManager::Instance()->addShader( mShaders[StereoShader], "Anaglyph_Amber_Blue",
					sgct_core::shaders_modern::Anaglyph_Vert_Shader,
					sgct_core::shaders_modern::Anaglyph_Amber_Blue_Frag_Shader,
					ShaderManager::SHADER_SRC_STRING );
		}
		else if( tmpNode->stereo == ClusterManager::Anaglyph_Red_Cyan_Wimmer )
		{
			mFixedOGLPipeline ? sgct::ShaderManager::Instance()->addShader( mShaders[StereoShader], "Anaglyph_Red_Cyan_Wimmer",
					sgct_core::shaders::Anaglyph_Vert_Shader,
					sgct_core::shaders::Anaglyph_Red_Cyan_Frag_Shader_Wimmer,
					ShaderManager::SHADER_SRC_STRING ) :
				sgct::ShaderManager::Instance()->addShader( mShaders[StereoShader], "Anaglyph_Red_Cyan_Wimmer",
					sgct_core::shaders_modern::Anaglyph_Vert_Shader,
					sgct_core::shaders_modern::Anaglyph_Red_Cyan_Frag_Shader_Wimmer,
					ShaderManager::SHADER_SRC_STRING );
		}
		else if( tmpNode->stereo == ClusterManager::Checkerboard )
		{
			mFixedOGLPipeline ? sgct::ShaderManager::Instance()->addShader( mShaders[StereoShader], "Checkerboard",
					sgct_core::shaders::Anaglyph_Vert_Shader,
					sgct_core::shaders::CheckerBoard_Frag_Shader,
					ShaderManager::SHADER_SRC_STRING ):
				sgct::ShaderManager::Instance()->addShader( mShaders[StereoShader], "Checkerboard",
					sgct_core::shaders_modern::Anaglyph_Vert_Shader,
					sgct_core::shaders_modern::CheckerBoard_Frag_Shader,
					ShaderManager::SHADER_SRC_STRING );
		}
		else if( tmpNode->stereo == ClusterManager::Checkerboard_Inverted )
		{
			mFixedOGLPipeline ? sgct::ShaderManager::Instance()->addShader( mShaders[StereoShader], "Checkerboard_Inverted",
					sgct_core::shaders::Anaglyph_Vert_Shader,
					sgct_core::shaders::CheckerBoard_Inverted_Frag_Shader,
					ShaderManager::SHADER_SRC_STRING ):
				sgct::ShaderManager::Instance()->addShader( mShaders[StereoShader], "Checkerboard_Inverted",
					sgct_core::shaders_modern::Anaglyph_Vert_Shader,
					sgct_core::shaders_modern::CheckerBoard_Inverted_Frag_Shader,
					ShaderManager::SHADER_SRC_STRING );
		}
		else if( tmpNode->stereo == ClusterManager::Vertical_Interlaced )
		{
			mFixedOGLPipeline ? sgct::ShaderManager::Instance()->addShader( mShaders[StereoShader], "Vertical_Interlaced",
					sgct_core::shaders::Anaglyph_Vert_Shader,
					sgct_core::shaders::Vertical_Interlaced_Frag_Shader,
					ShaderManager::SHADER_SRC_STRING ):
				sgct::ShaderManager::Instance()->addShader( mShaders[StereoShader], "Vertical_Interlaced",
					sgct_core::shaders_modern::Anaglyph_Vert_Shader,
					sgct_core::shaders_modern::Vertical_Interlaced_Frag_Shader,
					ShaderManager::SHADER_SRC_STRING );
		}
		else if( tmpNode->stereo == ClusterManager::Vertical_Interlaced_Inverted )
		{
			mFixedOGLPipeline ? sgct::ShaderManager::Instance()->addShader( mShaders[StereoShader], "Vertical_Interlaced_Inverted",
					sgct_core::shaders::Anaglyph_Vert_Shader,
					sgct_core::shaders::Vertical_Interlaced_Inverted_Frag_Shader,
					ShaderManager::SHADER_SRC_STRING ):
				sgct::ShaderManager::Instance()->addShader( mShaders[StereoShader], "Vertical_Interlaced_Inverted",
					sgct_core::shaders_modern::Anaglyph_Vert_Shader,
					sgct_core::shaders_modern::Vertical_Interlaced_Inverted_Frag_Shader,
					ShaderManager::SHADER_SRC_STRING );
		}
		else
		{
			mFixedOGLPipeline ? sgct::ShaderManager::Instance()->addShader( mShaders[StereoShader], "Dummy_Stereo",
					sgct_core::shaders::Anaglyph_Vert_Shader,
					sgct_core::shaders::Dummy_Stereo_Frag_Shader,
					ShaderManager::SHADER_SRC_STRING ):
				sgct::ShaderManager::Instance()->addShader( mShaders[StereoShader], "Dummy_Stereo",
					sgct_core::shaders_modern::Anaglyph_Vert_Shader,
					sgct_core::shaders_modern::Dummy_Stereo_Frag_Shader,
					ShaderManager::SHADER_SRC_STRING );
		}

		mShaders[StereoShader].use();
		if( !mFixedOGLPipeline )
			mShaderLocs[StereoMVP] = mShaders[StereoShader].getUniformLocation( "MVP" );
		mShaderLocs[StereoLeftTex] = mShaders[StereoShader].getUniformLocation( "LeftTex" );
		mShaderLocs[StereoRightTex] = mShaders[StereoShader].getUniformLocation( "RightTex" );
		glUniform1i( mShaderLocs[StereoLeftTex], 0 );
		glUniform1i( mShaderLocs[StereoRightTex], 1 );
		sgct::ShaderManager::Instance()->unBindShader();
	}
	
	/*!
		Used for overlays & mono.
	*/
	if( !mFixedOGLPipeline )
	{
		sgct::ShaderManager::Instance()->addShader( mShaders[FBOQuadShader], "FBOQuad",
			sgct_core::shaders_modern::Base_Vert_Shader,
			sgct_core::shaders_modern::Base_Frag_Shader, ShaderManager::SHADER_SRC_STRING );
		mShaders[FBOQuadShader].use();

		mShaderLocs[MonoMVP] = mShaders[FBOQuadShader].getUniformLocation( "MVP" );
		mShaderLocs[MonoTex] = mShaders[FBOQuadShader].getUniformLocation( "Tex" );
		glUniform1i( mShaderLocs[MonoTex], 0 );

		sgct::ShaderManager::Instance()->addShader( mShaders[OverlayShader], "OverlayQuad",
			sgct_core::shaders_modern::Overlay_Vert_Shader,
			sgct_core::shaders_modern::Overlay_Frag_Shader, ShaderManager::SHADER_SRC_STRING );
		mShaders[OverlayShader].use();

		mShaderLocs[OverlayMVP] = mShaders[OverlayShader].getUniformLocation( "MVP" );
		mShaderLocs[OverlayTex] = mShaders[OverlayShader].getUniformLocation( "Tex" );
		glUniform1i( mShaderLocs[OverlayTex], 0 );

		sgct::ShaderManager::Instance()->unBindShader();
	}
}

/*!
	This function creates textures that will act as FBO targets.
*/
void sgct::Engine::createTextures()
{
	//no target textures needed if not using FBO
	if(SGCTSettings::Instance()->getFBOMode() == SGCTSettings::NoFBO)
		return;
	
	if( mRunMode <= OpenGL_Compablity_Profile )
	{
		glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT);
		glEnable(GL_TEXTURE_2D);
	}

	//init
	mFrameBufferTextures[0] = 0;
	mFrameBufferTextures[1] = 0;
	mFrameBufferTextures[2] = 0;
	mFrameBufferTextures[3] = 0;

	//allocate
	glGenTextures(4, &mFrameBufferTextures[0]);

	/*
		Create left and right color & depth textures.
	*/
	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();
	//don't allocate the right eye image if stereo is not used
	//create a postFX texture for effects
	int numberOfTexturesToGenerate = (tmpNode->stereo > ClusterManager::NoStereo ? 2 : 1) + 1;
	for( int i=0; i<numberOfTexturesToGenerate; i++ )
	{
		glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[i]);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); //must be linear if warping, blending or fix resolution is used
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, getWindowPtr()->getXFramebufferResolution(), getWindowPtr()->getYFramebufferResolution(), 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	}
	/*
		Create cubemap texture for fisheye rendering if enabled.
	*/
	if(tmpNode->isUsingFisheyeRendering())
	{
		GLint cubeMapRes = SGCTSettings::Instance()->getCubeMapResolution();
		GLint MaxCubeMapRes;
		glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &MaxCubeMapRes);
		if(cubeMapRes > MaxCubeMapRes)
		{
			cubeMapRes = MaxCubeMapRes;
			SGCTSettings::Instance()->setCubeMapResolution(cubeMapRes);
			sgct::MessageHandler::Instance()->print("Info: Cubemap size set to max size: %d\n", MaxCubeMapRes);
		}

		//set up texture target
		glBindTexture(GL_TEXTURE_CUBE_MAP, mFrameBufferTextures[ FishEye ]);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		for (int i = 0; i < 6; ++i)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, cubeMapRes, cubeMapRes, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	}

	if( mRunMode <= OpenGL_Compablity_Profile )
		glPopAttrib();

	if( checkForOGLErrors() )
		sgct::MessageHandler::Instance()->print("Texture targets initiated successfully!\n");
	else
		sgct::MessageHandler::Instance()->print("Texture targets failed to initialize!\n");
}

/*!
	This function creates FBOs if they are supported.
	This is done in the initOGL function.
*/
void sgct::Engine::createFBOs()
{
	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();
	
	if(SGCTSettings::Instance()->getFBOMode() == SGCTSettings::NoFBO)
	{
		//disable anaglyph & checkerboard stereo if FBOs are not used
		if( tmpNode->stereo > ClusterManager::Active )
			tmpNode->stereo = ClusterManager::NoStereo;
		sgct::MessageHandler::Instance()->print("Warning! FBO rendering is not supported or enabled!\nAnaglyph & checkerboard (DLP) stereo modes are disabled.\n");
	}
	else
	{
		if( tmpNode->isUsingFisheyeRendering() )
		{
			mFinalFBO_Ptr->createFBO(getWindowPtr()->getXFramebufferResolution(),
			getWindowPtr()->getYFramebufferResolution(),
			1);
			
			mCubeMapFBO_Ptr->createFBO(SGCTSettings::Instance()->getCubeMapResolution(),
				SGCTSettings::Instance()->getCubeMapResolution(),
				tmpNode->numberOfSamples);

			mCubeMapFBO_Ptr->bind();
			for(int i=0; i<6; i++)
			{
				if(!mCubeMapFBO_Ptr->isMultiSampled())
				{								
					mCubeMapFBO_Ptr->attachCubeMapTexture( mFrameBufferTextures[FishEye], i );
				}
				
				SGCTSettings::Instance()->useFisheyeAlpha() ? glClearColor(0.0f, 0.0f, 0.0f, 0.0f) : glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

				//copy AA-buffer to "regular"/non-AA buffer
				if( mCubeMapFBO_Ptr->isMultiSampled() )
				{
					mCubeMapFBO_Ptr->bindBlit(); //bind separate read and draw buffers to prepare blit operation

					//update attachments
					mCubeMapFBO_Ptr->attachCubeMapTexture( mFrameBufferTextures[FishEye], i );

					mCubeMapFBO_Ptr->blit();
				}
			}

			OffScreenBuffer::unBind();

			MessageHandler::Instance()->print("Engine info: Initial cube map rendered.\n");

			//set ut the fisheye geometry etc.
			initFisheye();
		}
		else //regular viewport rendering
		{
			mFinalFBO_Ptr->createFBO(
				getWindowPtr()->getXFramebufferResolution(),
				getWindowPtr()->getYFramebufferResolution(),
				tmpNode->numberOfSamples);
		}

		sgct::MessageHandler::Instance()->print("FBOs initiated successfully!\n");
	}
}

void sgct::Engine::createVBOs()
{
	if( !mFixedOGLPipeline )
		glGenVertexArrays(NUMBER_OF_VBOS, &mVAO[0]);

	glGenBuffers(NUMBER_OF_VBOS, &mVBO[0]);
		
	if( !mFixedOGLPipeline )
		glBindVertexArray( mVAO[RenderQuad] );
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[RenderQuad]);
	glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), mPostFxQuadVerts, GL_STATIC_DRAW); //2TF + 3VF = 2*4 + 3*4 = 20

	if( !mFixedOGLPipeline )
		glBindVertexArray( mVAO[FishEyeQuad] );
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[FishEyeQuad]);
	glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), mFisheyeQuadVerts, GL_STREAM_DRAW); //2TF + 3VF = 2*4 + 3*4 = 20

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if( !mFixedOGLPipeline )
		glBindVertexArray(0);
}

/*!
	This function resizes the FBOs when the window is resized to achive 1:1 pixel-texel mapping.
*/
void sgct::Engine::resizeFBOs()
{
	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();
	
	if(!getWindowPtr()->isFixResolution())
	{
		if(SGCTSettings::Instance()->getFBOMode() != SGCTSettings::NoFBO)
		{
			glDeleteTextures(4,			&mFrameBufferTextures[0]);
			createTextures();

			if( tmpNode->isUsingFisheyeRendering() )
			{
				mFinalFBO_Ptr->resizeFBO(getWindowPtr()->getXFramebufferResolution(),
				getWindowPtr()->getYFramebufferResolution(),
				1);

				//Cube map FBO is constant in size so we don't need to resize that one
			
				//set ut the fisheye geometry etc.
				initFisheye();
			}
			else //regular viewport rendering
			{
				mFinalFBO_Ptr->resizeFBO(getWindowPtr()->getXFramebufferResolution(),
				getWindowPtr()->getYFramebufferResolution(),
				tmpNode->numberOfSamples);
			}

			sgct::MessageHandler::Instance()->print("FBOs resized successfully!\n");
		}
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
		SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

		//Set buffer
		if( tmpNode->stereo != ClusterManager::Active )
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
		sgct::MessageHandler::Instance()->print("OpenGL error: GL_INVALID_ENUM\n");
		break;

	case GL_INVALID_VALUE:
		sgct::MessageHandler::Instance()->print("OpenGL error: GL_INVALID_VALUE\n");
		break;

	case GL_INVALID_OPERATION:
		sgct::MessageHandler::Instance()->print("OpenGL error: GL_INVALID_OPERATION\n");
		break;

	case GL_INVALID_FRAMEBUFFER_OPERATION:
		sgct::MessageHandler::Instance()->print("OpenGL error: GL_INVALID_FRAMEBUFFER_OPERATION\n");
		break;

	case GL_STACK_OVERFLOW:
		sgct::MessageHandler::Instance()->print("OpenGL error: GL_STACK_OVERFLOW\n");
		break;

	case GL_STACK_UNDERFLOW:
		sgct::MessageHandler::Instance()->print("OpenGL error: GL_STACK_UNDERFLOW\n");
		break;

	case GL_OUT_OF_MEMORY:
		sgct::MessageHandler::Instance()->print("OpenGL error: GL_OUT_OF_MEMORY\n");
		break;

	case GL_TABLE_TOO_LARGE:
		sgct::MessageHandler::Instance()->print("OpenGL error: GL_TABLE_TOO_LARGE\n");
		break;
	}

	return oglError == GL_NO_ERROR; //returns true if no errors
}

/*!
	This functions saves a png screenshot or a stereoscopic pair in the current working directory.
	All screenshots are numbered so this function can be called several times whitout overwriting previous screenshots.

	The PNG images are saved as RGBA images with transparancy. Alpha is taken from the clear color alpha.
*/
void sgct::Engine::captureBuffer()
{
	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

	if(tmpNode->stereo == ClusterManager::NoStereo)
	{
		if(SGCTSettings::Instance()->getFBOMode() != SGCTSettings::NoFBO)
			mScreenCapture.SaveScreenCapture( mFrameBufferTextures[LeftEye], mShotCounter, ScreenCapture::FBO_Texture );
		else
			mScreenCapture.SaveScreenCapture( 0, mShotCounter, ScreenCapture::Front_Buffer );
	}
	else
	{
		if(SGCTSettings::Instance()->getFBOMode() != SGCTSettings::NoFBO)
		{
			mScreenCapture.SaveScreenCapture( mFrameBufferTextures[LeftEye], mShotCounter, ScreenCapture::FBO_Left_Texture );
			mScreenCapture.SaveScreenCapture( mFrameBufferTextures[RightEye], mShotCounter, ScreenCapture::FBO_Right_Texture );
		}
		else
		{
			mScreenCapture.SaveScreenCapture( 0, mShotCounter, ScreenCapture::Left_Front_Buffer );
			mScreenCapture.SaveScreenCapture( 0, mShotCounter, ScreenCapture::Right_Front_Buffer );
		}
	}

	mShotCounter++;
	mTakeScreenshot = false;
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
		if( getWindowPtr()->isUsingSwapGroups() )
		{
			//check if swapgroups are supported
			#ifdef __WIN32__
			if( wglewIsSupported("WGL_NV_swap_group") )
				sgct::MessageHandler::Instance()->print("Swap groups are supported by hardware.\n");
			#else
			if( glewIsSupported("GLX_NV_swap_group") )
				sgct::MessageHandler::Instance()->print("Swap groups are supported by hardware.\n");
			#endif
			else
				sgct::MessageHandler::Instance()->print("Swap groups are not supported by hardware.\n");
		}
		else
			sgct::MessageHandler::Instance()->print("Swapgroups (swap-lock) are disabled.\n");

		sgct::MessageHandler::Instance()->print("Waiting for all nodes to connect.");
		glfwSwapBuffers(); //render just black....

		SGCTMutexManager::Instance()->lockMutex( SGCTMutexManager::SyncMutex );
		while(mNetworkConnections->isRunning() &&
			!glfwGetKey( mExitKey ) &&
			glfwGetWindowParam( GLFW_OPENED ) &&
			!mTerminate)
		{
			sgct::MessageHandler::Instance()->print(".");

			// Swap front and back rendering buffers
			glfwSwapBuffers();

			if(mNetworkConnections->areAllNodesConnected())
				break;

			glfwWaitCond( NetworkManager::gCond,
				SGCTMutexManager::Instance()->getMutex(SGCTMutexManager::SyncMutex ),
				0.1 ); //wait maximum 0.1 sec per iteration
		}
		SGCTMutexManager::Instance()->unlockMutex( SGCTMutexManager::SyncMutex );

		//wait for user to release exit key
		while( glfwGetKey( mExitKey ) == GLFW_PRESS )
		{
			sgct::MessageHandler::Instance()->print("x");
			// Swap front and back rendering buffers
			// key buffers also swapped
			glfwSwapBuffers();

			glfwSleep(0.05);
		}

		sgct::MessageHandler::Instance()->print("\n");
	}
}

/*!
	This functions updates the frustum of all viewports on demand. However if the viewport is tracked this is done on the fly.
*/
void sgct::Engine::calculateFrustums()
{
	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

	for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
		if( !tmpNode->getViewport(i)->isTracked() ) //if not tracked update, otherwise this is done on the fly
		{
			SGCTUser * usrPtr = ClusterManager::Instance()->getUserPtr();

			if( !tmpNode->isUsingFisheyeRendering() )
			{
				tmpNode->getViewport(i)->calculateFrustum(
					Frustum::Mono,
					usrPtr->getPosPtr(),
					mNearClippingPlaneDist,
					mFarClippingPlaneDist);

				tmpNode->getViewport(i)->calculateFrustum(
					Frustum::StereoLeftEye,
					usrPtr->getPosPtr(Frustum::StereoLeftEye),
					mNearClippingPlaneDist,
					mFarClippingPlaneDist);

				tmpNode->getViewport(i)->calculateFrustum(
					Frustum::StereoRightEye,
					usrPtr->getPosPtr(Frustum::StereoRightEye),
					mNearClippingPlaneDist,
					mFarClippingPlaneDist);
			}
			else
			{
				tmpNode->getViewport(i)->calculateFisheyeFrustum(
					Frustum::Mono,
					usrPtr->getPosPtr(),
					usrPtr->getPosPtr(),
					mNearClippingPlaneDist,
					mFarClippingPlaneDist);

				tmpNode->getViewport(i)->calculateFisheyeFrustum(
					Frustum::StereoLeftEye,
					usrPtr->getPosPtr(),
					usrPtr->getPosPtr(Frustum::StereoLeftEye),
					mNearClippingPlaneDist,
					mFarClippingPlaneDist);

				tmpNode->getViewport(i)->calculateFisheyeFrustum(
					Frustum::StereoRightEye,
					usrPtr->getPosPtr(),
					usrPtr->getPosPtr(Frustum::StereoRightEye),
					mNearClippingPlaneDist,
					mFarClippingPlaneDist);
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
	sgct::MessageHandler::Instance()->print("Parsing arguments...");
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
			SGCTSettings::Instance()->setFBOMode( SGCTSettings::NoFBO );
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--Regular-FBO") == 0 )
		{
			SGCTSettings::Instance()->setFBOMode( SGCTSettings::RegularFBO );
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--MultiSampled-FBO") == 0 )
		{
			SGCTSettings::Instance()->setFBOMode( SGCTSettings::MultiSampledFBO );
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
			mScreenCapture.setFormat( ScreenCapture::TGA );
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--Capture-PNG") == 0 )
		{
			mScreenCapture.setFormat( ScreenCapture::PNG );
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

	sgct::MessageHandler::Instance()->print(" Done\n");
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
	mKeyboardCallbackFn = fnPtr;
}

void sgct::Engine::setCharCallbackFunction( void(*fnPtr)(int, int) )
{
	mCharCallbackFn = fnPtr;
}

void sgct::Engine::setMouseButtonCallbackFunction( void(*fnPtr)(int, int) )
{
	mMouseButtonCallbackFn = fnPtr;
}

void sgct::Engine::setMousePosCallbackFunction( void(*fnPtr)(int, int) )
{
	mMousePosCallbackFn = fnPtr;
}

void sgct::Engine::setMouseScrollCallbackFunction( void(*fnPtr)(int) )
{
	mMouseWheelCallbackFn = fnPtr;
}

void sgct::Engine::clearBuffer()
{
	const float * colorPtr = sgct::Engine::getPtr()->getClearColor();
	glClearColor(colorPtr[0], colorPtr[1], colorPtr[2], colorPtr[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/*!
	Print the node info to terminal.

	\param nodeId Which node to print
*/
void sgct::Engine::printNodeInfo(unsigned int nodeId)
{
	sgct::MessageHandler::Instance()->print("This node has index %d.\n", nodeId);
}

/*!
	Set up the current viewport.

	\param vs Which space to set up the viewport in.
*/
void sgct::Engine::enterCurrentViewport(ViewportSpace vs)
{
	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

	if( tmpNode->isUsingFisheyeRendering() && vs != ScreenSpace )
	{
		int cmRes = SGCTSettings::Instance()->getCubeMapResolution();
		currentViewportCoords[0] = 0;
		currentViewportCoords[1] = 0;
		currentViewportCoords[2] = cmRes;
		currentViewportCoords[3] = cmRes;
	}
	else
	{
		if( vs == ScreenSpace || SGCTSettings::Instance()->getFBOMode() == SGCTSettings::NoFBO )
		{
			currentViewportCoords[0] =
				static_cast<int>( tmpNode->getCurrentViewport()->getX() * static_cast<double>(getWindowPtr()->getXResolution()));
			currentViewportCoords[1] =
				static_cast<int>( tmpNode->getCurrentViewport()->getY() * static_cast<double>(getWindowPtr()->getYResolution()));
			currentViewportCoords[2] =
				static_cast<int>( tmpNode->getCurrentViewport()->getXSize() * static_cast<double>(getWindowPtr()->getXResolution()));
			currentViewportCoords[3] =
				static_cast<int>( tmpNode->getCurrentViewport()->getYSize() * static_cast<double>(getWindowPtr()->getYResolution()));
		}
		else
		{
			currentViewportCoords[0] =
				static_cast<int>( tmpNode->getCurrentViewport()->getX() * static_cast<double>(getWindowPtr()->getXFramebufferResolution()));
			currentViewportCoords[1] =
				static_cast<int>( tmpNode->getCurrentViewport()->getY() * static_cast<double>(getWindowPtr()->getYFramebufferResolution()));
			currentViewportCoords[2] =
				static_cast<int>( tmpNode->getCurrentViewport()->getXSize() * static_cast<double>(getWindowPtr()->getXFramebufferResolution()));
			currentViewportCoords[3] =
				static_cast<int>( tmpNode->getCurrentViewport()->getYSize() * static_cast<double>(getWindowPtr()->getYFramebufferResolution()));
		}
	}

	glViewport( currentViewportCoords[0],
		currentViewportCoords[1],
		currentViewportCoords[2],
		currentViewportCoords[3]);
}

/*!
	Init the fisheye data by creating geometry and precalculate textures
*/
void sgct::Engine::initFisheye()
{
	//create proxy geometry
	float leftcrop		= SGCTSettings::Instance()->getFisheyeCropValue(SGCTSettings::CropLeft);
	float rightcrop		= SGCTSettings::Instance()->getFisheyeCropValue(SGCTSettings::CropRight);
	float bottomcrop	= SGCTSettings::Instance()->getFisheyeCropValue(SGCTSettings::CropBottom);
	float topcrop		= SGCTSettings::Instance()->getFisheyeCropValue(SGCTSettings::CropTop);

	float cropAspect = ((1.0f-2.0f * bottomcrop) + (1.0f-2.0f*topcrop)) / ((1.0f-2.0f*leftcrop) + (1.0f-2.0f*rightcrop));

	float x = 1.0f;
	float y = 1.0f;
	float frameBufferAspect = static_cast<float>( getWindowPtr()->getXFramebufferResolution() ) /
		static_cast<float>( getWindowPtr()->getYFramebufferResolution() );
	
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
	if( !mFixedOGLPipeline )
		glBindVertexArray( mVAO[FishEyeQuad] );
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[FishEyeQuad]);
	
	GLvoid* PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	memcpy(PositionBuffer, mFisheyeQuadVerts, 20 * sizeof(float));
	glUnmapBuffer(GL_ARRAY_BUFFER);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if( !mFixedOGLPipeline )
		glBindVertexArray( 0 );

	//set alpha value
	mFisheyeClearColor[3] = SGCTSettings::Instance()->useFisheyeAlpha() ? 0.0f : 1.0f;
}

void sgct::Engine::calculateFPS(double timestamp)
{
	static double lastTimestamp = glfwGetTime();
	mStatistics.setFrameTime(timestamp - lastTimestamp);
	lastTimestamp = timestamp;
    static double renderedFrames = 0.0;
	static double tmpTime = 0.0;
	renderedFrames += 1.0;
	tmpTime += mStatistics.getFrameTime();
	if( tmpTime >= 1.0 )
	{
		mStatistics.setAvgFPS(renderedFrames / tmpTime);
		renderedFrames = 0.0;
		tmpTime = 0.0;

		//don't set if in full screen
		if(getWindowPtr()->getWindowMode() == GLFW_WINDOW)
			getWindowPtr()->setWindowTitle( getBasicInfo() );
	}
}

/*!
\returns the frame time (delta time) in seconds
*/
const double & sgct::Engine::getDt()
{
	return mStatistics.getFrameTime();
}

/*!
\returns the average frame time (delta time) in seconds
*/
const double & sgct::Engine::getAvgDt()
{
	return mStatistics.getAvgFrameTime();
}

/*!
\returns the draw time in seconds
*/
const double & sgct::Engine::getDrawTime()
{
	return mStatistics.getDrawTime();
}

/*!
\returns the sync time (time waiting for other nodes and network) in seconds
*/
const double & sgct::Engine::getSyncTime()
{
	return mStatistics.getSyncTime();
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
	mFisheyeClearColor[3] = SGCTSettings::Instance()->useFisheyeAlpha() ? 0.0f : 1.0f;
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
const char * sgct::Engine::getBasicInfo()
{
	#if (_MSC_VER >= 1400) //visual studio 2005 or later
	sprintf_s( basicInfo, sizeof(basicInfo), "Node: %s (%s) | fps: %.2f | AA: %s",
		localRunningMode == NetworkManager::NotLocal ? ClusterManager::Instance()->getThisNodePtr()->ip.c_str() : "127.0.0.1",
		mNetworkConnections->isComputerServer() ? "master" : "slave",
		mStatistics.getAvgFPS(),
        getAAInfo());
    #else
    sprintf( basicInfo, "Node: %s (%s) | fps: %.2f | AA: %s",
		localRunningMode == NetworkManager::NotLocal ? ClusterManager::Instance()->getThisNodePtr()->ip.c_str() : "127.0.0.1",
		mNetworkConnections->isComputerServer() ? "master" : "slave",
		mStatistics.getAvgFPS(),
        getAAInfo());
    #endif

	return basicInfo;
}

/*!
	This function returns the Anti-Aliasing (AA) settings.
	This function is called once per second.
*/
const char * sgct::Engine::getAAInfo()
{
    SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();
	
	if( SGCTSettings::Instance()->useFXAA() &&
		SGCTSettings::Instance()->getFBOMode() != SGCTSettings::NoFBO)
	{
		if( tmpNode->isUsingFisheyeRendering() && tmpNode->numberOfSamples > 1 )
		{
			#if (_MSC_VER >= 1400) //visual studio 2005 or later
				sprintf_s(aaInfo, sizeof(aaInfo), "FXAA+MSAAx%d", tmpNode->numberOfSamples);
			#else
				sprintf(aaInfo, "FXAA+MSAAx%d", tmpNode->numberOfSamples);
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
    else
    {
        if( tmpNode->numberOfSamples > 1  &&
			SGCTSettings::Instance()->getFBOMode() != SGCTSettings::RegularFBO )
        {
            #if (_MSC_VER >= 1400) //visual studio 2005 or later
            sprintf_s( aaInfo, sizeof(aaInfo), "MSAAx%d",
                tmpNode->numberOfSamples);
            #else
            sprintf( aaInfo, "MSAAx%d",
                tmpNode->numberOfSamples);
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
	return glfwGetKey(key);
}

/*!
	Checks if specified mouse button has been pressed.
	\returns SGCT_PRESS or SGCT_RELEASE
*/
int sgct::Engine::getMouseButton( int button )
{
	return glfwGetMouseButton(button);
}

void sgct::Engine::getMousePos( int * xPos, int * yPos )
{
	glfwGetMousePos(xPos, yPos);
}

void sgct::Engine::setMousePos( int xPos, int yPos )
{
	glfwSetMousePos(xPos, yPos);
}

int sgct::Engine::getMouseWheel()
{
	return glfwGetMouseWheel();
}

void sgct::Engine::setMouseWheel( int pos )
{
	glfwSetMouseWheel(pos);
}

void sgct::Engine::setMousePointerVisibility( bool state )
{
	state ? glfwEnable( GLFW_MOUSE_CURSOR ) : glfwDisable( GLFW_MOUSE_CURSOR );
}

int sgct::Engine::getJoystickParam( int joystick, int param )
{
	return glfwGetJoystickParam(joystick,param);
}

int sgct::Engine::getJoystickAxes( int joystick, float * values, int numOfValues)
{
	return glfwGetJoystickPos( joystick, values, numOfValues );
}

int sgct::Engine::getJoystickButtons( int joystick, unsigned char * values, int numOfValues)
{
	return glfwGetJoystickButtons( joystick, values, numOfValues );
}

void sgct::Engine::sleep(double secs)
{
	glfwSleep(secs);
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
        sgct::MessageHandler::Instance()->print("There was no timer with id: %d", id);
    }
}

double sgct::Engine::getTime()
{
	return glfwGetTime();
}

sgct::SGCTcond sgct::Engine::createCondition()
{
    return glfwCreateCond();
}

void sgct::Engine::destroyCond(sgct::SGCTcond cond)
{
    glfwDestroyCond(cond);
}

void sgct::Engine::waitCond(sgct::SGCTcond cond, sgct::SGCTmutex mutex, double timeout)
{
    glfwWaitCond(cond, mutex, timeout);
}

void sgct::Engine::signalCond(sgct::SGCTcond cond)
{
    glfwSignalCond(cond);
}

/*!
	Returns true if fisheye rendering is active
*/
bool sgct::Engine::isFisheye()
{
	return ClusterManager::Instance()->getThisNodePtr()->isUsingFisheyeRendering();
}

/*!
	Returns pointer to FBO container
*/
sgct_core::OffScreenBuffer * sgct::Engine::getFBOPtr()
{
	return ClusterManager::Instance()->getThisNodePtr()->isUsingFisheyeRendering() ? mCubeMapFBO_Ptr : mFinalFBO_Ptr;
}

/*!
	Get the width and height of FBO in pixels
*/
void sgct::Engine::getFBODimensions( int & width, int & height )
{
	if( ClusterManager::Instance()->getThisNodePtr()->isUsingFisheyeRendering() )
	{
		width = SGCTSettings::Instance()->getCubeMapResolution();
		height = SGCTSettings::Instance()->getCubeMapResolution();
	}
	else
	{
		width = getWindowPtr()->getXFramebufferResolution();
		height = getWindowPtr()->getYFramebufferResolution();
	}
}
