/*************************************************************************
Copyright (c) 2012 Miroslav Andel
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
		//#include <GL/glxew.h>
	#else  //linux
		#include <GL/glext.h>
		#include <GL/glxew.h>
	#endif
	
	#include <GL/glfw.h>
#endif

#endif
