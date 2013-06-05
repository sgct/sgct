/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _RENDER_ENGINE_H_
#define _RENDER_ENGINE_H_

#include "NetworkManager.h"
#include "ClusterManager.h"
#include "SGCTMutexManager.h"
#include "Statistics.h"
#include "ReadConfig.h"
#include "OffScreenBuffer.h"
#include "ScreenCapture.h"
#include "ShaderProgram.h"

#define MAX_UNIFORM_LOCATIONS 128
#define NUMBER_OF_SHADERS 6
#define NUMBER_OF_VBOS 2

/*! \namespace sgct
\brief Simple Graphics Cluster Toolkit.
This namespace contains the most basic functionality of the toolkit.
*/
namespace sgct
{

/*!
	Wrapper for GLFWcond
*/
typedef void * SGCTcond;

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
	/*!
		The different run modes used by the init function
	*/
	enum RunMode { Default_Mode = 0, OSG_Encapsulation_Mode, OpenGL_Compablity_Profile, OpenGL_3_3_Core_Profile, OpenGL_4_0_Core_Profile, OpenGL_4_1_Core_Profile, OpenGL_4_2_Core_Profile, OpenGL_4_3_Core_Profile };

private:
	enum VBOIndexes { RenderQuad = 0, FishEyeQuad };
	enum TextureIndexes { PostFX = 0, LeftEye, RightEye, FishEye };
	enum SyncStage { PreStage = 0, PostStage };
	enum BufferMode { BackBuffer = 0, BackBufferBlack, RenderToTexture };
	enum ViewportSpace { ScreenSpace = 0, FBOSpace };
	enum ShaderIndexes { FBOQuadShader = 0, FXAAShader, FisheyeShader, StereoShader, OverlayShader, NULLShader };
	enum ShaderLocIndexes { MonoMVP = 0, MonoTex,
			OverlayMVP, OverlayTex,
			StereoMVP, StereoLeftTex, StereoRightTex,
			FisheyeMVP, Cubemap, FishEyeHalfFov, FisheyeOffset,
			SizeX, SizeY, FXAAMVP, FXAASubPixShift, FXAASpanMax, FXAARedMul, FXAAOffset, FXAATexture };

public:
	Engine( int& argc, char**& argv );
	~Engine();

	bool init(RunMode rm = Default_Mode);
	void terminate();
	void render();
	/*!
		\returns the static pointer to the engine instance
	*/
	static Engine * getPtr() { return mInstance; }
	/*!
		\returns the static pointer to the engine instance
	*/
	static Engine * Instance() { return mInstance; }

	const double & getDt();
	const double & getAvgDt();
	const double & getDrawTime();
	const double & getSyncTime();

	/*!
		\returns the clear color as 4 floats (RGBA)
	*/
	const float * getClearColor() { return mClearColor; }
	/*!
		\returns the clear color surrounding the fisheye circle as 4 floats (RGBA)
	*/
	const float * getFisheyeClearColor() { return mFisheyeClearColor; }
	/*!
		\returns the near clipping plane distance in meters
	*/
	const float& getNearClippingPlane() const { return mNearClippingPlaneDist; }
	/*!
		\returns the far clipping plane distance in meters
	*/
	const float& getFarClippingPlane() const { return mFarClippingPlaneDist; }

	void setNearAndFarClippingPlanes(float nearClippingPlane, float farClippingPlane);
	void setEyeSeparation(float eyeSeparation);
	void setClearColor(float red, float green, float blue, float alpha);
	void setFisheyeClearColor(float red, float green, float blue);
	void setExitKey(int key);

	/*!
		\param state of the wireframe rendering
	*/
	void setWireframe(bool state) { mShowWireframe = state; }
	/*!
		Set if the info text should be visible or not

		\param state of the info text rendering
	*/
	void setDisplayInfoVisibility(bool state) { mShowInfo = state; }

	/*!
		Set if the statistics graph should be visible or not

		\param state of the statistics graph rendering
	*/
	void setStatsGraphVisibility(bool state) { mShowGraph = state; }

