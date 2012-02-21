/* Viewport.h

© 2012 Miroslav Andel

*/

#ifndef _VIEWPORT_H
#define _VIEWPORT_H

namespace core_sgct
{

class Viewport
{
public:
	Viewport(float x, float y, float xSize, float ySize)
	{
		mX = x;
		mY = y;
		mXSize = xSize;
		mYSize = ySize;
	}

	inline float getX() { return mX; }
	inline float getY() { return mY; }
	inline float getXSize() { return mXSize; }
	inline float getYSize() { return mYSize; }

private:
	float mX;
	float mY;
	float mXSize;
	float mYSize;
};

}

#endif