/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/SGCTVersion.h>

namespace sgct {

std::string getSGCTVersion() {
    std::string major = std::to_string(SGCT_VERSION_MAJOR);
    std::string minor = std::to_string(SGCT_VERSION_MINOR);
    std::string patch = std::to_string(SGCT_VERSION_REVISION);
    return "SGCT ver " + major + "." + minor + "." + patch;
}

} // namespace sgct
