/*****************************************************************************************
 *                                                                                       *
 * Copyright (c) 2012-2019 Miroslav Andel, Linköping University                          *
 *                                                                                       *
 * All rights reserved                                                                   *
 *                                                                                       *
 * Contributors: Alexander Fridlund, Alexander Bock, Joel Kronander, Daniel Jönsson,     *
 * Erik Sundén                                                                           *
 *                                                                                       *
 * For any questions or information about the SGCT project please contact:               *
 * alexander.bock@liu.se                                                                 *
 *                                                                                       *
 * Redistribution and use in source and binary forms, with or without modification, are  *
 * permitted provided that the following conditions are met:                             *
 *                                                                                       *
 * 1. Redistributions of source code must retain the above copyright notice, this list   * 
 *    of conditions and the following disclaimer.                                        *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this     *
 *    list of conditions and the following disclaimer in the documentation and/or other  *
 *    materials provided with the distribution.                                          *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be  *
 *    used to endorse or promote products derived from this software without specific    *
 *    prior written permission.                                                          *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS' AND ANY   *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES  *
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT   *
 * SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,      *
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  *
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR    *
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN      *
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN    *
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
 * DAMAGE.                                                                               *
 ****************************************************************************************/

/*

SGCT is using the following libraries:

GLFW
=========================================================
Copyright © 2002-2006 Marcus Geelnard
Copyright © 2006-2011 Camilla Berglund
Web: http://www.glfw.org
License: http://www.glfw.org/license.html
---------------------------------------------------------

GLM - OpenGL Mathematics
=========================================================
Copyright (c) 2005 - 2012 G-Truc Creation (www.g-truc.net)
Web: http://glm.g-truc.net/
License: http://glm.g-truc.net/copying.txt
---------------------------------------------------------

Freetype
=========================================================
Copyright 1996-2002, 2006 by David Turner, Robert Wilhelm, and Werner Lemberg
Web: http://www.freetype.org
License: http://www.freetype.org/license.html
---------------------------------------------------------

GLEW - The OpenGL Extension Wrangler Library
=========================================================
Copyright (C) 2002-2007, Milan Ikits <milan.ikits@ieee.org>
Copyright (C) 2002-2007, Marcelo E. Magallon <mmagallo@debian.org>
Copyright (C) 2002, Lev Povalahev

All rights reserved.
Web: http://glew.sourceforge.net/
---------------------------------------------------------

libpng - Portable Network Graphics library
=========================================================
Copyright (c) 1998-2011 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
Web: http://www.libpng.org/
License: http://www.libpng.org/pub/png/src/libpng-LICENSE.txt
---------------------------------------------------------

TinyXML-2
=========================================================
Original code by Lee Thomason (www.grinninglizard.com)
Web: http://www.grinninglizard.com/tinyxml2/index.html
---------------------------------------------------------

VRPN - Virtual Reality Peripheral Network
=========================================================
The Virtual Reality Peripheral Network (VRPN) is public-domain software released by the
Department of Computer Science at the University of North Carolina at Chapel Hill
Web: http://www.cs.unc.edu/Research/vrpn/
---------------------------------------------------------

zlib
=========================================================
Copyright (C) 1995-2010 Jean-loup Gailly and Mark Adler
Web: http://www.zlib.net/
---------------------------------------------------------

libjpeg-turbo
=========================================================
Copyright (c) 1991-1998, Thomas G. Lane.
Modified 2002-2009 by Guido Vollbeding.
Copyright (C) 2009-2011, 2013, D. R. Commander.

Acknowledgement: "this software is based in part on the work of the Independent JPEG Group"
Web: http://www.libjpeg-turbo.org/



SGCT is optionally using the following libraries:
 
OpenVR
=========================================================
OpenVR provides a way to interact with Virtual Reality displays without relying on a
specific hardware vendor's SDK.
Copyright (c) 2015, Valve Corporation
Web: https://github.com/ValveSoftware/openvr
License: BSD-3-Clause
https://github.com/ValveSoftware/openvr/blob/master/LICENSE
*/

