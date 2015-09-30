/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_VERTEX_DATA
#define _SGCT_VERTEX_DATA

namespace sgct_helpers
{

/*!
	This class stores a vertex which are used to generate vertex buffer objects (VBOs) in SGCT.
*/
class SGCTVertexData
{	
public:
	SGCTVertexData()
	{
		mS = 0.0f;
		mT = 0.0f;
		mNx = 0.0f;
		mNy = 0.0f;
		mNz = 0.0f;
		mX = 0.0f;
		mY = 0.0f;
		mZ = 0.0f;
	}

	SGCTVertexData(float s, float t, float nx, float ny, float nz, float x, float y, float z)
	{
		mS = s;
		mT = t;
		mNx = nx;
		mNy = ny;
		mNz = nz;
		mX = x;
		mY = y;
		mZ = z;
	}

	void set(float s, float t, float nx, float ny, float nz, float x, float y, float z)
	{
		mS = s;
		mT = t;
		mNx = nx;
		mNy = ny;
		mNz = nz;
		mX = x;
		mY = y;
		mZ = z;
	}

	void setPositionX(float x) { mX = x; }
	void setPositionY(float y) { mY = y; }
	void setPositionZ(float z) { mZ = z; }
	void setTextureS(float s) { mS = s; }
	void setTextureT(float t) { mT = t; }
	void setNormalX(float nx) { mNx = nx; }
	void setNormalY(float ny) { mNy = ny; }
	void setNormalZ(float nz) { mNz = nz; }
	
private:
	float mS, mT;	//Texcoord0 8
	float mNx, mNy, mNz; //12
	float mX, mY, mZ;	//12 = total 32 = power of two
	//ATI performs better using sizes of power of two
};
}

#endif
