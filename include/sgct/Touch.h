/*************************************************************************
Copyright (c) 2017 Erik Sundén
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _TOUCH_H
#define _TOUCH_H

#include <../src/deps/glfw/include/GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <stddef.h> //get definition for NULL

struct GLFWtouch;

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
        glm::vec2 pixelCoords;
        glm::vec2 normPixelCoords;
        glm::vec2 normPixelDiff;
        TouchPoint(int i, TouchAction a, glm::vec2 p, glm::vec2 np) : id(i), action(a), pixelCoords(p), normPixelCoords(np), normPixelDiff(0.f, 0.f) {}
        TouchPoint(int i, TouchAction a, glm::vec2 p, glm::vec2 np, glm::vec2 nd) : id(i), action(a), pixelCoords(p), normPixelCoords(np), normPixelDiff(nd) {}
    };

    Touch();
    ~Touch();

    // Retrieve the lastest touch points to process them in an event
    std::vector<TouchPoint> getLatestTouchPoints() const;

    // Need to call this function after the latest touch points have been processed, to clear them
    void latestPointsHandled();

    // Adding touch points to the touch class
    // As an id is constant over the touch point, the order will be preserved
    void processPoint(int id, int action, double xpos, double ypos, int windowWidth, int windowHeight);
    void processPoints(GLFWtouch* touchPoints, int count, int windowWidth, int windowHeight);

    bool isAllPointsStationary() const;

    static std::string getTouchPointInfo(const TouchPoint*);

private:

    std::vector<TouchPoint> mTouchPoints;
    std::vector<TouchPoint> mPreviousTouchPoints;
    std::unordered_map<int, glm::vec2> mPreviousTouchPositions;
    std::vector<int> mPrevTouchIds;
    bool mAllPointsStationary;
};

}

#endif
