/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__PORTEDFUNCTIONS__H__
#define __SGCT__PORTEDFUNCTIONS__H__

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#ifdef WIN32
#define vscprintf(f,a) _vscprintf(f,a)
#else // WIN32
  // Workaround for calling vscprintf() or vscwprintf() in a non-windows OS
  static int vscprintf(const char* format, va_list pargs) {
      va_list argcopy;
      va_copy(argcopy, pargs);
      int retval = vsnprintf(nullptr, 0, format, argcopy);
      va_end(argcopy);
      return retval;
  }
#endif // WIN32

#endif // __SGCT__PORTEDFUNCTIONS__H__
