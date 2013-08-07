/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/SGCTNode.h"

sgct_core::SGCTNode::SGCTNode()
{
	mCurrentWindowIndex = 0;
}

void sgct_core::SGCTNode::addWindow(sgct_core::SGCTWindow &window)
{
	mWindows.push_back(window);
}

sgct_core::SGCTWindow * sgct_core::SGCTNode::getWindowPtr(std::size_t index)
{
	return &mWindows[index];
}

/*sgct_core::SGCTWindow * sgct_core::SGCTNode::getMasterWindowPtr()
{
	return &mWindows[0];
}*/

sgct_core::SGCTWindow * sgct_core::SGCTNode::getCurrentWindowPtr()
{
	return &mWindows[mCurrentWindowIndex];
}

void sgct_core::SGCTNode::setCurrentWindowIndex(std::size_t index)
{
	mCurrentWindowIndex = index;
}

/*!
	Get the number of windows
*/
std::size_t sgct_core::SGCTNode::getNumberOfWindows()
{
	return mWindows.size();
}

bool sgct_core::SGCTNode::getKeyPressed( int key )
{
	for(std::size_t i=0; i<mWindows.size(); i++)
		if( glfwGetKey( mWindows[i].getWindowHandle(), key) )
			return true;
	return false;
}

bool sgct_core::SGCTNode::shouldAllWindowsClose()
{
	std::size_t counter = 0;
	for(std::size_t i=0; i<mWindows.size(); i++)
		if( glfwWindowShouldClose( mWindows[i].getWindowHandle() ) )
		{
			mWindows[i].setVisibility( false );
			glfwSetWindowShouldClose( mWindows[i].getWindowHandle(), GL_FALSE );
		}

	for(std::size_t i=0; i<mWindows.size(); i++)
		if( !mWindows[i].isVisible() )
		{
			counter++;
		}

	return (counter == mWindows.size()) ? true : false;
}

void sgct_core::SGCTNode::showAllWindows()
{
	for(std::size_t i=0; i<mWindows.size(); i++)
		mWindows[i].setVisibility( true );
}

void sgct_core::SGCTNode::hideAllWindows()
{
	for(std::size_t i=0; i<mWindows.size(); i++)
		mWindows[i].setVisibility( false );
}
