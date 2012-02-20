#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/glfw.h>
#include "../include/sgct/Engine.h"
#include "../include/sgct/freetype.h"
#include "../include/sgct/FontManager.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/TextureManager.h"
#include "../include/sgct/SharedData.h"
#include "../include/sgct/ShaderManager.h"
#include "../include/sgct/NodeManager.h"
#include <math.h>
#include <sstream>

using namespace core_sgct;

sgct::Engine::Engine( int argc, char* argv[] )
{
	//init pointers
	mNetwork = NULL;
	mExternalControlNetwork = NULL;
	mWindow = NULL;
	mConfig = NULL;
	for( unsigned int i=0; i<3; i++)
		mFrustums[i] = NULL;

	//init function pointers
	mDrawFn = NULL;
	mPreDrawFn = NULL;
	mPostDrawFn = NULL;
	mInitOGLFn = NULL;
	mClearBufferFn = NULL;
	mInternalRenderFn = NULL;

	setClearBufferFunction( clearBuffer );
	nearClippingPlaneDist = 0.1f;
	farClippingPlaneDist = 100.0f;
	showInfo = false;
	showGraph = false;
	runningLocal = false;
	isServer = true;
	showWireframe = false;
	mTerminate = false;
	mThisClusterNodeId = -1;
	activeFrustum = Frustum::Mono;

	//parse needs to be before read config since the path to the XML is parsed here
	parseArguments( argc, argv );
}

bool sgct::Engine::init()
{
	// Initialize GLFW
	if( !glfwInit() )
		return false;
	mConfig = new ReadConfig( configFilename );
	if( !mConfig->isValid() ) //fatal error
	{
		sgct::MessageHandler::Instance()->print("Error in xml config file parsing.\n");
		return false;
	}
	if( !initNetwork() )
	{
		sgct::MessageHandler::Instance()->print("Network init error.\n");
		return false;
	}

	if( !initWindow() )
	{
		sgct::MessageHandler::Instance()->print("Window init error.\n");
		return false;
	}

	initOGL();

	//
	// Add fonts
	//
	if( !FontManager::Instance()->AddFont( "Verdana", "verdana.ttf" ) )
		FontManager::Instance()->GetFont( "Verdana", 14 );

	return true;
}

bool sgct::Engine::initNetwork()
{
	try
	{
		mNetwork = new SGCTNetwork();
	}
	catch( const char * err )
	{
		sgct::MessageHandler::Instance()->print("Network init error: %s\n", err);
		if(mNetwork != NULL)
			mNetwork->close();
		glfwTerminate();
		return false;
	}

	//check in cluster configuration if this node master or slave
	for(unsigned int i=0; i<NodeManager::Instance()->getNumberOfNodes(); i++)
	{
		//if this computer matches node[i]
		if( !runningLocal && mNetwork->matchAddress( NodeManager::Instance()->getNodePtr(i)->ip ) )
		{
			if( mNetwork->matchAddress( (*mConfig->getMasterIP()) ))
			{
				isServer = true;
				sgct::MessageHandler::Instance()->print("This computer is the network server.\n");
			}
			else
			{
				isServer = false;
				sgct::MessageHandler::Instance()->print("This computer is the network client.\n");
			}

			mThisClusterNodeId = i;
			break;
		}
	}

	if( mThisClusterNodeId == -1 ||
		mThisClusterNodeId >= static_cast<int>( NodeManager::Instance()->getNumberOfNodes() )) //fatal error
	{
		sgct::MessageHandler::Instance()->print("This computer is not a part of the cluster configuration!\n");
		mNetwork->close();
		glfwTerminate();
		return false;
	}
	else
	{
		printNodeInfo( static_cast<unsigned int>(mThisClusterNodeId) );
	}

	try
	{
		sgct::MessageHandler::Instance()->sendMessagesToServer(!isServer);
		sgct::MessageHandler::Instance()->print("Initiating network communication...\n");
		if( runningLocal )
			mNetwork->init(*(mConfig->getMasterPort()), "127.0.0.1", isServer);
		else
			mNetwork->init(*(mConfig->getMasterPort()), *(mConfig->getMasterIP()), isServer);
    }
    catch( const char * err )
    {
        sgct::MessageHandler::Instance()->print("Network error: %s\n", err);
        return false;
    }

	if( mConfig->isExternalControlPortSet() && isServer)
	{
		try
		{
			mExternalControlNetwork = new SGCTNetwork();
		}
		catch( const char * err )
		{
			sgct::MessageHandler::Instance()->print("External control Network init error: %s\n", err);
			if(mExternalControlNetwork != NULL)
				mExternalControlNetwork->close();
		}
		
		if( mExternalControlNetwork != NULL )
		{
			try
			{
				sgct::MessageHandler::Instance()->print("Initiating external control network...\n");
				mExternalControlNetwork->init(*(mConfig->getExternalControlPort()), "127.0.0.1", true, SGCTNetwork::ExternalControl);
			
				std::tr1::function< void(const char*, int, int) > callback;
				callback = std::tr1::bind(&sgct::Engine::decodeExternalControl, this,
					std::tr1::placeholders::_1,
					std::tr1::placeholders::_2,
					std::tr1::placeholders::_3);
				mExternalControlNetwork->setDecodeFunction(callback);
			}
			catch( const char * err )
			{
				sgct::MessageHandler::Instance()->print("External control network error: %s\n", err);
			}
		}
	}

    //set decoder for client
    if( isServer )
    {
        std::tr1::function< void(const char*, int, int) > callback;
        callback = std::tr1::bind(&sgct::MessageHandler::decode, sgct::MessageHandler::Instance(),
            std::tr1::placeholders::_1,
            std::tr1::placeholders::_2,
            std::tr1::placeholders::_3);
        mNetwork->setDecodeFunction(callback);
    }
    else
    {
        std::tr1::function< void(const char*, int, int) > callback;
        callback = std::tr1::bind(&sgct::SharedData::decode, sgct::SharedData::Instance(),
            std::tr1::placeholders::_1,
            std::tr1::placeholders::_2,
            std::tr1::placeholders::_3);
        mNetwork->setDecodeFunction(callback);
    }

    sgct::MessageHandler::Instance()->print("Done\n");
	return true;
}

