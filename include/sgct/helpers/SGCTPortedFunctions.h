/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__PORTED_FUNCTIONS__H__
#define __SGCT__PORTED_FUNCTIONS__H__

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef _WIN32
//Replacement for Visual Studio's _vscprintf function
#if (_MSC_VER < 1400) //if older than visual studio 2005
static int vscprintf(const char* format, va_list pargs) {
    int retval;
    va_list argcopy;
    va_copy(argcopy, pargs);
    retval = vsnprintf(nullptr, 0, format, argcopy);
    va_end(argcopy);
    return retval;
}

static int vscwprintf(const wchar_t* format, va_list pargs) {
    // Taken from:
    // http://www.stackoverflow.com/questions/16351523/vscwprintf-on-mac-os-x-linux
    //
    // Unlike vsnprintf(), vswprintf() does not tell you how many characters would have
    // been written if there was space enough in the buffer - it just reports an error
    // whjen there is not enough space. ssume a moderatly large machine, so kilobytes of
    // wchar_t on the stack is not a problem
    int bufSize = 1024;
    while (bufSize < 1024 * 1024) {
        va_list args;
        va_copy(args, pargs);
        wchar_t buffer[bufSize];
        int formatSize = vswprintf(
            buffer,
            sizeof(buffer) / sizeof(buffer[0]),
            format,
            args
        );
        if (formatSize >= 0) {
            return formatSize;
        }
        bufSize *= 2;
    }
    
    return -1;
}
#else // (_MSC_VER < 1400)
#define vscprintf(f,a) _vscprintf(f,a)
#define vscwprintf(f,a) _vscwprintf(f,a)
#endif // (_MSC_VER < 1400)
#else // _WIN32
  // Workaround for calling vscprintf() or vscwprintf() in a non-windows OS
  static int vscprintf(const char* format, va_list pargs) {
      int retval;
      va_list argcopy;
      va_copy(argcopy, pargs);
      retval = vsnprintf(nullptr, 0, format, argcopy);
      va_end(argcopy);
      return retval;
  }

  static int vscwprintf(const wchar_t* format, va_list argptr) {
     return(vswprintf(0, 0, format, argptr));
  }
#endif // _WIN32

#endif // __SGCT__PORTED_FUNCTIONS__H__
