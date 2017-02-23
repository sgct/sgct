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

std::vector<sgct_core::Touch::TouchPoint> sgct_core::Touch::getLatestTouchPoints() const {
	return mTouchPoints;
}

void sgct_core::Touch::latestPointsHandled() {
	mTouchPoints.clear();
}

void sgct_core::Touch::processPoint(int id, int action, double xpos, double ypos, int windowWidth, int windowHeight) {
	glm::vec2 windowSize = glm::vec2(static_cast<float>(windowWidth), static_cast<float>(windowHeight));
	glm::vec2 pos = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
	glm::vec2 normpos = pos / windowSize;

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
		if (touchAction == TouchPoint::Released) {
			mPreviousTouchPositions.erase(prevPosMapIt);
		}
		else {
			prevPosMapIt->second = pos;
		}
	}
	else {
		mPreviousTouchPositions.insert(std::pair<int, glm::ivec2>(id, pos));
	}

	// Add to end of corrected ordered vector if new touch point
	std::vector<int>::iterator lastIdIdx = std::find(mPrevTouchIds.begin(), mPrevTouchIds.end(), id);
	if (lastIdIdx == mPrevTouchIds.end()) {
		mPrevTouchIds.push_back(id);
	}

	// Check if position has not changed and make the point stationary then
	if(touchAction == TouchPoint::Moved && glm::distance(pos, prevPos) == 0)
		touchAction = TouchPoint::Stationary;

	mTouchPoints.push_back(TouchPoint(id, touchAction, pos, normpos, ((pos-prevPos)/ windowSize)));
}

void sgct_core::Touch::processPoints(GLFWtouch* touchPoints, int count, int windowWidth, int windowHeight) {
	for (int i = 0; i < count; ++i) {
		processPoint(touchPoints[i].id, touchPoints[i].action, touchPoints[i].x, touchPoints[i].y, windowWidth, windowHeight);
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
	mAllPointsStationary = true;
	int livePoints = 0;
	std::vector<int> endedTouchIds;
	for (auto touchPoint : mTouchPoints) {
		if (touchPoint.action != TouchPoint::Stationary)
			mAllPointsStationary = false;

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
}

bool sgct_core::Touch::isAllPointsStationary() const {
	return mAllPointsStationary;
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
	ss << "pixelCoords(" << touchPoint->pixelCoords.x << "," << touchPoint->pixelCoords.y << "),";
	ss << "normPixelCoords(" << touchPoint->normPixelCoords.x << "," << touchPoint->normPixelCoords.y << "),";
	ss << "normPixelDiff(" << touchPoint->normPixelDiff.x << "," << touchPoint->normPixelDiff.y << ")";

	return ss.str();
}