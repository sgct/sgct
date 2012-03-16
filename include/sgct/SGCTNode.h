/* SGCTNode.h

© 2012 Miroslav Andel

*/

#ifndef _SGCT_NODE
#define _SGCT_NODE

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
	void addViewport(Viewport &vp);
	Viewport * getCurrentViewport();
	Viewport * getViewport(unsigned int index);
	void getCurrentViewportPixelCoords(int &x, int &y, int &xSize, int &ySize);
	unsigned int getNumberOfViewports();

	SGCTWindow * getWindowPtr() { return &mWindow; }

	inline void setCurrentViewport(unsigned int index) { mCurrentViewportIndex = index; }

	bool lockVerticalSync;
	std::string ip;
	std::string port;
	int numberOfSamples;
	int stereo;

private:
	unsigned int mCurrentViewportIndex;
	std::vector<Viewport> mViewports;
	SGCTWindow mWindow;
};
}

#endif