bool sgct::Engine::initWindow()
{
	mWindow = new SGCTWindow();
	mWindow->setWindowResolution(
		NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->windowData[2],
		NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->windowData[3] );

	mWindow->setWindowPosition(
		NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->windowData[0],
		NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->windowData[1] );

	mWindow->setWindowMode( NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->fullscreen ?
		GLFW_FULLSCREEN : GLFW_WINDOW );

	mWindow->useSwapGroups( NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->useSwapGroups );
	mWindow->useQuadbuffer( NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->stereo == ReadConfig::Active );

	int antiAliasingSamples = NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->numberOfSamples;
	if( antiAliasingSamples > 1 ) //if multisample is used
		glfwOpenWindowHint( GLFW_FSAA_SAMPLES, antiAliasingSamples );

	if( !mWindow->openWindow() )
		return false;

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
	  //Problem: glewInit failed, something is seriously wrong.
	  sgct::MessageHandler::Instance()->print("Error: %s\n", glewGetErrorString(err));
	  return false;
	}
	sgct::MessageHandler::Instance()->print("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	mWindow->init( NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->lockVerticalSync );
	mWindow->setWindowTitle( getBasicInfo() );

	//Must wait until all nodes are running if using swap barrier
	if( NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->useSwapGroups )
	{
		while(!mNetwork->areAllNodesConnected())
		{
			sgct::MessageHandler::Instance()->print("Waiting for all nodes to connect...\n");
			// Swap front and back rendering buffers
			glfwSleep(0.25);
			glfwSwapBuffers();
		}
	}

	return true;
}

void sgct::Engine::initOGL()
{
	//Get OpenGL version
	int version[3];
	glfwGetGLVersion( &version[0], &version[1], &version[2] );
	sgct::MessageHandler::Instance()->print("OpenGL version %d.%d.%d\n", version[0], version[1], version[2]);

	if (!GLEW_ARB_texture_non_power_of_two)
	{
		sgct::MessageHandler::Instance()->print("Warning! Only power of two textures are supported!\n");
	}

	if( mInitOGLFn != NULL )
		mInitOGLFn();

	calculateFrustums();

	switch( NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->stereo )
	{
	case ReadConfig::Active:
		mInternalRenderFn = &Engine::setActiveStereoRenderingMode;
		break;

	default:
		mInternalRenderFn = &Engine::setNormalRenderingMode;
		break;
	}

	//init swap group barrier when ready to render
	mWindow->setBarrier(true);
	mWindow->resetSwapGroupFrameNumber();
}

