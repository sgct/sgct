/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

Contributors: Alexander Fridlund, Alexander Bock, Joel Kronander, Daniel Jönsson

For any questions or information about the SGCT project please contact: miroslav.andel@liu.se

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1.	Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
	
2.	Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
	
3.	Neither the name of the Linköping University nor the
    names of its contributors may be used to endorse or promote products
	derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ''AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************/

/*************************************************************************
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

libPNG - Portable Network Graphics library
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
The Virtual Reality Peripheral Network (VRPN) is public-domain software released by the Department of Computer Science at the University of North Carolina at Chapel Hill
Web: http://www.cs.unc.edu/Research/vrpn/
---------------------------------------------------------

ZLib
=========================================================
Copyright (C) 1995-2010 Jean-loup Gailly and Mark Adler
Web: http://www.zlib.net/
---------------------------------------------------------
*************************************************************************/

#ifdef __WIN32__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1
#endif

#ifndef _SGCT_H_
#define _SGCT_H_

#ifdef __WIN32__
	#ifndef SGCT_WINDOWS_INCLUDE
		#define SGCT_WINDOWS_INCLUDE
		#include <windows.h> //must be declared before glfw
		#include <winsock2.h>
	#endif	
#endif

#include "sgct/ogl_headers.h"
#include "sgct/Engine.h"
#include "sgct/SharedData.h"
#include "sgct/TextureManager.h"
#include "sgct/FontManager.h"
#include "sgct/MessageHandler.h"
#include "sgct/ShaderManager.h"
#include "sgct/freetype.h"
#include "sgct/SGCTVersion.h"

//utilities
/*!
	Ths namespace contains helper classes to draw simple geometry.
*/
namespace sgct_utils{}; //empty for doxygen documentation only
#include "sgct/utils/SGCTSphere.h"
#include "sgct/utils/SGCTBox.h"
#include "sgct/utils/SGCTDome.h"

//for backwards compability for sgct 0.9.5 and backwards
namespace Freetype = sgct_text;
namespace core_sgct = sgct_core;

//GLFW wrapping
#define SGCT_RELEASE		GLFW_RELEASE
#define SGCT_PRESS			GLFW_PRESS

#define SGCT_KEY_UNKNOWN		GLFW_KEY_UNKNOWN
#define SGCT_KEY_SPACE			GLFW_KEY_SPACE
#define SGCT_KEY_SPECIAL		GLFW_KEY_SPECIAL
#define SGCT_KEY_ESC			GLFW_KEY_ESC
#define SGCT_KEY_F1				GLFW_KEY_F1
#define SGCT_KEY_F2				GLFW_KEY_F2
#define SGCT_KEY_F3				GLFW_KEY_F3
#define SGCT_KEY_F4				GLFW_KEY_F4
#define SGCT_KEY_F5				GLFW_KEY_F5
#define SGCT_KEY_F6				GLFW_KEY_F6
#define SGCT_KEY_F7				GLFW_KEY_F7
#define SGCT_KEY_F8				GLFW_KEY_F8
#define SGCT_KEY_F9				GLFW_KEY_F9
#define SGCT_KEY_F10			GLFW_KEY_F10
#define SGCT_KEY_F11			GLFW_KEY_F11
#define SGCT_KEY_F12			GLFW_KEY_F12
#define SGCT_KEY_F13			GLFW_KEY_F13
#define SGCT_KEY_F14			GLFW_KEY_F14
#define SGCT_KEY_F15			GLFW_KEY_F15
#define SGCT_KEY_F16			GLFW_KEY_F16
#define SGCT_KEY_F17			GLFW_KEY_F17
#define SGCT_KEY_F18			GLFW_KEY_F18
#define SGCT_KEY_F19			GLFW_KEY_F19
#define SGCT_KEY_F20			GLFW_KEY_F20
#define SGCT_KEY_F21			GLFW_KEY_F21
#define SGCT_KEY_F22			GLFW_KEY_F22
#define SGCT_KEY_F23			GLFW_KEY_F23
#define SGCT_KEY_F24			GLFW_KEY_F24
#define SGCT_KEY_F25			GLFW_KEY_F25
#define SGCT_KEY_UP				GLFW_KEY_UP
#define SGCT_KEY_DOWN			GLFW_KEY_DOWN
#define SGCT_KEY_LEFT			GLFW_KEY_LEFT
#define SGCT_KEY_RIGHT			GLFW_KEY_RIGHT
#define SGCT_KEY_LSHIFT			GLFW_KEY_LSHIFT
#define SGCT_KEY_RSHIFT			GLFW_KEY_RSHIFT
#define SGCT_KEY_LCTRL			GLFW_KEY_LCTRL
#define SGCT_KEY_RCTRL			GLFW_KEY_RCTRL
#define SGCT_KEY_LALT			GLFW_KEY_LALT
#define SGCT_KEY_RALT			GLFW_KEY_RALT
#define SGCT_KEY_TAB			GLFW_KEY_TAB
#define SGCT_KEY_ENTER			GLFW_KEY_ENTER
#define SGCT_KEY_BACKSPACE		GLFW_KEY_BACKSPACE
#define SGCT_KEY_INSERT			GLFW_KEY_INSERT
#define SGCT_KEY_DEL			GLFW_KEY_DEL
#define SGCT_KEY_PAGEUP			GLFW_KEY_PAGEUP
#define SGCT_KEY_PAGEDOWN		GLFW_KEY_PAGEDOWN
#define SGCT_KEY_HOME			GLFW_KEY_HOME
#define SGCT_KEY_END			GLFW_KEY_END
#define SGCT_KEY_KP_0			GLFW_KEY_KP_0
#define SGCT_KEY_KP_1			GLFW_KEY_KP_1
#define SGCT_KEY_KP_2			GLFW_KEY_KP_2
#define SGCT_KEY_KP_3			GLFW_KEY_KP_3
#define SGCT_KEY_KP_4			GLFW_KEY_KP_4
#define SGCT_KEY_KP_5			GLFW_KEY_KP_5
#define SGCT_KEY_KP_6			GLFW_KEY_KP_6
#define SGCT_KEY_KP_7			GLFW_KEY_KP_7
#define SGCT_KEY_KP_8			GLFW_KEY_KP_8
#define SGCT_KEY_KP_9			GLFW_KEY_KP_9
#define SGCT_KEY_KP_DIVIDE		GLFW_KEY_KP_DIVIDE
#define SGCT_KEY_KP_MULTIPLY	GLFW_KEY_KP_MULTIPLY
#define SGCT_KEY_KP_SUBTRACT	GLFW_KEY_KP_SUBTRACT
#define SGCT_KEY_KP_ADD			GLFW_KEY_KP_ADD
#define SGCT_KEY_KP_DECIMAL		GLFW_KEY_KP_DECIMAL
#define SGCT_KEY_KP_EQUAL		GLFW_KEY_KP_EQUAL
#define SGCT_KEY_KP_ENTER		GLFW_KEY_KP_ENTER
#define SGCT_KEY_KP_NUM_LOCK	GLFW_KEY_KP_NUM_LOCK
#define SGCT_KEY_CAPS_LOCK		GLFW_KEY_CAPS_LOCK
#define SGCT_KEY_SCROLL_LOCK	GLFW_KEY_SCROLL_LOCK
#define SGCT_KEY_PAUSE			GLFW_KEY_PAUSE
#define SGCT_KEY_LSUPER			GLFW_KEY_LSUPER
#define SGCT_KEY_RSUPER			GLFW_KEY_RSUPER
#define SGCT_KEY_MENU			GLFW_KEY_MENU
#define SGCT_KEY_LAST			GLFW_KEY_LAST

