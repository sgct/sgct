/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_PORTED_FUNCTIONS
#define _SGCT_PORTED_FUNCTIONS

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

namespace sgct_helpers
{

//Replacement for Visual Studio's _vscprintf function
#ifndef _vscprintf
static int vscprintf (const char * format, va_list pargs)
{
    int retval;
    va_list argcopy;
    va_copy(argcopy, pargs);
    retval = vsnprintf(NULL, 0, format, argcopy);
    va_end(argcopy);
    return retval;
}
#else
#define vscprintf(f,a) _vscprintf(f,a)
#endif
    
}

#endif
