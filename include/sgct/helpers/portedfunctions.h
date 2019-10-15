/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__PORTED_FUNCTIONS__H__
#define __SGCT__PORTED_FUNCTIONS__H__

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef WIN32
// Replacement for Visual Studio's _vscprintf function
#define vscprintf(f,a) _vscprintf(f,a)
#define vscwprintf(f,a) _vscwprintf(f,a)
#else // _WIN32
  // Workaround for calling vscprintf() or vscwprintf() in a non-windows OS
  static int vscprintf(const char* format, va_list pargs) {
      va_list argcopy;
      va_copy(argcopy, pargs);
      int retval = vsnprintf(nullptr, 0, format, argcopy);
      va_end(argcopy);
      return retval;
  }

  static int vscwprintf(const wchar_t* format, va_list argptr) {
     return(vswprintf(0, 0, format, argptr));
  }
#endif // _WIN32

#endif // __SGCT__PORTED_FUNCTIONS__H__
