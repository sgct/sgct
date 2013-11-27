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
#include "ShaderProgram.h"

#define MAX_UNIFORM_LOCATIONS 16
#define NUMBER_OF_SHADERS 8

/*! \namespace sgct
\brief SGCT namespace contains the most basic functionality of the toolkit
*/
namespace sgct
{
/*!
The Engine class is the central part of sgct and handles most of the callbacks, rendering, network handling, input devices etc.

The figure below illustrates when different callbacks (gray and blue boxes) are called in the renderloop. The blue boxes illustrates internal processess.

\image html render_diagram.jpg
\image latex render_diagram.eps "Render diagram" width=7cm
*/
class Engine
{
//all enums
public:
	
	//! The different run modes used by the init function
	enum RunMode
	{ 
		/// The default mode using fixed OpenGL pipeline (compability mode)
		Default_Mode = 0,
		/// This option encapsulates Open Scene Graph (fixed OpenGL pipeline)
		OSG_Encapsulation_Mode,
		/// This option is using a fixed OpenGL pipeline that allows mixing legacy and modern OpenGL
		OpenGL_Compablity_Profile,
		/// This option is using a programmable OpenGL 3.3 pipeline using a core profile
		OpenGL_3_3_Core_Profile,
		/// This option is using a programmable OpenGL 4.0 pipeline using a core profile
		OpenGL_4_0_Core_Profile,
		/// This option is using a programmable OpenGL 4.1 pipeline using a core profile
		OpenGL_4_1_Core_Profile,
		/// This option is using a programmable OpenGL 4.2 pipeline using a core profile
		OpenGL_4_2_Core_Profile,
		/// This option is using a programmable OpenGL 4.3 pipeline using a core profile
		OpenGL_4_3_Core_Profile,
		/// This option is using a programmable OpenGL 4.4 pipeline using a core profile
		OpenGL_4_4_Core_Profile
	};
	//! The different texture indexes in window buffers
	enum TextureIndexes { LeftEye = 0, RightEye, Intermediate, FX1, FX2, Depth, CubeMap, CubeMapDepth, FisheyeColorSwap, FisheyeDepthSwap };

private:
	enum SyncStage { PreStage = 0, PostStage };
	enum BufferMode { BackBuffer = 0, BackBufferBlack, RenderToTexture };
	enum ViewportSpace { ScreenSpace = 0, FBOSpace };
	enum ShaderIndexes { FBOQuadShader = 0, FXAAShader, OverlayShader };
	enum ShaderLocIndexes { MonoMVP = 0, MonoTex,
			OverlayMVP, OverlayTex,
			SizeX, SizeY, FXAA_MVP, FXAA_SUBPIX_TRIM, FXAA_SUBPIX_OFFSET, FXAA_Texture };

public:
	Engine( int& argc, char**& argv );
	~Engine();

	bool init(RunMode rm = Default_Mode);
	void terminate();
	void render();
	
	/*!
		\returns the static pointer to the engine instance
	*/
	static Engine * instance() { return mInstance; }

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
	void setExitWaitTime(double time);
	void updateFrustums();
	void addPostFX( PostFX & fx );
	unsigned int getActiveDrawTexture();
	unsigned int getActiveDepthTexture();
	int getActiveXResolution();
	int getActiveYResolution();
	std::size_t getFocusedWindowIndex();

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

	size_t createTimer( double millisec, void(*fnPtr)(size_t) );
	void stopTimer(size_t id);

    //set callback functions
	void setInitOGLFunction( void(*fnPtr)(void) );
	void setPreWindowFunction( void(*fnPtr)(void) );
	void setPreSyncFunction( void(*fnPtr)(void) );
	void setPostSyncPreDrawFunction( void(*fnPtr)(void) );
	void setClearBufferFunction( void(*fnPtr)(void) );
	void setDrawFunction( void(*fnPtr)(void) );
	void setDraw2DFunction( void(*fnPtr)(void) );
	void setPostDrawFunction( void(*fnPtr)(void) );
	void setCleanUpFunction( void(*fnPtr)(void) );
	void setKeyboardCallbackFunction( void(*fnPtr)(int, int) ); //arguments: int key, int action
	void setCharCallbackFunction( void(*fnPtr)(unsigned int) ); //arguments: unsigned int unicode character
	void setMouseButtonCallbackFunction( void(*fnPtr)(int, int) ); //arguments: int button, int action
	void setMousePosCallbackFunction( void(*fnPtr)(double, double) ); //arguments: double x, double y
	void setMouseScrollCallbackFunction( void(*fnPtr)(double, double) ); //arguments: double xoffset, double yoffset

