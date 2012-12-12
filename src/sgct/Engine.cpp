/*************************************************************************
Copyright (c) 2012 Miroslav Andel
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
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/SGCTInternalShaders.h"
#include "../include/sgct/Image.h"
#include "../include/sgct/SGCTVersion.h"
#include "../include/sgct/SGCTSettings.h"
#include <glm/gtc/constants.hpp>
#include <math.h>
#include <iostream>
#include <sstream>
#include <deque>

using namespace sgct_core;

sgct::Engine *  sgct::Engine::mThis     = NULL;

#ifdef GLEW_MX
GLEWContext * glewGetContext();
#endif

/*!
This is the only valid constructor that also initiates [GLFW](http://www.glfw.org/). Command line parameters are used to load a configuration file and settings.
*/
sgct::Engine::Engine( int& argc, char**& argv )
{
	//init pointers
	mThis = this;
	mNetworkConnections = NULL;
	mConfig = NULL;
	mRunMode = Default_Mode;

	//init function pointers
	mDrawFn = NULL;
	mPreSyncFn = NULL;
	mPostSyncPreDrawFn = NULL;
	mPostDrawFn = NULL;
	mInitOGLFn = NULL;
	mClearBufferFn = NULL;
	mCleanUpFn = NULL;
	mInternalRenderFn = NULL;
	mNetworkCallbackFn = NULL;
	mKeyboardCallbackFn = NULL;
	mCharCallbackFn = NULL;
	mMouseButtonCallbackFn = NULL;
	mMousePosCallbackFn = NULL;
	mMouseWheelCallbackFn = NULL;

	mTerminate = false;
	mIgnoreSync = false;
	mRenderingOffScreen = false;

	localRunningMode = NetworkManager::NotLocal;

	currentViewportCoords[0] = 0;
	currentViewportCoords[1] = 0;
	currentViewportCoords[2] = 640;
	currentViewportCoords[3] = 480;

	//FBO stuff
	mFrameBuffers[0] = 0;
	mFrameBuffers[1] = 0;
	mMultiSampledFrameBuffers[0] = 0;
	mMultiSampledFrameBuffers[1] = 0;
	mFrameBufferTextures[0] = 0;
	mFrameBufferTextures[1] = 0;
	mRenderBuffers[0] = 0;
	mRenderBuffers[1] = 0;
	mDepthBuffers[0] = 0;
	mDepthBuffers[1] = 0;
	mFBOMode = MultiSampledFBO;

	for(unsigned int i=0; i<MAX_UNIFORM_LOCATIONS; i++)
		mShaderLocs[i] = -1;

	mAspectRatio = 1.0f;

	// Initialize GLFW
	if( !glfwInit() )
	{
		mTerminate = true;
		return;
	}

    NetworkManager::gMutex = createMutex();
	NetworkManager::gSyncMutex = createMutex();
    NetworkManager::gCond = createCondition();

    if(NetworkManager::gMutex == NULL ||
		NetworkManager::gSyncMutex == NULL ||
		NetworkManager::gCond == NULL)
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

	getWindowPtr()->useQuadbuffer( ClusterManager::Instance()->getThisNodePtr()->stereo == ClusterManager::Active );

	//disable MSAA if FXAA is in use
	if( SGCTSettings::Instance()->useFXAA() &&
		ClusterManager::Instance()->getThisNodePtr()->stereo <= ClusterManager::Active)
	{
		ClusterManager::Instance()->getThisNodePtr()->numberOfSamples = 1;
	}
	else //for glsl stereo types
		SGCTSettings::Instance()->setFXAA(false);

	if( ClusterManager::Instance()->getThisNodePtr()->isUsingFisheyeRendering() )
	{
		mFBOMode = CubeMapFBO;
		mActiveFrustum = Frustum::Mono;
		mClearColor[3] = 1.0f; //reflections of alpha will be white in cube map, therefore disable alpha
		//create the cube mapped viewports
		ClusterManager::Instance()->getThisNodePtr()->generateCubeMapViewports();
	}
	else
	{
		int antiAliasingSamples = ClusterManager::Instance()->getThisNodePtr()->numberOfSamples;
		if( antiAliasingSamples > 1 && mFBOMode == NoFBO ) //if multisample is used
			glfwOpenWindowHint( GLFW_FSAA_SAMPLES, antiAliasingSamples );
		else if( antiAliasingSamples < 2 && mFBOMode == MultiSampledFBO ) //on sample or less => no multisampling
			mFBOMode = RegularFBO;
	}

	/*glfwOpenWindowHint( GLFW_OPENGL_VERSION_MAJOR, 3 );
    glfwOpenWindowHint( GLFW_OPENGL_VERSION_MINOR, 2 );
	glfwOpenWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);*/

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

    /*
        Swap inerval:

        0 = vertical sync off
        1 = wait for vertical sync
        2 = fix when using swapgroups in xp and running half the framerate
    */

    glfwSwapInterval( ClusterManager::Instance()->getThisNodePtr()->swapInterval );

	//Also join swap group if enabled
	getWindowPtr()->init();

	getWindowPtr()->setWindowTitle( getBasicInfo() );

	waitForAllWindowsInSwapGroupToOpen();

	return true;
}

