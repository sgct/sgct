/* SGCTNode.h

© 2012 Miroslav Andel

*/

#ifndef _SGCT_NODE
#define _SGCT_NODE

#include "Point3.h"
#include "Viewport.h"
#include "SGCTWindow.h"
#include <string>
#include <vector>

namespace core_sgct
{
class SGCTNode
{
public:
	SGCTNode();
	void addViewport(float left, float right, float bottom, float top);
	Viewport * getCurrentViewport();
	SGCTWindow * getWindowPtr() { return &mWindow; }

	bool lockVerticalSync;
	std::string ip;
	std::string port;
	int numberOfSamples;
	int stereo;
	Point3f viewPlaneCoords[3];

private:
	unsigned int mCurrentViewportIndex;
	std::vector<Viewport> mViewports;
	SGCTWindow mWindow;
};
}

#endif