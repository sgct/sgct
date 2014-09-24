/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _FISHEYE_VIEWPORT_H
#define _FISHEYE_VIEWPORT_H

#include "Viewport.h"

namespace sgct_core
{

/*!
	This class holds and manages viewportdata and calculates frustums
*/
class FisheyeViewport : public BaseViewport
{
public:
    
private:
	sgct_core::Viewport mViewports[6];
};

}

#endif
