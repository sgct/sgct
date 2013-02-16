/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_VERSION_H_
#define _SGCT_VERSION_H_

#define SGCT_VERSION_MAJOR    1
#define SGCT_VERSION_MINOR    2
#define SGCT_VERSION_REVISION 0

#include <stdio.h>
#include <string>

namespace sgct
{
	std::string getSGCTVersion();
}

#endif
