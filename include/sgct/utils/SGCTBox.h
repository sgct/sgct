/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_BOX
#define _SGCT_BOX

#include "../helpers/SGCTVertexData.h"

namespace sgct_utils
{
class SGCTBox
{
public:
	enum TextureMappingMode { Regular = 0, CubeMap, SkyBox };

	SGCTBox(float size, TextureMappingMode tmm = Regular);
	~SGCTBox();
	void draw();

private:
	// Don't implement these, should give compile warning if used
	SGCTBox();
	SGCTBox( const SGCTBox & box );
	const SGCTBox & operator=(const SGCTBox & box );

	void cleanUp();
	void createVBO();

private:	
	unsigned int mVBO;
	sgct_helpers::SGCTVertexData * mVerts;
};
}

#endif
