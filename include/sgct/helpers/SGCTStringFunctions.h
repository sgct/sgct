/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__STRING_FUNCTIONS__H__
#define __SGCT__STRING_FUNCTIONS__H__

#include <sstream>
#include <string>
#include <vector>

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

static std::vector<std::string> split(std::string str, char delimiter) {
    std::vector<std::string> tmpVec;
    std::stringstream ss(std::move(str));
    std::string part;

    while (getline(ss, part, delimiter)) {
        tmpVec.push_back(part);
    }

    return tmpVec;
}

static std::vector<std::wstring> split(std::wstring str, wchar_t delimiter) {
    std::vector<std::wstring> tmpVec;
    std::wstringstream ss(std::move(str));
    std::wstring part;

    while (getline(ss, part, delimiter)) {
        tmpVec.push_back(part);
    }

    return tmpVec;
}

static std::vector<std::wstring> split(std::string str, wchar_t delimiter) {
    std::vector<std::wstring> tmpVec;
    std::wstring ws;
    ws.assign(str.begin(), str.end());

    std::wstringstream ss(ws);
    std::wstring part;

    while (getline(ss, part, delimiter)) {
        tmpVec.push_back(part);
    }

    return tmpVec;
}

static std::wstring makeWideString(const std::string& str) {
    std::wstring ws;
    ws.assign(str.begin(), str.end());
    return ws;
}

} // sgct_helpers

#endif // __SGCT__STRING_FUNCTIONS__H__
