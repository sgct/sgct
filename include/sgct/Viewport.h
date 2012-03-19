/* Viewport.h

© 2012 Miroslav Andel

*/

#ifndef _VIEWPORT_H
#define _VIEWPORT_H

#include <glm/glm.hpp>
#include "Frustum.h"
#include <stddef.h> //get definition for NULL

namespace core_sgct
{

class Viewport
{
public:
	Viewport();
	Viewport(float x, float y, float xSize, float ySize);

	void set(float x, float y, float xSize, float ySize);
	void setPos(float x, float y);
	void setSize(float x, float y);
	void setEye(Frustum::FrustumMode eye);
	void calculateFrustum(int frustumMode, glm::vec3 * userPos, float near, float far);

	inline float getX() { return mX; }
	inline float getY() { return mY; }
	inline float getXSize() { return mXSize; }
	inline float getYSize() { return mYSize; }
	inline Frustum::FrustumMode getEye() { return mEye; }
	inline Frustum * getFrustum(int frustumMode) { return &mFrustums[frustumMode]; }
	inline Frustum * getFrustum() { return &mFrustums[mEye]; }

	glm::vec3 viewPlaneCoords[3];

	enum corners { LowerLeft = 0, UpperLeft, UpperRight };

private:
	float mX;
	float mY;
	float mXSize;
	float mYSize;
	Frustum mFrustums[3];
	Frustum::FrustumMode mEye;
};

}

#endif