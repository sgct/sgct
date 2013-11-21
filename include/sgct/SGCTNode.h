/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_NODE
#define _SGCT_NODE

#include "SGCTWindow.h"
#include <string>
#include <vector>

namespace sgct_core
{
class SGCTNode
{
public:
	SGCTNode();
	
	
	bool getKeyPressed( int key );

	/*!
		Get the number of windows in the window vector
	*/
	inline std::size_t		getNumberOfWindows() { return mWindows.size(); }

	/*!
		Get the window pointer at index in window vector.
	*/
	inline sgct::SGCTWindow *	getWindowPtr(std::size_t index) { return &mWindows[index]; }
	
	/*!
		Get the active window pointer.
	*/
	inline sgct::SGCTWindow *		getActiveWindowPtr() { return &mWindows[mCurrentWindowIndex]; }

	/*! Get the current window index */
	inline std::size_t		getCurrentWindowIndex() { return mCurrentWindowIndex; }

	void addWindow(sgct::SGCTWindow &window);
	void setCurrentWindowIndex(std::size_t index);
	void setUseSwapGroups(bool state);

	bool shouldAllWindowsClose();
	bool isUsingSwapGroups();
	void showAllWindows();
	void hideAllWindows();

	void setAddress(std::string address);
	void setPort(std::string port);
	std::string getAddress();
	std::string getPort();

private:
	std::string mAddress;
	std::string mPort;

	std::size_t mCurrentWindowIndex;
	std::vector<sgct::SGCTWindow> mWindows;
	bool mUseSwapGroups;
};
}

#endif