	/*!
		Take a RGBA screenshot and save it as a PNG file. If stereo rendering is enabled then two screenshots will be saved per frame, one for the left eye and one for the right eye.
		To record frames for a movie simply call this function every frame you wish to record. The read to disk is multi-threaded and maximum number of threads can be set using:
		-numberOfCaptureThreads command line argument.
	*/
	void takeScreenshot() { mTakeScreenshot = true; }
	/*!
		\returns the current screenshot number (file index)
	*/
	int getScreenShotNumber() { return mShotCounter; }
	/*!
		Set the screenshot number (exising images will be replaced)
		\param mShotCounter is the frame number which will be added to the filename of the screenshot
	*/
	void setScreenShotNumber(int number) {  mShotCounter = number; }

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
	bool isExternalControlConnected();
	void setExternalControlBufferSize(unsigned int newSize);
	void decodeExternalControl(const char * receivedData, int receivedlength, int clientIndex);

    //GLFW wrapped functions
    static SGCTcond createCondition();
    static void destroyCond(SGCTcond cond);
	static void waitCond(SGCTcond cond, SGCTmutex mutex, double timeout);
	static void signalCond(SGCTcond cond);
	static double getTime();
	static int getKey( int key );
	static int getMouseButton( int button );
	static void getMousePos( int * xPos, int * yPos );
	static void setMousePos( int xPos, int yPos );
	static int getMouseWheel();
	static void setMouseWheel( int pos );
	static void setMousePointerVisibility( bool state );
	static int getJoystickParam( int joystick, int param );
	static int getJoystickAxes( int joystick, float * values, int numOfValues);
	static int getJoystickButtons( int joystick, unsigned char * values, int numOfValues);
	static void sleep(double secs);

	/*!
		Returns a pointer to the window object
	*/
	static sgct_core::SGCTWindow * getWindowPtr() { return sgct_core::ClusterManager::Instance()->getThisNodePtr()->getWindowPtr(); }

	/*!
		Returns a pinter to the user (VR observer position) object
	*/
	static sgct_core::SGCTUser * getUserPtr() { return sgct_core::ClusterManager::Instance()->getUserPtr(); }

	/*!
		Returns pointer to screen capture object/handler
	*/
	const sgct_core::ScreenCapture * getScreenCapturePointer() { return mScreenCapture; }

	/*!
		Returns the stereo mode. The value can be compared to the sgct_core::ClusterManager::StereoMode enum
	*/
	static int getStereoMode() { return sgct_core::ClusterManager::Instance()->getThisNodePtr()->stereo; }

	/*!
		Returns true if any kind of stereo is enabled
	*/
	static bool isStereo() { return (sgct_core::ClusterManager::Instance()->getThisNodePtr()->stereo != sgct_core::ClusterManager::NoStereo); }

	/*!
		Returns a pointer to the tracking manager pointer
	*/
	static sgct::SGCTTrackingManager * getTrackingManager() { return sgct_core::ClusterManager::Instance()->getTrackingManagerPtr(); }

	/*!
		Check and print if any openGL error has occured

		/returns false if any error occured
	*/
	static bool checkForOGLErrors();

	/*!
		Get the user's eye separation in meters
		*/
	static float getEyeSeparation() { return mInstance->getUserPtr()->getEyeSeparation(); }

	/*!
		Returns true if this node is the master
	*/
	inline bool isMaster() { return mNetworkConnections->isComputerServer(); }

	/*!
		Returns true if on-screen info is rendered.
	*/
	inline bool isDisplayInfoRendered() { return mShowInfo; }

	/*!
		Returns true if render target is off screen (FBO) or false if render target is the frame buffer.
	*/
	inline bool isRenderingOffScreen() { return mRenderingOffScreen; }

	/*!
		Returns the active frustum which can be one of the following:
		- Mono
		- Stereo Left
		- Stereo Right
	*/
	inline const sgct_core::Frustum::FrustumMode & getActiveFrustum() { return mActiveFrustum; }

	/*!
		Returns the active projection matrix (only valid inside in the draw callback function)
	*/
	inline const glm::mat4 & getActiveProjectionMatrix() { return sgct_core::ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport()->getProjectionMatrix( mActiveFrustum ); }

	/*!
		Returns the active view matrix (only valid inside in the draw callback function)
	*/
	inline const glm::mat4 & getActiveViewMatrix() { return sgct_core::ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport()->getViewMatrix( mActiveFrustum ); }

	/*!
		Returns the scene transform specified in the XML configuration, default is a identity matrix
	*/
	inline const glm::mat4 & getModelMatrix() { return sgct_core::ClusterManager::Instance()->getSceneTransform(); }