/*!
Initiates OpenGL.
*/
void sgct::Engine::initOGL()
{
	//Get OpenGL version
	int version[3];
	glfwGetGLVersion( &version[0], &version[1], &version[2] );
	sgct::MessageHandler::Instance()->print("OpenGL version %d.%d.%d\n", version[0], version[1], version[2]);

	if( !GLEW_ARB_texture_non_power_of_two )
	{
		sgct::MessageHandler::Instance()->print("Warning! Only power of two textures are supported!\n");
	}

	if( !GLEW_EXT_framebuffer_object )
	{
		sgct::MessageHandler::Instance()->print("Warning! Frame buffer objects are not supported!\n");
		mFBOMode = NoFBO;
	}

	createFBOs();
	loadShaders();
	mStatistics.initVBO();

	//load overlays if any
	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();
	for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
		tmpNode->getViewport(i)->loadData();

	if( mInitOGLFn != NULL )
		mInitOGLFn();

	calculateFrustums();
	mInternalRenderFn = &Engine::draw;

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

	//init swap group barrier when ready to render
	sgct::MessageHandler::Instance()->print("Joining swap barrier if enabled...\n");
	getWindowPtr()->setBarrier(true);

	sgct::MessageHandler::Instance()->print("Reseting swap group frame number...\n");
	getWindowPtr()->resetSwapGroupFrameNumber();

	//check for errors
	checkForOGLErrors();

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
	if(mFBOMode != NoFBO && GLEW_EXT_framebuffer_object)
	{
		sgct::MessageHandler::Instance()->print("Releasing OpenGL buffers...\n");
		glDeleteFramebuffers(2,	&mFrameBuffers[0]);
		if(mFBOMode == MultiSampledFBO)
			glDeleteFramebuffers(2,	&mMultiSampledFrameBuffers[0]);
		glDeleteTextures(2,			&mFrameBufferTextures[0]);
		glDeleteRenderbuffers(2, &mRenderBuffers[0]);
		glDeleteRenderbuffers(2, &mDepthBuffers[0]);
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

    sgct::MessageHandler::Instance()->print("Destroying network mutex...\n");
	if( NetworkManager::gMutex != NULL )
	{
		destroyMutex( NetworkManager::gMutex );
		NetworkManager::gMutex = NULL;
	}

	sgct::MessageHandler::Instance()->print("Destroying sync mutex...\n");
	if( NetworkManager::gSyncMutex != NULL )
	{
		destroyMutex( NetworkManager::gSyncMutex );
		NetworkManager::gSyncMutex = NULL;
	}

	sgct::MessageHandler::Instance()->print("Destroying condition...\n");
	if( NetworkManager::gCond != NULL )
	{
		destroyCond( NetworkManager::gCond );
		NetworkManager::gCond = NULL;
	}

	sgct::MessageHandler::Instance()->print("Destroying message handler...\n");
	MessageHandler::Destroy();

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
	mInternalRenderFn = NULL;
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
2. PostStage, locks master until slaves have rendered to the backbuffer
*/
void sgct::Engine::frameSyncAndLock(sgct::Engine::SyncStage stage)
{
	static double syncTime = 0.0;

	if(mIgnoreSync)
		return;

	double t0 = glfwGetTime();

	if( stage == PreStage )
	{
		mNetworkConnections->sync();

		//run only on clients/slaves
		if( !mNetworkConnections->isComputerServer() ) //not server
		{
			glfwLockMutex( NetworkManager::gSyncMutex );
			while(mNetworkConnections->isRunning() && mRunning)
			{
				if( mNetworkConnections->isSyncComplete() )
						break;

				//release lock once per second
				glfwWaitCond( NetworkManager::gCond,
					NetworkManager::gSyncMutex,
					1.0 );
			}

			glfwUnlockMutex( NetworkManager::gSyncMutex );
		}

		syncTime = glfwGetTime() - t0;
	}
	else //post stage
	{
		if( mNetworkConnections->isComputerServer() &&
			mConfig->isMasterSyncLocked() &&
			/*localRunningMode == NetworkManager::NotLocal &&*/
			!getWindowPtr()->isBarrierActive() )//post stage
		{
			glfwLockMutex( NetworkManager::gSyncMutex );
			while(mNetworkConnections->isRunning() && mRunning)
			{
				if( mNetworkConnections->isSyncComplete() )
						break;

				glfwWaitCond( NetworkManager::gCond,
					NetworkManager::gSyncMutex,
					1.0 );
			}

			glfwUnlockMutex( NetworkManager::gSyncMutex );
			syncTime += glfwGetTime() - t0;
		}

		mStatistics.setSyncTime(syncTime);
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
		calculateFPS(startFrameTime);

		glLineWidth(1.0);
		mShowWireframe ? glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ) : glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

		//check if re-size needed
		if( getWindowPtr()->isWindowResized() )
			resizeFBOs();

		//rendering offscreen if using FBOs
		mRenderingOffScreen = (mFBOMode != NoFBO);

		SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();
		SGCTUser * usrPtr = ClusterManager::Instance()->getUserPtr();

		//if fisheye rendering is used then render the cubemap
		if( mFBOMode == CubeMapFBO )
		{
			renderFisheye();
		}
		else
		{
			//if any stereo type (except passive) then set frustum mode to left eye
			mActiveFrustum = tmpNode->stereo != static_cast<int>(ClusterManager::NoStereo) ? Frustum::StereoLeftEye : Frustum::Mono;
			setRenderTarget(LeftEyeBuffer); //Set correct render target (Backbuffer, FBO etc..)

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
					(this->*mInternalRenderFn)();
				}
			}

			//render right eye view port(s)
			if( tmpNode->stereo != ClusterManager::NoStereo )
			{
				mActiveFrustum = Frustum::StereoRightEye;
				setRenderTarget(RightEyeBuffer); //Set correct render target (Backbuffer, FBO etc..)

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
						(this->*mInternalRenderFn)();
					}
				}
			}

			//restore polygon mode
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}

		//updates render targets and draw texture(s)
		updateRenderingTargets();

		mRenderingOffScreen = false;

		//draw viewport overlays if any
		drawOverlays();

        double endFrameTime = glfwGetTime();
		mStatistics.setDrawTime(endFrameTime - startFrameTime);

		//run post frame actions
		if( mPostDrawFn != NULL )
			mPostDrawFn();

		//draw info & stats
		//the cubemap viewports are all the same so it makes no sense to render everything several times
		//therefore just loop one iteration in that case.
		std::size_t numberOfIterations = ( mFBOMode == CubeMapFBO ) ? 1 : tmpNode->getNumberOfViewports();
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

        updateTimers( endFrameTime );

		//take screenshot
		if( mTakeScreenshot )
			captureBuffer();

#ifdef __SGCT_DEBUG__
		//check for errors
		checkForOGLErrors();
