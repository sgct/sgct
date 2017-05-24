/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_PORTED_FUNCTIONS
#define _SGCT_PORTED_FUNCTIONS

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#if defined(_WIN32)
//Replacement for Visual Studio's _vscprintf function
#if (_MSC_VER < 1400) //if older than visual studio 2005
static int vscprintf (const char * format, va_list pargs)
{
    int retval;
    va_list argcopy;
    va_copy(argcopy, pargs);
    retval = vsnprintf(NULL, 0, format, argcopy);
    va_end(argcopy);
    return retval;
}

static int vswcprintf(const wchar_t * format, va_list pargs)
{
	int retval;
	va_list argcopy;
	va_copy(argcopy, pargs);
	retval = vsnwprintf(NULL, 0, format, argcopy);
	va_end(argcopy);
	return retval;
}
#else
#define vscprintf(f,a) _vscprintf(f,a)
#define vscwprintf(f,a) _vscwprintf(f,a)
#endif
#else
  //Workaround for calling vscprintf() or vscwprintf() in a non-windows OS
  int vscprintf (const char * format, va_list pargs)
  {
      int retval;
      va_list argcopy;
      va_copy(argcopy, pargs);
      retval = vsnprintf(NULL, 0, format, argcopy);
      va_end(argcopy);
      return retval;
  }

  int vscwprintf(const wchar_t *format, va_list argptr)
  {
     return(vswprintf(0, 0, format, argptr));
  }
#endif

#endif
