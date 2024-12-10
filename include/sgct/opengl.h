/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__OPENGL_H__
#define __SGCT__OPENGL_H__

#ifdef __APPLE__
#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#undef __gl_h_
#endif

// Workaround for APIENTRY macro redefinition
// Problem: glad.h will define APIENTRY if it is not defined. But if windows.h is included
// after glad.h it will also unconditionally set APIENTRY and we get a macro redefinition
// warning.
// Solution: We manually define APIENTRY before including glad.h and keep track of if we
// did to be able to undefine it if we did define it.
#if defined(_WIN32)
#if !defined(APIENTRY) && !defined(__CYGWIN__) && !defined(__SCITECH_SNAP__)
#define APIENTRY __stdcall
#define SCGT_GLAD_APIENTRY_DEFINED
#endif
#endif

#include <glad/glad.h>

#ifdef SCGT_GLAD_APIENTRY_DEFINED
#undef APIENTRY
#undef SCGT_GLAD_APIENTRY_DEFINED
#endif


#endif // __SGCT__OPENGL_H__
