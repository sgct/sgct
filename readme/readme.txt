SGCT - Simple Graphics Cluster Toolkit
===========================================
SGCT is a free static cross-platform C++ library for developing OpenGL applications
that are synchronized across a cluster of image generating computers (IGs).
SGCT is designed to be as simple as possible for the developer and is
well suited for rapid prototyping of immersive virtual reality (VR) applications.
SGCT can also be used to create planetarium/dome content and can generate fisheye projections.
Its also possible to render to file to create extreeme hi-resolution stereoscopic 3D movies.

SGCT supports a variety of stereoscopic formats such as active quad buffered stereoscopy,
passive side-by-side stereoscopy, passive over-and-under stereoscopy,
checkerboard/DLP/pixel interlaced stereoscopy and anaglyphic stereoscopy.
SGCT applications are scalable and use a XML configuration file where all cluster nodes (IGs)
and their properties are specified. Therefore there is no need of recompiling an application for
different VR setups. SGCT does also support running cluster applications on a single computer
for testing purposes.


SOFTWARE PRODUCT LICENSE
========================
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

THIRD PARTY LIBRARIES
=====================
SGCT have incorporated source code and libraries under licenses as listed below.


GLFW
=========================================================
Copyright © 2002-2006 Marcus Geelnard
Copyright © 2006-2011 Camilla Berglund
Web: http://www.glfw.org
License: http://www.glfw.org/license.html
License type: zlib/libpng
---------------------------------------------------------


GLM - OpenGL Mathematics
=========================================================
Copyright (c) 2005 - 2012 G-Truc Creation (www.g-truc.net)
Web: http://glm.g-truc.net/
License: http://glm.g-truc.net/copying.txt
License type: MIT
---------------------------------------------------------


Freetype
=========================================================
The Freetype library is copyright © 2011 The FreeType Project (www.freetype.org). All rights reserved.
Copyright 1996-2002, 2006 by David Turner, Robert Wilhelm, and Werner Lemberg
Web: http://www.freetype.org
License: http://www.freetype.org/license.html
License type: BSD with credit clause or GPLv2 
---------------------------------------------------------


GLEW - The OpenGL Extension Wrangler Library
=========================================================
Copyright (C) 2008-2013, Nigel Stewart <nigels[]users sourceforge net>
Copyright (C) 2002-2008, Milan Ikits <milan ikits[]ieee org>
Copyright (C) 2002-2008, Marcelo E. Magallon <mmagallo[]debian org>
Copyright (C) 2002, Lev Povalahev

All rights reserved.
Web: http://glew.sourceforge.net/
License type: MIT
---------------------------------------------------------


libpng - Portable Network Graphics library
=========================================================
libpng versions 1.2.6, August 15, 2004, through 1.5.14, January 24, 2013, are
Copyright (c) 2004, 2006-2013 Glenn Randers-Pehrson, and are
distributed according to the same disclaimer and license as libpng-1.2.5
with the following individual added to the list of Contributing Authors:
 *    Cosmin Truta

libpng versions 1.0.7, July 1, 2000, through 1.2.5, October 3, 2002, are
Copyright (c) 2000-2002 Glenn Randers-Pehrson, and are
distributed according to the same disclaimer and license as libpng-1.0.6
with the following individuals added to the list of Contributing Authors:

 *    Simon-Pierre Cadieux
 *    Eric S. Raymond
 *    Gilles Vollant

and with the following additions to the disclaimer:

libpng versions 0.97, January 1998, through 1.0.6, March 20, 2000, are
Copyright (c) 1998, 1999, 2000 Glenn Randers-Pehrson, and are
distributed according to the same disclaimer and license as libpng-0.96,
with the following individuals added to the list of Contributing Authors:

 *    Tom Lane
 *    Glenn Randers-Pehrson
 *    Willem van Schaik

 * libpng versions 0.89, June 1996, through 0.96, May 1997, are
 * Copyright (c) 1996, 1997 Andreas Dilger
 * Distributed according to the same disclaimer and license as libpng-0.88,
 * with the following individuals added to the list of Contributing Authors:

 *    John Bowler
 *    Kevin Bracey
 *    Sam Bushell
 *    Magnus Holmgren
 *    Greg Roelofs
 *    Tom Tanner

libpng versions 0.5, May 1995, through 0.88, January 1996, are
Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.

For the purposes of this copyright and license, "Contributing Authors"
is defined as the following set of individuals:

 *    Andreas Dilger
 *    Dave Martindale
 *    Guy Eric Schalnat
 *    Paul Schmidt
 *    Tim Wegner

Web: http://www.libpng.org/
License: http://www.libpng.org/pub/png/src/libpng-LICENSE.txt
License type: libpng
---------------------------------------------------------


TinyXML-2
=========================================================
Original code by Lee Thomason (www.grinninglizard.com)
Web: http://www.grinninglizard.com/tinyxml2docs/index.html
License type: zlib
---------------------------------------------------------


VRPN - Virtual Reality Peripheral Network
=========================================================
The Virtual Reality Peripheral Network (VRPN) is public-domain software released by the Department of Computer Science at the University of North Carolina at Chapel Hill
Web: http://www.cs.unc.edu/Research/vrpn/obtaining_vrpn.html
Acknowledgement: The CISMM project at the University of North Carolina at Chapel Hill, supported by NIH/NCRR and NIH/NIBIB award #2P41EB002025
License type: Boost Software License 1.0 
---------------------------------------------------------


zlib
=========================================================
Copyright (C) 1995-2012 Jean-loup Gailly and Mark Adler
Web: http://www.zlib.net/
License type: zlib
---------------------------------------------------------


TinyThread++
=========================================================
Copyright (c) 2010-2012 Marcus Geelnard
Web: http://tinythreadpp.bitsnbites.eu/
License type: zlib/libpng
---------------------------------------------------------


libjpeg-turbo
=========================================================
Copyright (c) 1991-2012, Thomas G. Lane, Guido Vollbeding
Modified 2002-2009 by Guido Vollbeding.
Copyright (C) 2009-2011, 2013, D. R. Commander.

Acknowledgement: "this software is based in part on the work of the Independent JPEG Group"
Web: http://www.libjpeg-turbo.org/
License type: BSD
---------------------------------------------------------