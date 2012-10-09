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

#ifndef _RENDER_ENGINE_H_
#define _RENDER_ENGINE_H_

#include "ClusterManager.h"
#include "NetworkManager.h"
#include "Statistics.h"
#include "ReadConfig.h"

namespace sgct //simple graphics cluster toolkit
{

class Engine
{
public:
	Engine( int& argc, char**& argv );
	~Engine()
	{
		clean();
	}

	bool init();
	void render();
	void terminate() { mTerminate = true; }
	static Engine * getPtr() { return mThis; }

	const double & getDt();
	const double & getDrawTime();
	const double & getSyncTime();
	const float * getClearColor() { return mClearColor; }
	void setNearAndFarClippingPlanes(float _near, float _far);
	void setClearColor(float red, float green, float blue, float alpha);
	const float& getNearClippingPlane() const { return mNearClippingPlaneDist; }
	const float& getFarClippingPlaine() const { return mFarClippingPlaneDist; }
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

	static core_sgct::SGCTWindow * getWindowPtr() { return core_sgct::ClusterManager::Instance()->getThisNodePtr()->getWindowPtr(); }
	static core_sgct::SGCTUser * getUserPtr() { return core_sgct::ClusterManager::Instance()->getUserPtr(); }
	static sgct::SGCTTrackingManager * getTrackingManager() { return core_sgct::ClusterManager::Instance()->getTrackingManagerPtr(); }
	static void checkForOGLErrors();

	inline bool isMaster() { return mNetworkConnections->isComputerServer(); }
	inline bool isDisplayInfoRendered() { return mShowInfo; }
	inline const core_sgct::Frustum::FrustumMode & getActiveFrustum() { return mActiveFrustum; }

	//will only return valid values when called in the draw callback function
	inline const glm::mat4 & getActiveFrustumMatrix() { return core_sgct::ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport()->getFrustumMatrix( mActiveFrustum ); }
	inline const glm::mat4 & getActiveProjectionMatrix() { return core_sgct::ClusterManager::Instance()->getThisNodePtr()->getCurrentViewport()->getProjectionMatrix( mActiveFrustum ); }
	inline const int * getActiveViewport() { return currentViewportCoords; }
	inline unsigned long long getCurrentFrameNumber() { return mFrameCounter; }

	//can be called any time after Engine init
	inline const glm::mat4 & getSceneTransform() { return core_sgct::ClusterManager::Instance()->getSceneTrans(); }

private:
	enum SyncStage { PreStage = 0, PostStage };
	enum BufferMode { BackBuffer = 0, BackBufferBlack, RenderToTexture };

	bool initNetwork();
	bool initWindow();
	void initOGL();
	void clean();
	void clearAllCallbacks();

	void frameSyncAndLock(SyncStage stage);
	void calcFPS(double timestamp);
	void parseArguments( int& argc, char**& argv );
	void renderDisplayInfo();
	void calculateFrustums();
	void printNodeInfo(unsigned int nodeId);
	void enterCurrentViewport();
	const char * getBasicInfo();

	void draw();
	void setRenderTarget(int bufferIndex);
	void renderFBOTexture();
	void loadShaders();
	void createFBOs();
	void resizeFBOs();
	void setAndClearBuffer(BufferMode mode);
	void captureBuffer();
	void waitForAllWindowsInSwapGroupToOpen();

	static void clearBuffer(void);

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

	int localRunningMode;
	core_sgct::Frustum::FrustumMode mActiveFrustum;
	int currentViewportCoords[4];

	bool mShowInfo;
	bool mShowGraph;
	bool mShowWireframe;
	bool mTakeScreenshot;
	bool mTerminate;
	bool mIgnoreSync;

	//objects
	core_sgct::Statistics	mStatistics;

	//FBO stuff
	enum FBOModes { NoFBO = 0, RegularFBO, MultiSampledFBO };
	unsigned int mFrameBuffers[2];
	unsigned int mMultiSampledFrameBuffers[2];
	unsigned int mRenderBuffers[2];
	unsigned int mDepthBuffers[2];
	unsigned int mFrameBufferTextures[2];
	int mFrameBufferTextureLocs[2];
	int mFBOMode;

	//pointers
	core_sgct::NetworkManager * mNetworkConnections;
	core_sgct::ReadConfig	* mConfig;

	std::string configFilename;
	int mRunning;
	char basicInfo[48];

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
};

}

#endif