#endif

		//wait for nodes render before swapping
		frameSyncAndLock(PostStage);

		//swap frame id to keep track of sync
		mNetworkConnections->swapData();

		// Swap front and back rendering buffers
		glfwSwapBuffers();

		// Check if ESC key was pressed or window was closed
		mRunning = !glfwGetKey( GLFW_KEY_ESC ) && glfwGetWindowParam( GLFW_OPENED ) && !mTerminate;

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
	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 95, "Node ip: %s (%s)",
		tmpNode->ip.c_str(),
		mNetworkConnections->isComputerServer() ? "master" : "slave");
	glColor4f(0.8f,0.8f,0.0f,1.0f);
	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 80, "Frame rate: %.3f Hz, frame: %llu", mStatistics.getAvgFPS(), mFrameCounter);
	glColor4f(0.8f,0.0f,0.8f,1.0f);
	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 65, "Draw time: %.2f ms", getDrawTime()*1000.0);
	glColor4f(0.0f,0.8f,0.8f,1.0f);
	sgct_text::print(sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 50, "Sync time (size: %d, comp. ratio: %.3f): %.2f ms",
		SharedData::Instance()->getUserDataSize(),
		SharedData::Instance()->getCompressionRatio(),
		getSyncTime()*1000.0);
	glColor4f(0.8f,0.8f,0.8f,1.0f);
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
		sgct_text::print( sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 110, "Active eye: Left");
		glDrawBuffer(GL_BACK_RIGHT);
		sgct_text::print( sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 110, "Active eye:          Right");
		glDrawBuffer(GL_BACK);
	}
	else //if passive stereo
	{
		if( tmpNode->getCurrentViewport()->getEye() == Frustum::StereoLeftEye )
		{
			sgct_text::print( sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 110, "Active eye: Left");
		}
		else if( tmpNode->getCurrentViewport()->getEye() == Frustum::StereoRightEye )
		{
			sgct_text::print( sgct_text::FontManager::Instance()->GetFont( "SGCTFont", mConfig->getFontSize() ), 100, 110, "Active eye:          Right");
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

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	Viewport * tmpVP = ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport();
	glLoadMatrixf( glm::value_ptr(tmpVP->getProjectionMatrix(mActiveFrustum)) );

	glMatrixMode(GL_MODELVIEW);

	glLoadMatrixf( glm::value_ptr( getSceneTransform() ) );

	if( mDrawFn != NULL )
	{
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
	}
}

/*!
	Draw viewport overlays if there are any.
*/
void sgct::Engine::drawOverlays()
{
	glDrawBuffer(GL_BACK); //draw into both back buffers

	sgct_core::SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

	std::size_t numberOfIterations = ( mFBOMode == CubeMapFBO ) ? 1 : tmpNode->getNumberOfViewports();
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

			gluOrtho2D(-1.0, 1.0, -1.0, 1.0);

			glMatrixMode(GL_MODELVIEW);

			glPushAttrib( GL_ALL_ATTRIB_BITS );
			glDisable(GL_LIGHTING);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glLoadIdentity();

			glColor4f(1.0f,1.0f,1.0f,1.0f);

			glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByIndex( tmpVP->getOverlayTextureIndex() ) );

			if( mFBOMode == CubeMapFBO )
			{
				glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glEnableClientState(GL_VERTEX_ARRAY);
				glInterleavedArrays(GL_T2F_V3F, 0, mFisheyeQuadVerts);
				glDrawArrays(GL_QUADS, 0, 4);
				glPopClientAttrib();
			}
			else
			{
				glBegin(GL_QUADS);
				glTexCoord2d(0.0, 0.0);	glVertex2d(-1.0, -1.0);
				glTexCoord2d(0.0, 1.0);	glVertex2d(-1.0, 1.0);
				glTexCoord2d(1.0, 1.0);	glVertex2d(1.0, 1.0);
				glTexCoord2d(1.0, 0.0);	glVertex2d(1.0, -1.0);
				glEnd();
			}
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
void sgct::Engine::setRenderTarget(FBOBufferIndexes bi)
{
	if(mFBOMode != NoFBO)
	{
		//un-bind texture
		glBindTexture(GL_TEXTURE_2D, 0);

		if(mFBOMode == MultiSampledFBO)
			glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffers[ bi ]);
		else
			glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[ bi ]);

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
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//enter ortho mode
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glPushMatrix();
	gluOrtho2D(0.0, 1.0, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);

	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT );
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//clear buffers
	mActiveFrustum = tmpNode->stereo == ClusterManager::Active ? Frustum::StereoLeftEye : Frustum::Mono;
	setAndClearBuffer(BackBufferBlack);

	glLoadIdentity();

	glViewport (0, 0, getWindowPtr()->getHResolution(), getWindowPtr()->getVResolution());

	if( tmpNode->stereo > ClusterManager::Active )
	{
		switch(tmpNode->stereo)
		{
		case ClusterManager::Anaglyph_Red_Cyan:
			sgct::ShaderManager::Instance()->bindShader( "Anaglyph_Red_Cyan" );
			break;

		case ClusterManager::Anaglyph_Amber_Blue:
			sgct::ShaderManager::Instance()->bindShader( "Anaglyph_Amber_Blue" );
			break;

		case ClusterManager::Anaglyph_Red_Cyan_Wimmer:
			sgct::ShaderManager::Instance()->bindShader( "Anaglyph_Red_Cyan_Wimmer" );
			break;

		case ClusterManager::Checkerboard:
			sgct::ShaderManager::Instance()->bindShader( "Checkerboard" );
			break;

		case ClusterManager::Checkerboard_Inverted:
			sgct::ShaderManager::Instance()->bindShader( "Checkerboard_Inverted" );
			break;
		}

		glUniform1i( mShaderLocs[LeftTex], 0);
		glUniform1i( mShaderLocs[RightTex], 1);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[0]);
		glEnable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[1]);
		glEnable(GL_TEXTURE_2D);

		for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
			tmpNode->getViewport(i)->renderMesh();
		sgct::ShaderManager::Instance()->unBindShader();
	}
	else
	{
		glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
		glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[0]);
		glEnable(GL_TEXTURE_2D);

		if( SGCTSettings::Instance()->useFXAA() )
		{
			sgct::ShaderManager::Instance()->bindShader( "FXAA" );
			glUniform1f( mShaderLocs[SizeX], static_cast<float>(getWindowPtr()->getHFramebufferResolution()) );
			glUniform1f( mShaderLocs[SizeY], static_cast<float>(getWindowPtr()->getVFramebufferResolution()) );
			glUniform1i( mShaderLocs[FXAATexture], 0 );
		}

		for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
			tmpNode->getViewport(i)->renderMesh();

		if( SGCTSettings::Instance()->useFXAA() )
			sgct::ShaderManager::Instance()->unBindShader();
	}

	//render right eye in active stereo mode
	if( tmpNode->stereo == ClusterManager::Active )
	{
		//clear buffers
		mActiveFrustum = Frustum::StereoRightEye;
		setAndClearBuffer(BackBufferBlack);

		glLoadIdentity();

		glViewport (0, 0, getWindowPtr()->getHResolution(), getWindowPtr()->getVResolution());

		glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[1]);

		if( SGCTSettings::Instance()->useFXAA() )
		{
			sgct::ShaderManager::Instance()->bindShader( "FXAA" );
			glUniform1f( mShaderLocs[SizeX], static_cast<float>(getWindowPtr()->getHFramebufferResolution()) );
			glUniform1f( mShaderLocs[SizeY], static_cast<float>(getWindowPtr()->getVFramebufferResolution()) );
			glUniform1i( mShaderLocs[FXAATexture], 0 );
		}

		for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
			tmpNode->getViewport(i)->renderMesh();

		if( SGCTSettings::Instance()->useFXAA() )
			sgct::ShaderManager::Instance()->unBindShader();
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
void sgct::Engine::renderFisheye()
{
	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[CubeMapBuffer] );

	//iterate the cube sides
	for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
	{
		tmpNode->setCurrentViewport(i);

		if( tmpNode->getCurrentViewport()->isEnabled() )
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mFrameBufferTextures[ CubeMapBuffer ], 0);

			setAndClearBuffer(RenderToTexture);
			//render
			(this->*mInternalRenderFn)();
		}
	}//end for

	//restore polygon mode
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

	//bind fisheye target FBO
	glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[FishEyeBuffer] );
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

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glViewport(0, 0, getWindowPtr()->getHFramebufferResolution(), getWindowPtr()->getVFramebufferResolution());

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	//if for some reson the active texture has been reset
	glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
	glEnable(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, mFrameBufferTextures[CubeMapBuffer]);

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	sgct::ShaderManager::Instance()->bindShader( "Fisheye" );
	glUniform1i( mShaderLocs[Cubemap], 0);
	glUniform1f( mShaderLocs[FishEyeHalfFov], glm::radians<float>(SGCTSettings::Instance()->getFisheyeFOV()/2.0f) );

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glInterleavedArrays(GL_T2F_V3F, 0, mFisheyeQuadVerts);
	glDrawArrays(GL_QUADS, 0, 4);

	sgct::ShaderManager::Instance()->unBindShader();

	glPopClientAttrib();
	glPopAttrib();
	glPopMatrix();

	//unbind FBO
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/*!
	This function updates the renderingtargets and draws the rendered texture.
*/
void sgct::Engine::updateRenderingTargets()
{
	//copy AA-buffer to "regular"/non-AA buffer
	if(mFBOMode == MultiSampledFBO)
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mMultiSampledFrameBuffers[0]); // source
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFrameBuffers[0]); // dest
		glBlitFramebuffer(
			0, 0, getWindowPtr()->getHFramebufferResolution(), getWindowPtr()->getVFramebufferResolution(),
			0, 0, getWindowPtr()->getHFramebufferResolution(), getWindowPtr()->getVFramebufferResolution(),
			GL_COLOR_BUFFER_BIT, GL_NEAREST);

		//copy right buffers if used
		SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();
		if( tmpNode->stereo != ClusterManager::NoStereo )
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, mMultiSampledFrameBuffers[1]); // source
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFrameBuffers[1]); // dest
			glBlitFramebuffer(
				0, 0, getWindowPtr()->getHFramebufferResolution(), getWindowPtr()->getVFramebufferResolution(),
				0, 0, getWindowPtr()->getHFramebufferResolution(), getWindowPtr()->getVFramebufferResolution(),
				GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}

		//Draw FBO texture here
		renderFBOTexture();
	}
	else if(mFBOMode == RegularFBO || mFBOMode == CubeMapFBO)
	{
		renderFBOTexture();
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

	//create FXAA shaders
	sgct::ShaderManager::Instance()->addShader("FXAA", sgct_core::shaders::FXAA_Vert_Shader,
		sgct_core::shaders::FXAA_FRAG_Shader, ShaderManager::SHADER_SRC_STRING );
	sgct::ShaderManager::Instance()->bindShader( "FXAA" );

	mShaderLocs[SizeX] = sgct::ShaderManager::Instance()->getShader( "FXAA" ).getUniformLocation( "rt_w" );
	glUniform1f( mShaderLocs[SizeX], static_cast<float>(getWindowPtr()->getHFramebufferResolution()) );

	mShaderLocs[SizeY] = sgct::ShaderManager::Instance()->getShader( "FXAA" ).getUniformLocation( "rt_h" );
	glUniform1f( mShaderLocs[SizeY], static_cast<float>(getWindowPtr()->getVFramebufferResolution()) );

	mShaderLocs[FXAASubPixShift] = sgct::ShaderManager::Instance()->getShader( "FXAA" ).getUniformLocation( "FXAA_SUBPIX_SHIFT" );
	glUniform1f( mShaderLocs[FXAASubPixShift], 0.25f );

	mShaderLocs[FXAASpanMax] = sgct::ShaderManager::Instance()->getShader( "FXAA" ).getUniformLocation( "FXAA_SPAN_MAX" );
	glUniform1f( mShaderLocs[FXAASpanMax], 8.0f );

	mShaderLocs[FXAARedMul] = sgct::ShaderManager::Instance()->getShader( "FXAA" ).getUniformLocation( "FXAA_REDUCE_MUL" );
	glUniform1f( mShaderLocs[FXAARedMul], 1.0f/8.0f );

	mShaderLocs[FXAAOffset] = sgct::ShaderManager::Instance()->getShader( "FXAA" ).getUniformLocation( "vx_offset" );
	glUniform1f( mShaderLocs[FXAAOffset], 0.0f );

	mShaderLocs[FXAATexture] = sgct::ShaderManager::Instance()->getShader( "FXAA" ).getUniformLocation( "tex0" );
	glUniform1i( mShaderLocs[FXAATexture], 0 );

	sgct::ShaderManager::Instance()->unBindShader();


	if( mFBOMode == CubeMapFBO )
	{
		sgct::ShaderManager::Instance()->addShader("Fisheye", sgct_core::shaders::Base_Vert_Shader, sgct_core::shaders::Fisheye_Frag_Shader, ShaderManager::SHADER_SRC_STRING );
		sgct::ShaderManager::Instance()->bindShader( "Fisheye" );
		mShaderLocs[Cubemap] = sgct::ShaderManager::Instance()->getShader( "Fisheye" ).getUniformLocation( "cubemap" );
		glUniform1i( mShaderLocs[Cubemap], 0 );
		mShaderLocs[FishEyeHalfFov] = sgct::ShaderManager::Instance()->getShader( "Fisheye" ).getUniformLocation( "halfFov" );
		glUniform1f( mShaderLocs[FishEyeHalfFov], glm::half_pi<float>() );
		sgct::ShaderManager::Instance()->unBindShader();
	}
	else //stereo not supported using cubemap
	{
		if( tmpNode->stereo == ClusterManager::Anaglyph_Red_Cyan )
		{
			sgct::ShaderManager::Instance()->addShader("Anaglyph_Red_Cyan", sgct_core::shaders::Anaglyph_Vert_Shader, sgct_core::shaders::Anaglyph_Red_Cyan_Frag_Shader, ShaderManager::SHADER_SRC_STRING );
			sgct::ShaderManager::Instance()->bindShader( "Anaglyph_Red_Cyan" );
			mShaderLocs[LeftTex] = sgct::ShaderManager::Instance()->getShader( "Anaglyph_Red_Cyan" ).getUniformLocation( "LeftTex" );
			mShaderLocs[RightTex] = sgct::ShaderManager::Instance()->getShader( "Anaglyph_Red_Cyan" ).getUniformLocation( "RightTex" );
			glUniform1i( mShaderLocs[LeftTex], 0 );
			glUniform1i( mShaderLocs[RightTex], 1 );
			sgct::ShaderManager::Instance()->unBindShader();
		}
		else if( tmpNode->stereo == ClusterManager::Anaglyph_Amber_Blue )
		{
			sgct::ShaderManager::Instance()->addShader("Anaglyph_Amber_Blue", sgct_core::shaders::Anaglyph_Vert_Shader, sgct_core::shaders::Anaglyph_Amber_Blue_Frag_Shader, ShaderManager::SHADER_SRC_STRING );
			sgct::ShaderManager::Instance()->bindShader( "Anaglyph_Amber_Blue" );
			mShaderLocs[LeftTex] = sgct::ShaderManager::Instance()->getShader( "Anaglyph_Amber_Blue" ).getUniformLocation( "LeftTex" );
			mShaderLocs[RightTex] = sgct::ShaderManager::Instance()->getShader( "Anaglyph_Amber_Blue" ).getUniformLocation( "RightTex" );
			glUniform1i( mShaderLocs[LeftTex], 0 );
			glUniform1i( mShaderLocs[RightTex], 1 );
			sgct::ShaderManager::Instance()->unBindShader();
		}
		else if( tmpNode->stereo == ClusterManager::Anaglyph_Red_Cyan_Wimmer )
		{
			sgct::ShaderManager::Instance()->addShader("Anaglyph_Red_Cyan_Wimmer", sgct_core::shaders::Anaglyph_Vert_Shader, sgct_core::shaders::Anaglyph_Red_Cyan_Frag_Shader_Wimmer, ShaderManager::SHADER_SRC_STRING );
			sgct::ShaderManager::Instance()->bindShader( "Anaglyph_Red_Cyan_Wimmer" );
			mShaderLocs[LeftTex] = sgct::ShaderManager::Instance()->getShader( "Anaglyph_Red_Cyan_Wimmer" ).getUniformLocation( "LeftTex" );
			mShaderLocs[RightTex] = sgct::ShaderManager::Instance()->getShader( "Anaglyph_Red_Cyan_Wimmer" ).getUniformLocation( "RightTex" );
			glUniform1i( mShaderLocs[LeftTex], 0 );
			glUniform1i( mShaderLocs[RightTex], 1 );
			sgct::ShaderManager::Instance()->unBindShader();
		}
		else if( tmpNode->stereo == ClusterManager::Checkerboard )
		{
			sgct::ShaderManager::Instance()->addShader("Checkerboard", sgct_core::shaders::Anaglyph_Vert_Shader, sgct_core::shaders::CheckerBoard_Frag_Shader, ShaderManager::SHADER_SRC_STRING );
			sgct::ShaderManager::Instance()->bindShader( "Checkerboard" );
			mShaderLocs[LeftTex] = sgct::ShaderManager::Instance()->getShader( "Checkerboard" ).getUniformLocation( "LeftTex" );
			mShaderLocs[RightTex] = sgct::ShaderManager::Instance()->getShader( "Checkerboard" ).getUniformLocation( "RightTex" );
			glUniform1i( mShaderLocs[LeftTex], 0 );
			glUniform1i( mShaderLocs[RightTex], 1 );
			sgct::ShaderManager::Instance()->unBindShader();
		}
		else if( tmpNode->stereo == ClusterManager::Checkerboard_Inverted )
		{
			sgct::ShaderManager::Instance()->addShader("Checkerboard_Inverted", sgct_core::shaders::Anaglyph_Vert_Shader, sgct_core::shaders::CheckerBoard_Inverted_Frag_Shader, ShaderManager::SHADER_SRC_STRING );
			sgct::ShaderManager::Instance()->bindShader( "Checkerboard_Inverted" );
			mShaderLocs[LeftTex] = sgct::ShaderManager::Instance()->getShader( "Checkerboard_Inverted" ).getUniformLocation( "LeftTex" );
			mShaderLocs[RightTex] = sgct::ShaderManager::Instance()->getShader( "Checkerboard_Inverted" ).getUniformLocation( "RightTex" );
			glUniform1i( mShaderLocs[LeftTex], 0 );
			glUniform1i( mShaderLocs[RightTex], 1 );
			sgct::ShaderManager::Instance()->unBindShader();
		}
	}
}

