/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#define MAX_LINE_LENGTH 256

#include <stdio.h>
#include <fstream>
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/CorrectionMesh.h"
#include "../include/sgct/ClusterManager.h"
#include "../include/sgct/Engine.h"
#include "../include/sgct/Viewport.h"
#include <cstring>

struct SCISSTexturedVertex
{
	float x, y, z;
	float tx, ty, tz;
	SCISSTexturedVertex()
	{
		x = y = z = tx = ty = tz = 0.0f;
	};
};

struct SCISSViewData
{
	float qx, qy, qz, qw; // Rotation quaternion
	float x, y, z;		  // Position of view (currently unused in Uniview)
	float fovUp, fovDown, fovLeft, fovRight;

	SCISSViewData()
	{
		qx = qy = qz = x = y = z = 0.0f;
		qw = 1.0f;
		fovUp = fovDown = fovLeft = fovRight = 20.0f;
	};
};

enum SCISSDistortionType { MESHTYPE_PLANAR, MESHTYPE_CUBE };

sgct_core::CorrectionMeshGeometry::CorrectionMeshGeometry()
{
	mMeshData[0] = GL_FALSE;
	mMeshData[1] = GL_FALSE;
	mMeshData[2] = GL_FALSE;

	mGeometryType = GL_TRIANGLE_STRIP;
	mNumberOfVertices = 0;
	mNumberOfIndices = 0;
}

sgct_core::CorrectionMeshGeometry::~CorrectionMeshGeometry()
{
	if (ClusterManager::instance()->getMeshImplementation() == ClusterManager::DISPLAY_LIST)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMeshGeometry: Releasing correction mesh OpenGL data...\n");

		if (mMeshData[0] != GL_FALSE)
			glDeleteLists(mMeshData[0], 1);
	}
	else
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMeshGeometry: Releasing correction mesh OpenGL data...\n");

		if (mMeshData[2] != GL_FALSE)
			glDeleteVertexArrays(1, &mMeshData[2]);

		//delete VBO and IBO
		if (mMeshData[0] != GL_FALSE)
			glDeleteBuffers(2, &mMeshData[0]);
	}
}

sgct_core::CorrectionMesh::CorrectionMesh()
{
	mTempVertices = NULL;
	mTempIndices = NULL;

	mXSize = 1.0f;
	mYSize = 1.0f;
	mXOffset = 0.0f;
	mYOffset = 0.0f;

	mOrthoCoords[0] = 0.0;
	mOrthoCoords[1] = 1.0;
	mOrthoCoords[2] = 0.0;
	mOrthoCoords[3] = 1.0;
}

sgct_core::CorrectionMesh::~CorrectionMesh()
{	
	
}

void sgct_core::CorrectionMesh::setViewportCoords(float vpXSize, float vpYSize, float vpXPos, float vpYPos)
{
	mXSize = vpXSize;
	mYSize = vpYSize;
	mXOffset = vpXPos;
	mYOffset = vpYPos;
}

bool sgct_core::CorrectionMesh::readAndGenerateMesh(const char * meshPath, sgct_core::Viewport * parent)
{
	//generate unwarped mesh for mask
	setupMaskMesh();
	createMesh(&mGeometries[UnWarped]);
	cleanUp();
	
	int length = 0;
	if (meshPath != NULL)
	{
		while (meshPath[length] != '\0')
			length++;
	}

	if (length == 0)
	{
		setupSimpleMesh();
		createMesh(&mGeometries[Warped]);
		cleanUp();
		return false;
	}

	if (length > 4 && (strcmp(".sgc", &meshPath[length - 4]) == 0 || strcmp(".SGC", &meshPath[length - 4]) == 0))
	{
		if (!readAndGenerateScissMesh(meshPath, parent))
		{
			setupSimpleMesh();
			createMesh(&mGeometries[Warped]);
			cleanUp();
			return false;
		}
	}
	else if (length > 3 && (strcmp(".ol", &meshPath[length - 3]) == 0 || strcmp(".OL", &meshPath[length - 3]) == 0))
	{
		if( !readAndGenerateScalableMesh(meshPath, parent))
		{
			setupSimpleMesh();
			createMesh(&mGeometries[Warped]);
			cleanUp();
			return false;
		}
	}
	else
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh error: Loading failed (bad filename: %s)\n", meshPath);
		setupSimpleMesh();
		createMesh(&mGeometries[Warped]);
		cleanUp();
		return false;
	}

	return true;
}