void sgct::Engine::clean()
{
	sgct::MessageHandler::Instance()->print("Cleaning up...\n");

	//close external control connections
	if( mExternalControlNetwork != NULL )
	{
		mExternalControlNetwork->close();
		delete mExternalControlNetwork;
		mExternalControlNetwork = NULL;
	}

	//close TCP connections
	if( mNetwork != NULL )
	{
		mNetwork->close();
		delete mNetwork;
		mNetwork = NULL;
	}

	if( mConfig != NULL )
	{
		delete mConfig;
		mConfig = NULL;
	}
	if( mWindow != NULL )
	{
		delete mWindow;
		mWindow = NULL;
	}

	for( unsigned int i=0; i<3; i++)
		if( mFrustums[i] != NULL )
		{
			delete mFrustums[i];
			mFrustums[i] = NULL;
		}
	// Destroy explicitly to avoid memory leak messages
	FontManager::Destroy();
	ShaderManager::Destroy();
	SharedData::Destroy();
	TextureManager::Destroy();
	NodeManager::Destroy();
	MessageHandler::Destroy();

	// Close window and terminate GLFW
	glfwTerminate();
}

void sgct::Engine::frameSyncAndLock(int stage)
{
	double t0 = glfwGetTime();
	
	if( stage == PreStage )
	{
		mNetwork->sync();

		if( !isServer)
		{
			static int netFrameCounter = 0;
			
			//don't wait more then a sec, then something is wrong
			while(mNetwork->isRunning() && mRunning && (glfwGetTime()-t0) < 1.0)
			{
				if( netFrameCounter != mNetwork->getCurrentFrame() )
				{
					//iterate
					if( netFrameCounter < MAX_NET_SYNC_FRAME_NUMBER )
						netFrameCounter++;
					else
						netFrameCounter = 0;
					break;
				}
			}
		}

		mStatistics.setSyncTime(glfwGetTime() - t0);
	}
	else if( isServer && !runningLocal )//post stage
	{
		//don't wait more then a sec, then something is wrong
		while(mNetwork->isRunning() && mRunning && (glfwGetTime()-t0) < 1.0)
		{
			//check if all nodes has sent feedback to server
			if( mNetwork->getFeedbackCount() == 0 )
			{
				break;
			}
		}

		mStatistics.addSyncTime(glfwGetTime() - t0);
	}
}

void sgct::Engine::render()
{
	mRunning = GL_TRUE;

	while( mRunning )
	{
		if( mPreDrawFn != NULL )
			mPreDrawFn();

		if( isServer )
		{
			mNetwork->syncMutex(true);
			sgct::SharedData::Instance()->encode();
			mNetwork->syncMutex(false);
		}
		else
		{
			if( !mNetwork->isRunning() ) //exit if not running
				break;
		}

		frameSyncAndLock(PreStage);		

		double startFrameTime = glfwGetTime();
		calcFPS(startFrameTime);

		glLineWidth(1.0);
		showWireframe ? glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ) : glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

		(this->*mInternalRenderFn)();

		//restore
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

		mStatistics.setDrawTime(glfwGetTime() - startFrameTime);
		
		frameSyncAndLock(PostStage);

		if( showGraph )
			mStatistics.draw();
		if( showInfo )
			renderDisplayInfo();

		if( mPostDrawFn != NULL )
			mPostDrawFn();

		// Swap front and back rendering buffers
		glfwSwapBuffers();

		// Check if ESC key was pressed or window was closed
		mRunning = !glfwGetKey( GLFW_KEY_ESC ) && glfwGetWindowParam( GLFW_OPENED ) && !mTerminate;
	}
}

void sgct::Engine::renderDisplayInfo()
{
	glColor4f(0.8f,0.8f,0.0f,1.0f);
	unsigned int lFrameNumber = 0;
	mWindow->getSwapGroupFrameNumber(lFrameNumber);
	
	glDrawBuffer(GL_BACK); //draw into both back buffers
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 120, "Node ip: %s (%s)",
		NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->ip.c_str(),
		isServer ? "master" : "slave");
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 100, "Frame rate: %.3f Hz", mStatistics.getAvgFPS());
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 80, "Render time %.2f ms", getDrawTime()*1000.0);
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 60, "Sync time %.2f ms", getSyncTime()*1000.0);
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 40, "Swap groups: %s and %s (%s) [frame: %d]",
		mWindow->isUsingSwapGroups() ? "Enabled" : "Disabled",
		mWindow->isBarrierActive() ? "active" : "not active",
		mWindow->isSwapGroupMaster() ? "master" : "slave",
		lFrameNumber);

	//if stereoscopic rendering
	if( NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->stereo != ReadConfig::None )
	{
		glDrawBuffer(GL_BACK_LEFT);
		freetype::print( FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 140, "Active eye: Left");
		glDrawBuffer(GL_BACK_RIGHT);
		freetype::print( FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 140, "Active eye:        Right");
		glDrawBuffer(GL_BACK);
	}
}

