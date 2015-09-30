/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_DOME
#define _SGCT_DOME

#include "../helpers/SGCTVertexData.h"
#include <vector>

namespace sgct_utils
{

/*!
	Helper class to render a dome grid
*/
class SGCTDome
{
public:
	SGCTDome(float radius, float FOV, unsigned int azimuthSteps, unsigned int elevationSteps);
	~SGCTDome();
	void draw();

private:
	void init(float radius, float FOV, unsigned int azimuthSteps, unsigned int elevationSteps);
	// Don't implement these, should give compile warning if used
	SGCTDome();
	SGCTDome( const SGCTDome & dome );
	const SGCTDome & operator=(const SGCTDome & dome );

	void drawVBO();
	void drawVAO();

	typedef void (SGCTDome::*InternalCallbackFn)(void);
	InternalCallbackFn	mInternalDrawFn;

	void createVBO();
	void cleanup();

private:
	std::vector<sgct_helpers::SGCTVertexData> mVerts;
	std::vector<unsigned int> mIndices;
	int mElevationSteps;
	int mAzimuthSteps;

	enum bufferType { Vertex = 0, Index };
	unsigned int mVBO[2];
	unsigned int mVAO;
};

}

#endif
