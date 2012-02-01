/* RenderEngine.h

© 2012 Miroslav Andel

*/

#ifndef _RENDER_ENGINE_H_
#define _RENDER_ENGINE_H_

#include <GL/glew.h>
#include <GL/glfw.h>

#include "freetype.h"
#include "Network.h"
#include "ReadConfig.h"
#include "Frustum.h"
#include "SharedData.h"
#include "Statistics.h"
#include "Window.h"
#include "User.h"
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
	Window * getWindowPtr() { return &mWindow; }

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

	Window	mWindow;
	User	mUser;

	Network * mNetwork;
	ReadConfig * mConfig;
	SharedData * mSharedData;

	std::string configFilename;
	int mThisClusterNodeId;
};

}

#endif