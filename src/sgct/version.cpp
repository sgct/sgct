/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/version.h>

namespace sgct {

std::string getVersion() {
    std::string major = std::to_string(VersionMajor);
    std::string minor = std::to_string(VersionMinor);
    std::string patch = std::to_string(VersionRevision);
    return "SGCT ver " + major + "." + minor + "." + patch;
}

} // namespace sgct
