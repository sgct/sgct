/* Engine.h

© 2012 Miroslav Andel

*/

#ifndef _RENDER_ENGINE_H_
#define _RENDER_ENGINE_H_

#include "ClusterManager.h"
#include "NetworkManager.h"
#include "ReadConfig.h"
#include "Statistics.h"
#include "User.h"

namespace sgct //simple graphics cluster toolkit
{

class Engine
{
public:
	Engine( int argc, char* argv[] );
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
	void setNearAndFarClippingPlanes(float _near, float _far);
	void setWireframe(bool enabled) { showWireframe = enabled; }

	void setInitOGLFunction( void(*fnPtr)(void) );
	void setPreDrawFunction( void(*fnPtr)(void));
	void setClearBufferFunction( void(*fnPtr)(void) );
	void setDrawFunction( void(*fnPtr)(void) );
	void setPostDrawFunction( void(*fnPtr)(void) );

	//external control network functions
	void setExternalControlCallback( void(*fnPtr)(const char *, int, int) );
	void sendMessageToExternalControl(void * data, int lenght);
	void sendMessageToExternalControl(const std::string msg);
	void setExternalControlBufferSize(unsigned int newSize);
	void decodeExternalControl(const char * receivedData, int receivedLenght, int clientIndex);

	void setDisplayInfoVisibility(bool state) { showInfo = state; }
	void setStatsGraphVisibility(bool state) { showGraph = state; }

	inline core_sgct::SGCTWindow * getWindowPtr() { return core_sgct::ClusterManager::Instance()->getThisNodePtr()->getWindowPtr(); }
	inline bool isMaster() { return mNetworkConnections->isComputerServer(); }
	inline bool isDisplayInfoRendered() { return showInfo; }

	//Timer functionality - Returns time since engine object created in s.
	double getTime();

private:
	bool initNetwork();
	bool initWindow();
	void initOGL();
	void clean();

	void frameSyncAndLock(int stage);
	void calcFPS(double timestamp);
	void parseArguments( int argc, char* argv[] );
	void renderDisplayInfo();
	void calculateFrustums();
	void printNodeInfo(unsigned int nodeId);
	const char * getBasicInfo();

	//stereo render functions
	void setNormalRenderingMode();
	void setActiveStereoRenderingMode();

	static void clearBuffer(void);

private:
	// Convinience typedef
	typedef void (*CallbackFn)(void);
	typedef void (Engine::*InternalCallbackFn)(void);
	typedef void (*NetworkCallbackFn)(const char *, int, int);

	//function pointers
	CallbackFn mDrawFn;
	CallbackFn mPreDrawFn;
	CallbackFn mPostDrawFn;
	CallbackFn mInitOGLFn;
	CallbackFn mClearBufferFn;
	InternalCallbackFn mInternalRenderFn;
	NetworkCallbackFn mNetworkCallbackFn;

	enum SyncStage { PreStage = 0, PostStage };

	float nearClippingPlaneDist;
	float farClippingPlaneDist;

	int localRunningMode;
	int activeFrustum;

	bool showInfo;
	bool showGraph;
	bool showWireframe;
	bool mTerminate;

	//objects
	core_sgct::Statistics	mStatistics;
	core_sgct::User			mUser;

	//pointers
	core_sgct::NetworkManager * mNetworkConnections;
	core_sgct::ReadConfig	* mConfig;

	std::string configFilename;
	int mRunning;
	char basicInfo[48];

	static Engine * mThis;
};

}

#endif
