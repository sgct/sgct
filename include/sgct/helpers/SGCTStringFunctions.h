/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_STRING_FUNCTIONS
#define _SGCT_STRING_FUNCTIONS

#include <string>

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
}

#endif