/*!
	This function creates FBOs if they are supported.
	This is done in the initOGL function.
*/
void sgct::Engine::createFBOs()
{
	mAspectRatio = static_cast<float>( getWindowPtr()->getHFramebufferResolution() ) /
		static_cast<float>( getWindowPtr()->getVFramebufferResolution() );

	if(mFBOMode != NoFBO)
	{
		glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT);
		glEnable(GL_TEXTURE_2D);
		glGenFramebuffers(2,		&mFrameBuffers[0]);
		glGenRenderbuffers(2,		&mDepthBuffers[0]);
		glGenRenderbuffers(2,		&mRenderBuffers[0]);
		glGenTextures(2,			&mFrameBufferTextures[0]);

		if( !GLEW_EXT_framebuffer_multisample )
		{
			sgct::MessageHandler::Instance()->print("Warning! FBO multisampling is not supported!\n");
			mFBOMode = RegularFBO;
		}
		else
		{
			//create a multisampled buffer
			if(mFBOMode == MultiSampledFBO)
			{
				GLint MaxSamples;
				glGetIntegerv(GL_MAX_SAMPLES, &MaxSamples);
				if( ClusterManager::Instance()->getThisNodePtr()->numberOfSamples > MaxSamples )
					ClusterManager::Instance()->getThisNodePtr()->numberOfSamples = MaxSamples;
				if( MaxSamples < 2 )
					ClusterManager::Instance()->getThisNodePtr()->numberOfSamples = 0;

				sgct::MessageHandler::Instance()->print("Max samples supported: %d\n", MaxSamples);

				//generate the multisample buffer
				glGenFramebuffers(2, &mMultiSampledFrameBuffers[0]);
			}
		}

		if(mFBOMode == CubeMapFBO)
		{
			GLint cubeMapRes = SGCTSettings::Instance()->getCubeMapResolution();
			GLint MaxCubeMapRes;
			glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &MaxCubeMapRes);
			if(cubeMapRes > MaxCubeMapRes)
			{
				cubeMapRes = MaxCubeMapRes;
				sgct::MessageHandler::Instance()->print("Info: Cubemap size set to max size: %d\n", MaxCubeMapRes);
			}

			//set up texture target
			glBindTexture(GL_TEXTURE_CUBE_MAP, mFrameBufferTextures[ CubeMapBuffer ]);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[CubeMapBuffer] );

			for (int i = 0; i < 6; ++i)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, cubeMapRes, cubeMapRes, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

			//attach one of the faces
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, mFrameBufferTextures[ CubeMapBuffer ], 0);
			//set up depth buffer
			glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffers[ CubeMapBuffer ]);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cubeMapRes, cubeMapRes);
			//Attach depth buffer to FBO
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffers[ CubeMapBuffer ]);

			//Does the GPU support current FBO configuration?
			if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
				MessageHandler::Instance()->print("Engine: Something went wrong creating the fisheye cubemap FBO!\n");
			else //render faces
			{
				glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[CubeMapBuffer] );
				for(int i=0; i<6; i++)
				{
					glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mFrameBufferTextures[ CubeMapBuffer ], 0);
					glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
					glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
				}

				MessageHandler::Instance()->print("Engine info: Initial cube map rendered.\n");
			}

			//setup the fisheye FBO
			glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[FishEyeBuffer]);
			glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[FishEyeBuffer]);
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); //must be linear if warping, blending or fix resolution is used
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, getWindowPtr()->getHFramebufferResolution(), getWindowPtr()->getVFramebufferResolution(), 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
			//bind the fbo and assign the frame buffer
			glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[FishEyeBuffer] );
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFrameBufferTextures[FishEyeBuffer], 0);
			//set up depth buffer
			glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffers[ FishEyeBuffer ]);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, getWindowPtr()->getHFramebufferResolution(), getWindowPtr()->getVFramebufferResolution());
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffers[ FishEyeBuffer ]);

			//Does the GPU support current FBO configuration?
			if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
				MessageHandler::Instance()->print("Engine: Something went wrong creating the fisheye target FBO!\n");

			//set ut the fisheye geometry etc.
			initFisheye();
		}
		else
		{
			for( int i=0; i<2; i++ )
			{
				//setup render buffer
				(mFBOMode == MultiSampledFBO) ?
					glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffers[i]) :
					glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[i]);
				glBindRenderbuffer(GL_RENDERBUFFER, mRenderBuffers[i]);

				//Set render buffer storage depending on if buffer is multisampled or not
				(mFBOMode == MultiSampledFBO) ?
					glRenderbufferStorageMultisample(GL_RENDERBUFFER, ClusterManager::Instance()->getThisNodePtr()->numberOfSamples, GL_RGBA, getWindowPtr()->getHFramebufferResolution(), getWindowPtr()->getVFramebufferResolution()) :
					glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, getWindowPtr()->getHFramebufferResolution(), getWindowPtr()->getVFramebufferResolution());
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, mRenderBuffers[i]);

				//setup depth buffer
				(mFBOMode == MultiSampledFBO) ?
					glBindFramebuffer(GL_FRAMEBUFFER, mMultiSampledFrameBuffers[i]) :
					glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[i]);
				glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffers[i]);

				(mFBOMode == MultiSampledFBO) ?
					glRenderbufferStorageMultisample(GL_RENDERBUFFER, ClusterManager::Instance()->getThisNodePtr()->numberOfSamples, GL_DEPTH_COMPONENT32, getWindowPtr()->getHFramebufferResolution(), getWindowPtr()->getVFramebufferResolution()):
					glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, getWindowPtr()->getHFramebufferResolution(), getWindowPtr()->getVFramebufferResolution());
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffers[i]);

				//setup non-multisample buffer
				glBindFramebuffer(GL_FRAMEBUFFER, mFrameBuffers[i]);
				glBindTexture(GL_TEXTURE_2D, mFrameBufferTextures[i]);
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); //must be linear if warping, blending or fix resolution is used
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, getWindowPtr()->getHFramebufferResolution(), getWindowPtr()->getVFramebufferResolution(), 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mFrameBufferTextures[i], 0);
			}
		}

		// Unbind / Go back to regular frame buffer rendering
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glPopAttrib();

		sgct::MessageHandler::Instance()->print("FBOs initiated successfully!\n");
	}
	else
	{
		SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

		//disable anaglyph & checkerboard stereo if FBOs are not used
		if( tmpNode->stereo > ClusterManager::Active )
			tmpNode->stereo = ClusterManager::NoStereo;
		sgct::MessageHandler::Instance()->print("Warning! FBO rendering is not supported or enabled!\nAnaglyph & checkerboard (DLP) stereo modes are disabled.\n");
	}

}