	//external control network functions
	void setExternalControlCallback( void(*fnPtr)(const char *, int, int) ); //arguments: chonst char * buffer, int buffer length, int clientIndex
	void sendMessageToExternalControl(void * data, int length);
	void sendMessageToExternalControl(const std::string msg);
	bool isExternalControlConnected();
	void setExternalControlBufferSize(unsigned int newSize);
	void decodeExternalControl(const char * receivedData, int receivedlength, int clientIndex);

    //GLFW wrapped functions
	static double getTime();
	static int getKey( std::size_t winIndex, int key );
	static int getMouseButton( std::size_t winIndex, int button );
	static void getMousePos( std::size_t winIndex, double * xPos, double * yPos );
	static void setMousePos( std::size_t winIndex, double xPos, double yPos );
	static void setMouseCursorVisibility( std::size_t winIndex, bool state );
	static const char * getJoystickName( int joystick );
	static const float * getJoystickAxes( int joystick, int * numOfValues);
	static const unsigned char * getJoystickButtons( int joystick, int * numOfValues);
	static void sleep(double secs);

	/*!
		Returns a pointer to this node (running on this computer).
	*/
	inline sgct_core::SGCTNode * getThisNodePtr(std::size_t index) { return mThisNode; }

	/*!
		Returns a pointer to a specified window by index on this node.
	*/
	inline sgct::SGCTWindow * getWindowPtr(std::size_t index) { return mThisNode->getWindowPtr(index); }

	/*!
		Returns the number of windows for this node.
	*/
	inline std::size_t getNumberOfWindows() { return mThisNode->getNumberOfWindows(); }

	/*!
		Returns a pointer to the current window that is beeing rendered
	*/
	inline sgct::SGCTWindow * getActiveWindowPtr() { return mThisNode->getActiveWindowPtr(); }

	/*!
		Returns the active viewport in pixels (only valid inside in the draw callback function)
	*/
	inline const int * getActiveViewportPixelCoords() { return currentViewportCoords; }

	/*!
		Returns a pinter to the user (VR observer position) object
	*/
	static sgct_core::SGCTUser * getUserPtr() { return sgct_core::ClusterManager::instance()->getUserPtr(); }

	/*!
		Returns a pointer to the tracking manager pointer
	*/
	static sgct::SGCTTrackingManager * getTrackingManager() { return sgct_core::ClusterManager::instance()->getTrackingManagerPtr(); }

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
		Returns the active frustum mode which can be one of the following:
		- Mono
		- Stereo Left
		- Stereo Right
	*/
	inline const sgct_core::Frustum::FrustumMode & getActiveFrustumMode() { return mActiveFrustumMode; }

	/*!
		Returns the active projection matrix (only valid inside in the draw callback function)
	*/
	inline const glm::mat4 & getActiveProjectionMatrix() { return getActiveWindowPtr()->getCurrentViewport()->getProjectionMatrix( mActiveFrustumMode ); }

	/*!
		Returns the active view matrix (only valid inside in the draw callback function)
	*/
	inline const glm::mat4 & getActiveViewMatrix() { return getActiveWindowPtr()->getCurrentViewport()->getViewMatrix( mActiveFrustumMode ); }

	/*!
		Returns the scene transform specified in the XML configuration, default is a identity matrix
	*/
	inline const glm::mat4 & getModelMatrix() { return sgct_core::ClusterManager::instance()->getSceneTransform(); }

	/*!
		Returns the active VP = Projection * View matrix (only valid inside in the draw callback function)
	*/
	inline const glm::mat4 & getActiveViewProjectionMatrix() { return mThisNode->getActiveWindowPtr()->getCurrentViewport()->getViewProjectionMatrix( mActiveFrustumMode ); }

	/*!
		Returns the active MVP = Projection * View * Model matrix (only valid inside in the draw callback function)
	*/
	inline glm::mat4 getActiveModelViewProjectionMatrix() { return getActiveWindowPtr()->getCurrentViewport()->getViewProjectionMatrix( mActiveFrustumMode )
		* sgct_core::ClusterManager::instance()->getSceneTransform(); }
	
	/*!
		Returns the active MV = View * Model matrix (only valid inside in the draw callback function)
	*/
	inline glm::mat4 getActiveModelViewMatrix() { return getActiveWindowPtr()->getCurrentViewport()->getViewMatrix( mActiveFrustumMode )
		* sgct_core::ClusterManager::instance()->getSceneTransform(); }

	/*!
		Returns the current frame number
	*/
	inline unsigned int getCurrentFrameNumber() { return mFrameCounter; }

