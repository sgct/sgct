/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__MOUSE__H__
#define __SGCT__MOUSE__H__

// abock(2019-10-02);  The values here are hardcoded as I don't want to pull in the
// entirety of GLFW in this header just for the definitions.  And they most likely will
// not change anytime soon anyway (famous last words)

namespace sgct::mouse {
    constexpr const int Button1 = 0; // = GLFW_MOUSE_BUTTON_1;
    constexpr const int Button2 = 1; // = GLFW_MOUSE_BUTTON_2;
    constexpr const int Button3 = 2; // = GLFW_MOUSE_BUTTON_3;
    constexpr const int Button4 = 3; // = GLFW_MOUSE_BUTTON_4;
    constexpr const int Button5 = 4; // = GLFW_MOUSE_BUTTON_5;
    constexpr const int Button6 = 5; // = GLFW_MOUSE_BUTTON_6;
    constexpr const int Button7 = 6; // = GLFW_MOUSE_BUTTON_7;
    constexpr const int Button8 = 7; // = GLFW_MOUSE_BUTTON_8;
    constexpr const int ButtonLeft = Button1;
    constexpr const int ButtonRight = Button2;
    constexpr const int ButtonMiddle = Button3;
    constexpr const int ButtonLast = Button8;
} // namespace sgct::mouse

#endif // __SGCT__MOUSE__H__