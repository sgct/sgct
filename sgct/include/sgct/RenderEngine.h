/* RenderEngine.h

© 2012 Miroslav Andel

*/

#ifndef _RENDER_ENGINE_H_
#define _RENDER_ENGINE_H_

#include "sgct/Network.h"
#include "sgct/Window.h"
#include "sgct/freetype.h"
#include "sgct/ReadConfig.h"
#include "sgct/Frustum.h"
#include "sgct/SharedData.h"
#include "sgct/Statistics.h"
#include "sgct/User.h"
#include <string>

namespace sgct //small graphics cluster toolkit
{

class RenderEngine
{
public:
	RenderEngine( SharedData & sharedData, int argc, char* argv[] );
	void clean();
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
	Window * getWindowPtr() { return mWindow; }

	inline bool isSyncServer() { return isServer; }
	inline bool isDisplayInfoRendered() { return displayInfo; }

private:
	void init();
	void initNetwork();
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
	typedef void (RenderEngine::*InternalCallbackFn)(void);

	//function pointers
	CallbackFn mDrawFn;
	CallbackFn mPreDrawFn;
	CallbackFn mInitOGLFn;
	CallbackFn mClearBufferFn;
	InternalCallbackFn mInternalRenderFn;

	Frustum *mFrustums[3];
	float nearClippingPlaneDist;
	float farClippingPlaneDist;

	Statistics mStatistics;

	int activeFrustum;

	freetype::font_data font;
	bool isServer;
	bool runningLocal; //possible to run a cluster setup for testing on a single computer
	bool displayInfo;

	User	mUser;

	Window	* mWindow;
	Network * mNetwork;
	ReadConfig * mConfig;
	SharedData * mSharedData;

	std::string configFilename;
	int mThisClusterNodeId;
};

}

#endif