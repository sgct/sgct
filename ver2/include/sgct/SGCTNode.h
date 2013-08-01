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
	
	void addWindow(SGCTWindow &window);
	SGCTWindow * getWindowPtr(std::size_t index);
	//SGCTWindow * getMasterWindowPtr();
	SGCTWindow * getCurrentWindowPtr();
	std::size_t getNumberOfWindows();
	void setCurrentWindowIndex(std::size_t index);

	bool getKeyPressed( int key );
	bool shouldAllWindowsClose();

	std::string ip;
	std::string port;

private:
	std::size_t mCurrentWindowIndex;
	std::vector<SGCTWindow> mWindows;
};
}

#endif
