/*************************************************************************
Copyright (c) 2017 Erik Sundén
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_OPENVR_H_
#define _SGCT_OPENVR_H_
#ifdef OPENVR_ENABLED

#ifndef SGCT_DONT_USE_EXTERNAL
#include <external/tinyxml2.h>
#else
#include <tinyxml2.h>
#endif

namespace sgct
{
    
class SGCTWindow;

/*!
Class for using OpenVR in SGCT in an easy manor
*/
class SGCTOpenVR
{
public:
	SGCTOpenVR();

    static void configure(tinyxml2::XMLElement* element, SGCTWindow* window); // static member function
};

} // sgct_core

#endif
#endif
