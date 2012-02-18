/* Engine.h

© 2012 Miroslav Andel

*/

#ifndef _RENDER_ENGINE_H_
#define _RENDER_ENGINE_H_

#include "SGCTNetwork.h"
#include "SGCTWindow.h"
#include "ReadConfig.h"
#include "Frustum.h"
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

	double getDt();
	double getDrawTime();
	void setNearAndFarClippingPlanes(float _near, float _far);
	void setWireframe(bool enabled) { showWireframe = enabled; }

	void setDrawFunction(void(*fnPtr)(void));
	void setPreDrawFunction(void(*fnPtr)(void));
	void setPostDrawFunction(void(*fnPtr)(void));
	void setInitOGLFunction(void(*fnPtr)(void));
	void setClearBufferFunction(void(*fnPtr)(void));

	//external control network functions
	void setExternalControlCallback(void(*fnPtr)(const char *, int, int));
	void sendMessageToExternalControl(void * data, int lenght);
	void sendMessageToExternalControl(const std::string msg);
	void setExternalControlBufferSize(unsigned int newSize);

	void setDisplayInfoVisibility(bool state) { displayInfo = state; }

	inline core_sgct::SGCTWindow * getWindowPtr() { return mWindow; }
	inline bool isSyncServer() { return isServer; }
	inline bool isDisplayInfoRendered() { return displayInfo; }

private:
	bool initNetwork();
	bool initWindow();
	void initOGL();
	void clean();

	void frameLock();
	void calcFPS(double timestamp);
	void parseArguments( int argc, char* argv[] );
	void renderDisplayInfo();
	void calculateFrustums();
	void printNodeInfo(unsigned int nodeId);
	const char * getBasicInfo();

	//stereo render functions
	void setNormalRenderingMode();
	void setActiveStereoRenderingMode();
	void decodeExternalControl(const char * receivedData, int receivedLenght, int clientIndex);

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

	float nearClippingPlaneDist;
	float farClippingPlaneDist;

	int activeFrustum;

	bool isServer;
	bool runningLocal; //possible to run a cluster setup for testing on a single computer
	bool displayInfo;
	bool showWireframe;
	bool mTerminate;

	//objects
	core_sgct::User			mUser;
	core_sgct::Statistics	mStatistics;

	//pointers
	core_sgct::SGCTWindow	* mWindow;
	core_sgct::SGCTNetwork	* mNetwork;
	core_sgct::SGCTNetwork	* mExternalControlNetwork;
	core_sgct::ReadConfig	* mConfig;
	core_sgct::Frustum		* mFrustums[3];

	std::string configFilename;
	int mThisClusterNodeId;
	char basicInfo[48];
};

}

#endif
