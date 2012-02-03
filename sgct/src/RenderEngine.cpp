#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/glfw.h>
#include "sgct/RenderEngine.h"
#include "sgct/freetype.h"
#include "sgct/FontManager.h"
#include <math.h>
#include <sstream>

sgct::RenderEngine::RenderEngine( SharedData & sharedData, int argc, char* argv[] )
{	
	mSharedData = &sharedData;
	//init pointers
	mNetwork = NULL;
	mWindow = NULL;
	mConfig = NULL;
	for( unsigned int i=0; i<3; i++)
		mFrustums[i] = NULL;
	//init function pointers
	mDrawFn = NULL;
	mPreDrawFn = NULL;
	mInitOGLFn = NULL;
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

bool sgct::RenderEngine::init()
{
	// Initialize GLFW
	if( !glfwInit() )
		return false;
	
	mConfig = new core_sgct::ReadConfig( configFilename );
	if( !mConfig->isValid() ) //fatal error
	{
		fprintf(stderr, "Error in xml config file parsing.\n");
		glfwTerminate();
		return false;
	} 
	
	if( !initNetwork() )
		return false;

	if( !initWindow() )
	{
		clean();
		glfwTerminate();
		return false;
	}

	initOGL();

	//
	// Add fonts
	//

	char winDir[128];
	GetWindowsDirectory(winDir,128);
	char fontPath[256];
	sprintf( fontPath, "%s\\Fonts\\verdanab.ttf", winDir);
	FontManager::Instance()->AddFont( "Verdana", fontPath );
	FontManager::Instance()->GetFont( "Verdana", 10 );
	//try
	//{
	//	char winDir[128];
	//	GetWindowsDirectory(winDir,128);
	//	char fontPath[256];
	//	sprintf( fontPath, "%s\\Fonts\\verdanab.ttf", winDir);
	//	
	//	fprintf(stdout, "Loading font '%s'\n", fontPath);
	//	mFont.init(fontPath, 10); //Build the freetype font
	//}
	//catch(std::runtime_error const & e)
	//{
	//	fprintf(stdout, "Font reading error: %s\n", e.what());
	//}
	
	return true;
}

bool sgct::RenderEngine::initNetwork()
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
			mNetwork->init(*(mConfig->getMasterPort()), "127.0.0.1", isServer, mSharedData);
		else
			mNetwork->init(*(mConfig->getMasterPort()), *(mConfig->getMasterIP()), isServer, mSharedData);
		fprintf(stderr, "Done\n");
	}
	catch( char * err )
	{
		fprintf(stderr, "Network error: %s\n", err);
		return false;
	}

	return true;
}

bool sgct::RenderEngine::initWindow()
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
	sprintf( windowTitle, "Node: %s (%s)", mConfig->getNodePtr(mThisClusterNodeId)->ip.c_str(),
		isServer ? "server" : "slave");
	mWindow->init(windowTitle);

	return true;
}

void sgct::RenderEngine::initOGL()
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
		mInternalRenderFn = &RenderEngine::setActiveStereoRenderingMode;
		break;
	
	default:
		mInternalRenderFn = &RenderEngine::setNormalRenderingMode;
		break;
	}
}

void sgct::RenderEngine::clean()
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
	//font.clean();
}

void sgct::RenderEngine::render()
{
	int running = GL_TRUE;
	
	while( running )
	{
		if( mPreDrawFn != NULL )
			mPreDrawFn();
	
		if( isServer )
		{
			mSharedData->encode();	
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
		
		// Swap front and back rendering buffers
		glfwSwapBuffers();
		// Check if ESC key was pressed or window was closed
		running = !glfwGetKey( GLFW_KEY_ESC ) && glfwGetWindowParam( GLFW_OPENED );
	}

	// Close window and terminate GLFW
	glfwTerminate();
}

void sgct::RenderEngine::renderDisplayInfo()
{
	glColor4f(0.8f,0.8f,0.8f,1.0f);
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 10 ), 100, 85, "Frame rate: %.3f Hz", mStatistics.AvgFPS);
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 10 ), 100, 70, "Render time %.2f ms", getDrawTime()*1000.0);
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 10 ), 100, 55, "Master: %s", isServer ? "True" : "False");
	freetype::print(FontManager::Instance()->GetFont( "Verdana", 10 ), 100, 40, "Swap groups: %s and %s (%s)",
		mWindow->isUsingSwapGroups() ? "enabled" : "disabled",
		mWindow->isBarrierActive() ? "active" : "not active",
		mWindow->isSwapGroupMaster() ? "master" : "slave");
}

void sgct::RenderEngine::setNormalRenderingMode()
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
		glColor4f(0.8f,0.8f,0.8f,1.0f);
		//reetype::print(font, 100, 100, "Active frustum: mono");
		freetype::print( FontManager::Instance()->GetFont( "Verdana", 10 ), 100, 100, "Active frustum: mono");
	}
}

void sgct::RenderEngine::setActiveStereoRenderingMode()
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
		glColor4f(0.8f,0.8f,0.8f,1.0f);
		//freetype::print(font, 100, 100, "Active frustum: stereo left eye");
		freetype::print( FontManager::Instance()->GetFont( "Verdana", 10 ), 100, 100, "Active frustum: stereo left eye");
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
		glColor4f(0.8f,0.8f,0.8f,1.0f);
		//freetype::print(font, 100, 100, "Active frustum: stereo right eye");
		freetype::print( FontManager::Instance()->GetFont( "Verdana", 10 ), 100, 100, "Active frustum: stereo right eye");
	}
}

void sgct::RenderEngine::calculateFrustums()
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

void sgct::RenderEngine::parseArguments( int argc, char* argv[] )
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

void sgct::RenderEngine::setDrawFunction(void(*fnPtr)(void))
{
	mDrawFn = fnPtr;
}

void sgct::RenderEngine::setPreDrawFunction(void(*fnPtr)(void))
{
	mPreDrawFn = fnPtr;
}

void sgct::RenderEngine::setInitOGLFunction(void(*fnPtr)(void))
{
	mInitOGLFn = fnPtr;
}

void sgct::RenderEngine::setClearBufferFunction(void(*fnPtr)(void))
{
	mClearBufferFn = fnPtr;
}

void sgct::RenderEngine::clearBuffer(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void sgct::RenderEngine::printNodeInfo(unsigned int nodeId)
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

void sgct::RenderEngine::calcFPS(double timestamp)
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

double sgct::RenderEngine::getDt()
{
	return mStatistics.FrameTime;
}

double sgct::RenderEngine::getTime()
{
	return mStatistics.TotalTime;
}

double sgct::RenderEngine::getDrawTime()
{
	return mStatistics.DrawTime;
}

void sgct::RenderEngine::setNearAndFarClippingPlanes(float _near, float _far)
{
	nearClippingPlaneDist = _near;
	farClippingPlaneDist = _far;
	calculateFrustums();
}