/* Mouse button definitions */
#define SGCT_MOUSE_BUTTON_1		GLFW_MOUSE_BUTTON_1
#define SGCT_MOUSE_BUTTON_2		GLFW_MOUSE_BUTTON_2
#define SGCT_MOUSE_BUTTON_3		GLFW_MOUSE_BUTTON_3
#define SGCT_MOUSE_BUTTON_4		GLFW_MOUSE_BUTTON_4
#define SGCT_MOUSE_BUTTON_5		GLFW_MOUSE_BUTTON_5
#define SGCT_MOUSE_BUTTON_6		GLFW_MOUSE_BUTTON_6
#define SGCT_MOUSE_BUTTON_7		GLFW_MOUSE_BUTTON_7
#define SGCT_MOUSE_BUTTON_8		GLFW_MOUSE_BUTTON_8
#define SGCT_MOUSE_BUTTON_LAST	GLFW_MOUSE_BUTTON_LAST

/* Mouse button aliases */
#define SGCT_MOUSE_BUTTON_LEFT		GLFW_MOUSE_BUTTON_LEFT
#define SGCT_MOUSE_BUTTON_RIGHT		GLFW_MOUSE_BUTTON_RIGHT
#define SGCT_MOUSE_BUTTON_MIDDLE	GLFW_MOUSE_BUTTON_MIDDLE


/* Joystick identifiers */
#define SGCT_JOYSTICK_1		GLFW_JOYSTICK_1
#define SGCT_JOYSTICK_2		GLFW_JOYSTICK_2
#define SGCT_JOYSTICK_3		GLFW_JOYSTICK_3
#define SGCT_JOYSTICK_4		GLFW_JOYSTICK_4
#define SGCT_JOYSTICK_5		GLFW_JOYSTICK_5
#define SGCT_JOYSTICK_6		GLFW_JOYSTICK_6
#define SGCT_JOYSTICK_7		GLFW_JOYSTICK_7
#define SGCT_JOYSTICK_8		GLFW_JOYSTICK_8
#define SGCT_JOYSTICK_9		GLFW_JOYSTICK_9
#define SGCT_JOYSTICK_10	GLFW_JOYSTICK_10
#define SGCT_JOYSTICK_11	GLFW_JOYSTICK_11
#define SGCT_JOYSTICK_12	GLFW_JOYSTICK_12
#define SGCT_JOYSTICK_13	GLFW_JOYSTICK_13
#define SGCT_JOYSTICK_14	GLFW_JOYSTICK_14
#define SGCT_JOYSTICK_15	GLFW_JOYSTICK_15
#define SGCT_JOYSTICK_16	GLFW_JOYSTICK_16
#define SGCT_JOYSTICK_LAST	GLFW_JOYSTICK_LAST

/* glfwEnable/glfwDisable tokens */
#define SGCT_MOUSE_CURSOR			GLFW_MOUSE_CURSOR
#define SGCT_STICKY_KEYS			GLFW_STICKY_KEYS
#define SGCT_STICKY_MOUSE_BUTTONS	GLFW_STICKY_MOUSE_BUTTONS
#define SGCT_SYSTEM_KEYS			GLFW_SYSTEM_KEYS
#define SGCT_KEY_REPEAT				GLFW_KEY_REPEAT
#define SGCT_AUTO_POLL_EVENTS		GLFW_AUTO_POLL_EVENTS

#define SGCT_PRESENT		GLFW_PRESENT
#define SGCT_AXES			GLFW_AXES
#define SGCT_BUTTONS		GLFW_BUTTONS

#endif
