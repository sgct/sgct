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
	
	SGCTWindow *	getWindowPtr(std::size_t index);
	SGCTWindow *	getActiveWindowPtr();
	std::size_t		getNumberOfWindows();
	bool			getKeyPressed( int key );

	/*! Get the current window index */
	inline std::size_t getCurrentWindowIndex() { return mCurrentWindowIndex; }

	void addWindow(SGCTWindow &window);
	void setCurrentWindowIndex(std::size_t index);

	bool shouldAllWindowsClose();
	void showAllWindows();
	void hideAllWindows();

	std::string ip;
	std::string port;

private:
	std::size_t mCurrentWindowIndex;
	std::vector<SGCTWindow> mWindows;
};
}

#endif