bool sgct_core::CorrectionMesh::readAndGenerateScalableMesh(const char * meshPath, sgct_core::Viewport * parent)
{
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO,
		"CorrectionMesh: Reading scalable mesh data from '%s'.\n", meshPath);

	FILE * meshFile = NULL;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if( fopen_s(&meshFile, meshPath, "r") != 0 || !meshFile )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Failed to open warping mesh file!\n");
		return false;
	}
#else
	meshFile = fopen(meshPath, "r");
	if( meshFile == NULL )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Failed to open warping mesh file!\n");
		return false;
	}
#endif

	float x,y,s,t;
	unsigned int intensity;
	unsigned int a,b,c;
	unsigned int numOfVerticesRead = 0;
	unsigned int numOfFacesRead = 0;
	unsigned int numberOfFaces = 0;
	unsigned int numberOfVertices = 0;
	unsigned int numberOfIndices = 0;

	char lineBuffer[MAX_LINE_LENGTH];
	while( !feof( meshFile ) )
	{
		if( fgets(lineBuffer, MAX_LINE_LENGTH, meshFile ) != NULL )
		{
#if (_MSC_VER >= 1400) //visual studio 2005 or later
			if( sscanf_s(lineBuffer, "%f %f %u %f %f", &x, &y, &intensity, &s, &t) == 5 )
#else
			if( sscanf(lineBuffer, "%f %f %u %f %f", &x, &y, &intensity, &s, &t) == 5 )
#endif
			{
				if( mTempVertices != NULL && mResolution[0] != 0 && mResolution[1] != 0 )
				{
					mTempVertices[numOfVerticesRead].x = (x / static_cast<float>(mResolution[0])) * mXSize + mXOffset;
					mTempVertices[numOfVerticesRead].y = (y / static_cast<float>(mResolution[1])) * mYSize + mYOffset;
					mTempVertices[numOfVerticesRead].r = static_cast<unsigned char>(intensity);
					mTempVertices[numOfVerticesRead].g = static_cast<unsigned char>(intensity);
					mTempVertices[numOfVerticesRead].b = static_cast<unsigned char>(intensity);
					mTempVertices[numOfVerticesRead].s = (1.0f - t) * mXSize + mXOffset;
					mTempVertices[numOfVerticesRead].t = (1.0f - s) * mYSize + mYOffset;

					numOfVerticesRead++;
				}
			}
#if (_MSC_VER >= 1400) //visual studio 2005 or later
			else if( sscanf_s(lineBuffer, "[ %u %u %u ]", &a, &b, &c) == 3 )
#else
			else if( sscanf(lineBuffer, "[ %u %u %u ]", &a, &b, &c) == 3 )
#endif
			{
				if (mTempIndices != NULL)
				{
					mTempIndices[numOfFacesRead * 3] = a;
					mTempIndices[numOfFacesRead * 3 + 1] = b;
					mTempIndices[numOfFacesRead * 3 + 2] = c;
				}

				numOfFacesRead++;
			}
			else
			{
				char tmpString[16];
				tmpString[0] = '\0';
				double tmpD = 0.0;
				unsigned int tmpUI = 0;

#if (_MSC_VER >= 1400) //visual studio 2005 or later
				if( sscanf_s(lineBuffer, "VERTICES %u", &numberOfVertices) == 1 )
#else
				if( sscanf(lineBuffer, "VERTICES %u", &numberOfVertices) == 1 )
#endif
				{
					mTempVertices = new CorrectionMeshVertex[ numberOfVertices ];
					memset(mTempVertices, 0, numberOfVertices * sizeof(CorrectionMeshVertex));
				}

#if (_MSC_VER >= 1400) //visual studio 2005 or later
				else if( sscanf_s(lineBuffer, "FACES %u", &numberOfFaces) == 1 )
#else
				else if (sscanf(lineBuffer, "FACES %u", &numberOfFaces) == 1)
#endif
				{
					numberOfIndices = numberOfFaces * 3;
					mTempIndices = new unsigned int[numberOfIndices];
					memset(mTempIndices, 0, numberOfIndices * sizeof(unsigned int));
				}

#if (_MSC_VER >= 1400) //visual studio 2005 or later
				else if( sscanf_s(lineBuffer, "ORTHO_%s %lf", tmpString, 16, &tmpD) == 2 )
#else
				else if( sscanf(lineBuffer, "ORTHO_%s %lf", tmpString, &tmpD) == 2 )
#endif
				{
					if( strcmp(tmpString, "LEFT") == 0 )
						mOrthoCoords[0] = tmpD;
					else if( strcmp(tmpString, "RIGHT") == 0 )
						mOrthoCoords[1] = tmpD;
					else if( strcmp(tmpString, "BOTTOM") == 0 )
						mOrthoCoords[2] = tmpD;
					else if( strcmp(tmpString, "TOP") == 0 )
						mOrthoCoords[3] = tmpD;
				}

#if (_MSC_VER >= 1400) //visual studio 2005 or later
				else if( sscanf_s(lineBuffer, "NATIVEXRES %u", &tmpUI) == 1 )
#else
				else if( sscanf(lineBuffer, "NATIVEXRES %u", &tmpUI) == 1 )
#endif
					mResolution[0] = tmpUI;

#if (_MSC_VER >= 1400) //visual studio 2005 or later
				else if( sscanf_s(lineBuffer, "NATIVEYRES %u", &tmpUI) == 1 )
#else
				else if( sscanf(lineBuffer, "NATIVEYRES %u", &tmpUI) == 1 )
#endif
					mResolution[1] = tmpUI;
			}

			//fprintf(stderr, "Row text: %s", lineBuffer);
		}

	}

	if (numberOfVertices != numOfVerticesRead || numberOfFaces != numOfFacesRead)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Incorrect mesh data geometry!");
		return false;
	}

	fclose( meshFile );

	mGeometries[Warped].mNumberOfVertices = numberOfVertices;
	mGeometries[Warped].mNumberOfIndices = numberOfIndices;
	mGeometries[Warped].mGeometryType = GL_TRIANGLES;

	createMesh(&mGeometries[Warped]);

	cleanUp();

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: Correction mesh read successfully! Vertices=%u, Faces=%u.\n", numOfVerticesRead, numOfFacesRead);

	return true;
}

