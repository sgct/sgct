/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__JOYSTICK__H__
#define __SGCT__JOYSTICK__H__

// abock(2019-10-02);  The values here are hardcoded as I don't want to pull in the
// entirety of GLFW in this header just for the definitions.  And they most likely will
// not change anytime soon anyway (famous last words)

namespace sgct::joystick {
    constexpr const int Joystick1 = 0; // = GLFW_JOYSTICK_1;
    constexpr const int Joystick2 = 1; // = GLFW_JOYSTICK_2;
    constexpr const int Joystick3 = 2; // = GLFW_JOYSTICK_3;
    constexpr const int Joystick4 = 3; // = GLFW_JOYSTICK_4;
    constexpr const int Joystick5 = 4; // = GLFW_JOYSTICK_5;
    constexpr const int Joystick6 = 5; // = GLFW_JOYSTICK_6;
    constexpr const int Joystick7 = 6; // = GLFW_JOYSTICK_7;
    constexpr const int Joystick8 = 7; // = GLFW_JOYSTICK_8;
    constexpr const int Joystick9 = 8; // = GLFW_JOYSTICK_9;
    constexpr const int Joystick10 = 9; // = GLFW_JOYSTICK_10;
    constexpr const int Joystick11 = 10; // = GLFW_JOYSTICK_11;
    constexpr const int Joystick12 = 11; // = GLFW_JOYSTICK_12;
    constexpr const int Joystick13 = 12; // = GLFW_JOYSTICK_13;
    constexpr const int Joystick14 = 13; // = GLFW_JOYSTICK_14;
    constexpr const int Joystick15 = 14; // = GLFW_JOYSTICK_15;
    constexpr const int Joystick16 = 15; // = GLFW_JOYSTICK_16;
    constexpr const int JoystickLast = Joystick16;
} // namespace sgct::joystick

#endif // __SGCT__JOYSTICK__H__