/*!
	This function resizes the FBOs when the window is resized to achive 1:1 pixel-texel mapping.
*/
void sgct::Engine::resizeFBOs()
{
	if(mFBOMode != NoFBO)
	{
		//delete all
		glDeleteFramebuffers(2,	&mFrameBuffers[0]);
		if(mFBOMode == MultiSampledFBO)
			glDeleteFramebuffers(2,	&mMultiSampledFrameBuffers[0]);
		glDeleteTextures(2,			&mFrameBufferTextures[0]);
		glDeleteRenderbuffers(2, &mRenderBuffers[0]);
		glDeleteRenderbuffers(2, &mDepthBuffers[0]);

		//init
		mFrameBuffers[0] = 0;
		mFrameBuffers[1] = 0;
		mMultiSampledFrameBuffers[0] = 0;
		mMultiSampledFrameBuffers[1] = 0;
		mFrameBufferTextures[0] = 0;
		mFrameBufferTextures[1] = 0;
		mRenderBuffers[0] = 0;
		mRenderBuffers[1] = 0;
		mDepthBuffers[0] = 0;
		mDepthBuffers[1] = 0;

		createFBOs();

		sgct::MessageHandler::Instance()->print("FBOs resized successfully!\n");
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
			glDrawBuffer(GL_BACK);
		else if( mActiveFrustum == Frustum::StereoLeftEye ) //if active left
			glDrawBuffer(GL_BACK_LEFT);
		else if( mActiveFrustum == Frustum::StereoRightEye ) //if active right
			glDrawBuffer(GL_BACK_RIGHT);
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
	This function is running in the same thread as the renderloop so calling this function for each frame will slow down rendering.
	Performance is improved by using SSD drives.

	The PNG images are saved as RGBA images with transparancy. Alpha is taken from the clear color alpha.
*/
void sgct::Engine::captureBuffer()
{
	double t0 = getTime();

	//allocate
	unsigned char * raw_img = (unsigned char*)malloc( sizeof(unsigned char)*( 4 * getWindowPtr()->getHFramebufferResolution() * getWindowPtr()->getVFramebufferResolution() ) );

	static int shotCounter = 0;
	char screenShotFilenameLeft[32];
	char screenShotFilenameRight[32];

	int thisNodeId = ClusterManager::Instance()->getThisNodeId();


#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if( shotCounter < 10 )
	{
		sprintf_s( screenShotFilenameLeft, sizeof(screenShotFilenameLeft), "sgct_node%d_000%d_l.png", thisNodeId, shotCounter);
		sprintf_s( screenShotFilenameRight, sizeof(screenShotFilenameRight), "sgct_node%d_000%d_r.png", thisNodeId, shotCounter);
	}
	else if( shotCounter < 100 )
	{
		sprintf_s( screenShotFilenameLeft, sizeof(screenShotFilenameLeft), "sgct_node%d_00%d_l.png", thisNodeId, shotCounter);
		sprintf_s( screenShotFilenameRight, sizeof(screenShotFilenameRight), "sgct_node%d_00%d_r.png", thisNodeId, shotCounter);
	}
	else if( shotCounter < 1000 )
	{
		sprintf_s( screenShotFilenameLeft, sizeof(screenShotFilenameLeft), "sgct_node%d_0%d_l.png", thisNodeId, shotCounter);
		sprintf_s( screenShotFilenameRight, sizeof(screenShotFilenameRight), "sgct_node%d_0%d_r.png", thisNodeId, shotCounter);
	}
	else if( shotCounter < 10000 )
	{
		sprintf_s( screenShotFilenameLeft, sizeof(screenShotFilenameLeft), "sgct_node%d_%d_l.png", thisNodeId, shotCounter);
		sprintf_s( screenShotFilenameRight, sizeof(screenShotFilenameRight), "sgct_node%d_%d_r.png", thisNodeId, shotCounter);
	}
#else
    if( shotCounter < 10 )
	{
		sprintf( screenShotFilenameLeft, "sgct_node%d_000%d_l.png", thisNodeId, shotCounter);
		sprintf( screenShotFilenameRight, "sgct_node%d_000%d_r.png", thisNodeId, shotCounter);
	}
	else if( shotCounter < 100 )
	{
		sprintf( screenShotFilenameLeft, "sgct_node%d_00%d_l.png", thisNodeId, shotCounter);
		sprintf( screenShotFilenameRight, "sgct_node%d_00%d_r.png", thisNodeId, shotCounter);
	}
	else if( shotCounter < 1000 )
	{
		sprintf( screenShotFilenameLeft, "sgct_node%d_0%d_l.png", thisNodeId, shotCounter);
		sprintf( screenShotFilenameRight, "sgct_node%d_0%d_r.png", thisNodeId, shotCounter);
	}
	else if( shotCounter < 10000 )
	{
		sprintf( screenShotFilenameLeft, "sgct_node%d_%d_l.png", thisNodeId, shotCounter);
		sprintf( screenShotFilenameRight, "sgct_node%d_%d_r.png", thisNodeId, shotCounter);
	}
#endif

	shotCounter++;

	glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

	if(mFBOMode != NoFBO)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, mFrameBuffers[0]);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, raw_img);
	}
	else if(tmpNode->stereo == ClusterManager::NoStereo)
	{
		glReadBuffer(GL_FRONT);
		glReadPixels(0, 0, getWindowPtr()->getHFramebufferResolution(), getWindowPtr()->getVFramebufferResolution(), GL_RGBA, GL_UNSIGNED_BYTE, raw_img);
	}
	else if(tmpNode->stereo == ClusterManager::Active)
	{
		glReadBuffer(GL_FRONT_LEFT);
		glReadPixels(0, 0, getWindowPtr()->getHFramebufferResolution(), getWindowPtr()->getVFramebufferResolution(), GL_RGBA, GL_UNSIGNED_BYTE, raw_img);
	}

	Image img;
	img.setChannels(4);
	img.setDataPtr( raw_img );
	img.setSize( getWindowPtr()->getHFramebufferResolution(), getWindowPtr()->getVFramebufferResolution() );
	img.savePNG(screenShotFilenameLeft);

	if( tmpNode->stereo != ClusterManager::NoStereo )
	{
		if(mFBOMode != NoFBO)
		{
			glBindTexture(GL_TEXTURE_2D, mFrameBuffers[1]);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, raw_img);
		}
		else if(tmpNode->stereo == ClusterManager::Active)
		{
			glReadBuffer(GL_FRONT_RIGHT);
			glReadPixels(0, 0, getWindowPtr()->getHFramebufferResolution(), getWindowPtr()->getVFramebufferResolution(), GL_RGBA, GL_UNSIGNED_BYTE, raw_img);
		}

		img.savePNG(screenShotFilenameRight);
	}

	img.cleanup();

	glPopAttrib();

	mTakeScreenshot = false;

	sgct::MessageHandler::Instance()->print("Screenshot %d completed in %fs\n", shotCounter, getTime()-t0 );
}