bool sgct_core::CorrectionMesh::readAndGenerateScissMesh(const char * meshPath, sgct_core::Viewport * parent)
{
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO,
		"CorrectionMesh: Reading sciss mesh data from '%s'.\n", meshPath);

	FILE * meshFile = NULL;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if (fopen_s(&meshFile, meshPath, "rb") != 0 || !meshFile)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Failed to open warping mesh file!\n");
		return false;
	}
#else
	meshFile = fopen(meshPath, "rb");
	if (meshFile == NULL)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Failed to open warping mesh file!\n");
		return false;
	}
#endif

	size_t retval;
	unsigned int numberOfVertices = 0;
	unsigned int numberOfIndices = 0;

	char fileID[3];
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	retval = fread_s(fileID, sizeof(char)*3, sizeof(char), 3, meshFile);
#else
	retval = fread(fileID, sizeof(char), 3, meshFile);
#endif

	//check fileID
	if (fileID[0] != 'S' || fileID[1] != 'G' || fileID[2] != 'C' || retval != 3)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Incorrect file id!\n");
		fclose(meshFile);
		return false;
	}

	//read file version
	unsigned char fileVersion;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	retval = fread_s(&fileVersion, sizeof(unsigned char), sizeof(unsigned char), 1, meshFile);
#else
	retval = fread(&fileVersion, sizeof(unsigned char), 1, meshFile);
#endif
	if (retval != 1)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Error parsing file!\n");
		fclose(meshFile);
		return false;
	}
	else
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: file version %u\n", fileVersion);

	//read mapping type
	unsigned int mappingType;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	retval = fread_s(&mappingType, sizeof(unsigned int), sizeof(unsigned int), 1, meshFile);
#else
	retval = fread(&mappingType, sizeof(unsigned int), 1, meshFile);
