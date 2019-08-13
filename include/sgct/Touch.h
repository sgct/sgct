/*************************************************************************
Copyright (c) 2017 Erik Sundén
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__TOUCH__H__
#define __SGCT__TOUCH__H__

#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

struct GLFWtouch;

namespace sgct_core {

/*!
    This class holds and manages touch points
*/
class Touch {
public:
    //! Touch point information
    struct TouchPoint {
        enum class TouchAction {
            NoAction,
            Released,
            Pressed,
            Stationary,
            Moved
        };
        int id;
        TouchAction action;
        glm::vec2 pixelCoords;
        glm::vec2 normPixelCoords;
        glm::vec2 normPixelDiff;
    };

    // Retrieve the lastest touch points to process them in an event
    std::vector<TouchPoint> getLatestTouchPoints() const;

    // Need to call this after the latest touch points have been processed to clear them
    void setLatestPointsHandled();

    // Adding touch points to the touch class
    // As an id is constant over the touch point, the order will be preserved
    void processPoint(int id, int action, double xpos, double ypos, int windowWidth,
        int windowHeight);
    void processPoints(GLFWtouch* touchPoints, int count, int windowWidth,
        int windowHeight);

    bool areAllPointsStationary() const;

private:
    std::vector<TouchPoint> mTouchPoints;
    std::vector<TouchPoint> mPreviousTouchPoints;
    std::unordered_map<int, glm::vec2> mPreviousTouchPositions;
    std::vector<int> mPrevTouchIds;
    bool mAllPointsStationary;
};

std::string getTouchPointInfo(const Touch::TouchPoint& tp);


} // sgct_core

#endif // __SGCT__TOUCH__H__
