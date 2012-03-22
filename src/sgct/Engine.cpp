/*************************************************************************
Copyright (c) 2012 Miroslav Andel, Linköping University.
All rights reserved.
 
Original Authors:
Miroslav Andel, Alexander Fridlund

For any questions or information about the SGCT project please contact: miroslav.andel@liu.se

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to
Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************/

#include "../include/sgct/Engine.h"
#include "../include/sgct/freetype.h"
#include "../include/sgct/FontManager.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/TextureManager.h"
#include "../include/sgct/SharedData.h"
#include "../include/sgct/ShaderManager.h"
#include "../include/sgct/ogl_headers.h"
#include <math.h>
#include <sstream>
#include <glm/gtx/euler_angles.hpp>

using namespace core_sgct;

sgct::Engine *  sgct::Engine::mThis     = NULL;

#ifdef GLEW_MX
GLEWContext * glewGetContext();
#endif

sgct::Engine::Engine( int argc, char* argv[] )
{
	//init pointers
	mThis = this;
	mNetworkConnections = NULL;
	mConfig = NULL;

	//init function pointers
	mDrawFn = NULL;
	mPreDrawFn = NULL;
	mPostDrawFn = NULL;
	mInitOGLFn = NULL;
	mClearBufferFn = NULL;
	mInternalRenderFn = NULL;
	mNetworkCallbackFn = NULL;
	mKeyboardCallbackFn = NULL;
	mCharCallbackFn = NULL;
	mMouseButtonCallbackFn = NULL;
	mMousePosCallbackFn = NULL;
	mMouseWheelCallbackFn = NULL;

	mTerminate = false;

	localRunningMode = NetworkManager::NotLocal;

	// Initialize GLFW
	if( !glfwInit() )
	{
		mTerminate = true;
		return;
	}

    NetworkManager::gMutex = createMutex();
    NetworkManager::gCond = createCondition();
	NetworkManager::gStartConnectionCond = createCondition();

    if(NetworkManager::gMutex == NULL ||
		NetworkManager::gCond == NULL ||
		NetworkManager::gStartConnectionCond == NULL)
    {
		mTerminate = true;
		return;
	}

	setClearBufferFunction( clearBuffer );
	nearClippingPlaneDist = 0.1f;
	farClippingPlaneDist = 100.0f;
	showInfo = false;
	showGraph = false;
	showWireframe = false;
	mActiveFrustum = Frustum::Mono;

	//parse needs to be before read config since the path to the XML is parsed here
	parseArguments( argc, argv );
}

bool sgct::Engine::init()
{
	if(mTerminate)
	{
		sgct::MessageHandler::Instance()->print("Failed to init GLFW.\n");
		return false;
	}

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

	return true;
}

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
	sgct::MessageHandler::Instance()->sendMessagesToServer( !mNetworkConnections->isComputerServer() );

	if(!mNetworkConnections->init())
		return false;

    sgct::MessageHandler::Instance()->print("Done\n");
	return true;
}

