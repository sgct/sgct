/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__VERSION__H__
#define __SGCT__VERSION__H__

#include <string>

namespace sgct {
    constexpr const int VersionMajor = 3;
    constexpr const int VersionMinor = 0;
    constexpr const int VersionRevision = 0;

    std::string getVersion();
}

#endif // __SGCT__VERSION__H__
