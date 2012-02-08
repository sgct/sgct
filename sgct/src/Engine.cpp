#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/glfw.h>
#include "sgct/Engine.h"
#include "sgct/freetype.h"
#include "sgct/FontManager.h"
#include "sgct/MessageHandler.h"
#include "sgct/TextureManager.h"
#include "sgct/SharedData.h"
#include <math.h>
#include <sstream>

sgct::Engine::Engine( int argc, char* argv[] )
{	
	//init pointers
	mNetwork = NULL;
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
	mStatistics.AvgFPS = 0.0;
	mStatistics.DrawTime = 0.0;
	mStatistics.FrameTime = 0.0;
	mStatistics.TotalTime = 0.0;
	nearClippingPlaneDist = 0.1f;
	farClippingPlaneDist = 100.0f;
	displayInfo = false;
	runningLocal = false;
	isServer = true;
	mThisClusterNodeId = -1;
	activeFrustum = core_sgct::Frustum::Mono;

	//parse needs to be before read config since the path to the XML is parsed here
	parseArguments( argc, argv );
}

bool sgct::Engine::init()
{
	// Initialize GLFW
	if( !glfwInit() )
		return false;
	mConfig = new core_sgct::ReadConfig( configFilename );
	if( !mConfig->isValid() ) //fatal error
	{
		fprintf(stderr, "Error in xml config file parsing.\n");
		return false;
	} 
	if( !initNetwork() )
	{
		fprintf(stderr, "Network init error.\n");
		return false;
	}

	if( !initWindow() )
	{
		fprintf(stderr, "Window init error.\n");
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
		mNetwork = new core_sgct::SGCTNetwork();
	}
	catch( char * err )
	{
		fprintf(stderr, "Network init error: %s\n", err);
		mNetwork->close();
		glfwTerminate();
		return false;
	}

	//check in cluster configuration if this node master or slave
	for(unsigned int i=0; i<mConfig->getNumberOfNodes(); i++)
	{
		if( mNetwork->matchAddress( mConfig->getNodePtr(i)->ip ) )
		{
			if( !runningLocal && mConfig->getNodePtr(i)->master)
				isServer = true;
			else if( !runningLocal && !mConfig->getNodePtr(i)->master)
				isServer = false;

			mThisClusterNodeId = i;
			break;
		}
	}

	if( mThisClusterNodeId == -1 || mThisClusterNodeId >= static_cast<int>(mConfig->getNumberOfNodes()) ) //fatal error
	{
		fprintf(stderr, "This computer is not a part of the cluster configuration!\n");
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
		fprintf(stderr, "Initiating network communication...\n");
		if( runningLocal )
			mNetwork->init(*(mConfig->getMasterPort()), "127.0.0.1", isServer, mConfig->getNumberOfNodes());
		else
			mNetwork->init(*(mConfig->getMasterPort()), *(mConfig->getMasterIP()), isServer, mConfig->getNumberOfNodes());
		
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

		fprintf(stderr, "Done\n");
	}
	catch( char * err )
	{
		fprintf(stderr, "Network error: %s\n", err);
		return false;
	}

	return true;
}

