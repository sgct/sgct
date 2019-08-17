/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__STRING_FUNCTIONS__H__
#define __SGCT__STRING_FUNCTIONS__H__

#include <string>

namespace sgct_helpers {

static void findAndReplace(std::string& src, const std::string& pattern,
                           const std::string& replaceStr)
{
    size_t found = src.find(pattern);
    while (found != std::string::npos) {
        src.replace(found, pattern.length(), replaceStr);
        found = src.find(pattern);
    }
}

} // sgct_helpers

#endif // __SGCT__STRING_FUNCTIONS__H__
