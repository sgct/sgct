/* SGCTNode.h

© 2012 Miroslav Andel

*/

#ifndef _SGCT_NODE
#define _SGCT_NODE

#include "Point3.h"
#include <string>

namespace core_sgct
{
class SGCTNode
{
public:
	SGCTNode();

	bool useSwapGroups;
	bool lockVerticalSync;
	std::string ip;
	bool fullscreen;
	int numberOfSamples;
	int stereo;
	int windowData[4]; //offset x y and size x y
	Point3f viewPlaneCoords[3];
};
}

#endif