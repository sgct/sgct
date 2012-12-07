/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _RENDER_ENGINE_H_
#define _RENDER_ENGINE_H_

#include "ClusterManager.h"
#include "NetworkManager.h"
#include "Statistics.h"
#include "ReadConfig.h"

#define MAX_UNIFORM_LOCATIONS 64

/*! \namespace sgct
\brief simple graphics cluster toolkit.
This namespace contains the most basic functionality of the toolkit.
*/
namespace sgct
{

/*!
The Engine class is the central part of sgct and handles most of the callbacks, rendering, network handling, input devices etc.

The figure below illustrates when different callbacks (gray boxes) are called in the renderloop. The blue boxes illustrates internal processess.

\image html render_diagram.jpg
\image latex render_diagram.eps "Render diagram" width=7cm
*/
class Engine
{
//all enums
public:
	enum RunMode { Default_Mode = 0, OSG_Encapsulation_Mode };

private:
	enum FBOBufferIndexes { LeftEyeBuffer = 0, RightEyeBuffer };
	enum FBOCubeMapBufferIndexes { FishEyeBuffer = 0, CubeMapBuffer };
	enum FBOModes { NoFBO = 0, RegularFBO, MultiSampledFBO, CubeMapFBO };
	enum SyncStage { PreStage = 0, PostStage };
	enum BufferMode { BackBuffer = 0, BackBufferBlack, RenderToTexture };
	enum ViewportSpace { ScreenSpace = 0, FBOSpace };
	enum ShaderLocIndexes { LeftTex = 0, RightTex, Cubemap, FishEyeHalfFov,
			SizeX, SizeY, FXAASubPixShift, FXAASpanMax, FXAARedMul, FXAAOffset, FXAATexture };

public:
	Engine( int& argc, char**& argv );
	~Engine();

	bool init(RunMode rm = Default_Mode);
	void terminate();
	void render();
	static Engine * getPtr() { return mThis; }

	const double & getDt();
	const double & getDrawTime();
	const double & getSyncTime();
	const float * getClearColor() { return mClearColor; }
	const float * getFisheyeClearColor() { return mFisheyeClearColor; }
	void setNearAndFarClippingPlanes(float _near, float _far);
	void setClearColor(float red, float green, float blue, float alpha);
	void setFisheyeClearColor(float red, float green, float blue);
	const float& getNearClippingPlane() const { return mNearClippingPlaneDist; }
	const float& getFarClippingPlane() const { return mFarClippingPlaneDist; }
	void setWireframe(bool state) { mShowWireframe = state; }
	void setDisplayInfoVisibility(bool state) { mShowInfo = state; }
	void setStatsGraphVisibility(bool state) { mShowGraph = state; }
	void takeScreenshot() { mTakeScreenshot = true; }

	size_t createTimer( double millisec, void(*fnPtr)(size_t) );
	void stopTimer(size_t id);

    //set callback functions
	void setInitOGLFunction( void(*fnPtr)(void) );
	void setPreSyncFunction( void(*fnPtr)(void));
	void setPostSyncPreDrawFunction( void(*fnPtr)(void));
	void setClearBufferFunction( void(*fnPtr)(void) );
	void setDrawFunction( void(*fnPtr)(void) );
	void setPostDrawFunction( void(*fnPtr)(void) );
	void setCleanUpFunction( void(*fnPtr)(void) );
	void setKeyboardCallbackFunction( void(*fnPtr)(int, int) ); //arguments: int key, int action
	void setCharCallbackFunction( void(*fnPtr)(int, int) ); //arguments: int character, int action
	void setMouseButtonCallbackFunction( void(*fnPtr)(int, int) ); //arguments: int button, int action
	void setMousePosCallbackFunction( void(*fnPtr)(int, int) ); //arguments: int x, int y
	void setMouseScrollCallbackFunction( void(*fnPtr)(int) ); //arguments: int pos

	//external control network functions
	void setExternalControlCallback( void(*fnPtr)(const char *, int, int) ); //arguments: chonst char * buffer, int buffer length, int clientIndex
	void sendMessageToExternalControl(void * data, int length);
	void sendMessageToExternalControl(const std::string msg);
	void setExternalControlBufferSize(unsigned int newSize);
	void decodeExternalControl(const char * receivedData, int receivedlength, int clientIndex);

    //GLFW wrapped functions
    static GLFWmutex createMutex();
    static GLFWcond createCondition();
    static void destroyCond(GLFWcond &cond);
    static void destroyMutex(GLFWmutex &mutex);
	static void lockMutex(GLFWmutex &mutex);
	static void unlockMutex(GLFWmutex &mutex);
	static void waitCond(GLFWcond &cond, GLFWmutex &mutex, double timeout);
	static void signalCond(GLFWcond &cond);
	static double getTime();
	static int getKey( const int &key );
	static int getMouseButton( const int &button );
	static void getMousePos( int * xPos, int * yPos );
	static void setMousePos( const int &xPos, const int &yPos );
	static int getMouseWheel();
	static void setMouseWheel( const int &pos );
	static void setMousePointerVisibility( bool state );
	static int getJoystickParam( const int &joystick, const int &param );
	static int getJoystickAxes( const int &joystick, float * values, const int &numOfValues);
	static int getJoystickButtons( const int &joystick, unsigned char * values, const int &numOfValues);
	static void sleep(double secs);