/**
 *
 * \mainpage SGCT - Simple Graphics Cluster Toolkit
 *
 * \section intro_sec Introduction
 * SGCT is a static cross-platform C++ library for developing OpenGL (both legacy 
 * compatible profile and modern core profile) applications that are synchronized across a
 * cluster of image generating computers (IGs). SGCT is designed to be as simple as
 * possible for the developer and is well suited for rapid prototyping of immersive
 * virtual reality (VR) applications. SGCT can also be used to create planetarium/dome
 * content and can generate fisheye projections. Its also possible to render to file to
 * create hi-resolution movies. SGCT supports a variety of stereoscopic formats such as
 * active quad buffered stereoscopy, side-by-side stereoscopy, top-bottom stereoscopy,
 * checkerboard/DLP/pixel interlaced stereoscopy, line interlaced stereoscopy and
 * anaglyphic stereoscopy. SGCT applications are scalable and use a XML configuration file
 * where all cluster nodes (IGs) and their properties are specified. Therefore there is no
 * need of recompiling an application for different VR setups. SGCT does also support
 * running cluster applications on a single computer for testing purposes.
 *
 * \subsection dependencies Third party libraries
 * SGCT is using the following libraries.
 *   - <a href="http://www.glfw.org">GLFW 3</a>
 *   - <a href="http://glew.sourceforge.net/">GLEW - The OpenGL Extension Wrangler Library</a>
 *   - <a href="http://glm.g-truc.net/">GLM - OpenGL Mathematics</a>
 *   - <a href="http://www.grinninglizard.com/tinyxml2/index.html">TinyXML-2</a>
 *   - <a href="http://www.cs.unc.edu/Research/vrpn/">VRPN - Virtual Reality Peripheral Network</a>
 *   - <a href="http://www.freetype.org">Freetype 2</a>
 *   - <a href="http://www.zlib.net/">zlib</a>
 *   - <a href="http://www.libpng.org/">libpng</a>
 *   - <a href="http://www.libjpeg-turbo.org/">libjpeg-turbo</a>
 *
 */

#ifndef __SGCT__SGCT__H__
#define __SGCT__SGCT__H__

// If windows OS detected
#if defined(_WIN64) || defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__)
    #ifndef __WIN32__
    #define __WIN32__
    #endif
// if linux
#elif defined(__linux)
    #ifndef __LINUX__
    #define __LINUX__
    #endif
#endif

#ifdef __WIN32__
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif

    #ifndef SGCT_WINDOWS_INCLUDE
        #define SGCT_WINDOWS_INCLUDE
        #define VC_EXTRALEAN
        #include <windows.h> //must be declared before glfw
    #endif

    #ifndef SGCT_WINSOCK_INCLUDE
        #define SGCT_WINSOCK_INCLUDE
        #include <winsock2.h>
    #endif
#endif

#include <sgct/engine.h>
#include <sgct/shareddata.h>
#include <sgct/texturemanager.h>
#ifdef SGCT_HAS_TEXT
    #include <sgct/fontmanager.h>
    #include <sgct/freetype.h>
#endif // SGCT_HAS_TEXT
#include <sgct/messagehandler.h>
#include <sgct/shadermanager.h>
#include <sgct/settings.h>
#include <sgct/version.h>
#include <sgct/ogl_headers.h>

/// This namespace contains helper classes to draw simple geometry.
namespace sgct::utils {}; // empty for doxygen documentation only

#include <sgct/utils/box.h>
#include <sgct/utils/dome.h>
#include <sgct/utils/domegrid.h>
#include <sgct/utils/plane.h>
#include <sgct/utils/sphere.h>

/// This namespace contains various help classes and functions.
namespace sgct::helpers {}; // empty for doxygen documentation only
#include <sgct/helpers/stringfunctions.h>
#include <sgct/helpers/vertexdata.h>

// GLFW wrapping
namespace sgct::action {
    constexpr const int Release = GLFW_RELEASE;
    constexpr const int Press = GLFW_PRESS;
    constexpr const int Repeat = GLFW_REPEAT;
} // namespace sgct::action

namespace sgct::modifier {
    constexpr const int Shift = GLFW_MOD_SHIFT;
    constexpr const int Control = GLFW_MOD_CONTROL;
    constexpr const int Alt = GLFW_MOD_ALT;
    constexpr const int Super = GLFW_MOD_SUPER;
} // namespace sgct::modifier

