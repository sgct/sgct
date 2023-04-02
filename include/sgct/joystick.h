/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__JOYSTICK__H__
#define __SGCT__JOYSTICK__H__

// abock(2019-10-02);  The values here are hardcoded as I don't want to pull in the
// entirety of GLFW in this header just for the definitions.  And they most likely will
// not change anytime soon anyway (famous last words)

namespace sgct {
enum class Joystick {
    Joystick1 = 0, // = GLFW_JOYSTICK_1;
    Joystick2 = 1, // = GLFW_JOYSTICK_2;
    Joystick3 = 2, // = GLFW_JOYSTICK_3;
    Joystick4 = 3, // = GLFW_JOYSTICK_4;
    Joystick5 = 4, // = GLFW_JOYSTICK_5;
    Joystick6 = 5, // = GLFW_JOYSTICK_6;
    Joystick7 = 6, // = GLFW_JOYSTICK_7;
    Joystick8 = 7, // = GLFW_JOYSTICK_8;
    Joystick9 = 8, // = GLFW_JOYSTICK_9;
    Joystick10 = 9, // = GLFW_JOYSTICK_10;
    Joystick11 = 10, // = GLFW_JOYSTICK_11;
    Joystick12 = 11, // = GLFW_JOYSTICK_12;
    Joystick13 = 12, // = GLFW_JOYSTICK_13;
    Joystick14 = 13, // = GLFW_JOYSTICK_14;
    Joystick15 = 14, // = GLFW_JOYSTICK_15;
    Joystick16 = 15 // = GLFW_JOYSTICK_16;
};
} // namespace sgct

#endif // __SGCT__JOYSTICK__H__