	/*!
		Returns the active VP = Projection * View matrix (only valid inside in the draw callback function)
	*/
	inline const glm::mat4 & getActiveViewProjectionMatrix() { return sgct_core::ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport()->getViewProjectionMatrix( mActiveFrustum ); }

	/*!
		Returns the active MVP = Projection * View * Model matrix (only valid inside in the draw callback function)
	*/
	inline glm::mat4 getActiveModelViewProjectionMatrix() { return sgct_core::ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport()->getViewProjectionMatrix( mActiveFrustum )
		* sgct_core::ClusterManager::Instance()->getSceneTransform(); }

	/*!
		Returns the active viewport in pixels (only valid inside in the draw callback function)
	*/
	inline const int * getActiveViewport() { return currentViewportCoords; }

	/*!
		Returns the current frame number
	*/
	inline unsigned int getCurrentFrameNumber() { return mFrameCounter; }

	/*!
		Return true if OpenGL pipeline is fixed (openGL 1-2) or false if OpenGL pipeline is programmable (openGL 3-4)
	*/
	inline bool isOGLPipelineFixed() { return mFixedOGLPipeline; }

	bool isFisheye();
	sgct_core::OffScreenBuffer * getFBOPtr();
	void getFBODimensions( int & width, int & height );

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
	void renderFBOTexture();
	void renderPostFx(TextureIndexes ti );
	void renderFisheye(TextureIndexes ti);

	void drawFixedPipeline();
	void drawOverlaysFixedPipeline();
	void renderFBOTextureFixedPipeline();
	void renderPostFxFixedPipeline(TextureIndexes ti );
	void renderFisheyeFixedPipeline(TextureIndexes ti);

	void setRenderTarget(TextureIndexes ti);
	void updateRenderingTargets(TextureIndexes ti);
	void updateTimers(double timeStamp);
	void loadShaders();
	void createTextures();
	void createFBOs();
	void createVBOs();
	void initFisheye();
	void resizeFBOs();
	void setAndClearBuffer(BufferMode mode);
	void captureBuffer();
	void waitForAllWindowsInSwapGroupToOpen();

	static void clearBuffer();

private:
	static Engine * mInstance;

	// Convinience typedef
	typedef void (*CallbackFn)(void);
	typedef void (Engine::*InternalCallbackFn)(void);
	typedef void (Engine::*InternalCallbackTexArgFn)(TextureIndexes);
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
	InternalCallbackFn			mInternalDrawFn;
	InternalCallbackFn			mInternalRenderFBOFn;
	InternalCallbackFn			mInternalDrawOverlaysFn;
	InternalCallbackTexArgFn	mInternalRenderPostFXFn;
	InternalCallbackTexArgFn	mInternalRenderFisheyeFn;
	NetworkCallbackFn			mNetworkCallbackFn;

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
	int mShotCounter;
	sgct_core::Frustum::FrustumMode mActiveFrustum;
	int currentViewportCoords[4];

	bool mShowInfo;
	bool mShowGraph;
	bool mShowWireframe;
	bool mTakeScreenshot;
	bool mTerminate;
	bool mIgnoreSync;
	bool mRenderingOffScreen;
	bool mFixedOGLPipeline;

	//objects
	ShaderProgram mShaders[NUMBER_OF_SHADERS];

	//FBO stuff
	sgct_core::OffScreenBuffer * mFinalFBO_Ptr;
	sgct_core::OffScreenBuffer * mCubeMapFBO_Ptr;

	unsigned int mFrameBufferTextures[4];

	//glsl
	int mShaderLocs[MAX_UNIFORM_LOCATIONS];

	//pointers
	sgct_core::NetworkManager * mNetworkConnections;
	sgct_core::ReadConfig	* mConfig;
	sgct_core::Statistics	 * mStatistics;
	sgct_core::ScreenCapture * mScreenCapture;

	float mPostFxQuadVerts[20];

	std::string configFilename;
	int mRunning;
	char basicInfo[256];
	char aaInfo[32];

	unsigned int mFrameCounter;

    typedef struct  {
        size_t mId;
        double mLastFired;
        double mInterval;
        timerCallbackFn mCallback;
    } TimerInformation;

    std::vector<TimerInformation> mTimers; //< stores all active timers
    size_t mTimerID; //< the timer created next will use this ID

	//VBO:s
	unsigned int mVBO[NUMBER_OF_VBOS];
	//VAO:s
	unsigned int mVAO[NUMBER_OF_VBOS];

	RunMode mRunMode;
	int mExitKey;
};

}

#endif
