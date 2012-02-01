#include <math.h>
#include <sstream>
#include "RenderEngine.h"

HDC hDC;

void clearBuffer(void);
void GLFWCALL windowResizeCallback( int width, int height );

using namespace sgct;

RenderEngine * instancePtr;

RenderEngine::RenderEngine( SharedData & sharedData, int argc, char* argv[] )
{
	// Initialize GLFW
	if( !glfwInit() )
	{
		exit( EXIT_FAILURE );
	}
	
	instancePtr = this;
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
	useSwapGroups = false;
	swapGroupMaster = false;
	runningLocal = false;
	isServer = true;
	thisClusterNodeId = -1;
	activeFrustum = Frustum::Mono;

	windowRes[0] = 640;
	windowRes[1] = 480;
	windowMode = GLFW_WINDOW;

	//parse needs to be before read config since the path to the XML is parsed here
	parseArguments( argc, argv );

	mConfig = new ReadConfig( configFilename );
	if( !mConfig->isValid() ) //fatal error
	{
		fprintf(stderr, "Error in xml config file parsing.\n");
		glfwTerminate();
		exit( EXIT_FAILURE );
	}

	mSharedData = &sharedData; 

	try
	{
		mNetwork = new Network();
	}
	catch( char * err )
	{
		fprintf(stderr, "Network init error: %s\n", err);
		mNetwork->close();
		glfwTerminate();
		exit( EXIT_FAILURE );
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

			thisClusterNodeId = i;
			break;
		}
	}

	if( thisClusterNodeId == -1 || thisClusterNodeId >= static_cast<int>(mConfig->getNumberOfNodes()) ) //fatal error
	{
		fprintf(stderr, "This computer is not a part of the cluster configuration!\n");
		mNetwork->close();
		glfwTerminate();
		exit( EXIT_FAILURE );
	}
	else
	{
		fprintf(stderr, "This node is = %d.\nView plane coordinates: \n", thisClusterNodeId);
		fprintf(stderr, "\tLower left: %.4f  %.4f  %.4f\n",
			mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].x,
			mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].y,
			mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].z);
		fprintf(stderr, "\tUpper left: %.4f  %.4f  %.4f\n",
			mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperLeft ].x,
			mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperLeft ].y,
			mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperLeft ].z);
		fprintf(stderr, "\tUpper right: %.4f  %.4f  %.4f\n\n",
			mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperRight ].x,
			mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperRight ].y,
			mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperRight ].z);
	}

	int antiAliasingSamples = mConfig->getNodePtr(thisClusterNodeId)->numberOfSamples;
	if( antiAliasingSamples != 0 )
		glfwOpenWindowHint( GLFW_FSAA_SAMPLES, antiAliasingSamples );

	//get window resolution
	windowRes[0] = mConfig->getNodePtr(thisClusterNodeId)->windowData[2];
	windowRes[1] = mConfig->getNodePtr(thisClusterNodeId)->windowData[3];

	windowMode = (mConfig->getNodePtr(thisClusterNodeId)->fullscreen ? GLFW_FULLSCREEN : GLFW_WINDOW);

	useSwapGroups = mConfig->getNodePtr(thisClusterNodeId)->useSwapGroups;

	//Active quad-buffered stereo
	if( mConfig->getNodePtr(thisClusterNodeId)->stereo == ReadConfig::Active )
		glfwOpenWindowHint(GLFW_STEREO, GL_TRUE);

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
	}

	//glfwSleep( 0.25 ); //wait for all threads to start

	/* Open an OpenGL window
	param: 
	int width
	int height
	int redbits
	int greenbits
	int bluebits
	int alphabits
	int depthbits
	int stencilbits
	int mode
	*/
	if( !glfwOpenWindow( getHorizontalWindowResolution(),
		getVerticalWindowResolution(),
		0,0,0,0,0,0,
		getWindowMode()) )
	{
		clean();
		glfwTerminate();
		exit( EXIT_FAILURE );
	}

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
	  //Problem: glewInit failed, something is seriously wrong.
	  fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}
	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	init();
}