/*!
	This function waits for all windows to be created on the whole cluster in order to set the barrier (hardware swap-lock).
	Under some Nvida drivers the stability is improved by first join a swapgroup and then set the barrier then all windows in a swapgroup are created.
*/
void sgct::Engine::waitForAllWindowsInSwapGroupToOpen()
{
	sgct::MessageHandler::Instance()->print("Joining swap group if enabled/supported...\n");

	//Must wait until all nodes are running if using swap barrier
	if( getWindowPtr()->isUsingSwapGroups() && ClusterManager::Instance()->getNumberOfNodes() > 1)
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


		sgct::MessageHandler::Instance()->print("Waiting for all nodes to connect.");
		glfwSwapBuffers(); //render just black....

		glfwLockMutex( NetworkManager::gSyncMutex );
		while(mNetworkConnections->isRunning() &&
			!glfwGetKey( GLFW_KEY_ESC ) &&
			glfwGetWindowParam( GLFW_OPENED ) &&
			!mTerminate)
		{
			sgct::MessageHandler::Instance()->print(".");

			if(mNetworkConnections->areAllNodesConnected())
				break;

			glfwWaitCond( NetworkManager::gCond,
				NetworkManager::gSyncMutex,
				0.1 ); //wait maximum 0.1 sec per iteration

			// Swap front and back rendering buffers
			glfwSwapBuffers();
		}
		glfwUnlockMutex( NetworkManager::gSyncMutex );

		sgct::MessageHandler::Instance()->print("\n");
	}
	else
		sgct::MessageHandler::Instance()->print("Swapgroups (swap-lock) are disabled.\n");
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
		else if( strcmp(argv[i],"--Ignore-Sync") == 0 )
		{
			mIgnoreSync = true;
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--No-FBO") == 0 )
		{
			mFBOMode = NoFBO;
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--Regular-FBO") == 0 )
		{
			mFBOMode = RegularFBO;
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--MultiSampled-FBO") == 0 )
		{
			mFBOMode = MultiSampledFBO;
			argumentsToRemove.push_back(i);
			i++;
		}
		else if( strcmp(argv[i],"--FXAA") == 0 )
		{
			SGCTSettings::Instance()->setFXAA(true);
			argumentsToRemove.push_back(i);
			i++;
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

	if( mFBOMode == CubeMapFBO && vs != ScreenSpace )
	{
		int cmRes = sgct_core::SGCTSettings::Instance()->getCubeMapResolution();
		currentViewportCoords[0] = 0;
		currentViewportCoords[1] = 0;
		currentViewportCoords[2] = cmRes;
		currentViewportCoords[3] = cmRes;
	}
	else
	{
		if( vs == ScreenSpace || mFBOMode == NoFBO )
		{
			currentViewportCoords[0] =
				static_cast<int>( tmpNode->getCurrentViewport()->getX() * static_cast<double>(getWindowPtr()->getHResolution()));
			currentViewportCoords[1] =
				static_cast<int>( tmpNode->getCurrentViewport()->getY() * static_cast<double>(getWindowPtr()->getVResolution()));
			currentViewportCoords[2] =
				static_cast<int>( tmpNode->getCurrentViewport()->getXSize() * static_cast<double>(getWindowPtr()->getHResolution()));
			currentViewportCoords[3] =
				static_cast<int>( tmpNode->getCurrentViewport()->getYSize() * static_cast<double>(getWindowPtr()->getVResolution()));
		}
		else
		{
			currentViewportCoords[0] =
				static_cast<int>( tmpNode->getCurrentViewport()->getX() * static_cast<double>(getWindowPtr()->getHFramebufferResolution()));
			currentViewportCoords[1] =
				static_cast<int>( tmpNode->getCurrentViewport()->getY() * static_cast<double>(getWindowPtr()->getVFramebufferResolution()));
			currentViewportCoords[2] =
				static_cast<int>( tmpNode->getCurrentViewport()->getXSize() * static_cast<double>(getWindowPtr()->getHFramebufferResolution()));
			currentViewportCoords[3] =
				static_cast<int>( tmpNode->getCurrentViewport()->getYSize() * static_cast<double>(getWindowPtr()->getVFramebufferResolution()));
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
	float leftcrop		= SGCTSettings::Instance()->getFisheyeCropValue(SGCTSettings::Left);
	float rightcrop		= SGCTSettings::Instance()->getFisheyeCropValue(SGCTSettings::Right);
	float bottomcrop	= SGCTSettings::Instance()->getFisheyeCropValue(SGCTSettings::Bottom);
	float topcrop		= SGCTSettings::Instance()->getFisheyeCropValue(SGCTSettings::Top);

	float cropAspect = ((1.0f-2.0f * bottomcrop) + (1.0f-2.0f*topcrop)) / ((1.0f-2.0f*leftcrop) + (1.0f-2.0f*rightcrop));

	float x = 1.0f;
	float y = 1.0f;
	float aspect = mAspectRatio * cropAspect;
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
	mFisheyeQuadVerts[11] = 1.0f - topcrop;
	mFisheyeQuadVerts[12] = x;
	mFisheyeQuadVerts[13] = y;
	mFisheyeQuadVerts[14] = -1.0f;

	mFisheyeQuadVerts[15] = 1.0f - rightcrop;
	mFisheyeQuadVerts[16] = bottomcrop;
	mFisheyeQuadVerts[17] = x;
	mFisheyeQuadVerts[18] = -y;
	mFisheyeQuadVerts[19] = -1.0f;
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

const double & sgct::Engine::getDt()
{
	return mStatistics.getFrameTime();
}

const double & sgct::Engine::getDrawTime()
{
	return mStatistics.getDrawTime();
}

const double & sgct::Engine::getSyncTime()
{
	return mStatistics.getSyncTime();
}

void sgct::Engine::setNearAndFarClippingPlanes(float _near, float _far)
{
	mNearClippingPlaneDist = _near;
	mFarClippingPlaneDist = _far;
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
}

void sgct::Engine::decodeExternalControl(const char * receivedData, int receivedlength, int clientIndex)
{
	if(mNetworkCallbackFn != NULL && receivedlength > 0)
		mNetworkCallbackFn(receivedData, receivedlength, clientIndex);
}

void sgct::Engine::sendMessageToExternalControl(void * data, int length)
{
	if( mNetworkConnections->getExternalControlPtr() != NULL )
		mNetworkConnections->getExternalControlPtr()->sendData( data, length );
}

void sgct::Engine::sendMessageToExternalControl(const std::string msg)
{
	if( mNetworkConnections->getExternalControlPtr() != NULL )
		mNetworkConnections->getExternalControlPtr()->sendData( (void *)msg.c_str(), static_cast<int>(msg.size()) );
}

void sgct::Engine::setExternalControlBufferSize(unsigned int newSize)
{
	if( mNetworkConnections->getExternalControlPtr() != NULL )
		mNetworkConnections->getExternalControlPtr()->setBufferSize(newSize);
}

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

const char * sgct::Engine::getAAInfo()
{
    if( SGCTSettings::Instance()->useFXAA() && mFBOMode != NoFBO)
    #if (_MSC_VER >= 1400) //visual studio 2005 or later
        strcpy_s(aaInfo, sizeof(aaInfo), "FXAA");
    #else
        strcpy(aaInfo, "FXAA");
    #endif
    else
    {
        if( ClusterManager::Instance()->getThisNodePtr()->numberOfSamples > 1  && mFBOMode != RegularFBO )
        {
            #if (_MSC_VER >= 1400) //visual studio 2005 or later
            sprintf_s( aaInfo, sizeof(aaInfo), "MSAAx%d",
                ClusterManager::Instance()->getThisNodePtr()->numberOfSamples);
            #else
            sprintf( aaInfo, "MSAAx%d",
                ClusterManager::Instance()->getThisNodePtr()->numberOfSamples);
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

int sgct::Engine::getKey( const int &key )
{
	return glfwGetKey(key);
}

int sgct::Engine::getMouseButton( const int &button )
{
	return glfwGetMouseButton(button);
}

void sgct::Engine::getMousePos( int * xPos, int * yPos )
{
	glfwGetMousePos(xPos, yPos);
}

void sgct::Engine::setMousePos( const int &xPos, const int &yPos )
{
	glfwSetMousePos(xPos, yPos);
}

int sgct::Engine::getMouseWheel()
{
	return glfwGetMouseWheel();
}

void sgct::Engine::setMouseWheel( const int &pos )
{
	glfwSetMouseWheel(pos);
}

void sgct::Engine::setMousePointerVisibility( bool state )
{
	state ? glfwEnable( GLFW_MOUSE_CURSOR ) : glfwDisable( GLFW_MOUSE_CURSOR );
}

int sgct::Engine::getJoystickParam( const int &joystick, const int &param )
{
	return glfwGetJoystickParam(joystick,param);
}

int sgct::Engine::getJoystickAxes( const int &joystick, float * values, const int &numOfValues)
{
	return glfwGetJoystickPos( joystick, values, numOfValues );
}

int sgct::Engine::getJoystickButtons( const int &joystick, unsigned char * values, const int &numOfValues)
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
        sgct::MessageHandler::Instance()->print("There was no timer with id: %i", id);
    }
}

double sgct::Engine::getTime()
{
	return glfwGetTime();
}

GLFWmutex sgct::Engine::createMutex()
{
    return glfwCreateMutex();
}

GLFWcond sgct::Engine::createCondition()
{
    return glfwCreateCond();
}

void sgct::Engine::destroyCond(GLFWcond &cond)
{
    glfwDestroyCond(cond);
}

void sgct::Engine::destroyMutex(GLFWmutex &mutex)
{
    glfwDestroyMutex(mutex);
}

void sgct::Engine::lockMutex(GLFWmutex &mutex)
{
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Locking mutex...\n");
#endif
    if(mutex != NULL)
		glfwLockMutex(mutex);
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Done\n");
#endif
}

void sgct::Engine::unlockMutex(GLFWmutex &mutex)
{
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Unlocking mutex...\n");
#endif
	if(mutex != NULL)
		glfwUnlockMutex(mutex);
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Done\n");
#endif
}

void sgct::Engine::waitCond(GLFWcond &cond, GLFWmutex &mutex, double timeout)
{
    glfwWaitCond(cond, mutex, timeout);
}

void sgct::Engine::signalCond(GLFWcond &cond)
{
    glfwSignalCond(cond);
}
