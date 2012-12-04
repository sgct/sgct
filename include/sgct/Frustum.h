/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _FRUSTUM
#define _FRUSTUM

namespace sgct_core
{

class Frustum
{
public:
	// Frustum mode enum
	enum FrustumMode { Mono = 0, StereoLeftEye, StereoRightEye };

	Frustum()
	{
		mLeft = -1.0f;
		mRight = 1.0f;
		mBottom = -1.0f;
		mTop = 1.0f;
		mNear = 0.1f;
		mFar = 100.0f;
	}

	Frustum(float left, float right, float bottom, float top, float nearClippingPlane=0.1f, float farClippingPlane=100.0f)
	{
		mLeft = left;
		mRight = right;
		mBottom = bottom;
		mTop = top;
		mNear = nearClippingPlane;
		mFar = farClippingPlane;
	}

	void set(float left, float right, float bottom, float top, float nearClippingPlane=0.1f, float farClippingPlane=100.0f)
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
