/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/SGCTVersion.h>

namespace sgct {
/*!
    This function returns the version string of SGCT.
*/
std::string getSGCTVersion() {
    return "SGCT ver " + std::to_string(SGCT_VERSION_MAJOR) + "." +
        std::to_string(SGCT_VERSION_MINOR) + "." + std::to_string(SGCT_VERSION_REVISION);
}

} // namespace sgct
