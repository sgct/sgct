#include "../include/sgct/SGCTNode.h"

core_sgct::SGCTNode::SGCTNode()
{
	mCurrentViewportIndex = 0;
	numberOfSamples = 0;

	//init optional parameters
	lockVerticalSync = true;				
}

void core_sgct::SGCTNode::addViewport(float left, float right, float bottom, float top)
{
	Viewport tmpVP(left, right, bottom, top);
	mViewports.push_back(tmpVP);
}

void core_sgct::SGCTNode::addViewport(core_sgct::Viewport &vp)
{
	mViewports.push_back(vp);
}

core_sgct::Viewport * core_sgct::SGCTNode::getCurrentViewport()
{
	return &mViewports[mCurrentViewportIndex];
}

core_sgct::Viewport * core_sgct::SGCTNode::getViewport(unsigned int index)
{
	return &mViewports[index];
}

void core_sgct::SGCTNode::getCurrentViewportPixelCoords(int &x, int &y, int &xSize, int &ySize)
{
	x = static_cast<int>(getCurrentViewport()->getX()) *
		getWindowPtr()->getHResolution();
	y = static_cast<int>(getCurrentViewport()->getY()) *
		getWindowPtr()->getVResolution();
	xSize = static_cast<int>(getCurrentViewport()->getXSize()) *
		getWindowPtr()->getHResolution();
	ySize = static_cast<int>(getCurrentViewport()->getYSize()) *
		getWindowPtr()->getVResolution();
}

unsigned int core_sgct::SGCTNode::getNumberOfViewports()
{
	return mViewports.size();
}