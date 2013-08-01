/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SGCT_OGL_HEADERS_
#define _SGCT_OGL_HEADERS_

#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif

#ifndef SGCT_OGL_HEADERS
	#define SGCT_OGL_HEADERS
	#include <GL/glew.h>
	#ifdef __WIN32__
		#include <GL/wglew.h>
	#elif defined __APPLE__
		#include <OpenGL/glext.h>
		/*
            glxew depends on X11 libs
            X11 is not included with Mountain Lion, but X11 server and client libraries for OS X Mountain Lion
            are available from the XQuartz project: http://xquartz.macosforge.org. You should use XQuartz version 2.7.2 or later.
        */
		//#include <GL/glxew.h>
	#else  //linux
		#include <GL/glext.h>
		#include <GL/glxew.h>
	#endif

	#include <GL/glfw3.h>
#endif

#endif