bool sgct::Engine::initWindow()
{
	getWindowPtr()->useQuadbuffer( ClusterManager::Instance()->getThisNodePtr()->stereo == ReadConfig::Active );

	int antiAliasingSamples = ClusterManager::Instance()->getThisNodePtr()->numberOfSamples;
	if( antiAliasingSamples > 1 ) //if multisample is used
		glfwOpenWindowHint( GLFW_FSAA_SAMPLES, antiAliasingSamples );

    //glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    //glfwOpenWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    //glfwOpenWindowHint( GLFW_OPENGL_VERSION_MAJOR, 3 );
    //glfwOpenWindowHint( GLFW_OPENGL_VERSION_MINOR, 2 );

	if( !getWindowPtr()->openWindow() )
		return false;

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
	  //Problem: glewInit failed, something is seriously wrong.
	  sgct::MessageHandler::Instance()->print("Error: %s\n", glewGetErrorString(err));
	  return false;
	}
	sgct::MessageHandler::Instance()->print("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
#ifdef __WIN32__
		if( glewIsSupported("WGL_NV_swap_group") )
			sgct::MessageHandler::Instance()->print("Swap groups are supported by hardware.\n");
#else
		if( glewIsSupported("GLX_NV_swap_group") )
			sgct::MessageHandler::Instance()->print("Swap groups are supported by hardware.\n");
#endif

	getWindowPtr()->init( ClusterManager::Instance()->getThisNodePtr()->lockVerticalSync );
	getWindowPtr()->setWindowTitle( getBasicInfo() );

	//Must wait until all nodes are running if using swap barrier
	if( getWindowPtr()->isUsingSwapGroups() )
	{
		sgct::MessageHandler::Instance()->print("Waiting for all nodes to connect...\n");
		glfwSwapBuffers();
		//render just black....
		while(mNetworkConnections->isRunning() &&
			!glfwGetKey( GLFW_KEY_ESC ) &&
			glfwGetWindowParam( GLFW_OPENED ) &&
			!mTerminate)
		{
			if(mNetworkConnections->areAllNodesConnected())
				break;

			glfwLockMutex( NetworkManager::gMutex );
				glfwWaitCond( NetworkManager::gCond,
					NetworkManager::gMutex,
					1.0 );
			glfwUnlockMutex( NetworkManager::gMutex );
			// Swap front and back rendering buffers
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
	mInternalRenderFn = &Engine::draw;

	//
	// Add fonts
	//
#if __WIN32__
	if( !FontManager::Instance()->AddFont( "Verdana", "verdanab.ttf" ) )
#elif __APPLE__
	if( !FontManager::Instance()->AddFont( "Verdana", "Verdana Bold.ttf" ) )
#else
    //@TODO Miro: SET SUITABLE FONT NAME FOR LINUX SYSTEMS
    if( !FontManager::Instance()->AddFont( "Verdana", "Verdana Bold.ttf" ) )
#endif
		FontManager::Instance()->GetFont( "Verdana", 12 );

	//init swap group barrier when ready to render
	getWindowPtr()->setBarrier(true);
	getWindowPtr()->resetSwapGroupFrameNumber();
}

void sgct::Engine::clean()
{
	sgct::MessageHandler::Instance()->print("Cleaning up...\n");

	//de-init window and unbind swapgroups...
	SGCTNode * nPtr = ClusterManager::Instance()->getThisNodePtr();
	if(nPtr != NULL)
		nPtr->getWindowPtr()->close();

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
	FontManager::Destroy();
	ShaderManager::Destroy();
	SharedData::Destroy();
	TextureManager::Destroy();
	ClusterManager::Destroy();
	MessageHandler::Destroy();

	if( NetworkManager::gMutex != NULL )
	{
		glfwDestroyMutex( NetworkManager::gMutex );
		NetworkManager::gMutex = NULL;
	}
	if( NetworkManager::gCond != NULL )
	{
		glfwDestroyCond( NetworkManager::gCond );
		NetworkManager::gCond = NULL;
	}
	if( NetworkManager::gStartConnectionCond != NULL )
	{
		glfwDestroyCond( NetworkManager::gStartConnectionCond );
		NetworkManager::gStartConnectionCond = NULL;
	}

	// Close window and terminate GLFW
	glfwTerminate();
}

void sgct::Engine::frameSyncAndLock(int stage)
{
	double t0 = glfwGetTime();
	static double syncTime = 0.0;

	if( stage == PreStage )
	{
		mNetworkConnections->sync();

		if( !mNetworkConnections->isComputerServer() ) //not server
			while(mNetworkConnections->isRunning() && mRunning)
			{
				if( mNetworkConnections->isSyncComplete() )
						break;

				glfwLockMutex( NetworkManager::gMutex );
					glfwWaitCond( NetworkManager::gCond,
						NetworkManager::gMutex,
						1.0 );
				glfwUnlockMutex( NetworkManager::gMutex );
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
			while(mNetworkConnections->isRunning() && mRunning)
			{
				if( mNetworkConnections->isSyncComplete() )
						break;

				glfwLockMutex( NetworkManager::gMutex );
					glfwWaitCond( NetworkManager::gCond,
						NetworkManager::gMutex,
						1.0 );
				glfwUnlockMutex( NetworkManager::gMutex );
			}

			syncTime += glfwGetTime() - t0;
		}

		mStatistics.setSyncTime(syncTime);
	}
}

void sgct::Engine::render()
{

	mRunning = GL_TRUE;

	while( mRunning )
	{
		if( mPreDrawFn != NULL )
			mPreDrawFn();

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

		double startFrameTime = glfwGetTime();
		calcFPS(startFrameTime);

		glLineWidth(1.0);
		showWireframe ? glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ) : glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

		SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();

		if( tmpNode->stereo == ReadConfig::Active )
		{
			mActiveFrustum = Frustum::StereoLeftEye;
			glDrawBuffer(GL_BACK_LEFT);
		}
		else
		{
			mActiveFrustum = Frustum::Mono;
			glDrawBuffer(GL_BACK);
		}
		//clear buffers
		mClearBufferFn();

		//render all viewports for mono or left eye
		for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
		{
			tmpNode->setCurrentViewport(i);
			//if passive stereo or mono
			if( tmpNode->stereo != ReadConfig::Active )
				mActiveFrustum = tmpNode->getCurrentViewport()->getEye();
			(this->*mInternalRenderFn)();
		}

		if( tmpNode->stereo == ReadConfig::Active )
		{
			glDrawBuffer(GL_BACK_RIGHT);

			//clear buffers
			mClearBufferFn();

			mActiveFrustum = Frustum::StereoRightEye;
			for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
			{
				tmpNode->setCurrentViewport(i);
				(this->*mInternalRenderFn)();
			}
		}

		//restore
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

		mStatistics.setDrawTime(glfwGetTime() - startFrameTime);

		//render post frame
		if( mPostDrawFn != NULL )
			mPostDrawFn();

		for(unsigned int i=0; i<tmpNode->getNumberOfViewports(); i++)
		{
			tmpNode->setCurrentViewport(i);
			enterCurrentViewport();
			if( showGraph )
				mStatistics.draw();
			if( showInfo )
				renderDisplayInfo();
		}


		//wait for nodes render before swapping
		frameSyncAndLock(PostStage);

		// Swap front and back rendering buffers
		glfwSwapBuffers();

		mNetworkConnections->swapData();

		// Check if ESC key was pressed or window was closed
		mRunning = !glfwGetKey( GLFW_KEY_ESC ) && glfwGetWindowParam( GLFW_OPENED ) && !mTerminate;
	}
}

void sgct::Engine::renderDisplayInfo()
{
	glPushAttrib(GL_CURRENT_BIT);
	glColor4f(0.8f,0.8f,0.8f,1.0f);
	unsigned int lFrameNumber = 0;
	getWindowPtr()->getSwapGroupFrameNumber(lFrameNumber);

	glDrawBuffer(GL_BACK); //draw into both back buffers
	Freetype::print(FontManager::Instance()->GetFont( "Verdana", 12 ), 100, 120, "Node ip: %s (%s)",
		ClusterManager::Instance()->getThisNodePtr()->ip.c_str(),
		mNetworkConnections->isComputerServer() ? "master" : "slave");
	glColor4f(0.8f,0.8f,0.0f,1.0f);
	Freetype::print(FontManager::Instance()->GetFont( "Verdana", 12 ), 100, 100, "Frame rate: %.3f Hz", mStatistics.getAvgFPS());
	glColor4f(0.8f,0.0f,0.8f,1.0f);
	Freetype::print(FontManager::Instance()->GetFont( "Verdana", 12 ), 100, 80, "Draw time: %.2f ms", getDrawTime()*1000.0);
	glColor4f(0.0f,0.8f,0.8f,1.0f);
	Freetype::print(FontManager::Instance()->GetFont( "Verdana", 12 ), 100, 60, "Sync time [%d]: %.2f ms",
		SharedData::Instance()->getDataSize(),
		getSyncTime()*1000.0);
	glColor4f(0.8f,0.8f,0.8f,1.0f);
	Freetype::print(FontManager::Instance()->GetFont( "Verdana", 12 ), 100, 40, "Swap groups: %s and %s (%s) [frame: %d]",
		getWindowPtr()->isUsingSwapGroups() ? "Enabled" : "Disabled",
		getWindowPtr()->isBarrierActive() ? "active" : "not active",
		getWindowPtr()->isSwapGroupMaster() ? "master" : "slave",
		lFrameNumber);

	//if active stereoscopic rendering
	if( ClusterManager::Instance()->getThisNodePtr()->stereo != ReadConfig::None )
	{
		glDrawBuffer(GL_BACK_LEFT);
		Freetype::print( FontManager::Instance()->GetFont( "Verdana", 12 ), 100, 140, "Active eye: Left");
		glDrawBuffer(GL_BACK_RIGHT);
		Freetype::print( FontManager::Instance()->GetFont( "Verdana", 12 ), 100, 140, "Active eye:          Right");
		glDrawBuffer(GL_BACK);
	}
	//if passive stereo
	if( ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport()->getEye() == Frustum::StereoLeftEye )
	{
		Freetype::print( FontManager::Instance()->GetFont( "Verdana", 12 ), 100, 140, "Active eye: Left");
	}
	else if( ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport()->getEye() == Frustum::StereoRightEye )
	{
		Freetype::print( FontManager::Instance()->GetFont( "Verdana", 12 ), 100, 140, "Active eye:          Right");
	}
	glPopAttrib();
}

void sgct::Engine::draw()
{
	enterCurrentViewport();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	Frustum * tmpFrustum = ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport()->getFrustum(mActiveFrustum);
	glFrustum( tmpFrustum->getLeft(),
		tmpFrustum->getRight(),
        tmpFrustum->getBottom(),
		tmpFrustum->getTop(),
        tmpFrustum->getNear(),
		tmpFrustum->getFar());

	//translate to user pos
	/*User * usrPtr = ClusterManager::Instance()->getUserPtr();
	glTranslatef(-usrPtr->getPosPtr(mActiveFrustum)->x,
              -usrPtr->getPosPtr(mActiveFrustum)->y,
              -usrPtr->getPosPtr(mActiveFrustum)->z);*/
	Viewport * tmpVP = ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport();
	glMultMatrixf( glm::value_ptr(tmpVP->viewMatrix[mActiveFrustum]) );

	glMatrixMode(GL_MODELVIEW);

	glm::mat4 modelMat =
		glm::translate( glm::mat4(1.0f), (*mConfig->getSceneOffset())) 
		* glm::yawPitchRoll( 
			mConfig->getYaw(),
			mConfig->getPitch(),
			mConfig->getRoll());
	
	glLoadMatrixf( glm::value_ptr(modelMat) );

	if( mDrawFn != NULL )
		mDrawFn();
}

void sgct::Engine::calculateFrustums()
{
	for(unsigned int i=0; i<ClusterManager::Instance()->getThisNodePtr()->getNumberOfViewports(); i++)
	{
		User * usrPtr = ClusterManager::Instance()->getUserPtr();
		ClusterManager::Instance()->getThisNodePtr()->getViewport(i)->calculateFrustum(
			Frustum::Mono,
			usrPtr->getPosPtr(),
			nearClippingPlaneDist,
			farClippingPlaneDist);

		ClusterManager::Instance()->getThisNodePtr()->getViewport(i)->calculateFrustum(
			Frustum::StereoLeftEye,
			usrPtr->getPosPtr(Frustum::StereoLeftEye),
			nearClippingPlaneDist,
			farClippingPlaneDist);

		ClusterManager::Instance()->getThisNodePtr()->getViewport(i)->calculateFrustum(
			Frustum::StereoRightEye,
			usrPtr->getPosPtr(Frustum::StereoRightEye),
			nearClippingPlaneDist,
			farClippingPlaneDist);
	}
}

void sgct::Engine::parseArguments( int argc, char* argv[] )
{
	//parse arguments
	sgct::MessageHandler::Instance()->print("Parsing arguments...");
	int i=0;
	while( i<argc )
	{
		if( strcmp(argv[i],"-config") == 0 && argc > (i+1))
		{
			configFilename.assign(argv[i+1]);
			i+=2;
		}
		else if( strcmp(argv[i],"--client") == 0 )
		{
			localRunningMode = NetworkManager::LocalClient;
			i++;
		}
		else if( strcmp(argv[i],"--slave") == 0 )
		{
			localRunningMode = NetworkManager::LocalClient;
			i++;
		}
		else if( strcmp(argv[i],"-local") == 0 && argc > (i+1) )
		{
			localRunningMode = NetworkManager::LocalServer;
			int tmpi = -1;
			std::stringstream ss( argv[i+1] );
			ss >> tmpi;
			ClusterManager::Instance()->setThisNodeId(tmpi);
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

void sgct::Engine::clearBuffer(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void sgct::Engine::printNodeInfo(unsigned int nodeId)
{
	sgct::MessageHandler::Instance()->print("This node has index %d.\n", nodeId);
}

void sgct::Engine::enterCurrentViewport()
{
	SGCTNode * tmpNode = ClusterManager::Instance()->getThisNodePtr();
	glViewport(
		static_cast<int>(tmpNode->getCurrentViewport()->getX() * static_cast<float>(getWindowPtr()->getHResolution())),
		static_cast<int>(tmpNode->getCurrentViewport()->getY() * static_cast<float>(getWindowPtr()->getVResolution())),
		static_cast<int>(tmpNode->getCurrentViewport()->getXSize() * static_cast<float>(getWindowPtr()->getHResolution())),
		static_cast<int>(tmpNode->getCurrentViewport()->getYSize() * static_cast<float>(getWindowPtr()->getVResolution())));
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
	nearClippingPlaneDist = _near;
	farClippingPlaneDist = _far;
	calculateFrustums();
}

void sgct::Engine::decodeExternalControl(const char * receivedData, int receivedLenght, int clientIndex)
{
	if(mNetworkCallbackFn != NULL && receivedLenght > 0)
		mNetworkCallbackFn(receivedData, receivedLenght, clientIndex);
}

void sgct::Engine::sendMessageToExternalControl(void * data, int lenght)
{
	if( mNetworkConnections->getExternalControlPtr() != NULL )
		mNetworkConnections->getExternalControlPtr()->sendData( data, lenght );
}

void sgct::Engine::sendMessageToExternalControl(const std::string msg)
{
	if( mNetworkConnections->getExternalControlPtr() != NULL )
		mNetworkConnections->getExternalControlPtr()->sendData( (void *)msg.c_str(), msg.size() );
}

void sgct::Engine::setExternalControlBufferSize(unsigned int newSize)
{
	if( mNetworkConnections->getExternalControlPtr() != NULL )
		mNetworkConnections->getExternalControlPtr()->setBufferSize(newSize);
}

const char * sgct::Engine::getBasicInfo()
{
	#if (_MSC_VER >= 1400) //visual studio 2005 or later
	sprintf_s( basicInfo, sizeof(basicInfo), "Node: %s (%s) | fps: %.2f",
		localRunningMode == NetworkManager::NotLocal ? ClusterManager::Instance()->getThisNodePtr()->ip.c_str() : "127.0.0.1",
		mNetworkConnections->isComputerServer() ? "master" : "slave",
		mStatistics.getAvgFPS());
    #else
    sprintf( basicInfo, "Node: %s (%s) | fps: %.2f",
		localRunningMode == NetworkManager::NotLocal ? ClusterManager::Instance()->getThisNodePtr()->ip.c_str() : "127.0.0.1",
		mNetworkConnections->isComputerServer() ? "master" : "slave",
		mStatistics.getAvgFPS());
    #endif

	return basicInfo;
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

void sgct::Engine::lockMutex(GLFWmutex &mutex)
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("Locking mutex... ");
#endif
    glfwLockMutex(mutex);
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("Done\n");
#endif
}

void sgct::Engine::unlockMutex(GLFWmutex &mutex)
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("Unlocking mutex... ");
#endif
    glfwUnlockMutex(mutex);
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("Done\n");
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