void sgct::Engine::setNormalRenderingMode()
{
	activeFrustum = Frustum::Mono;
	glViewport (0, 0, mWindow->getHResolution(), mWindow->getVResolution());

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glFrustum(mFrustums[Frustum::Mono]->getLeft(),
		mFrustums[Frustum::Mono]->getRight(),
        mFrustums[Frustum::Mono]->getBottom(),
		mFrustums[Frustum::Mono]->getTop(),
        mFrustums[Frustum::Mono]->getNear(),
		mFrustums[Frustum::Mono]->getFar());

	//translate to user pos
	glTranslatef(-mConfig->getUserPos()->x, -mConfig->getUserPos()->y, -mConfig->getUserPos()->z);
	glMatrixMode(GL_MODELVIEW);
	glDrawBuffer(GL_BACK); //draw into both back buffers
	mClearBufferFn(); //clear buffers
	glLoadIdentity();

	if( mDrawFn != NULL )
		mDrawFn();
}

void sgct::Engine::setActiveStereoRenderingMode()
{
	glViewport (0, 0, mWindow->getHResolution(), mWindow->getVResolution());
	activeFrustum = Frustum::StereoLeftEye;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(mFrustums[Frustum::StereoLeftEye]->getLeft(),
		mFrustums[Frustum::StereoLeftEye]->getRight(),
        mFrustums[Frustum::StereoLeftEye]->getBottom(),
		mFrustums[Frustum::StereoLeftEye]->getTop(),
        mFrustums[Frustum::StereoLeftEye]->getNear(),
		mFrustums[Frustum::StereoLeftEye]->getFar());

	//translate to user pos
	glTranslatef(-mUser.LeftEyePos.x , -mUser.LeftEyePos.y, -mUser.LeftEyePos.z);
	glMatrixMode(GL_MODELVIEW);
	glDrawBuffer(GL_BACK_LEFT);
	mClearBufferFn(); //clear buffers
	glLoadIdentity();

	if( mDrawFn != NULL )
		mDrawFn();

	activeFrustum = Frustum::StereoRightEye;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(mFrustums[Frustum::StereoRightEye]->getLeft(),
		mFrustums[Frustum::StereoRightEye]->getRight(),
        mFrustums[Frustum::StereoRightEye]->getBottom(),
		mFrustums[Frustum::StereoRightEye]->getTop(),
        mFrustums[Frustum::StereoRightEye]->getNear(),
		mFrustums[Frustum::StereoRightEye]->getFar());

	//translate to user pos
	glTranslatef(-mUser.RightEyePos.x , -mUser.RightEyePos.y, -mUser.RightEyePos.z);
	glMatrixMode(GL_MODELVIEW);
	glDrawBuffer(GL_BACK_RIGHT);
	mClearBufferFn(); //clear buffers
	glLoadIdentity();

	if( mDrawFn != NULL )
		mDrawFn();
}

void sgct::Engine::calculateFrustums()
{
	mUser.LeftEyePos.x = mConfig->getUserPos()->x - mConfig->getEyeSeparation()/2.0f;
	mUser.LeftEyePos.y = mConfig->getUserPos()->y;
	mUser.LeftEyePos.z = mConfig->getUserPos()->z;

	mUser.RightEyePos.x = mConfig->getUserPos()->x + mConfig->getEyeSeparation()/2.0f;
	mUser.RightEyePos.y = mConfig->getUserPos()->y;
	mUser.RightEyePos.z = mConfig->getUserPos()->z;

	//nearFactor = near clipping plane / focus plane dist
	float nearFactor = nearClippingPlaneDist / (NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].z - mConfig->getUserPos()->z);
	if( nearFactor < 0 )
		nearFactor = -nearFactor;

	mFrustums[Frustum::Mono] = new Frustum(
		(NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].x - mConfig->getUserPos()->x)*nearFactor,
		(NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperRight ].x - mConfig->getUserPos()->x)*nearFactor,
		(NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].y - mConfig->getUserPos()->y)*nearFactor,
		(NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperRight ].y - mConfig->getUserPos()->y)*nearFactor,
		nearClippingPlaneDist, farClippingPlaneDist);

	mFrustums[Frustum::StereoLeftEye] = new Frustum(
		(NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].x - mUser.LeftEyePos.x)*nearFactor,
		(NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperRight ].x - mUser.LeftEyePos.x)*nearFactor,
		(NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].y - mUser.LeftEyePos.y)*nearFactor,
		(NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperRight ].y - mUser.LeftEyePos.y)*nearFactor,
		nearClippingPlaneDist, farClippingPlaneDist);

	mFrustums[Frustum::StereoRightEye] = new Frustum(
		(NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].x - mUser.RightEyePos.x)*nearFactor,
		(NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperRight ].x - mUser.RightEyePos.x)*nearFactor,
		(NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].y - mUser.RightEyePos.y)*nearFactor,
		(NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperRight ].y - mUser.RightEyePos.y)*nearFactor,
		nearClippingPlaneDist, farClippingPlaneDist);
}

