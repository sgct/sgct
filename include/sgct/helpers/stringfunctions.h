/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__STRINGFUNCTIONS__H__
#define __SGCT__STRINGFUNCTIONS__H__

#include <sstream>
#include <string>

namespace sgct::helpers {

static void findAndReplace(std::string& src, const std::string_view& pattern,
                           const std::string& replaceStr)
{
    size_t found = src.find(pattern);
    while (found != std::string::npos) {
        src.replace(found, pattern.length(), replaceStr);
        found = src.find(pattern);
    }
}

static std::vector<std::string> split(std::string str, char delimiter) {
    std::vector<std::string> tmpVec;
    std::stringstream ss(std::move(str));
    std::string part;
    while (getline(ss, part, delimiter)) {
        tmpVec.push_back(part);
    }

    return tmpVec;
}

} // namespace sgct::helpers

#endif // __SGCT__STRINGFUNCTIONS__H__