bool sgct::Engine::initWindow()
{
	mWindow = new core_sgct::SGCTWindow();
	mWindow->setWindowResolution(
		mConfig->getNodePtr(mThisClusterNodeId)->windowData[2],
		mConfig->getNodePtr(mThisClusterNodeId)->windowData[3] );

	mWindow->setWindowPosition(
		mConfig->getNodePtr(mThisClusterNodeId)->windowData[0],
		mConfig->getNodePtr(mThisClusterNodeId)->windowData[1] );
	
	mWindow->setWindowMode( mConfig->getNodePtr(mThisClusterNodeId)->fullscreen ? 
		GLFW_FULLSCREEN : GLFW_WINDOW );
	
	mWindow->useSwapGroups( mConfig->getNodePtr(mThisClusterNodeId)->useSwapGroups );
	mWindow->useQuadbuffer( mConfig->getNodePtr(mThisClusterNodeId)->stereo == core_sgct::ReadConfig::Active );
	
	int antiAliasingSamples = mConfig->getNodePtr(mThisClusterNodeId)->numberOfSamples;
	if( antiAliasingSamples > 1 ) //if multisample is used
		glfwOpenWindowHint( GLFW_FSAA_SAMPLES, antiAliasingSamples );

	if( !mWindow->openWindow() )
		return false;

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
	  //Problem: glewInit failed, something is seriously wrong.
	  fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	  return false;
	}
	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	char windowTitle[32];
	sprintf_s( windowTitle, sizeof(windowTitle), "Node: %s (%s)", mConfig->getNodePtr(mThisClusterNodeId)->ip.c_str(),
		isServer ? "server" : "slave");
	mWindow->init(windowTitle);

	//Must wait until all nodes are running if using swap barrier
	if( mConfig->getNodePtr(mThisClusterNodeId)->useSwapGroups )
	{
		while(!mNetwork->areAllNodesConnected())
		{
			fprintf(stdout, "Waiting for all nodes to connect...\n");
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
	fprintf( stderr, "OpenGL version %d.%d.%d\n", version[0], version[1], version[2]);

	if (!GLEW_ARB_texture_non_power_of_two)
	{
		fprintf( stderr, "Warning! Only power of two textures are supported!\n");
	}

	if( mInitOGLFn != NULL )
		mInitOGLFn();

	calculateFrustums();
	
	switch( mConfig->getNodePtr(mThisClusterNodeId)->stereo )
	{
	case core_sgct::ReadConfig::Active:
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
	fprintf(stderr, "Cleaning up...\n");
	
	//close TCP connections
	if( mNetwork != NULL )
	{
		mNetwork->close();
		delete mNetwork;
	}

	if( mConfig != NULL )
		delete mConfig;
	if( mWindow != NULL )
		delete mWindow;

	for( unsigned int i=0; i<3; i++)
		if( mFrustums[i] != NULL )
			delete mFrustums[i];
	
	// Destroy explicitly to avoid memory leak messages
	FontManager::Destroy();

	sgct::SharedData::Instance()->Destroy();
	sgct::TextureManager::Instance()->Destroy();
	MessageHandler::Destroy();

	// Close window and terminate GLFW
	glfwTerminate();
}

void sgct::Engine::render()
{
	int running = GL_TRUE;
	
	while( running )
	{
		if( mPreDrawFn != NULL )
			mPreDrawFn();
	
		if( isServer )
		{
			sgct::SharedData::Instance()->encode();	
			mNetwork->sync();
		}
		else
		{
			if( !mNetwork->isRunning() ) //exit if not running
				break;
		}
	
		double startFrameTime = glfwGetTime();
		calcFPS(startFrameTime);
		
		(this->*mInternalRenderFn)();

		mStatistics.DrawTime = (glfwGetTime() - startFrameTime);

		glDrawBuffer(GL_BACK); //draw into both back buffers
		if( displayInfo )
			renderDisplayInfo();

		if( mPostDrawFn != NULL )
			mPostDrawFn();
		
		// Swap front and back rendering buffers
		glfwSwapBuffers();
		// Check if ESC key was pressed or window was closed
		running = !glfwGetKey( GLFW_KEY_ESC ) && glfwGetWindowParam( GLFW_OPENED );
	}
}

void sgct::Engine::renderDisplayInfo()
{
	glColor4f(0.8f,0.8f,0.0f,1.0f);
	unsigned int lFrameNumber = 0;
	mWindow->getSwapGroupFrameNumber(lFrameNumber);
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 100, "Node ip: %s (%s)",
		mConfig->getNodePtr(mThisClusterNodeId)->ip.c_str(),
		isServer ? "master" : "slave");
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 80, "Frame rate: %.3f Hz", mStatistics.AvgFPS);
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 60, "Render time %.2f ms", getDrawTime()*1000.0);
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 40, "Swap groups: %s and %s (%s) [frame: %d]",
		mWindow->isUsingSwapGroups() ? "Enabled" : "Disabled",
		mWindow->isBarrierActive() ? "active" : "not active",
		mWindow->isSwapGroupMaster() ? "master" : "slave",
		lFrameNumber);
}

