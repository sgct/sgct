/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/touch.h>

#include <GLFW/glfw3.h>
#include <algorithm>

namespace sgct::core {

std::string getTouchPointInfo(const Touch::TouchPoint& tp) {
    std::string result = "id(" + std::to_string(tp.id) + "),";

    result += "action(";
    switch (tp.action) {
        case Touch::TouchPoint::TouchAction::Pressed:
            result += "Pressed";
            break;
        case Touch::TouchPoint::TouchAction::Moved:
            result += "Moved";
            break;
        case Touch::TouchPoint::TouchAction::Released:
            result += "Released";
            break;
        case Touch::TouchPoint::TouchAction::Stationary:
            result += "Stationary";
            break;
        default:
            result += "NoAction";
    }
    result += "),";

    result += "pixelCoords(" + std::to_string(tp.pixelCoords.x) + "," +
              std::to_string(tp.pixelCoords.y) + "),";

    result += "normPixelCoords(" + std::to_string(tp.normPixelCoords.x) + "," +
              std::to_string(tp.normPixelCoords.y) + "),";

    result += "normPixelDiff(" + std::to_string(tp.normPixelDiff.x) + "," +
              std::to_string(tp.normPixelDiff.y) + ")";

    return result;
}


std::vector<Touch::TouchPoint> Touch::getLatestTouchPoints() const {
    return _touchPoints;
}

void Touch::setLatestPointsHandled() {
    _touchPoints.clear();
}

void Touch::processPoint(int id, int action, double xpos, double ypos, int windowWidth,
                         int windowHeight)
{
    glm::vec2 windowSize = glm::vec2(
        static_cast<float>(windowWidth),
        static_cast<float>(windowHeight)
    );
    glm::vec2 pos = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
    glm::vec2 normpos = pos / windowSize;

    using TouchAction = TouchPoint::TouchAction;
    TouchAction touchAction;
    switch (action) {
        case GLFW_PRESS:
            touchAction = TouchAction::Pressed;
            break;
        case GLFW_MOVE:
            touchAction = TouchAction::Moved;
            break;
        case GLFW_RELEASE:
            touchAction = TouchAction::Released;
            break;
        case GLFW_REPEAT:
            touchAction = TouchAction::Stationary;
            break;
        default:
            touchAction = TouchAction::NoAction;
    }

    glm::vec2 prevPos = pos;

    auto prevPosMapIt = _previousTouchPositions.find(id);
    if (prevPosMapIt != _previousTouchPositions.end()) {
        prevPos = prevPosMapIt->second;
        if (touchAction == TouchAction::Released) {
            _previousTouchPositions.erase(prevPosMapIt);
        }
        else {
            prevPosMapIt->second = pos;
        }
    }
    else {
        _previousTouchPositions.insert(std::pair<int, glm::ivec2>(id, pos));
    }

    // Add to end of corrected ordered vector if new touch point
    std::vector<int>::const_iterator lastIdIdx = std::find(
        _prevTouchIds.begin(),
        _prevTouchIds.end(),
        id
    );
    if (lastIdIdx == _prevTouchIds.end()) {
        _prevTouchIds.push_back(id);
    }

    // Check if position has not changed and make the point stationary then
    if (touchAction == TouchAction::Moved && glm::distance(pos, prevPos) == 0) {
        touchAction = TouchAction::Stationary;
    }

    _touchPoints.push_back({id, touchAction, pos, normpos, (pos - prevPos) / windowSize});
}

void Touch::processPoints(GLFWtouch* touchPoints, int count, int windowWidth,
                          int windowHeight)
{
    for (int i = 0; i < count; ++i) {
        processPoint(
            touchPoints[i].id,
            touchPoints[i].action,
            touchPoints[i].x,
            touchPoints[i].y,
            windowWidth,
            windowHeight
        );
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

    int touchIndex = 0; // Index to first unsorted element in touchPoints array
    for (int prevTouchPointId : _prevTouchIds) {
        const auto touchPointIt = std::find_if(
            _touchPoints.begin(),
            _touchPoints.end(),
            [prevTouchPointId](const TouchPoint& p) { return p.id == prevTouchPointId; }
        );
        // Swap current container location with the location it was in last touch event
        if (touchPointIt != _touchPoints.end() &&
            std::distance(_touchPoints.begin(), touchPointIt) != touchIndex)
        {
            std::swap(*(_touchPoints.begin() + touchIndex), *touchPointIt);
            ++touchIndex;
        }
    }

    // Ignore stationary state and count none ended points
    _allPointsStationary = true;
    std::vector<int> endedTouchIds;
    for (const TouchPoint& touchPoint : _touchPoints) {
        if (touchPoint.action != TouchPoint::TouchAction::Stationary) {
            _allPointsStationary = false;
        }

        if (touchPoint.action == TouchPoint::TouchAction::Released) {
            endedTouchIds.push_back(touchPoint.id);
        }
    }

    for (int endedId : endedTouchIds) {
        std::vector<int>::const_iterator foundIdx = std::find(
            _prevTouchIds.begin(),
            _prevTouchIds.end(),
            endedId
        );
        if (foundIdx != _prevTouchIds.end()) {
            _prevTouchIds.erase(foundIdx);
        }
    }
}

bool Touch::areAllPointsStationary() const {
    return _allPointsStationary;
}

} // namespace sgct::core
