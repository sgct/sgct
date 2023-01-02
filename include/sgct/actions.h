/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__ACTIONS__H__
#define __SGCT__ACTIONS__H__

// abock(2019-10-02);  The values here are hardcoded as I don't want to pull in the
// entirety of GLFW in this header just for the definitions.  And they most likely will
// not change anytime soon anyway (famous last words)

namespace sgct {
enum class Action {
    Release = 0, // = GLFW_RELEASE
    Press = 1, // = GLFW_PRESS;
    Repeat = 2 // = GLFW_REPEAT;
};
} // namespace sgct

#endif // __SGCT__ACTIONS__H__
