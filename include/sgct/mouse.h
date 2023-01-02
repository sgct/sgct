/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__MOUSE__H__
#define __SGCT__MOUSE__H__

// abock(2019-10-02);  The values here are hardcoded as I don't want to pull in the
// entirety of GLFW in this header just for the definitions.  And they most likely will
// not change anytime soon anyway (famous last words)

namespace sgct {
enum class MouseButton {
#ifdef WIN32
    Button1 = 0, // = GLFW_MOUSE_BUTTON_1;
    Button2 = 1, // = GLFW_MOUSE_BUTTON_2;
    Button3 = 2, // = GLFW_MOUSE_BUTTON_3;
    Button4 = 3, // = GLFW_MOUSE_BUTTON_4;
    Button5 = 4, // = GLFW_MOUSE_BUTTON_5;
    Button6 = 5, // = GLFW_MOUSE_BUTTON_6;
    Button7 = 6, // = GLFW_MOUSE_BUTTON_7;
    Button8 = 7, // = GLFW_MOUSE_BUTTON_8;
    ButtonLeft = Button1,
    ButtonRight = Button2,
    ButtonMiddle = Button3
#else
    ButtonLeft = 0,
    ButtonRight = 1,
    ButtonMiddle = 2
#endif
};
} // namespace sgct

#endif // __SGCT__MOUSE__H__
