#ifndef _USER_H_
#define _USER_H_

#include "Point3.h"

namespace sgct
{

/*!
Helper class for setting user variables
*/
struct User
{
	Point3f LeftEyePos;
	Point3f RightEyePos;
};

} // sgct

#endif