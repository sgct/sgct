/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__OGL_HEADERS__H__
#define __SGCT__OGL_HEADERS__H__

#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif


#ifdef SGCT_HAS_GLBINDINGS
#ifdef __APPLE__
#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#undef __gl_h_
#endif


#include <glbinding/gl33core/gl.h>
#include <glbinding/Binding.h>

#ifndef __gl_h_
#define __gl_h_
#endif // __gl_h_

// Evil 'using namespace' in the header to make the transition from GLEW to glbinding
// as easy as possible
using namespace gl;

#endif // SGCT_HAS_GLBINDINGS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN

//#ifdef _WIN32
//#define GLFW_EXPOSE_NATIVE_WIN32
//#define GLFW_EXPOSE_NATIVE_WGL
//#elif defined __linux__
//#define GLFW_EXPOSE_NATIVE_X11
//#define GLFW_EXPOSE_NATIVE_GLX
//#elif defined __APPLE__
//#define GLFW_EXPOSE_NATIVE_COCOA
//#define GLFW_EXPOSE_NATIVE_NSGL
//#endif
//
//#include <GLFW/glfw3.h>
//#include <GLFW/glfw3native.h>

#endif // __SGCT__OGL_HEADERS__H__
