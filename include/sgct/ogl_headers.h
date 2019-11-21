/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__OGL_HEADERS__H__
#define __SGCT__OGL_HEADERS__H__

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

#endif // __SGCT__OGL_HEADERS__H__
