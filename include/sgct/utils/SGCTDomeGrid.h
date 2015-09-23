/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_DOME_GRID
#define _SGCT_DOME_GRID

namespace sgct_utils
{

/*!
	Helper class to render a dome grid
*/
class SGCTDomeGrid
{
public:
	SGCTDomeGrid(float radius, float FOV, unsigned int segments, unsigned int rings, unsigned int resolution = 128);
	~SGCTDomeGrid();
	void draw();

private:
	void init(float radius, float FOV, unsigned int segments, unsigned int rings, unsigned int resolution);
	// Don't implement these, should give compile warning if used
	SGCTDomeGrid();
	SGCTDomeGrid( const SGCTDomeGrid & dome );
	const SGCTDomeGrid & operator=(const SGCTDomeGrid & dome );

	void drawVBO();
	void drawVAO();

	typedef void (SGCTDomeGrid::*InternalCallbackFn)(void);
	InternalCallbackFn	mInternalDrawFn;

	void createVBO();
	void cleanup();

private:
	float * mVerts;
	unsigned int mNumberOfVertices;
	unsigned int mResolution;
	unsigned int mRings;
	unsigned int mSegments;
	unsigned int mVBO;
	unsigned int mVAO;
};

}

#endif