	static sgct_core::SGCTWindow * getWindowPtr() { return sgct_core::ClusterManager::Instance()->getThisNodePtr()->getWindowPtr(); }
	static sgct_core::SGCTUser * getUserPtr() { return sgct_core::ClusterManager::Instance()->getUserPtr(); }
	static sgct::SGCTTrackingManager * getTrackingManager() { return sgct_core::ClusterManager::Instance()->getTrackingManagerPtr(); }
	static bool checkForOGLErrors();

	inline bool isMaster() { return mNetworkConnections->isComputerServer(); }
	inline bool isDisplayInfoRendered() { return mShowInfo; }
	//! Returns true if render target is off screen (FBO) or false if render target is the frame buffer.
	inline bool isRenderingOffScreen() { return mRenderingOffScreen; }
	inline const sgct_core::Frustum::FrustumMode & getActiveFrustum() { return mActiveFrustum; }

	//will only return valid values when called in the draw callback function
	inline const glm::mat4 & getActiveFrustumMatrix() { return sgct_core::ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport()->getFrustumMatrix( mActiveFrustum ); }
	inline const glm::mat4 & getActiveProjectionMatrix() { return sgct_core::ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport()->getProjectionMatrix( mActiveFrustum ); }
	inline const int * getActiveViewport() { return currentViewportCoords; }
	inline unsigned long long getCurrentFrameNumber() { return mFrameCounter; }
	//! Returns the framebuffer's aspect ratio
	inline float getAspectRatio() { return mAspectRatio; }

	//can be called any time after Engine init
	inline const glm::mat4 & getSceneTransform() { return sgct_core::ClusterManager::Instance()->getSceneTransform(); }

private:
	Engine() {;} //to prevent users to start without requred parameters

	bool initNetwork();
	bool initWindow();
	void initOGL();
	void clean();
	void clearAllCallbacks();

	void frameSyncAndLock(SyncStage stage);
	void calculateFPS(double timestamp);
	void parseArguments( int& argc, char**& argv );
	void renderDisplayInfo();
	void calculateFrustums();
	void printNodeInfo(unsigned int nodeId);
	void enterCurrentViewport(ViewportSpace vs);
	const char * getBasicInfo();
	const char * getAAInfo();

	void draw();
	void drawOverlays();
	void setRenderTarget(FBOBufferIndexes bi);
	void renderFBOTexture();
	void renderFisheye();
	void updateRenderingTargets();
	void updateTimers(double timeStamp);
	void loadShaders();
	void createFBOs();
	void initFisheye();
	void resizeFBOs();
	void setAndClearBuffer(BufferMode mode);
	void captureBuffer();
	void waitForAllWindowsInSwapGroupToOpen();

	static void clearBuffer();

private:
	// Convinience typedef
	typedef void (*CallbackFn)(void);
	typedef void (Engine::*InternalCallbackFn)(void);
	typedef void (*NetworkCallbackFn)(const char *, int, int);
	typedef void (*inputCallbackFn)(int, int);
	typedef void (*scrollCallbackFn)(int);
    typedef void (*timerCallbackFn)(size_t);

	//function pointers
	CallbackFn mDrawFn;
	CallbackFn mPreSyncFn;
	CallbackFn mPostSyncPreDrawFn;
	CallbackFn mPostDrawFn;
	CallbackFn mInitOGLFn;
	CallbackFn mClearBufferFn;
	CallbackFn mCleanUpFn;
	InternalCallbackFn mInternalRenderFn;
	NetworkCallbackFn mNetworkCallbackFn;

	//GLFW wrapped function pointers
	inputCallbackFn mKeyboardCallbackFn;
	inputCallbackFn mCharCallbackFn;
	inputCallbackFn mMouseButtonCallbackFn;
	inputCallbackFn mMousePosCallbackFn;
	scrollCallbackFn mMouseWheelCallbackFn;

	float mNearClippingPlaneDist;
	float mFarClippingPlaneDist;
	float mClearColor[4];

	//fisheye stuff
	float mFisheyeClearColor[4];
	float mFisheyeQuadVerts[20];

	int localRunningMode;
	sgct_core::Frustum::FrustumMode mActiveFrustum;
	int currentViewportCoords[4];

	bool mShowInfo;
	bool mShowGraph;
	bool mShowWireframe;
	bool mTakeScreenshot;
	bool mTerminate;
	bool mIgnoreSync;
	bool mRenderingOffScreen;

	//objects
	sgct_core::Statistics	mStatistics;

	//FBO stuff
	unsigned int mFrameBuffers[2];
	unsigned int mMultiSampledFrameBuffers[2];
	unsigned int mRenderBuffers[2];
	unsigned int mDepthBuffers[2];
	unsigned int mFrameBufferTextures[2];
	int mFBOMode;

	//glsl
	int mShaderLocs[MAX_UNIFORM_LOCATIONS];

	//pointers
	sgct_core::NetworkManager * mNetworkConnections;
	sgct_core::ReadConfig	* mConfig;

	std::string configFilename;
	int mRunning;
	float mAspectRatio;
	char basicInfo[128];
	char aaInfo[16];

	unsigned long long mFrameCounter;

    typedef struct  {
        size_t mId;
        double mLastFired;
        double mInterval;
        timerCallbackFn mCallback;
    } TimerInformation;

    std::vector<TimerInformation> mTimers; //< stores all active timers
    size_t mTimerID; //< the timer created next will use this ID

	static Engine * mThis;
	RunMode mRunMode;
};

}

#endif
