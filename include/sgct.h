/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel, Linköping University
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

3.	Neither the name of the copyright holder nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

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
The Virtual Reality Peripheral Network (VRPN) is public-domain software released by the Department of Computer Science at the University of North Carolina at Chapel Hill
Web: http://www.cs.unc.edu/Research/vrpn/
---------------------------------------------------------

zlib
=========================================================
Copyright (C) 1995-2010 Jean-loup Gailly and Mark Adler
Web: http://www.zlib.net/
---------------------------------------------------------

TinyThread++
=========================================================
Copyright (c) 2010-2012 Marcus Geelnard
Web: http://tinythreadpp.bitsnbites.eu/
---------------------------------------------------------

libjpeg-turbo
=========================================================
Copyright (c) 1991-1998, Thomas G. Lane.
Modified 2002-2009 by Guido Vollbeding.
Copyright (C) 2009-2011, 2013, D. R. Commander.

Acknowledgement: "this software is based in part on the work of the Independent JPEG Group"
Web: http://www.libjpeg-turbo.org/
---------------------------------------------------------

*************************************************************************/

/*!

\mainpage SGCT - Simple Graphics Cluster Toolkit

\section intro_sec Introduction
SGCT is a static cross-platform C++ library for developing OpenGL (both legacty compatible profile and modern core profile) applications that are synchronized across a cluster of image generating computers (IGs).
SGCT is designed to be as simple as possible for the developer and is well suited for rapid prototyping of immersive virtual reality (VR) applications.
SGCT can also be used to create planetarium/dome content and can generate fisheye projections. Its also possible to render to file to create hi-resolution movies.
SGCT supports a variety of stereoscopic formats such as active quad buffered stereoscopy, side-by-side stereoscopy, top-bottom stereoscopy, checkerboard/DLP/pixel interlaced stereoscopy, line interlaced stereoscopy and anaglyphic stereoscopy.
SGCT applications are scalable and use a XML configuration file where all cluster nodes (IGs) and their properties are specified. Therefore there is no need of recompiling an application for different VR setups.
SGCT does also support running cluster applications on a single computer for testing purposes.

\subsection licence Licence
Copyright (c) 2012-2015 Miroslav Andel, Link&ouml;ping University\n
All rights reserved.\n
\n
Contributors: Alexander Fridlund, Alexander Bock, Joel Kronander, Daniel J&ouml;nsson\n
For any questions or information about the SGCT project please contact: miroslav.andel@liu.se\n
\n
Redistribution and use in source and binary forms, with or without modification,\n
are permitted provided that the following conditions are met:
    -# Redistributions of source code must retain the above copyright\n
	notice, this list of conditions and the following disclaimer.

    -# Redistributions in binary form must reproduce the above copyright\n
	notice, this list of conditions and the following disclaimer in the\n
	documentation and/or other materials provided with the distribution.

    -# Neither the name of the copyright holder nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

\n
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ''AS IS''\n
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,\n
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.\n
IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT,\n
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\n
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,\n
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,\n
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)\n
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n

\subsection dependencies Third party libraries
SGCT is using the following libraries.
- <a href="http://www.glfw.org">GLFW 3</a>
- <a href="http://glew.sourceforge.net/">GLEW - The OpenGL Extension Wrangler Library</a>
- <a href="http://glm.g-truc.net/">GLM - OpenGL Mathematics</a>
- <a href="http://tinythreadpp.bitsnbites.eu/">TinyThread++</a>
- <a href="http://www.grinninglizard.com/tinyxml2/index.html">TinyXML-2</a>
- <a href="http://www.cs.unc.edu/Research/vrpn/">VRPN - Virtual Reality Peripheral Network</a>
- <a href="http://www.freetype.org">Freetype 2</a>
- <a href="http://www.zlib.net/">zlib</a>
- <a href="http://www.libpng.org/">libpng</a>
- <a href="http://www.libjpeg-turbo.org/">libjpeg-turbo</a>

\subsection copyrights Third party copyright notes

<SMALL>
\subsubsection glfw GLFW
Copyright &copy; 2002-2006 Marcus Geelnard\n
Copyright &copy; 2006-2011 Camilla Berglund\n
\n
This software is provided ‘as-is’, without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.\n
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:\n

   -# The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
   -# Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
   -# This notice may not be removed or altered from any source distribution.

License type: zlib/libpng\n

\subsubsection glew GLEW - The OpenGL Extension Wrangler Library
The OpenGL Extension Wrangler Library\n
Copyright &copy; 2008-2013, Nigel Stewart <nigels[]users sourceforge net>\n
Copyright &copy; 2002-2008, Milan Ikits <milan ikits[]ieee org>\n
Copyright &copy; 2002-2008, Marcelo E. Magallon <mmagallo[]debian org>\n
Copyright &copy; 2002, Lev Povalahev\n
All rights reserved.\n
\n
Redistribution and use in source and binary forms, with or without\n
modification, are permitted provided that the following conditions are met:\n

   - Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
   - The name of the author may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\n
\n
License type: MIT

\subsubsection glm GLM - OpenGL Mathematics
Copyright &copy; 2005 - 2013 G-Truc Creation (www.g-truc.net)\n
License: <a href="http://glm.g-truc.net/copying.txt">http://glm.g-truc.net/copying.txt</a>\n
License type: MIT\n

\subsubsection tinythread TinyThread++
Copyright &copy; 2010-2012 Marcus Geelnard\n
\n
This software is provided 'as-is', without any express or implied\n
warranty. In no event will the authors be held liable for any damages\n
arising from the use of this software.\n
\n
Permission is granted to anyone to use this software for any purpose,\n
including commercial applications, and to alter it and redistribute it\n
freely, subject to the following restrictions:\n
\n
   -# The origin of this software must not be misrepresented; you must not\n
   claim that you wrote the original software. If you use this software\n
   in a product, an acknowledgment in the product documentation would be\n
   appreciated but is not required.

   -# Altered source versions must be plainly marked as such, and must not be\n
   misrepresented as being the original software.

   -# This notice may not be removed or altered from any source distribution.


License type: zlib/libpng

\subsubsection tinyxml TinyXML-2
Original code by Lee Thomason (www.grinninglizard.com)\n
\n
This software is provided 'as-is', without any express or implied\n
warranty. In no event will the authors be held liable for any\n
damages arising from the use of this software.\n
\n
Permission is granted to anyone to use this software for any\n
purpose, including commercial applications, and to alter it and\n
redistribute it freely, subject to the following restrictions:\n
\n
   -# The origin of this software must not be misrepresented; you must\n
   not claim that you wrote the original software. If you use this\n
   software in a product, an acknowledgment in the product documentation\n
   would be appreciated but is not required.

   -# Altered source versions must be plainly marked as such, and\n
   must not be misrepresented as being the original software.

   -# This notice may not be removed or altered from any source distribution.


License type: zlib

\subsubsection vrpn VRPN - Virtual Reality Peripheral Network
The Virtual Reality Peripheral Network (VRPN) is public-domain software\n
released by the Department of Computer Science at\n
the University of North Carolina at Chapel Hill\n
Acknowledgement: The CISMM project at the University of North Carolina at Chapel Hill, supported by NIH/NCRR and NIH/NIBIB award #2P41EB002025\n
\n
License type: Boost Software License 1.0\n

\subsubsection freetype Freetype 2
Copyright &copy; 2011 The FreeType Project (www.freetype.org). All rights reserved.\n
Copyright &copy; 1996-2002, 2006 by David Turner, Robert Wilhelm, and Werner Lemberg\n
\n
License: <a href="http://www.freetype.org/license.html">http://www.freetype.org/license.html</a>\n
License type: BSD\n

\subsubsection zlib zlib
&copy; 1995-2012 Jean-loup Gailly and Mark Adler\n
\n
This software is provided 'as-is', without any express or implied\n
warranty.  In no event will the authors be held liable for any damages\n
arising from the use of this software.\n
\n
Permission is granted to anyone to use this software for any purpose,\n
including commercial applications, and to alter it and redistribute it\n
freely, subject to the following restrictions:\n

   -# The origin of this software must not be misrepresented; you must not\n
   claim that you wrote the original software. If you use this software\n
   in a product, an acknowledgment in the product documentation would be\n
   appreciated but is not required.
   -# Altered source versions must be plainly marked as such, and must not be\n
   misrepresented as being the original software.
   -# This notice may not be removed or altered from any source distribution.

Jean-loup Gailly jloup@gzip.org\n
Mark Adler madler@alumni.caltech.edu\n
\n
License type: zlib

\subsubsection libpng libpng - Portable Network Graphics library
This copy of the libpng notices is provided for your convenience.  In case of\n
any discrepancy between this copy and the notices in the file png.h that is\n
included in the libpng distribution, the latter shall prevail.\n
\n
COPYRIGHT NOTICE, DISCLAIMER, and LICENSE:\n
\n
If you modify libpng you may insert additional notices immediately following\n
this sentence.\n
\n
This code is released under the libpng license.\n
\n
libpng versions 1.2.6, August 15, 2004, through 1.5.14, January 24, 2013, are\n
Copyright &copy; 2004, 2006-2012 Glenn Randers-Pehrson, and are\n
distributed according to the same disclaimer and license as libpng-1.2.5\n
with the following individual added to the list of Contributing Authors\n

   - Cosmin Truta

libpng versions 1.0.7, July 1, 2000, through 1.2.5 - October 3, 2002, are\n
Copyright &copy; 2000-2002 Glenn Randers-Pehrson, and are\n
distributed according to the same disclaimer and license as libpng-1.0.6\n
with the following individuals added to the list of Contributing Authors\n

   - Simon-Pierre Cadieux
   - Eric S. Raymond
   - Gilles Vollant

and with the following additions to the disclaimer:\n
\n
   There is no warranty against interference with your enjoyment of the\n
   library or against infringement.  There is no warranty that our\n
   efforts or the library will fulfill any of your particular purposes\n
   or needs.  This library is provided with all faults, and the entire\n
   risk of satisfactory quality, performance, accuracy, and effort is with\n
   the user.\n
   \n
libpng versions 0.97, January 1998, through 1.0.6, March 20, 2000, are\n
Copyright (c) 1998, 1999 Glenn Randers-Pehrson, and are\n
distributed according to the same disclaimer and license as libpng-0.96,\n
with the following individuals added to the list of Contributing Authors:\n

   - Tom Lane
   - Glenn Randers-Pehrson
   - Willem van Schaik

libpng versions 0.89, June 1996, through 0.96, May 1997, are\n
Copyright &copy; 1996, 1997 Andreas Dilger\n
Distributed according to the same disclaimer and license as libpng-0.88,\n
with the following individuals added to the list of Contributing Authors:\n

   - John Bowler
   - Kevin Bracey
   - Sam Bushell
   - Magnus Holmgren
   - Greg Roelofs
   - Tom Tanner

libpng versions 0.5, May 1995, through 0.88, January 1996, are\n
Copyright &copy; 1995, 1996 Guy Eric Schalnat, Group 42, Inc.\n
\n
For the purposes of this copyright and license, "Contributing Authors"\n
is defined as the following set of individuals:\n

   - Andreas Dilger
   - Dave Martindale
   - Guy Eric Schalnat
   - Paul Schmidt
   - Tim Wegner

The PNG Reference Library is supplied "AS IS".  The Contributing Authors\n
and Group 42, Inc. disclaim all warranties, expressed or implied,\n
including, without limitation, the warranties of merchantability and of\n
fitness for any purpose.  The Contributing Authors and Group 42, Inc.\n
assume no liability for direct, indirect, incidental, special, exemplary,\n
or consequential damages, which may result from the use of the PNG\n
Reference Library, even if advised of the possibility of such damage.\n
\n
Permission is hereby granted to use, copy, modify, and distribute this\n
source code, or portions hereof, for any purpose, without fee, subject\n
to the following restrictions:\n
\n
   -# The origin of this source code must not be misrepresented.
   -# Altered versions must be plainly marked as such and must not\n
   be misrepresented as being the original source.

   -# This Copyright notice may not be removed or altered from any\n
   source or altered source distribution.

The Contributing Authors and Group 42, Inc. specifically permit, without\n
fee, and encourage the use of this source code as a component to\n
supporting the PNG file format in commercial products.  If you use this\n
source code in a product, acknowledgment is not required but would be\n
appreciated.\n
\n
A "png_get_copyright" function is available, for convenient use in "about"\n
boxes and the like:\n
\n
   printf("%s",png_get_copyright(NULL));\n
\n
Also, the PNG logo (in PNG format, of course) is supplied in the\n
files "pngbar.png" and "pngbar.jpg (88x31) and "pngnow.png" (98x31).\n
\n
Libpng is OSI Certified Open Source Software.  OSI Certified Open Source is a\n
certification mark of the Open Source Initiative.\n
\n
Glenn Randers-Pehrson\n
glennrp at users.sourceforge.net\n
January 24, 2013\n
\n
License: <a href="http://www.libpng.org/pub/png/src/libpng-LICENSE.txt">http://www.libpng.org/pub/png/src/libpng-LICENSE.txt</a>\n
License type: libpng

\subsubsection libjpegturbo libjpeg-turbo
Copyright &copy; 1991-2012, Thomas G. Lane, Guido Vollbeding\n
Modified 2002-2009 by Guido Vollbeding.\n
Copyright &copy; 2009-2011, 2013, D. R. Commander.\n
Acknowledgement: "this software is based in part on the work of the Independent JPEG Group"\n
\n
License type: BSD\n
</SMALL>
\n
*/

