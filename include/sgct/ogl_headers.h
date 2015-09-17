/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SGCT_OGL_HEADERS_
#define _SGCT_OGL_HEADERS_

#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef SGCT_OGL_HEADERS
	#define SGCT_OGL_HEADERS
	#include <GL/glew.h>

    //should be defined by compiler
	#if  defined (linux)  || defined (__linux)
        #ifndef __LINUX__
        #define __LINUX__
        #endif
    #endif

    //should be defined by compiler
    #if defined(_WIN64) || defined(_WIN32)
        #ifndef __WIN32__
        #define __WIN32__
        #endif
    #endif

    // __APPLE__ should be defined

	#ifdef __WIN32__
        #ifndef SGCT_WINDOWS_INCLUDE
            #define SGCT_WINDOWS_INCLUDE
            #include <windows.h> //must be declared before glfw
        #endif
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

    //#define GLFW_INCLUDE_GLU

	#ifdef __WIN32__
		#define GLFW_EXPOSE_NATIVE_WIN32
		#define GLFW_EXPOSE_NATIVE_WGL
	#elif defined __LINUX__
		#define GLFW_EXPOSE_NATIVE_X11
		#define GLFW_EXPOSE_NATIVE_GLX
	#elif defined __APPLE__
		#define GLFW_EXPOSE_NATIVE_COCOA
		#define GLFW_EXPOSE_NATIVE_NSGL
	#endif

	#include <GL/glfw3.h>
	#include <GL/glfw3native.h>
#endif

#endif
