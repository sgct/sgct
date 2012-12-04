/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_NODE
#define _SGCT_NODE

#include "Viewport.h"
#include "SGCTWindow.h"
#include <string>
#include <vector>

namespace sgct_core
{
class SGCTNode
{
public:
	SGCTNode();
	void addViewport(float left, float right, float bottom, float top);
	void addViewport(Viewport &vp);
	bool isUsingFisheyeRendering();
	void generateCubeMapViewports();
	Viewport * getCurrentViewport();
	Viewport * getViewport(unsigned int index);
	void getCurrentViewportPixelCoords(int &x, int &y, int &xSize, int &ySize);
	unsigned int getNumberOfViewports();

	SGCTWindow * getWindowPtr() { return &mWindow; }

	inline void setCurrentViewport(unsigned int index) { mCurrentViewportIndex = index; }
	//! Set if fisheye rendering is active (only valid before init).
	inline void setFisheyeRendering(bool state) { mFisheyeMode = state; }

	int swapInterval;
	std::string ip;
	std::string port;
	int numberOfSamples;
	int stereo;

private:
	void deleteAllViewports();

private:
	unsigned int mCurrentViewportIndex;
	std::vector<Viewport> mViewports;
	SGCTWindow mWindow;
	bool mFisheyeMode;
};
}

#endif
