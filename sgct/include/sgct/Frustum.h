/* Frustum.h

© 2012 Miroslav Andel

*/

#ifndef _FRUSTUM
#define _FRUSTUM

namespace core_sgct
{

class Frustum
{
public:
	// Frustum mode enum
	enum Mode { Mono = 0, StereoLeftEye, StereoRightEye };

	Frustum(float left, float right, float bottom, float top, float nearClippingPlane=0.1f, float farClippingPlane=100.0f)
	{
		mLeft = left;
		mRight = right;
		mBottom = bottom;
		mTop = top;
		mNear = nearClippingPlane;
		mFar = farClippingPlane;
	}

	inline float getLeft() { return mLeft; }
	inline float getRight() { return mRight; }
	inline float getBottom() { return mBottom; }
	inline float getTop() { return mTop; }
	inline float getNear() { return mNear; }
	inline float getFar() { return mFar; }

private:
	float mLeft;
	float mRight;
	float mBottom;
	float mTop;
	float mNear;
	float mFar;
};

}

#endif