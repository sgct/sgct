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

/*!
	Add a window to the window vector. Note that a window must be opened to become visible.
*/
void sgct_core::SGCTNode::addWindow(sgct::SGCTWindow &window)
{
	mWindows.push_back(window);
}

/*!
	Set which window that will render the draw calls.
*/
void sgct_core::SGCTNode::setCurrentWindowIndex(std::size_t index)
{
	mCurrentWindowIndex = index;
}

/*!
	Check if a key is pressed for all windows.
*/
bool sgct_core::SGCTNode::getKeyPressed( int key )
{
	for(std::size_t i=0; i<mWindows.size(); i++)
		if( glfwGetKey( mWindows[i].getWindowHandle(), key) )
			return true;
	return false;
}

/*!
	Check if all windows are set to close and close them.
*/
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

/*!
	Show all hidden windows.
*/
void sgct_core::SGCTNode::showAllWindows()
{
	for(std::size_t i=0; i<mWindows.size(); i++)
		mWindows[i].setVisibility( true );
}

/*!
	Hide all windows.
*/
void sgct_core::SGCTNode::hideAllWindows()
{
	for(std::size_t i=0; i<mWindows.size(); i++)
		mWindows[i].setVisibility( false );
}