void RenderEngine::init()
{
	char windowTitle[32];
	sprintf( windowTitle, "Node: %s (%s)", mConfig->getNodePtr(thisClusterNodeId)->ip.c_str(),
		isServer ? "server" : "slave");
	glfwSetWindowTitle( windowTitle );
	glfwSetWindowSizeCallback( windowResizeCallback );

	glfwSwapInterval( 1 ); //0: vsync off, 1: vsync on
	glfwSetWindowPos( mConfig->getNodePtr(thisClusterNodeId)->windowData[0],
		mConfig->getNodePtr(thisClusterNodeId)->windowData[1]);

	//Get OpenGL version
	int version[3];
	glfwGetGLVersion( &version[0], &version[1], &version[2] );
	fprintf( stderr, "OpenGL version %d.%d.%d\n", version[0], version[1], version[2]);

	if( useSwapGroups )
		initNvidiaSwapGroups();

	if( mInitOGLFn != NULL )
		mInitOGLFn();

	calculateFrustums();

	switch( mConfig->getNodePtr(thisClusterNodeId)->stereo )
	{
	case ReadConfig::Active:
		mInternalRenderFn = &RenderEngine::setActiveStereoRenderingMode;
		break;
	
	default:
		mInternalRenderFn = &RenderEngine::setNormalRenderingMode;
		break;
	}

	try
	{
		font.init("font/verdanab.ttf", 10); //Build the freetype font
	}
	catch(std::runtime_error const & e)
	{
		fprintf(stdout, "Font reading error: %s\n", e.what());
	}
}

void RenderEngine::initNvidiaSwapGroups()
{
	if (wglewIsSupported("WGL_NV_swap_group"))
	{
		useSwapGroups = true;
		
		hDC = wglGetCurrentDC();
		fprintf(stdout, "WGL_NV_swap_group is supported\n");

		if( wglJoinSwapGroupNV(hDC,1) )
			fprintf(stdout, "Joining swapgroup 1 [ok].\n");
		else
		{
			fprintf(stdout, "Joining swapgroup 1 [failed].\n");
			useSwapGroups = false;
		}

		if( wglBindSwapBarrierNV(1,1) )
			fprintf(stdout, "Setting up swap barrier [ok].\n");
		else
		{
			fprintf(stdout, "Setting up swap barrier [failed].\n");
			useSwapGroups = false;
		}
		
		if( wglResetFrameCountNV(hDC) )
		{
			swapGroupMaster = true;
			fprintf(stdout, "Resetting frame counter. This computer is the master.\n");
		}
		else
			fprintf(stdout, "Resetting frame counter failed. This computer is the slave.\n");
	}
	else
		useSwapGroups = false;
}

void RenderEngine::clean()
{
	//close TCP connections
	mNetwork->close();
	
	delete mNetwork;
	delete mConfig;

	delete mFrustums[Frustum::Mono];
	delete mFrustums[Frustum::StereoLeftEye];
	delete mFrustums[Frustum::StereoRightEye];
	
	font.clean();
}

void RenderEngine::calcFPS(double timestamp)
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

double RenderEngine::getDt()
{
	return mStatistics.FrameTime;
}

double RenderEngine::getTime()
{
	return mStatistics.TotalTime;
}

double RenderEngine::getDrawTime()
{
	return mStatistics.DrawTime;
}

void RenderEngine::setNearAndFarClippingPlanes(float _near, float _far)
{
	nearClippingPlaneDist = _near;
	farClippingPlaneDist = _far;
	calculateFrustums();
}

void RenderEngine::setDrawFunction(void(*fnPtr)(void))
{
	mDrawFn = fnPtr;
}

void RenderEngine::setPreDrawFunction(void(*fnPtr)(void))
{
	mPreDrawFn = fnPtr;
}

void RenderEngine::setInitOGLFunction(void(*fnPtr)(void))
{
	mInitOGLFn = fnPtr;
}

void RenderEngine::setClearBufferFunction(void(*fnPtr)(void))
{
	mClearBufferFn = fnPtr;
}

