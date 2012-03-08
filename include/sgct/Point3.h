#ifndef _POINT3_H_
#define _POINT3_H_

/*!
Helper struct for handling points with three elements
*/
template<class T>
struct Point3
{
	T x;
	T y;
	T z;
};

// Convinience typedefs
typedef Point3<float> Point3f;

#endif