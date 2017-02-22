/*************************************************************************
Copyright (c) 2017 Erik Sundén
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/Touch.h>
#include <algorithm>
#include <sstream>

sgct_core::Touch::Touch()
{
}

sgct_core::Touch::~Touch()
{
}

const std::vector<sgct_core::Touch::TouchPoint>& sgct_core::Touch::getLatestTouchPoints() const {
	return mTouchPoints;
}

void sgct_core::Touch::addPoint(int id, int action, double xpos, double ypos) {
	glm::vec2 pos = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));

	TouchPoint::TouchAction touchAction;
	switch (action)
	{
	case GLFW_PRESS:
		touchAction = TouchPoint::Pressed;
		break;
	case GLFW_MOVE:
		touchAction = TouchPoint::Moved;
		break;
	case GLFW_RELEASE:
		touchAction = TouchPoint::Released;
		break;
	case GLFW_REPEAT:
		touchAction = TouchPoint::Stationary;
		break;
	default:
		touchAction = TouchPoint::NoAction;
	}

	glm::vec2 prevPos = pos;

	std::unordered_map<int, glm::vec2>::iterator prevPosMapIt = mPreviousTouchPositions.find(id);
	if (prevPosMapIt != mPreviousTouchPositions.end()) {
		prevPos = prevPosMapIt->second;
		if (touchAction == TouchPoint::Released)
			mPreviousTouchPositions.erase(prevPosMapIt);
		else
			prevPosMapIt->second = pos;
	}
	else {
		mPreviousTouchPositions.insert(std::pair<int, glm::vec2>(id, pos));
	}

	// Add to end of corrected ordered vector if new touch point
	std::vector<int>::iterator lastIdIdx = std::find(mPrevTouchIds.begin(), mPrevTouchIds.end(), id);
	if (lastIdIdx == mPrevTouchIds.end()) {
		mPrevTouchIds.push_back(id);
	}

	mTouchPoints.push_back(TouchPoint(id, touchAction, pos));
}

void sgct_core::Touch::addPoints(GLFWtouch* touchPoints, int count) {
	for (int i = 0; i < count; ++i) {
		addPoint(touchPoints[i].id, touchPoints[i].action, touchPoints[i].x, touchPoints[i].y);
	}

	// Ensure that the order to the touch points are the same as last touch event.
	// Note that the ID of a touch point is always the same but the order in which
	// they are given can vary.
	// Example
	// lastTouchIds_    touchPoints
	//     0                 0
	//     3                 1 
	//     2                 2
	//     4
	// Will result in:
	//                  touchPoints
	//                       0 (no swap)
	//                       2 (2 will swap with 1)
	//                       1

	auto touchIndex = 0; // Index to first unsorted element in touchPoints array
	for (const auto& prevTouchPointId : mPrevTouchIds) {
		const auto touchPointIt = std::find_if(mTouchPoints.begin(), mTouchPoints.end(), [prevTouchPointId](const TouchPoint& p) { return p.id == prevTouchPointId; });
		// Swap current location in the container with the location it was in last touch event.
		if (touchPointIt != mTouchPoints.end() && std::distance(mTouchPoints.begin(), touchPointIt) != touchIndex) {
			std::swap(*(mTouchPoints.begin() + touchIndex), *touchPointIt);
			++touchIndex;
		}
	}

	//Ignore stationary state and count none ended points
	bool allPointsStationary = true;
	int livePoints = 0;
	std::vector<int> endedTouchIds;
	for (auto touchPoint : mTouchPoints) {
		if (touchPoint.action != TouchPoint::Stationary)
			allPointsStationary = false;

		if (touchPoint.action != TouchPoint::Released)
			livePoints++;
		else
			endedTouchIds.push_back(touchPoint.id);
	}

	for (auto& endedId : endedTouchIds) {
		std::vector<int>::iterator foundIdx = std::find(mPrevTouchIds.begin(), mPrevTouchIds.end(), endedId);
		if (foundIdx != mPrevTouchIds.end())
			mPrevTouchIds.erase(foundIdx);
	}

	if (allPointsStationary) {
		mTouchPoints.clear();
	}
}

std::string sgct_core::Touch::getTouchPointInfo(const TouchPoint* touchPoint) {
	std::stringstream ss;

	ss << "id(" << touchPoint->id << "),";
	ss << "action(";
	switch (touchPoint->action)
	{
	case TouchPoint::Pressed:
		ss << "Pressed";
		break;
	case TouchPoint::Moved:
		ss << "Moved";
		break;
	case TouchPoint::Released:
		ss << "Released";
		break;
	case TouchPoint::Stationary:
		ss << "Stationary";
		break;
	default:
		ss << "NoAction";
	}
	ss << "),";
	ss << "pos(" << touchPoint->pos.x << "," << touchPoint->pos.y << ")";

	return ss.str();
}