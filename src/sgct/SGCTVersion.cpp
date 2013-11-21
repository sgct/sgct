/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/SGCTVersion.h"

/*!
	This function returns the version string of SGCT.
*/
std::string sgct::getSGCTVersion()
{
    char buffer[64];
#if (_MSC_VER >= 1400) //visual studio 2005 or later
    sprintf_s(buffer, sizeof(buffer), "SGCT ver %d.%d.%d", SGCT_VERSION_MAJOR, SGCT_VERSION_MINOR, SGCT_VERSION_REVISION);
#else
    sprintf(buffer, "SGCT ver %d.%d.%d", SGCT_VERSION_MAJOR, SGCT_VERSION_MINOR, SGCT_VERSION_REVISION);
#endif
    return std::string(buffer);
}