void sgct::Engine::setNormalRenderingMode()
{
	activeFrustum = core_sgct::Frustum::Mono;
	glViewport (0, 0, mWindow->getHResolution(), mWindow->getVResolution());
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glFrustum(mFrustums[core_sgct::Frustum::Mono]->getLeft(), 
		mFrustums[core_sgct::Frustum::Mono]->getRight(),
        mFrustums[core_sgct::Frustum::Mono]->getBottom(),
		mFrustums[core_sgct::Frustum::Mono]->getTop(),
        mFrustums[core_sgct::Frustum::Mono]->getNear(), 
		mFrustums[core_sgct::Frustum::Mono]->getFar());

	//translate to user pos
	glTranslatef(-mConfig->getUserPos()->x, -mConfig->getUserPos()->y, -mConfig->getUserPos()->z);
	glMatrixMode(GL_MODELVIEW);
	glDrawBuffer(GL_BACK); //draw into both back buffers
	mClearBufferFn(); //clear buffers
	glLoadIdentity();
	
	if( mDrawFn != NULL )
		mDrawFn();
	if( displayInfo )
	{
		glColor4f(0.8f,0.8f,0.0f,1.0f);
		freetype::print( FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 120, "Active frustum: mono");
	}
}

void sgct::Engine::setActiveStereoRenderingMode()
{
	glViewport (0, 0, mWindow->getHResolution(), mWindow->getVResolution());
	activeFrustum = core_sgct::Frustum::StereoLeftEye;
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(mFrustums[core_sgct::Frustum::StereoLeftEye]->getLeft(), 
		mFrustums[core_sgct::Frustum::StereoLeftEye]->getRight(),
        mFrustums[core_sgct::Frustum::StereoLeftEye]->getBottom(),
		mFrustums[core_sgct::Frustum::StereoLeftEye]->getTop(),
        mFrustums[core_sgct::Frustum::StereoLeftEye]->getNear(), 
		mFrustums[core_sgct::Frustum::StereoLeftEye]->getFar());

	//translate to user pos
	glTranslatef(-mUser.LeftEyePos.x , -mUser.LeftEyePos.y, -mUser.LeftEyePos.z);
	glMatrixMode(GL_MODELVIEW);
	glDrawBuffer(GL_BACK_LEFT);
	mClearBufferFn(); //clear buffers
	glLoadIdentity();
	
	if( mDrawFn != NULL )
		mDrawFn();
	if( displayInfo )
	{
		glColor4f(0.8f,0.8f,0.0f,1.0f);
		freetype::print( FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 120, "Active frustum: stereo left eye");
	}

	activeFrustum = core_sgct::Frustum::StereoRightEye;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(mFrustums[core_sgct::Frustum::StereoRightEye]->getLeft(), 
		mFrustums[core_sgct::Frustum::StereoRightEye]->getRight(),
        mFrustums[core_sgct::Frustum::StereoRightEye]->getBottom(),
		mFrustums[core_sgct::Frustum::StereoRightEye]->getTop(),
        mFrustums[core_sgct::Frustum::StereoRightEye]->getNear(), 
		mFrustums[core_sgct::Frustum::StereoRightEye]->getFar());

	//translate to user pos
	glTranslatef(-mUser.RightEyePos.x , -mUser.RightEyePos.y, -mUser.RightEyePos.z);
	glMatrixMode(GL_MODELVIEW);
	glDrawBuffer(GL_BACK_RIGHT);
	mClearBufferFn(); //clear buffers
	glLoadIdentity();
	
	if( mDrawFn != NULL )
		mDrawFn();
	if( displayInfo )
	{
		glColor4f(0.8f,0.8f,0.0f,1.0f);
		freetype::print( FontManager::Instance()->GetFont( "Verdana", 14 ), 100, 120, "Active frustum: stereo right eye");
	}
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
	float nearFactor = nearClippingPlaneDist / (mConfig->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ core_sgct::ReadConfig::LowerLeft ].z - mConfig->getUserPos()->z);
	if( nearFactor < 0 )
		nearFactor = -nearFactor;
	
	mFrustums[core_sgct::Frustum::Mono] = new core_sgct::Frustum(
		(mConfig->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ core_sgct::ReadConfig::LowerLeft ].x - mConfig->getUserPos()->x)*nearFactor,
		(mConfig->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ core_sgct::ReadConfig::UpperRight ].x - mConfig->getUserPos()->x)*nearFactor,
		(mConfig->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ core_sgct::ReadConfig::LowerLeft ].y - mConfig->getUserPos()->y)*nearFactor,
		(mConfig->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ core_sgct::ReadConfig::UpperRight ].y - mConfig->getUserPos()->y)*nearFactor,
		nearClippingPlaneDist, farClippingPlaneDist);

	mFrustums[core_sgct::Frustum::StereoLeftEye] = new core_sgct::Frustum(
		(mConfig->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ core_sgct::ReadConfig::LowerLeft ].x - mUser.LeftEyePos.x)*nearFactor,
		(mConfig->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ core_sgct::ReadConfig::UpperRight ].x - mUser.LeftEyePos.x)*nearFactor,
		(mConfig->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ core_sgct::ReadConfig::LowerLeft ].y - mUser.LeftEyePos.y)*nearFactor,
		(mConfig->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ core_sgct::ReadConfig::UpperRight ].y - mUser.LeftEyePos.y)*nearFactor,
		nearClippingPlaneDist, farClippingPlaneDist);

	mFrustums[core_sgct::Frustum::StereoRightEye] = new core_sgct::Frustum(
		(mConfig->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ core_sgct::ReadConfig::LowerLeft ].x - mUser.RightEyePos.x)*nearFactor,
		(mConfig->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ core_sgct::ReadConfig::UpperRight ].x - mUser.RightEyePos.x)*nearFactor,
		(mConfig->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ core_sgct::ReadConfig::LowerLeft ].y - mUser.RightEyePos.y)*nearFactor,
		(mConfig->getNodePtr(mThisClusterNodeId)->viewPlaneCoords[ core_sgct::ReadConfig::UpperRight ].y - mUser.RightEyePos.y)*nearFactor,
		nearClippingPlaneDist, farClippingPlaneDist);
}

