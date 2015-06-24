/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_SPHERE
#define _SGCT_SPHERE

#include "../helpers/SGCTVertexData.h"

namespace sgct_utils
{

/*!
	This class creates and renders a textured sphere.
*/
class SGCTSphere
{
public:
	SGCTSphere(float radius, unsigned int segments);
	~SGCTSphere();
	void draw();

private:
	void addVertexData(unsigned int pos,
		const float &t, const float &s,
		const float &nx, const float &ny, const float &nz,
		const float &x, const float &y, const float &z);

	void drawVBO();
	void drawVAO();

	typedef void (SGCTSphere::*InternalCallbackFn)(void);
	InternalCallbackFn	mInternalDrawFn;

	void createVBO();
	void cleanUp();

private:
	// Don't implement these, should give compile warning if used
	SGCTSphere();
	SGCTSphere( const SGCTSphere & sphere );
	const SGCTSphere & operator=(const SGCTSphere & sphere );
	
	sgct_helpers::SGCTVertexData * mVerts;
	unsigned int * mIndices;

	unsigned int mNumberOfVertices;
	unsigned int mNumberOfFaces;

	enum bufferType { Vertex = 0, Index };
	unsigned int mVBO[2];
	unsigned int mVAO;
};

}

#endif
