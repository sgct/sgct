/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _BASE_VIEWPORT_H
#define _BASE_VIEWPORT_H

#include <string>
#include "SGCTUser.h"

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
	void setUser(SGCTUser * user);
	void setUserName(std::string userName);
	
	std::string getName();
	float getX();
	float getY();
	float getXSize();
	float getYSize();

	inline SGCTUser * getUser() { return mUser; }

	bool isEnabled();
	void linkUserName();
    
protected:
	SGCTUser * mUser;
	std::string mName;
	std::string mUserName;
	bool mEnabled;
	float mX;
	float mY;
	float mXSize;
	float mYSize;
};

}

#endif
