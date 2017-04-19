/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sstream>
#include <sgct/SGCTVersion.h>

/*!
    This function returns the version string of SGCT.
*/
std::string sgct::getSGCTVersion()
{
    std::stringstream ss;
    ss << "SGCT ver " << SGCT_VERSION_MAJOR << "." << SGCT_VERSION_MINOR << "." << SGCT_VERSION_REVISION;

    return ss.str();
}
