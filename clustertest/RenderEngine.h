/* RenderEngine.h

© 2012 Miroslav Andel

*/

#ifndef _RENDER_ENGINE
#define _RENDER_ENGINE

#include "freetype.h"
#include "Network.h"
#include "ReadConfig.h"
#include "Frustum.h"
#include "SharedData.h"
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

	//swaplock functions
	void getSwapGroupFrameNumber(GLuint & frameNumber);
	void resetSwapGroupFrameNumber();
	
	void setDisplayInfoVisibility(bool state) { displayInfo = state; }

	inline bool isSyncServer() { return isServer; }
	inline bool isUsingSwapGroups() { return useSwapGroups; }
	inline int getWindowMode() { return windowMode; }
	inline int getHorizontalWindowResolution() { return windowRes[0]; }
	inline int getVerticalWindowResolution() { return windowRes[1]; }
	inline bool isDisplayInfoRendered() { return displayInfo; }

	void setWindowResolution(int x, int y) { windowRes[0] = x; windowRes[1] = y; }

private:
	void init();
	void calcFPS(double timestamp);
	void parseArguments( int argc, char* argv[] );
	void initNvidiaSwapGroups();
	void renderDisplayInfo();
	void calculateFrustums();

	//stereo render functions
	void setNormalRenderingMode();
	void setActiveStereoRenderingMode();

	//function pointers
	void (*mDrawFn) (void);
	void (*mPreDrawFn) (void);
	void (*mInitOGLFn) (void);
	void (*mClearBufferFn) (void);
	void (RenderEngine::*mInternalRenderFn) (void);

	enum FrustumMode { Mono = 0, StereoLeftEye, StereoRightEye };
	Frustum *mFrustums[3];
	float nearClippingPlaneDist;
	float farClippingPlaneDist;

	Point3 userLeftEyePos;
	Point3 userRightEyePos;

	double fps;
	double avgfps;
	double frameTime;
	double drawTime;
	double totalTime;
	int activeFrustum;
	freetype::font_data font;
	bool useSwapGroups;
	bool swapGroupMaster;
	bool isServer;
	bool runningLocal; //possible to run a cluster setup for testing on a single computer
	bool displayInfo;

	int windowRes[2];
	int windowMode;

	Network * mNetwork;
	ReadConfig * mConfig;
	SharedData * mSharedData;

	std::string configFilename;
	int thisClusterNodeId;
};

}

#endif