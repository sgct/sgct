/*
	(c)2012 Miroslav Andel, Alexander Fridlund
*/

#ifndef _SGCT_OGL_HEADERS_
#define _SGCT_OGL_HEADERS_

#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif

#include <GL/glew.h>
#ifdef __WIN32__
    #include <GL/wglew.h>
#else //APPLE LINUX
    #include <OpenGL/glext.h>
    #include <GL/glxew.h>
#endif

#include <GL/glfw.h>

#endif