#endif
	if (retval != 1)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Error parsing file!\n");
		fclose(meshFile);
		return false;
	}
	else
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: Mapping type = %s (%u)\n", mappingType == 0 ? "planar" : "cube", mappingType);

	//read viewdata
	SCISSViewData viewData;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	retval = fread_s(&viewData, sizeof(SCISSViewData), sizeof(SCISSViewData), 1, meshFile);
#else
	retval = fread(&viewData, sizeof(SCISSViewData), 1, meshFile);
#endif
	if (retval != 1)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Error parsing file!\n");
		fclose(meshFile);
		return false;
	}
	else
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: Rotation quat = [%f %f %f %f]\n",
			viewData.qx, viewData.qy, viewData.qz, viewData.qw);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: Position = [%f %f %f]\n",
			viewData.x, viewData.y, viewData.z);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: FOV up = %f\n",
			viewData.fovUp);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: FOV down = %f\n",
			viewData.fovDown);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: FOV left = %f\n",
			viewData.fovLeft);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: FOV right = %f\n",
			viewData.fovRight);
	}

	//read number of vertices
	unsigned int size[2];
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	retval = fread_s(size, sizeof(unsigned int)*2, sizeof(unsigned int), 2, meshFile);
#else
	retval = fread(size, sizeof(unsigned int), 2, meshFile);
#endif
	if (retval != 2)
	{ 
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Error parsing file!\n");
		fclose(meshFile);
		return false;
	}
	else
	{
		numberOfVertices = size[0]*size[1];
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: Number of vertices = %u (%ux%u)\n", numberOfVertices, size[0], size[1]);
	}
	//read vertices
	SCISSTexturedVertex * texturedVertexList = new SCISSTexturedVertex[numberOfVertices];
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	retval = fread_s(texturedVertexList, sizeof(SCISSTexturedVertex)* numberOfVertices, sizeof(SCISSTexturedVertex), numberOfVertices, meshFile);
#else
	retval = fread(texturedVertexList, sizeof(SCISSTexturedVertex), mNumberOfVertices, meshFile);
#endif
	if (retval != numberOfVertices)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Error parsing file!\n");
		fclose(meshFile);
		return false;
	}

	//read number of indices
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	retval = fread_s(&numberOfIndices, sizeof(unsigned int), sizeof(unsigned int), 1, meshFile);
#else
	retval = fread(&mNumberOfFaces, sizeof(unsigned int), 1, meshFile);
