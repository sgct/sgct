/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _BASE_VIEWPORT_H
#define _BASE_VIEWPORT_H

#include <string>

namespace sgct_core
{

/*!
	This class holds and manages viewportdata and calculates frustums
*/
class BaseViewport
{
public:
	void setName(const std::string & name);
	void setPos(float x, float y);
	void setSize(float x, float y);
	void setEnabled(bool state);
	
	std::string getName();
	float getX();
	float getY();
	float getXSize();
	float getYSize();

	bool isEnabled();
    
protected:
	std::string mName;
	bool mEnabled;
	float mX;
	float mY;
	float mXSize;
	float mYSize;
};

}

#endif
