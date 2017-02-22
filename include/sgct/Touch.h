/*************************************************************************
Copyright (c) 2017 Erik Sundén
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _TOUCH_H
#define _TOUCH_H

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <stddef.h> //get definition for NULL

namespace sgct_core
{

/*!
	This class holds and manages touch points
*/
class Touch
{
public:
	//! Touch point information
	struct TouchPoint {
		enum TouchAction { NoAction, Released, Pressed, Stationary, Moved };
		int id;
		TouchAction action;
		glm::vec2 pos;
		TouchPoint(int i, TouchAction a, glm::vec2 p) : id(i), action(a), pos(p) {}
	};

	Touch();
	~Touch();

	const std::vector<TouchPoint>& getLatestTouchPoints() const;

	void addPoint(int id, int action, double xpos, double ypos);
	void addPoints(GLFWtouch* touchPoints, int count);

	static std::string getTouchPointInfo(const TouchPoint*);

private:

	std::vector<TouchPoint> mTouchPoints;
	std::vector<TouchPoint> mPreviousTouchPoints;
	std::unordered_map<int, glm::vec2> mPreviousTouchPositions;
	std::vector<int> mPrevTouchIds;
};

}

#endif