	/*!
		Return true if OpenGL pipeline is fixed (openGL 1-2) or false if OpenGL pipeline is programmable (openGL 3-4)
	*/
	inline bool isOGLPipelineFixed() { return mFixedOGLPipeline; }

	/*!
		Get the current run mode setting (context version and compability modes)
	*/
	inline RunMode getRunMode() { return mRunMode; }

	void getActiveViewportSize(int & x, int & y);

private:
	Engine() {;} //to prevent users to start without requred parameters

	bool initNetwork();
	bool initWindows();
	void initOGL();
	void clean();
	void clearAllCallbacks();

	void frameLock(SyncStage stage);
	void calculateFPS(double timestamp);
	void parseArguments( int& argc, char**& argv );
	void renderDisplayInfo();
	void printNodeInfo(unsigned int nodeId);
	void enterCurrentViewport();
	void enterFisheyeViewport();
	const char * getBasicInfo(std::size_t winIndex);
	const char * getAAInfo(std::size_t winIndex);

	void draw();
	void drawOverlays();
	void renderFBOTexture();
	void renderPostFX(TextureIndexes ti );
	void renderFisheye(TextureIndexes ti);
	void renderViewports(TextureIndexes ti);
	void render2D();

	void drawFixedPipeline();
	void drawOverlaysFixedPipeline();
	void renderFBOTextureFixedPipeline();
	void renderPostFXFixedPipeline(TextureIndexes finalTargetIndex );
	void renderFisheyeFixedPipeline(TextureIndexes finalTargetIndex);

	void prepareBuffer(TextureIndexes ti);
	void updateRenderingTargets(TextureIndexes ti);
	void updateTimers(double timeStamp);
	void loadShaders();
	void setAndClearBuffer(BufferMode mode);
	void waitForAllWindowsInSwapGroupToOpen();

	static void clearBuffer();
	static void internal_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void internal_key_char_callback(GLFWwindow* window, unsigned int ch);
	static void internal_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	static void internal_mouse_pos_callback(GLFWwindow* window, double xPos, double yPos);
	static void internal_mouse_scroll_callback(GLFWwindow* window, double xOffset, double yOffset);
	static void internal_glfw_error_callback(int error, const char* description);
	static void outputHelpMessage();

private:
	static Engine * mInstance;

	// Convinience typedef
	typedef void (*CallbackFn)(void);
	typedef void (Engine::*InternalCallbackFn)(void);
	typedef void (Engine::*InternalCallbackTexArgFn)(TextureIndexes);
	typedef void (*NetworkCallbackFn)(const char *, int, int);
    typedef void (*timerCallbackFn)(size_t);

	//function pointers
	CallbackFn mDrawFn;
	CallbackFn mPreSyncFn;
	CallbackFn mPostSyncPreDrawFn;
	CallbackFn mPostDrawFn;
	CallbackFn mPreWindowFn;
	CallbackFn mInitOGLFn;
	CallbackFn mClearBufferFn;
	CallbackFn mCleanUpFn;
	CallbackFn mDraw2DFn;
	InternalCallbackFn			mInternalDrawFn;
	InternalCallbackFn			mInternalRenderFBOFn;
	InternalCallbackFn			mInternalDrawOverlaysFn;
	InternalCallbackTexArgFn	mInternalRenderPostFXFn;
	InternalCallbackTexArgFn	mInternalRenderFisheyeFn;
	NetworkCallbackFn			mNetworkCallbackFn;

	float mNearClippingPlaneDist;
	float mFarClippingPlaneDist;
	float mClearColor[4];
	float mFisheyeClearColor[4];

	int localRunningMode;
	sgct_core::Frustum::FrustumMode mActiveFrustumMode;
	int currentViewportCoords[4];

	bool mShowInfo;
	bool mShowGraph;
	bool mShowWireframe;
	bool mTakeScreenshot;
	bool mTerminate;
	bool mIgnoreSync;
	bool mRenderingOffScreen;
	bool mFixedOGLPipeline;
	bool mHelpMode;

	//objects
	ShaderProgram mShaders[NUMBER_OF_SHADERS];

	//glsl
	int mShaderLocs[MAX_UNIFORM_LOCATIONS];

	//pointers
	sgct_core::NetworkManager	* mNetworkConnections;
	sgct_core::ReadConfig		* mConfig;
	sgct_core::Statistics		* mStatistics;
	sgct_core::SGCTNode			* mThisNode;

	tthread::thread * mThreadPtr;

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

	RunMode mRunMode;
	int mExitKey;
};

}

#endif