#ifndef _SGCT_H_
#define _SGCT_H_

//If windows OS detected
#if defined(_WIN64) || defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__)
    #ifndef __WIN32__
    #define __WIN32__
    #endif
//if linux
#elif defined(__linux)
    #ifndef __LINUX__
    #define __LINUX__
    #endif
#endif

#ifdef __WIN32__
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif

	//#define _CRT_SECURE_NO_DEPRECATE 1
	//#define _CRT_NONSTDC_NO_DEPRECATE 1

	#ifndef SGCT_WINDOWS_INCLUDE
		#define SGCT_WINDOWS_INCLUDE
		#include <windows.h> //must be declared before glfw
    #endif

    #ifndef SGCT_WINSOCK_INCLUDE
        #define SGCT_WINSOCK_INCLUDE
		#include <winsock2.h>
	#endif
#endif

#include "sgct/SGCTConfig.h"
#include "sgct/Engine.h"
#include "sgct/SharedData.h"
#include "sgct/TextureManager.h"
#if INCLUDE_SGCT_TEXT
	#include "sgct/FontManager.h"
	#include "sgct/freetype.h"
	//for backwards compability
	namespace Freetype = sgct_text;
#endif
#include "sgct/MessageHandler.h"
#include "sgct/ShaderManager.h"
#include "sgct/SGCTSettings.h"
#include "sgct/SGCTVersion.h"
#include "sgct/ogl_headers.h"