#endif

	if (retval != 1)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Error parsing file!\n");
		fclose(meshFile);
		return false;
	}
	else
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: Number of indices = %u\n", numberOfIndices);

	//read faces
	if (numberOfIndices > 0)
	{
		mTempIndices = new unsigned int[numberOfIndices];
#if (_MSC_VER >= 1400) //visual studio 2005 or later
		retval = fread_s(mTempIndices, sizeof(unsigned int)* numberOfIndices, sizeof(unsigned int), numberOfIndices, meshFile);
#else
		retval = fread(texturedVertexList, sizeof(unsigned int), mNumberOfIndices, meshFile);
#endif
		if (retval != numberOfIndices)
		{
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Error parsing file!\n");
			fclose(meshFile);
			return false;
		}
	}

	fclose(meshFile);

	parent->setViewPlaneCoordsUsingFOVs(
		viewData.fovUp,
		viewData.fovDown,
		viewData.fovLeft,
		viewData.fovRight,
		glm::quat(viewData.qw, viewData.qx, viewData.qy, viewData.qz)
		);

	sgct_core::ClusterManager::instance()->getUserPtr()->setPos(
		viewData.x, viewData.y, viewData.z);

	sgct::Engine::instance()->updateFrustums();

	//store all verts in sgct format
	mTempVertices = new CorrectionMeshVertex[numberOfVertices];
	for (unsigned int i = 0; i < numberOfVertices; i++)
	{
		mTempVertices[i].r = 255;
		mTempVertices[i].g = 255;
		mTempVertices[i].b = 255;

		//clamp
		if (texturedVertexList[i].x > 1.0f)
			texturedVertexList[i].x = 1.0f;
		if (texturedVertexList[i].x < 0.0f)
			texturedVertexList[i].x = 0.0f;

		if (texturedVertexList[i].y > 1.0f)
			texturedVertexList[i].y = 1.0f;
		if (texturedVertexList[i].y < 0.0f)
			texturedVertexList[i].y = 0.0f;

		if (texturedVertexList[i].tx > 1.0f)
			texturedVertexList[i].tx = 1.0f;
		if (texturedVertexList[i].tx < 0.0f)
			texturedVertexList[i].tx = 0.0f;

		if (texturedVertexList[i].ty > 1.0f)
			texturedVertexList[i].ty = 1.0f;
		if (texturedVertexList[i].ty < 0.0f)
			texturedVertexList[i].ty = 0.0f;

		/*if (texturedVertexList[i].x > 1.0f || texturedVertexList[i].x < 0.0f ||
			texturedVertexList[i].y > 1.0f || texturedVertexList[i].y < 0.0f)
		{
			fprintf(stderr, "Coords: %f %f %f\tTex: %f %f %f\n",
				texturedVertexList[i].x, texturedVertexList[i].y, texturedVertexList[i].z,
				texturedVertexList[i].tx, texturedVertexList[i].ty, texturedVertexList[i].tz);
		}*/


		mTempVertices[i].x = texturedVertexList[i].x * mXSize + mXOffset;
		mTempVertices[i].y = (1.0f - texturedVertexList[i].y) * mYSize + mYOffset;

		mTempVertices[i].s = texturedVertexList[i].tx * mXSize + mXOffset;
		mTempVertices[i].t = texturedVertexList[i].ty * mYSize + mYOffset;

		/*fprintf(stderr, "Coords: %f %f %f\tTex: %f %f %f\n",
			texturedVertexList[i].x, texturedVertexList[i].y, texturedVertexList[i].z,
			texturedVertexList[i].tx, texturedVertexList[i].ty, texturedVertexList[i].tz);*/
	}

	mGeometries[Warped].mNumberOfVertices = numberOfVertices;
	mGeometries[Warped].mNumberOfIndices = numberOfIndices;
	
	//GL_QUAD_STRIP removed in OpenGL 3.3+
	//mGeometries[Warped].mGeometryType = GL_QUAD_STRIP;
	mGeometries[Warped].mGeometryType = GL_TRIANGLE_STRIP;

	//clean up
	delete [] texturedVertexList;
	texturedVertexList = NULL;

	createMesh(&mGeometries[Warped]);
	cleanUp();

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: Correction mesh read successfully! Vertices=%u, Indices=%u.\n", numberOfVertices, numberOfIndices);
	
	return true;
}

void sgct_core::CorrectionMesh::setupSimpleMesh()
{
	unsigned int numberOfVertices = 4;
	unsigned int numberOfIndices = 4;
	
	mGeometries[Warped].mNumberOfVertices = numberOfVertices;
	mGeometries[Warped].mNumberOfIndices = numberOfIndices;
	mGeometries[Warped].mGeometryType = GL_TRIANGLE_STRIP;

	mTempVertices = new CorrectionMeshVertex[ numberOfVertices ];
	memset(mTempVertices, 0, numberOfVertices * sizeof(CorrectionMeshVertex));

	mTempIndices = new unsigned int[numberOfIndices];
	memset(mTempIndices, 0, numberOfIndices * sizeof(unsigned int));

	mTempIndices[0] = 0;
	mTempIndices[1] = 3;
	mTempIndices[2] = 1;
	mTempIndices[3] = 2;

	mTempVertices[0].r = 255;
	mTempVertices[0].g = 255;
	mTempVertices[0].b = 255;
	mTempVertices[0].s = 0.0f * mXSize + mXOffset;
	mTempVertices[0].t = 0.0f * mYSize + mYOffset;
	mTempVertices[0].x = 0.0f * mXSize + mXOffset;
	mTempVertices[0].y = 0.0f * mYSize + mYOffset;

	mTempVertices[1].r = 255;
	mTempVertices[1].g = 255;
	mTempVertices[1].b = 255;
	mTempVertices[1].s = 1.0f * mXSize + mXOffset;
	mTempVertices[1].t = 0.0f * mYSize + mYOffset;
	mTempVertices[1].x = 1.0f * mXSize + mXOffset;
	mTempVertices[1].y = 0.0f * mYSize + mYOffset;

	mTempVertices[2].r = 255;
	mTempVertices[2].g = 255;
	mTempVertices[2].b = 255;
	mTempVertices[2].s = 1.0f * mXSize + mXOffset;
	mTempVertices[2].t = 1.0f * mYSize + mYOffset;
	mTempVertices[2].x = 1.0f * mXSize + mXOffset;
	mTempVertices[2].y = 1.0f * mYSize + mYOffset;

	mTempVertices[3].r = 255;
	mTempVertices[3].g = 255;
	mTempVertices[3].b = 255;
	mTempVertices[3].s = 0.0f * mXSize + mXOffset;
	mTempVertices[3].t = 1.0f * mYSize + mYOffset;
	mTempVertices[3].x = 0.0f * mXSize + mXOffset;
	mTempVertices[3].y = 1.0f * mYSize + mYOffset;
}

