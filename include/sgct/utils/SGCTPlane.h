/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_PLANE
#define _SGCT_PLANE

#include "../helpers/SGCTVertexData.h"

/*! \namespace sgct_utils
\brief SGCT utils namespace contains basic utilities for geometry rendering
*/
namespace sgct_utils
{

/*!
	This class creates and renders a textured box.
*/
class SGCTPlane
{
public:
	SGCTPlane(float width, float height);
	~SGCTPlane();
	void draw();

private:
	// Don't implement these, should give compile warning if used
	SGCTPlane();
	SGCTPlane(const SGCTPlane & box);
	const SGCTPlane & operator=(const SGCTPlane & box);

	void drawVBO();
	void drawVAO();

	typedef void (SGCTPlane::*InternalCallbackFn)(void);
	InternalCallbackFn	mInternalDrawFn;

	void cleanUp();
	void createVBO();

private:	
	unsigned int mVBO;
	unsigned int mVAO;
	sgct_helpers::SGCTVertexData * mVerts;
};
}

#endif
