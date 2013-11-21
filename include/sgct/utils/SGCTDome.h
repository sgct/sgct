/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_DOME
#define _SGCT_DOME

namespace sgct_utils
{

/*!
	Helper class to render a dome grid
*/
class SGCTDome
{
public:
	SGCTDome(float radius, float FOV, unsigned int segments, unsigned int rings, float tilt, unsigned int resolution = 128);
	SGCTDome(float radius, float FOV, unsigned int segments, unsigned int rings, unsigned int resolution = 128);
	~SGCTDome();
	void draw();

private:
	void init(float radius, float FOV, unsigned int segments, unsigned int rings, float tilt, unsigned int resolution);
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