#ifndef TINYXML_H
	#ifndef SGCT_DONT_USE_EXTERNAL
		#define TINYXML_H "external/tinyxml2.h"
	#else
		#define TINYXML_H "tinyxml2.h"
	#endif
#endif

#ifndef SGCT_DONT_USE_EXTERNAL
	#include "external/tinythread.h"
#else
	#include <tinythread.h>
#endif

//utilities
/*!
	Ths namespace contains helper classes to draw simple geometry.
*/
namespace sgct_utils{}; //empty for doxygen documentation only
#include "sgct/utils/SGCTSphere.h"
#include "sgct/utils/SGCTBox.h"
#include "sgct/utils/SGCTDome.h"
#include "sgct/utils/SGCTPlane.h"

#include "sgct/utils/SGCTDomeGrid.h"

//helpers
/*!
Ths namespace contains various help classes and functions.
*/
namespace sgct_helpers{}; //empty for doxygen documentation only
//#include "sgct/helpers/SGCTStringFunctions.h"
#include "sgct/helpers/SGCTVertexData.h"
#include "sgct/helpers/SGCTCPPEleven.h"
//#include "sgct/helpers/SGCTPortedFunctions.h"

//GLFW wrapping
#define SGCT_RELEASE		GLFW_RELEASE
#define SGCT_PRESS			GLFW_PRESS
#define SGCT_REPEAT			GLFW_REPEAT

