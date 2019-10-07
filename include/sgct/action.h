/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__ACTION__H__
#define __SGCT__ACTION__H__

// abock(2019-10-02);  The values here are hardcoded as I don't want to pull in the
// entirety of GLFW in this header just for the definitions.  And they most likely will
// not change anytime soon anyway (famous last words)

namespace sgct::action {
    constexpr inline const int Release = 0; // = GLFW_RELEASE
    constexpr inline const int Press = 1; // = GLFW_PRESS;
    constexpr inline const int Repeat = 2; // = GLFW_REPEAT;
} // namespace sgct::action

#endif // __SGCT__ACTION__H__