void sgct_core::CorrectionMesh::setupMaskMesh()
{
	unsigned int numberOfVertices = 4;
	unsigned int numberOfIndices = 4;

	mGeometries[UnWarped].mNumberOfVertices = numberOfVertices;
	mGeometries[UnWarped].mNumberOfIndices = numberOfIndices;
	mGeometries[UnWarped].mGeometryType = GL_TRIANGLE_STRIP;

	mTempVertices = new CorrectionMeshVertex[numberOfVertices];
	memset(mTempVertices, 0, numberOfVertices * sizeof(CorrectionMeshVertex));

	mTempIndices = new unsigned int[numberOfIndices];
	memset(mTempIndices, 0, numberOfIndices * sizeof(unsigned int));

	mTempIndices[0] = 0;
	mTempIndices[1] = 3;
	mTempIndices[2] = 1;
	mTempIndices[3] = 2;

	mTempVertices[0].r = 255;
	mTempVertices[0].g = 255;
	mTempVertices[0].b = 255;
	mTempVertices[0].s = 0.0f;
	mTempVertices[0].t = 0.0f;
	mTempVertices[0].x = 0.0f * mXSize + mXOffset;
	mTempVertices[0].y = 0.0f * mYSize + mYOffset;

	mTempVertices[1].r = 255;
	mTempVertices[1].g = 255;
	mTempVertices[1].b = 255;
	mTempVertices[1].s = 1.0f;
	mTempVertices[1].t = 0.0f;
	mTempVertices[1].x = 1.0f * mXSize + mXOffset;
	mTempVertices[1].y = 0.0f * mYSize + mYOffset;

	mTempVertices[2].r = 255;
	mTempVertices[2].g = 255;
	mTempVertices[2].b = 255;
	mTempVertices[2].s = 1.0f;
	mTempVertices[2].t = 1.0f;
	mTempVertices[2].x = 1.0f * mXSize + mXOffset;
	mTempVertices[2].y = 1.0f * mYSize + mYOffset;

	mTempVertices[3].r = 255;
	mTempVertices[3].g = 255;
	mTempVertices[3].b = 255;
	mTempVertices[3].s = 0.0f;
	mTempVertices[3].t = 1.0f;
	mTempVertices[3].x = 0.0f * mXSize + mXOffset;
	mTempVertices[3].y = 1.0f * mYSize + mYOffset;
}