#define SGCT_MOD_SHIFT          GLFW_MOD_SHIFT
#define SGCT_MOD_CONTROL        GLFW_MOD_CONTROL
#define SGCT_MOD_ALT            GLFW_MOD_ALT
#define SGCT_MOD_SUPER          GLFW_MOD_SUPER

#define SGCT_KEY_UNKNOWN		GLFW_KEY_UNKNOWN
#define SGCT_KEY_SPACE			GLFW_KEY_SPACE
#define SGCT_KEY_APOSTROPHE		GLFW_KEY_APOSTROPHE
#define SGCT_KEY_COMMA			GLFW_KEY_COMMA
#define SGCT_KEY_MINUS			GLFW_KEY_MINUS
#define SGCT_KEY_PERIOD			GLFW_KEY_PERIOD
#define SGCT_KEY_SLASH			GLFW_KEY_SLASH
#define SGCT_KEY_0				GLFW_KEY_0
#define SGCT_KEY_1				GLFW_KEY_1
#define SGCT_KEY_2				GLFW_KEY_2
#define SGCT_KEY_3				GLFW_KEY_3
#define SGCT_KEY_4				GLFW_KEY_4
#define SGCT_KEY_5				GLFW_KEY_5
#define SGCT_KEY_6				GLFW_KEY_6
#define SGCT_KEY_7				GLFW_KEY_7
#define SGCT_KEY_8				GLFW_KEY_8
#define SGCT_KEY_9				GLFW_KEY_9
#define SGCT_KEY_SEMICOLON		GLFW_KEY_SEMICOLON
#define SGCT_KEY_EQUAL			GLFW_KEY_EQUAL
#define SGCT_KEY_A				GLFW_KEY_A
#define SGCT_KEY_B				GLFW_KEY_B
#define SGCT_KEY_C				GLFW_KEY_C
#define SGCT_KEY_D				GLFW_KEY_D
#define SGCT_KEY_E				GLFW_KEY_E
#define SGCT_KEY_F				GLFW_KEY_F
#define SGCT_KEY_G				GLFW_KEY_G
#define SGCT_KEY_H				GLFW_KEY_H
#define SGCT_KEY_I				GLFW_KEY_I
#define SGCT_KEY_J				GLFW_KEY_J
#define SGCT_KEY_K				GLFW_KEY_K
#define SGCT_KEY_L				GLFW_KEY_L
#define SGCT_KEY_M				GLFW_KEY_M
#define SGCT_KEY_N				GLFW_KEY_N
#define SGCT_KEY_O				GLFW_KEY_O
#define SGCT_KEY_P				GLFW_KEY_P
#define SGCT_KEY_Q				GLFW_KEY_Q
#define SGCT_KEY_R				GLFW_KEY_R
#define SGCT_KEY_S				GLFW_KEY_S
#define SGCT_KEY_T				GLFW_KEY_T
#define SGCT_KEY_U				GLFW_KEY_U
#define SGCT_KEY_V				GLFW_KEY_V
#define SGCT_KEY_W				GLFW_KEY_W
#define SGCT_KEY_X				GLFW_KEY_X
#define SGCT_KEY_Y				GLFW_KEY_Y
#define SGCT_KEY_Z				GLFW_KEY_Z
#define SGCT_KEY_LEFT_BRACKET	GLFW_KEY_LEFT_BRACKET
#define SGCT_KEY_BACKSLASH		GLFW_KEY_BACKSLASH
#define SGCT_KEY_RIGHT_BRACKET	GLFW_KEY_RIGHT_BRACKET
#define SGCT_KEY_GRAVE_ACCENT	GLFW_KEY_GRAVE_ACCENT
#define SGCT_KEY_WORLD_1		GLFW_KEY_WORLD_1
#define SGCT_KEY_WORLD_2		GLFW_KEY_WORLD_2
#define SGCT_KEY_ESC			GLFW_KEY_ESCAPE
#define SGCT_KEY_ESCAPE			GLFW_KEY_ESCAPE
#define SGCT_KEY_ENTER			GLFW_KEY_ENTER
#define SGCT_KEY_TAB			GLFW_KEY_TAB
#define SGCT_KEY_BACKSPACE		GLFW_KEY_BACKSPACE
#define SGCT_KEY_INSERT			GLFW_KEY_INSERT
#define SGCT_KEY_DEL			GLFW_KEY_DELETE
#define SGCT_KEY_DELETE			GLFW_KEY_DELETE
#define SGCT_KEY_RIGHT			GLFW_KEY_RIGHT
#define SGCT_KEY_LEFT			GLFW_KEY_LEFT
#define SGCT_KEY_DOWN			GLFW_KEY_DOWN
#define SGCT_KEY_UP				GLFW_KEY_UP
#define SGCT_KEY_PAGEUP			GLFW_KEY_PAGE_UP
#define SGCT_KEY_PAGEDOWN		GLFW_KEY_PAGE_DOWN
#define SGCT_KEY_PAGE_UP		GLFW_KEY_PAGE_UP
#define SGCT_KEY_PAGE_DOWN		GLFW_KEY_PAGE_DOWN
#define SGCT_KEY_HOME			GLFW_KEY_HOME
#define SGCT_KEY_END			GLFW_KEY_END
#define SGCT_KEY_CAPS_LOCK		GLFW_KEY_CAPS_LOCK
#define SGCT_KEY_SCROLL_LOCK	GLFW_KEY_SCROLL_LOCK
#define SGCT_KEY_NUM_LOCK		GLFW_KEY_NUM_LOCK
#define SGCT_KEY_PRINT_SCREEN	GLFW_KEY_PRINT_SCREEN
#define SGCT_KEY_PAUSE			GLFW_KEY_PAUSE
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
#define SGCT_KEY_KP_DECIMAL		GLFW_KEY_KP_DECIMAL
#define SGCT_KEY_KP_DIVIDE		GLFW_KEY_KP_DIVIDE
#define SGCT_KEY_KP_MULTIPLY	GLFW_KEY_KP_MULTIPLY
#define SGCT_KEY_KP_SUBTRACT	GLFW_KEY_KP_SUBTRACT
#define SGCT_KEY_KP_ADD			GLFW_KEY_KP_ADD
#define SGCT_KEY_KP_ENTER		GLFW_KEY_KP_ENTER
#define SGCT_KEY_KP_EQUAL		GLFW_KEY_KP_EQUAL
#define SGCT_KEY_LSHIFT			GLFW_KEY_LEFT_SHIFT
#define SGCT_KEY_LEFT_SHIFT		GLFW_KEY_LEFT_SHIFT
#define SGCT_KEY_LCTRL			GLFW_KEY_LEFT_CONTROL
#define SGCT_KEY_LEFT_CONTROL	GLFW_KEY_LEFT_CONTROL
#define SGCT_KEY_LALT			GLFW_KEY_LEFT_ALT
#define SGCT_KEY_LEFT_ALT		GLFW_KEY_LEFT_ALT
#define SGCT_KEY_LEFT_SUPER		GLFW_KEY_LEFT_SUPER
#define SGCT_KEY_RSHIFT			GLFW_KEY_RIGHT_SHIFT
#define SGCT_KEY_RIGHT_SHIFT	GLFW_KEY_RIGHT_SHIFT
#define SGCT_KEY_RCTRL			GLFW_KEY_RIGHT_CONTROL
#define SGCT_KEY_RIGHT_CONTROL	GLFW_KEY_RIGHT_CONTROL
#define SGCT_KEY_RALT			GLFW_KEY_RIGHT_ALT
#define SGCT_KEY_RIGHT_ALT		GLFW_KEY_RIGHT_ALT
#define SGCT_KEY_RIGHT_SUPER	GLFW_KEY_RIGHT_SUPER
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

#endif
