#include "../include/sgct/Viewport.h"

core_sgct::Viewport::Viewport()
{
	mX = 0.0f;
	mY = 0.0f;
	mXSize = 1.0f;
	mYSize = 1.0f;
	mEye = Frustum::Mono;
}

core_sgct::Viewport::Viewport(float x, float y, float xSize, float ySize)
{
	mX = x;
	mY = y;
	mXSize = xSize;
	mYSize = ySize;
	mEye = Frustum::Mono;
}

void core_sgct::Viewport::set(float x, float y, float xSize, float ySize)
{
	mX = x;
	mY = y;
	mXSize = xSize;
	mYSize = ySize;
	mEye = Frustum::Mono;
}

void core_sgct::Viewport::setPos(float x, float y)
{
	mX = x;
	mY = y;
}

void core_sgct::Viewport::setSize(float x, float y)
{
	mXSize = x;
	mYSize = y;
}

void core_sgct::Viewport::setEye(core_sgct::Frustum::FrustumMode eye)
{
	mEye = eye;
}

void core_sgct::Viewport::calculateFrustum(int frustumMode, float x, float y, float z, float near, float far)
{
	//nearFactor = near clipping plane / focus plane dist
	float nearFactor = near / (viewPlaneCoords[ LowerLeft ].z - z);
	if( nearFactor < 0 )
		nearFactor = -nearFactor;

	mFrustums[frustumMode].set(
		(viewPlaneCoords[ LowerLeft ].x - x)*nearFactor,
		(viewPlaneCoords[ UpperRight ].x - x)*nearFactor,
		(viewPlaneCoords[ LowerLeft ].y - y)*nearFactor,
		(viewPlaneCoords[ UpperRight ].y - y)*nearFactor,
		near,
		far);
}