void sgct_core::CorrectionMesh::createMesh(sgct_core::CorrectionMeshGeometry * geomPtr)
{
	/*sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Uploading mesh data (type=%d)...\n",
		ClusterManager::instance()->getMeshImplementation());*/

	if (ClusterManager::instance()->getMeshImplementation() == ClusterManager::DISPLAY_LIST)
	{
		geomPtr->mMeshData[Vertex] = glGenLists(1);
		glNewList(geomPtr->mMeshData[Vertex], GL_COMPILE);

		glBegin(geomPtr->mGeometryType);
		for (unsigned int i = 0; i < geomPtr->mNumberOfIndices; i++)
		{
			glColor3ub(mTempVertices[mTempIndices[i]].r, mTempVertices[mTempIndices[i]].g, mTempVertices[mTempIndices[i]].b);
			glTexCoord2f(mTempVertices[mTempIndices[i]].s, mTempVertices[mTempIndices[i]].t);
			glVertex2f(mTempVertices[mTempIndices[i]].x, mTempVertices[mTempIndices[i]].y);
		}
		glEnd();

		glEndList();
	}
	else
	{
		if(ClusterManager::instance()->getMeshImplementation() == ClusterManager::VAO)
		{
			glGenVertexArrays(1, &(geomPtr->mMeshData[Array]));
			glBindVertexArray(geomPtr->mMeshData[Array]);

			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Generating VAO: %d\n", geomPtr->mMeshData[Array]);
		}

		glGenBuffers(2, &(geomPtr->mMeshData[0]));
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Generating VBOs: %d %d\n", geomPtr->mMeshData[0], geomPtr->mMeshData[1]);

		glBindBuffer(GL_ARRAY_BUFFER, geomPtr->mMeshData[Vertex]);
		glBufferData(GL_ARRAY_BUFFER, geomPtr->mNumberOfVertices * sizeof(CorrectionMeshVertex), &mTempVertices[0], GL_STATIC_DRAW);

		if(ClusterManager::instance()->getMeshImplementation() == ClusterManager::VAO)
		{
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(
				0, // The attribute we want to configure
				2,                           // size
				GL_FLOAT,                    // type
				GL_FALSE,                    // normalized?
				sizeof(CorrectionMeshVertex),// stride
				reinterpret_cast<void*>(0)   // array buffer offset
			);

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(
				1, // The attribute we want to configure
				2,                           // size
				GL_FLOAT,                    // type
				GL_FALSE,                    // normalized?
				sizeof(CorrectionMeshVertex),// stride
				reinterpret_cast<void*>(8)   // array buffer offset
			);

			glEnableVertexAttribArray(2);
			glVertexAttribPointer(
				2, // The attribute we want to configure
				3,                           // size
				GL_UNSIGNED_BYTE,            // type
				GL_TRUE,                    // normalized?
				sizeof(CorrectionMeshVertex),// stride
				reinterpret_cast<void*>(16)  // array buffer offset
			);
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geomPtr->mMeshData[Index]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, geomPtr->mNumberOfIndices * sizeof(unsigned int), &mTempIndices[0], GL_STATIC_DRAW);

		//unbind
		if(ClusterManager::instance()->getMeshImplementation() == ClusterManager::VAO)
			glBindVertexArray(0);
		else
		{
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
	}
}

void sgct_core::CorrectionMesh::cleanUp()
{
	//clean up
	if( mTempVertices != NULL )
	{
		delete[] mTempVertices;
		mTempVertices = NULL;
	}

	if( mTempIndices != NULL )
	{
		delete[] mTempIndices;
		mTempIndices = NULL;
	}
}

/*!
Render the final mesh where for mapping the frame buffer to the screen
\param warped if warping should be enabled or not
*/
void sgct_core::CorrectionMesh::render(bool warped)
{
	CorrectionMeshGeometry * geomPtr = warped ? &mGeometries[Warped] : &mGeometries[UnWarped];
	
	if( ClusterManager::instance()->getMeshImplementation() == ClusterManager::VBO )
	{
		glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

		glEnableClientState(GL_VERTEX_ARRAY);
		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		glBindBuffer(GL_ARRAY_BUFFER, geomPtr->mMeshData[Vertex]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geomPtr->mMeshData[Index]);
		
		glVertexPointer(2, GL_FLOAT, sizeof(CorrectionMeshVertex), reinterpret_cast<void*>(0));		
		glTexCoordPointer(2, GL_FLOAT, sizeof(CorrectionMeshVertex), reinterpret_cast<void*>(8));
		glColorPointer(3, GL_UNSIGNED_BYTE, sizeof(CorrectionMeshVertex), reinterpret_cast<void*>(16));

		glDrawElements(geomPtr->mGeometryType, geomPtr->mNumberOfIndices, GL_UNSIGNED_INT, NULL);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glPopClientAttrib();
	}
	else if( ClusterManager::instance()->getMeshImplementation() == ClusterManager::VAO )
	{
		glBindVertexArray(geomPtr->mMeshData[Array]);
		glDrawElements(geomPtr->mGeometryType, geomPtr->mNumberOfIndices, GL_UNSIGNED_INT, NULL);
		glBindVertexArray(0);
	}
	else
	{
		glCallList(geomPtr->mMeshData[Vertex]);
	}
}