void sgct::Engine::parseArguments( int argc, char* argv[] )
{
	//parse arguments
	sgct::MessageHandler::Instance()->print("Parsing arguments...");
	int i=0;
	while( i<argc )
	{
		if( strcmp(argv[i],"-config") == 0 )
		{
			configFilename.assign(argv[i+1]);
			i+=2;
		}
		else if( strcmp(argv[i],"--client") == 0 )
		{
			isServer = false;
			i++;
		}
		else if( strcmp(argv[i],"-local") == 0 )
		{
			runningLocal = true;
			std::stringstream ss( argv[i+1] );
			ss >> mThisClusterNodeId;
			i+=2;
		}
		else
			i++; //iterate
	}

	sgct::MessageHandler::Instance()->print(" Done\n");
}

void sgct::Engine::setDrawFunction(void(*fnPtr)(void))
{
	mDrawFn = fnPtr;
}

void sgct::Engine::setPreDrawFunction(void(*fnPtr)(void))
{
	mPreDrawFn = fnPtr;
}

void sgct::Engine::setPostDrawFunction(void(*fnPtr)(void))
{
	mPostDrawFn = fnPtr;
}

void sgct::Engine::setInitOGLFunction(void(*fnPtr)(void))
{
	mInitOGLFn = fnPtr;
}

void sgct::Engine::setClearBufferFunction(void(*fnPtr)(void))
{
	mClearBufferFn = fnPtr;
}

void sgct::Engine::setExternalControlCallback(void(*fnPtr)(const char *, int, int))
{
	mNetworkCallbackFn = fnPtr;
}

void sgct::Engine::clearBuffer(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void sgct::Engine::printNodeInfo(unsigned int nodeId)
{
	sgct::MessageHandler::Instance()->print("This node has index %d.\n", nodeId);
}

void sgct::Engine::calcFPS(double timestamp)
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
		if(mWindow->getWindowMode() == GLFW_WINDOW)
			mWindow->setWindowTitle( getBasicInfo() );
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
	nearClippingPlaneDist = _near;
	farClippingPlaneDist = _far;
	calculateFrustums();
}

void sgct::Engine::decodeExternalControl(const char * receivedData, int receivedLenght, int clientIndex)
{
	if(mNetworkCallbackFn != NULL)
		mNetworkCallbackFn(receivedData, receivedLenght, clientIndex);
}

void sgct::Engine::sendMessageToExternalControl(void * data, int lenght)
{
	if(mExternalControlNetwork != NULL)
		mExternalControlNetwork->sendDataToAllClients( data, lenght );
}

void sgct::Engine::sendMessageToExternalControl(const std::string msg)
{
	if(mExternalControlNetwork != NULL)
		mExternalControlNetwork->sendStrToAllClients( msg );
}

void sgct::Engine::setExternalControlBufferSize(unsigned int newSize)
{
	if(mExternalControlNetwork != NULL)
		mExternalControlNetwork->setBufferSize(newSize);
}

const char * sgct::Engine::getBasicInfo()
{
	#if (_MSC_VER >= 1400) //visual studio 2005 or later
	sprintf_s( basicInfo, sizeof(basicInfo), "Node: %s (%s) | fps: %.2f",
		runningLocal ? "127.0.0.1" : NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->ip.c_str(),
		isServer ? "server" : "slave",
		mStatistics.getAvgFPS());
    #else
    sprintf( basicInfo, "Node: %s (%s) | fps: %.2f",
		runningLocal ? "127.0.0.1" : NodeManager::Instance()->getNodePtr(mThisClusterNodeId)->ip.c_str(),
		isServer ? "server" : "slave",
		mStatistics.getAvgFPS());
    #endif

	return basicInfo;
}
