/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _CORRECTION_MESH_H_
#define _CORRECTION_MESH_H_

struct CorrectionMeshVertex
{
	float x, y;	//Vertex 8
	float s, t;	//Texcoord0 8
	unsigned char r, g, b; //color 3

	//ATI performs better using sizes of power of two
	unsigned char padding[13]; //32 - 8 - 8 - 3 = 13
};

namespace sgct_core
{

/*!
Helper class for reading and rendering a correction mesh.
A correction mesh is used for warping and edge-blending.
*/
class CorrectionMesh
{
public:
	CorrectionMesh();
	~CorrectionMesh();
	void setViewportCoords(float vpXSize, float vpYSize, float vpXPos, float vpYPos);
	bool readAndGenerateMesh(const char * meshPath);
	void render();
	inline const double * getOrthoCoords() { return &mOrthoCoords[0]; }

private:
	bool readAndGenerateScalableMesh(const char * meshPath);
	bool readAndGenerateScissMesh(const char * meshPath);
	void setupSimpleMesh();
	void createMesh();
	void cleanUp();
	void renderMesh();

	enum buffer { Vertex = 0, Index, Array };

	CorrectionMeshVertex * mVertices;
	//CorrectionMeshVertex * mVertexList;
	unsigned int * mFaces;
    double mOrthoCoords[5];
	unsigned int mResolution[2];

	unsigned int mNumberOfVertices;
	unsigned int mNumberOfFaces;
	unsigned int mMeshData[3];

	float mXSize;
	float mYSize;
	float mXOffset;
	float mYOffset;
};

} //sgct_core

#endif
