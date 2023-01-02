/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__MODIFIERS__H__
#define __SGCT__MODIFIERS__H__

// abock(2019-10-02);  The values here are hardcoded as I don't want to pull in the
// entirety of GLFW in this header just for the definitions.  And they most likely will
// not change anytime soon anyway (famous last words)

namespace sgct {

enum class Modifier {
    Shift = 0x0001, // = GLFW_MOD_SHIFT;
    Control = 0x0002, // = GLFW_MOD_CONTROL;
    Alt = 0x0004, // = GLFW_MOD_ALT;
    Super = 0x0008 // = GLFW_MOD_SUPER;
};

} // namespace sgct

#endif // __SGCT__MODIFIERS__H__