void sgct::Engine::parseArguments( int argc, char* argv[] )
{
	//parse arguments
	fprintf(stderr, "Parsing arguments...");
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

	fprintf(stderr, " Done\n");
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

void sgct::Engine::clearBuffer(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void sgct::Engine::printNodeInfo(unsigned int nodeId)
{
	fprintf(stderr, "This node is = %d.\nView plane coordinates: \n", nodeId);
	fprintf(stderr, "\tLower left: %.4f  %.4f  %.4f\n",
		mConfig->getNodePtr(nodeId)->viewPlaneCoords[ core_sgct::ReadConfig::LowerLeft ].x,
		mConfig->getNodePtr(nodeId)->viewPlaneCoords[ core_sgct::ReadConfig::LowerLeft ].y,
		mConfig->getNodePtr(nodeId)->viewPlaneCoords[ core_sgct::ReadConfig::LowerLeft ].z);
	fprintf(stderr, "\tUpper left: %.4f  %.4f  %.4f\n",
		mConfig->getNodePtr(nodeId)->viewPlaneCoords[ core_sgct::ReadConfig::UpperLeft ].x,
		mConfig->getNodePtr(nodeId)->viewPlaneCoords[ core_sgct::ReadConfig::UpperLeft ].y,
		mConfig->getNodePtr(nodeId)->viewPlaneCoords[ core_sgct::ReadConfig::UpperLeft ].z);
	fprintf(stderr, "\tUpper right: %.4f  %.4f  %.4f\n\n",
		mConfig->getNodePtr(nodeId)->viewPlaneCoords[ core_sgct::ReadConfig::UpperRight ].x,
		mConfig->getNodePtr(nodeId)->viewPlaneCoords[ core_sgct::ReadConfig::UpperRight ].y,
		mConfig->getNodePtr(nodeId)->viewPlaneCoords[ core_sgct::ReadConfig::UpperRight ].z);
}

void sgct::Engine::calcFPS(double timestamp)
{
	static double lastTimestamp = glfwGetTime();
	mStatistics.FrameTime = timestamp - lastTimestamp;
	mStatistics.TotalTime += mStatistics.FrameTime;
	lastTimestamp = timestamp;
    static double renderedFrames = 0.0;
	static double tmpTime = 0.0;
	mStatistics.FPS = 1.0/mStatistics.FrameTime;
	renderedFrames += 1.0;
	tmpTime += mStatistics.FrameTime;
	if( tmpTime >= 1.0 )
	{
		mStatistics.AvgFPS = renderedFrames / tmpTime;
		renderedFrames = 0.0;
		tmpTime = 0.0;
	}
}

double sgct::Engine::getDt()
{
	return mStatistics.FrameTime;
}

double sgct::Engine::getTime()
{
	return mStatistics.TotalTime;
}

double sgct::Engine::getDrawTime()
{
	return mStatistics.DrawTime;
}

void sgct::Engine::setNearAndFarClippingPlanes(float _near, float _far)
{
	nearClippingPlaneDist = _near;
	farClippingPlaneDist = _far;
	calculateFrustums();
}