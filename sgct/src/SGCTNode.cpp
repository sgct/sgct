#include "../include/sgct/SGCTNode.h"

core_sgct::SGCTNode::SGCTNode()
{
	mCurrentViewportIndex = 0;  

	//init optional parameters
	lockVerticalSync = true;				
}

void core_sgct::SGCTNode::addViewport(float left, float right, float bottom, float top)
{
	Viewport tmpVP(left, right, bottom, top);
	mViewports.push_back(tmpVP);
}

core_sgct::Viewport * core_sgct::SGCTNode::getCurrentViewport()
{
	return &mViewports[mCurrentViewportIndex];
}