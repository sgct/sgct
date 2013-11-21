/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/SGCTNode.h"
#include "../include/sgct/MessageHandler.h"
#include <algorithm>

sgct_core::SGCTNode::SGCTNode()
{
	mCurrentWindowIndex = 0;
	mUseSwapGroups = false;
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
	Set to true if this node's windows should belong to a nvida swap group. Only valid before window opens.
*/
void sgct_core::SGCTNode::setUseSwapGroups(bool state)
{
	mUseSwapGroups = state;
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
	Is this node using nvidia swap groups for it's windows?
*/
bool sgct_core::SGCTNode::isUsingSwapGroups()
{
	return mUseSwapGroups;
}

/*!
	Hide all windows.
*/
void sgct_core::SGCTNode::hideAllWindows()
{
	for(std::size_t i=0; i<mWindows.size(); i++)
		mWindows[i].setVisibility( false );
}

/*!
\param address is the hostname, DNS-name or ip
*/
void sgct_core::SGCTNode::setAddress(std::string address)
{
	std::transform(address.begin(), address.end(), address.begin(), ::tolower);
	mAddress.assign( address );

	sgct::MessageHandler::instance()->print( sgct::MessageHandler::NOTIFY_DEBUG,
		"SGCTNode: Setting address to %s\n", mAddress.c_str() );
}

/*!
\param port is the number of the tcp port used for communication with this node
*/
void sgct_core::SGCTNode::setPort(std::string port)
{
	mPort.assign( port );
	
	sgct::MessageHandler::instance()->print( sgct::MessageHandler::NOTIFY_DEBUG,
		"SGCTNode: Setting port to %s\n", mPort.c_str() );
}

/*!
\returns the address of this node
*/
std::string sgct_core::SGCTNode::getAddress()
{
	return mAddress;
}

/*!
\returns the port of this node
*/
std::string sgct_core::SGCTNode::getPort()
{
	return mPort;
}

