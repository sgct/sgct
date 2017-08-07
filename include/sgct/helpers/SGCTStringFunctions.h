/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_STRING_FUNCTIONS
#define _SGCT_STRING_FUNCTIONS

#include <string>
#include <vector>
#include <sstream>

namespace sgct_helpers
{

/*!
    This function finds a pattern and replaces it.
*/
static void findAndReplace(std::string & src, std::string pattern, std::string replaceStr)
{
    while (true)
    {
        std::size_t found = src.find(pattern);
        if (found != std::string::npos)
            src.replace(found, pattern.length(), replaceStr);
        else
            break;
    }
}

static std::vector<std::string> split(std::string str, char delimiter)
{
	std::vector<std::string> tmpVec;
	std::stringstream ss(str);
	std::string part;

	while (getline(ss, part, delimiter)) {
		tmpVec.push_back(part);
	}

	return tmpVec;
}

static std::vector<std::wstring> split(std::wstring str, wchar_t delimiter)
{
	std::vector<std::wstring> tmpVec;
	std::wstringstream ss(str);
	std::wstring part;

	while (getline(ss, part, delimiter)) {
		tmpVec.push_back(part);
	}

	return tmpVec;
}

static std::vector<std::wstring> split(std::string str, wchar_t delimiter)
{
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

static std::wstring makeWideString(const std::string & str)
{
	std::wstring ws;
	ws.assign(str.begin(), str.end());
	return ws;
}

}

#endif