namespace sgct::key {
    constexpr const int Unknown = GLFW_KEY_UNKNOWN;
    constexpr const int Space = GLFW_KEY_SPACE;
    constexpr const int Apostrophe = GLFW_KEY_APOSTROPHE;
    constexpr const int Comma = GLFW_KEY_COMMA;
    constexpr const int Minus = GLFW_KEY_MINUS;
    constexpr const int Period = GLFW_KEY_PERIOD;
    constexpr const int Slash = GLFW_KEY_SLASH;
    constexpr const int Key0 = GLFW_KEY_0;
    constexpr const int Key1 = GLFW_KEY_1;
    constexpr const int Key2 = GLFW_KEY_2;
    constexpr const int Key3 = GLFW_KEY_3;
    constexpr const int Key4 = GLFW_KEY_4;
    constexpr const int Key5 = GLFW_KEY_5;
    constexpr const int Key6 = GLFW_KEY_6;
    constexpr const int Key7 = GLFW_KEY_7;
    constexpr const int Key8 = GLFW_KEY_8;
    constexpr const int Key9 = GLFW_KEY_9;
    constexpr const int Semicolon = GLFW_KEY_SEMICOLON;
    constexpr const int Equal = GLFW_KEY_EQUAL;
    constexpr const int A = GLFW_KEY_A;
    constexpr const int B = GLFW_KEY_B;
    constexpr const int C = GLFW_KEY_C;
    constexpr const int D = GLFW_KEY_D;
    constexpr const int E = GLFW_KEY_E;
    constexpr const int F = GLFW_KEY_F;
    constexpr const int G = GLFW_KEY_G;
    constexpr const int H = GLFW_KEY_H;
    constexpr const int I = GLFW_KEY_I;
    constexpr const int J = GLFW_KEY_J;
    constexpr const int K = GLFW_KEY_K;
    constexpr const int L = GLFW_KEY_L;
    constexpr const int M = GLFW_KEY_M;
    constexpr const int N = GLFW_KEY_N;
    constexpr const int O = GLFW_KEY_O;
    constexpr const int P = GLFW_KEY_P;
    constexpr const int Q = GLFW_KEY_Q;
    constexpr const int R = GLFW_KEY_R;
    constexpr const int S = GLFW_KEY_S;
    constexpr const int T = GLFW_KEY_T;
    constexpr const int U = GLFW_KEY_U;
    constexpr const int V = GLFW_KEY_V;
    constexpr const int W = GLFW_KEY_W;
    constexpr const int X = GLFW_KEY_X;
    constexpr const int Y = GLFW_KEY_Y;
    constexpr const int Z = GLFW_KEY_Z;
    constexpr const int LeftBracket = GLFW_KEY_LEFT_BRACKET;
    constexpr const int Backslash = GLFW_KEY_BACKSLASH;
    constexpr const int RightBracket = GLFW_KEY_RIGHT_BRACKET;
    constexpr const int GraveAccent = GLFW_KEY_GRAVE_ACCENT;
    constexpr const int World1 = GLFW_KEY_WORLD_1;
    constexpr const int World2 = GLFW_KEY_WORLD_2;
    constexpr const int Esc = GLFW_KEY_ESCAPE;
    constexpr const int Escape = GLFW_KEY_ESCAPE;
    constexpr const int Enter = GLFW_KEY_ENTER;
    constexpr const int Tab = GLFW_KEY_TAB;
    constexpr const int Backspace = GLFW_KEY_BACKSPACE;
    constexpr const int Insert = GLFW_KEY_INSERT;
    constexpr const int Del = GLFW_KEY_DELETE;
    constexpr const int Delete = GLFW_KEY_DELETE;
    constexpr const int Right = GLFW_KEY_RIGHT;
    constexpr const int Left = GLFW_KEY_LEFT;
    constexpr const int Down = GLFW_KEY_DOWN;
    constexpr const int Up = GLFW_KEY_UP;
    constexpr const int Pageup = GLFW_KEY_PAGE_UP;
    constexpr const int Pagedown = GLFW_KEY_PAGE_DOWN;
    constexpr const int PageUp = GLFW_KEY_PAGE_UP;
    constexpr const int PageDown = GLFW_KEY_PAGE_DOWN;
    constexpr const int Home = GLFW_KEY_HOME;
    constexpr const int End = GLFW_KEY_END;
    constexpr const int CapsLock = GLFW_KEY_CAPS_LOCK;
    constexpr const int ScrollLock = GLFW_KEY_SCROLL_LOCK;
    constexpr const int NumLock = GLFW_KEY_NUM_LOCK;
    constexpr const int PrintScreen = GLFW_KEY_PRINT_SCREEN;
    constexpr const int Pause = GLFW_KEY_PAUSE;
    constexpr const int F1 = GLFW_KEY_F1;
    constexpr const int F2 = GLFW_KEY_F2;
    constexpr const int F3 = GLFW_KEY_F3;
    constexpr const int F4 = GLFW_KEY_F4;
    constexpr const int F5 = GLFW_KEY_F5;
    constexpr const int F6 = GLFW_KEY_F6;
    constexpr const int F7 = GLFW_KEY_F7;
    constexpr const int F8 = GLFW_KEY_F8;
    constexpr const int F9 = GLFW_KEY_F9;
    constexpr const int F10 = GLFW_KEY_F10;
    constexpr const int F11 = GLFW_KEY_F11;
    constexpr const int F12 = GLFW_KEY_F12;
    constexpr const int F13 = GLFW_KEY_F13;
    constexpr const int F14 = GLFW_KEY_F14;
    constexpr const int F15 = GLFW_KEY_F15;
    constexpr const int F16 = GLFW_KEY_F16;
    constexpr const int F17 = GLFW_KEY_F17;
    constexpr const int F18 = GLFW_KEY_F18;
    constexpr const int F19 = GLFW_KEY_F19;
    constexpr const int F20 = GLFW_KEY_F20;
    constexpr const int F21 = GLFW_KEY_F21;
    constexpr const int F22 = GLFW_KEY_F22;
    constexpr const int F23 = GLFW_KEY_F23;
    constexpr const int F24 = GLFW_KEY_F24;
    constexpr const int F25 = GLFW_KEY_F25;
    constexpr const int Kp0 = GLFW_KEY_KP_0;
    constexpr const int Kp1 = GLFW_KEY_KP_1;
    constexpr const int Kp2 = GLFW_KEY_KP_2;
    constexpr const int Kp3 = GLFW_KEY_KP_3;
    constexpr const int Kp4 = GLFW_KEY_KP_4;
    constexpr const int Kp5 = GLFW_KEY_KP_5;
    constexpr const int Kp6 = GLFW_KEY_KP_6;
    constexpr const int Kp7 = GLFW_KEY_KP_7;
    constexpr const int Kp8 = GLFW_KEY_KP_8;
    constexpr const int Kp9 = GLFW_KEY_KP_9;
    constexpr const int KpDecimal = GLFW_KEY_KP_DECIMAL;
    constexpr const int KpDivide = GLFW_KEY_KP_DIVIDE;
    constexpr const int KpMultiply = GLFW_KEY_KP_MULTIPLY;
    constexpr const int KpSubtract = GLFW_KEY_KP_SUBTRACT;
    constexpr const int KpAdd = GLFW_KEY_KP_ADD;
    constexpr const int KpEnter = GLFW_KEY_KP_ENTER;
    constexpr const int KpEqual = GLFW_KEY_KP_EQUAL;
    constexpr const int LeftShift = GLFW_KEY_LEFT_SHIFT;
    constexpr const int LeftControl = GLFW_KEY_LEFT_CONTROL;
    constexpr const int LeftAlt = GLFW_KEY_LEFT_ALT;
    constexpr const int LeftSuper = GLFW_KEY_LEFT_SUPER;
    constexpr const int RightShift = GLFW_KEY_RIGHT_SHIFT;
    constexpr const int RightControl = GLFW_KEY_RIGHT_CONTROL;
    constexpr const int RightAlt = GLFW_KEY_RIGHT_ALT;
    constexpr const int RightSuper = GLFW_KEY_RIGHT_SUPER;
    constexpr const int Menu = GLFW_KEY_MENU;
    constexpr const int Last = GLFW_KEY_LAST;
} // namespace sgct::key

