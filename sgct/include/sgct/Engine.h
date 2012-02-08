/* Engine.h

© 2012 Miroslav Andel

*/

#ifndef _RENDER_ENGINE_H_
#define _RENDER_ENGINE_H_

#include "sgct/SGCTNetwork.h"
#include "sgct/SGCTWindow.h"
#include "sgct/ReadConfig.h"
#include "sgct/Frustum.h"
#include "sgct/Statistics.h"
#include "sgct/User.h"

namespace sgct //small graphics cluster toolkit
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
	double getDt();
	double getTime();
	double getDrawTime();
	void setNearAndFarClippingPlanes(float _near, float _far);

	void setDrawFunction(void(*fnPtr)(void));
	void setPreDrawFunction(void(*fnPtr)(void));
	void setInitOGLFunction(void(*fnPtr)(void));
	void setClearBufferFunction(void(*fnPtr)(void));
	
	void setDisplayInfoVisibility(bool state) { displayInfo = state; }
	
	inline core_sgct::SGCTWindow * getWindowPtr() { return mWindow; }
	inline bool isSyncServer() { return isServer; }
	inline bool isDisplayInfoRendered() { return displayInfo; }

private:
	bool initNetwork();
	bool initWindow();
	void initOGL();
	void clean();

	void calcFPS(double timestamp);
	void parseArguments( int argc, char* argv[] );
	void renderDisplayInfo();
	void calculateFrustums();
	void printNodeInfo(unsigned int nodeId);
	
	//stereo render functions
	void setNormalRenderingMode();
	void setActiveStereoRenderingMode();
	
	static void clearBuffer(void);

private:
	// Convinience typedef
	typedef void (*CallbackFn)(void);
	typedef void (Engine::*InternalCallbackFn)(void);

	//function pointers
	CallbackFn mDrawFn;
	CallbackFn mPreDrawFn;
	CallbackFn mInitOGLFn;
	CallbackFn mClearBufferFn;
	InternalCallbackFn mInternalRenderFn;

	float nearClippingPlaneDist;
	float farClippingPlaneDist;

	int activeFrustum;

	bool isServer;
	bool runningLocal; //possible to run a cluster setup for testing on a single computer
	bool displayInfo;

	//objects
	core_sgct::User			mUser;
	core_sgct::Statistics	mStatistics;
	
	//pointers
	core_sgct::SGCTWindow	* mWindow;
	core_sgct::SGCTNetwork	* mNetwork;
	core_sgct::ReadConfig	* mConfig;
	core_sgct::Frustum		* mFrustums[3];

	std::string configFilename;
	int mThisClusterNodeId;
};

}

#endif