void RenderEngine::calculateFrustums()
{
	mUser.LeftEyePos.x = mConfig->getUserPos()->x - mConfig->getEyeSeparation()/2.0f;
	mUser.LeftEyePos.y = mConfig->getUserPos()->y;
	mUser.LeftEyePos.z = mConfig->getUserPos()->z;

	mUser.RightEyePos.x = mConfig->getUserPos()->x + mConfig->getEyeSeparation()/2.0f;
	mUser.RightEyePos.y = mConfig->getUserPos()->y;
	mUser.RightEyePos.z = mConfig->getUserPos()->z;

	//nearFactor = near clipping plane / focus plane dist
	float nearFactor = nearClippingPlaneDist / (mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].z - mConfig->getUserPos()->z);
	if( nearFactor < 0 )
		nearFactor = -nearFactor;
	
	mFrustums[Frustum::Mono] = new Frustum(
		(mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].x - mConfig->getUserPos()->x)*nearFactor,
		(mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperRight ].x - mConfig->getUserPos()->x)*nearFactor,
		(mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].y - mConfig->getUserPos()->y)*nearFactor,
		(mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperRight ].y - mConfig->getUserPos()->y)*nearFactor,
		nearClippingPlaneDist, farClippingPlaneDist);

	mFrustums[Frustum::StereoLeftEye] = new Frustum(
		(mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].x - mUser.LeftEyePos.x)*nearFactor,
		(mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperRight ].x - mUser.LeftEyePos.x)*nearFactor,
		(mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].y - mUser.LeftEyePos.y)*nearFactor,
		(mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperRight ].y - mUser.LeftEyePos.y)*nearFactor,
		nearClippingPlaneDist, farClippingPlaneDist);

	mFrustums[Frustum::StereoRightEye] = new Frustum(
		(mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].x - mUser.RightEyePos.x)*nearFactor,
		(mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperRight ].x - mUser.RightEyePos.x)*nearFactor,
		(mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::LowerLeft ].y - mUser.RightEyePos.y)*nearFactor,
		(mConfig->getNodePtr(thisClusterNodeId)->viewPlaneCoords[ ReadConfig::UpperRight ].y - mUser.RightEyePos.y)*nearFactor,
		nearClippingPlaneDist, farClippingPlaneDist);
}

void RenderEngine::setNormalRenderingMode()
{
	activeFrustum = Frustum::Mono;
	glViewport (0, 0, windowRes[0], windowRes[1]);
	
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
	glLoadIdentity();
	
	if( mDrawFn != NULL )
		mDrawFn();
	if( displayInfo )
	{
		glColor4f(0.8f,0.8f,0.8f,1.0f);
		freetype::print(font, 100, 100, "Active frustum: mono");
	}
}

void RenderEngine::setActiveStereoRenderingMode()
{
	glViewport (0, 0, windowRes[0], windowRes[1]);
	activeFrustum = Frustum::StereoLeftEye;
	glDrawBuffer(GL_BACK_LEFT);
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
	glLoadIdentity();
	
	if( mDrawFn != NULL )
		mDrawFn();
	if( displayInfo )
	{
		glColor4f(0.8f,0.8f,0.8f,1.0f);
		freetype::print(font, 100, 100, "Active frustum: stereo left eye");
	}

	activeFrustum = Frustum::StereoRightEye;
	glDrawBuffer(GL_BACK_RIGHT);
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
	glLoadIdentity();
	
	if( mDrawFn != NULL )
		mDrawFn();
	if( displayInfo )
	{
		glColor4f(0.8f,0.8f,0.8f,1.0f);
		freetype::print(font, 100, 100, "Active frustum: stereo right eye");
	}
}

void RenderEngine::render()
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
		glDrawBuffer(GL_BACK); //draw into both back buffers
		mClearBufferFn();
		
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

	clean(); //disconnect and clean up

	// Close window and terminate GLFW
	glfwTerminate();
}

void RenderEngine::renderDisplayInfo()
{
	glColor4f(0.8f,0.8f,0.8f,1.0f);
	freetype::print(font, 100, 85, "Frame rate: %.3f Hz", mStatistics.AvgFPS);
	freetype::print(font, 100, 70, "Render time %.2f ms", getDrawTime()*1000.0);
	freetype::print(font, 100, 55, "Master: %s", isServer ? "True" : "False");
	freetype::print(font, 100, 40, "SwapLock: %s (%s)", useSwapGroups ? "enabled" : "disabled", swapGroupMaster ? "master" : "slave");
}

void RenderEngine::getSwapGroupFrameNumber(GLuint & frameNumber)
{
	frameNumber = 0;
	if (useSwapGroups)
	{
		wglQueryFrameCountNV(hDC, &frameNumber);
	}		
}

void RenderEngine::resetSwapGroupFrameNumber()
{
	if (useSwapGroups)
	{
		wglResetFrameCountNV(hDC);
	}
}

void clearBuffer(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void GLFWCALL windowResizeCallback( int width, int height )
{ 
	instancePtr->setWindowResolution(width, height > 0 ? height : 1);
}

void RenderEngine::parseArguments( int argc, char* argv[] )
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
			ss >> thisClusterNodeId;
			i+=2;
		}
		else
			i++; //iterate
	}

	fprintf(stderr, " Done\n");
}