namespace sgct::mouse {
    constexpr const int ButtonLeft = GLFW_MOUSE_BUTTON_LEFT;
    constexpr const int ButtonRight = GLFW_MOUSE_BUTTON_RIGHT;
    constexpr const int ButtonMiddle = GLFW_MOUSE_BUTTON_MIDDLE;
    constexpr const int Button1 = GLFW_MOUSE_BUTTON_1;
    constexpr const int Button2 = GLFW_MOUSE_BUTTON_2;
    constexpr const int Button3 = GLFW_MOUSE_BUTTON_3;
    constexpr const int Button4 = GLFW_MOUSE_BUTTON_4;
    constexpr const int Button5 = GLFW_MOUSE_BUTTON_5;
    constexpr const int Button6 = GLFW_MOUSE_BUTTON_6;
    constexpr const int Button7 = GLFW_MOUSE_BUTTON_7;
    constexpr const int Button8 = GLFW_MOUSE_BUTTON_8;
    constexpr const int ButtonLast = GLFW_MOUSE_BUTTON_LAST;
} // namespace sgct::mouse

namespace sgct::joystick {
    constexpr const int Joystick1 = GLFW_JOYSTICK_1;
    constexpr const int Joystick2 = GLFW_JOYSTICK_2;
    constexpr const int Joystick3 = GLFW_JOYSTICK_3;
    constexpr const int Joystick4 = GLFW_JOYSTICK_4;
    constexpr const int Joystick5 = GLFW_JOYSTICK_5;
    constexpr const int Joystick6 = GLFW_JOYSTICK_6;
    constexpr const int Joystick7 = GLFW_JOYSTICK_7;
    constexpr const int Joystick8 = GLFW_JOYSTICK_8;
    constexpr const int Joystick9 = GLFW_JOYSTICK_9;
    constexpr const int Joystick10 = GLFW_JOYSTICK_10;
    constexpr const int Joystick11 = GLFW_JOYSTICK_11;
    constexpr const int Joystick12 = GLFW_JOYSTICK_12;
    constexpr const int Joystick13 = GLFW_JOYSTICK_13;
    constexpr const int Joystick14 = GLFW_JOYSTICK_14;
    constexpr const int Joystick15 = GLFW_JOYSTICK_15;
    constexpr const int Joystick16 = GLFW_JOYSTICK_16;
    constexpr const int JoystickLast = GLFW_JOYSTICK_LAST;
} // namespace sgct::joystick

#endif // __SGCT__SGCT__H__
