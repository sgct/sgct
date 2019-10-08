/*****************************************************************************************
 *                                                                                       *
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * Miroslav Andel, Linköping University                                                  *
 * Alexander Bock, Linköping University                                                  *
 *                                                                                       *
 * Contributors: Alexander Fridlund, Joel Kronander, Daniel Jönsson, Erik Sundén,        *
 * Gene Payne                                                                            *
 *                                                                                       *
 * For any questions or information about the project contact: alexander.bock@liu.se     *
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
 *                                                                                       *
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
#include <GLFW/glfw3.h>
#include <sgct/keys.h>
#include <sgct/mouse.h>
#include <sgct/joystick.h>

#endif // __SGCT__SGCT__H__
