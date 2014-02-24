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
		if (mMeshData[0])
        {
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMeshGeometry: Releasing correction mesh OpenGL data...\n");
            glDeleteLists(mMeshData[0], 1);
        }
	}
	else
	{
		if (mMeshData[2])
            glDeleteVertexArrays(1, &mMeshData[2]);

		//delete VBO and IBO
		if (mMeshData[0])
        {
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMeshGeometry: Releasing correction mesh OpenGL data...\n");
			glDeleteBuffers(2, &mMeshData[0]);
        }
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
	if(parent->hasMaskTexture())
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Creating mask mesh\n");
        
        setupMaskMesh();
        createMesh(&mGeometries[Mask]);
        cleanUp();
    }
	
	int length = 0;
	if (meshPath != NULL)
	{
		while (meshPath[length] != '\0')
			length++;
	}

	if (length == 0)
	{
		setupSimpleMesh();
		createMesh(&mGeometries[NoMask]);
		cleanUp();
		return false;
	}

	if (length > 4 && (strcmp(".sgc", &meshPath[length - 4]) == 0 || strcmp(".SGC", &meshPath[length - 4]) == 0))
	{
		if (!readAndGenerateScissMesh(meshPath, parent))
		{
			setupSimpleMesh();
			createMesh(&mGeometries[NoMask]);
			cleanUp();
			return false;
		}
	}
	else if (length > 3 && (strcmp(".ol", &meshPath[length - 3]) == 0 || strcmp(".OL", &meshPath[length - 3]) == 0))
	{
		if( !readAndGenerateScalableMesh(meshPath, parent))
		{
			setupSimpleMesh();
			createMesh(&mGeometries[NoMask]);
			cleanUp();
			return false;
		}
	}
	else
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh error: Loading failed (bad filename: %s)\n", meshPath);
		setupSimpleMesh();
		createMesh(&mGeometries[NoMask]);
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

	double orthoCoords[4];
	orthoCoords[0] = -1.0;
	orthoCoords[1] = 1.0;
	orthoCoords[2] = -1.0;
	orthoCoords[3] = 1.0;
	unsigned int resolution[2];
	resolution[0] = 0;
	resolution[1] = 0;

	CorrectionMeshVertex * vertexPtr;

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
				if( mTempVertices != NULL && resolution[0] != 0 && resolution[1] != 0 )
				{
					vertexPtr = &mTempVertices[numOfVerticesRead];
					vertexPtr->x = (x / static_cast<float>(resolution[0])) * mXSize + mXOffset;
					vertexPtr->y = (y / static_cast<float>(resolution[1])) * mYSize + mYOffset;
					vertexPtr->r = static_cast<float>(intensity)/255.0f;
					vertexPtr->g = static_cast<float>(intensity)/255.0f;
					vertexPtr->b = static_cast<float>(intensity)/255.0f;
					vertexPtr->a = 1.0f;
					vertexPtr->s = (1.0f - t) * mXSize + mXOffset;
					vertexPtr->t = (1.0f - s) * mYSize + mYOffset;

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
						orthoCoords[0] = tmpD;
					else if( strcmp(tmpString, "RIGHT") == 0 )
						orthoCoords[1] = tmpD;
					else if( strcmp(tmpString, "BOTTOM") == 0 )
						orthoCoords[2] = tmpD;
					else if( strcmp(tmpString, "TOP") == 0 )
						orthoCoords[3] = tmpD;
				}

#if (_MSC_VER >= 1400) //visual studio 2005 or later
				else if( sscanf_s(lineBuffer, "NATIVEXRES %u", &tmpUI) == 1 )
#else
				else if( sscanf(lineBuffer, "NATIVEXRES %u", &tmpUI) == 1 )
#endif
					resolution[0] = tmpUI;

#if (_MSC_VER >= 1400) //visual studio 2005 or later
				else if( sscanf_s(lineBuffer, "NATIVEYRES %u", &tmpUI) == 1 )
#else
				else if( sscanf(lineBuffer, "NATIVEYRES %u", &tmpUI) == 1 )
#endif
					resolution[1] = tmpUI;
			}

			//fprintf(stderr, "Row text: %s", lineBuffer);
		}

	}

	if (numberOfVertices != numOfVerticesRead || numberOfFaces != numOfFacesRead)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Incorrect mesh data geometry!");
		return false;
	}

	//normalize
	for (unsigned int i = 0; i < numberOfVertices; i++)
	{
		float xMin = static_cast<float>(orthoCoords[0]);
		float xMax = static_cast<float>(orthoCoords[1]);
		float yMin = static_cast<float>(orthoCoords[2]);
		float yMax = static_cast<float>(orthoCoords[3]);

		//normalize between 0.0 and 1.0
		float xVal = (mTempVertices[i].x - xMin) / (xMax - xMin);
		float yVal = (mTempVertices[i].y - yMin) / (yMax - yMin);

		//normalize between -1.0 to 1.0
		mTempVertices[i].x = xVal * 2.0f - 1.0f;
		mTempVertices[i].y = yVal * 2.0f - 1.0f;
	}

	fclose( meshFile );

	mGeometries[NoMask].mNumberOfVertices = numberOfVertices;
	mGeometries[NoMask].mNumberOfIndices = numberOfIndices;
	mGeometries[NoMask].mGeometryType = GL_TRIANGLES;

	createMesh(&mGeometries[NoMask]);

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
	retval = fread_s(texturedVertexList, sizeof(SCISSTexturedVertex) * numberOfVertices, sizeof(SCISSTexturedVertex), numberOfVertices, meshFile);
#else
	retval = fread(texturedVertexList, sizeof(SCISSTexturedVertex), numberOfVertices, meshFile);
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
	retval = fread(&numberOfIndices, sizeof(unsigned int), 1, meshFile);
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
		retval = fread(mTempIndices, sizeof(unsigned int), numberOfIndices, meshFile);
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

	CorrectionMeshVertex * vertexPtr;
	SCISSTexturedVertex * scissVertexPtr;

	//store all verts in sgct format
	mTempVertices = new CorrectionMeshVertex[numberOfVertices];
	for (unsigned int i = 0; i < numberOfVertices; i++)
	{
		vertexPtr = &mTempVertices[i];
		scissVertexPtr = &texturedVertexList[i];

		vertexPtr->r = 1.0f;
		vertexPtr->g = 1.0f;
		vertexPtr->b = 1.0f;
		vertexPtr->a = 1.0f;

		//clamp
		if (scissVertexPtr->x > 1.0f)
			scissVertexPtr->x = 1.0f;
		if (scissVertexPtr->x < 0.0f)
			scissVertexPtr->x = 0.0f;

		if (scissVertexPtr->y > 1.0f)
			scissVertexPtr->y = 1.0f;
		if (scissVertexPtr->y < 0.0f)
			scissVertexPtr->y = 0.0f;

		if (scissVertexPtr->tx > 1.0f)
			scissVertexPtr->tx = 1.0f;
		if (scissVertexPtr->tx < 0.0f)
			scissVertexPtr->tx = 0.0f;

		if (scissVertexPtr->ty > 1.0f)
			scissVertexPtr->ty = 1.0f;
		if (scissVertexPtr->ty < 0.0f)
			scissVertexPtr->ty = 0.0f;

		/*if (scissVertexPtr->x > 1.0f || scissVertexPtr->x < 0.0f ||
			scissVertexPtr->y > 1.0f || scissVertexPtr->y < 0.0f)
		{
			fprintf(stderr, "Coords: %f %f %f\tTex: %f %f %f\n",
				scissVertexPtr->x, scissVertexPtr->y, scissVertexPtr->z,
				scissVertexPtr->tx, scissVertexPtr->ty, scissVertexPtr->tz);
		}*/

		//convert to [-1, 1]
		vertexPtr->x = 2.0f*(scissVertexPtr->x * mXSize + mXOffset) - 1.0f;
		vertexPtr->y = 2.0f*((1.0f - scissVertexPtr->y) * mYSize + mYOffset) - 1.0f;

		vertexPtr->s = scissVertexPtr->tx * mXSize + mXOffset;
		vertexPtr->t = scissVertexPtr->ty * mYSize + mYOffset;

		/*fprintf(stderr, "Coords: %f %f %f\tTex: %f %f %f\n",
			scissVertexPtr->x, scissVertexPtr->y, scissVertexPtr->z,
			scissVertexPtr->tx, scissVertexPtr->ty, scissVertexPtr->tz);*/
	}

	mGeometries[NoMask].mNumberOfVertices = numberOfVertices;
	mGeometries[NoMask].mNumberOfIndices = numberOfIndices;
	
	//GL_QUAD_STRIP removed in OpenGL 3.3+
	//mGeometries[NoMask].mGeometryType = GL_QUAD_STRIP;
	mGeometries[NoMask].mGeometryType = GL_TRIANGLE_STRIP;

	//clean up
	delete [] texturedVertexList;
	texturedVertexList = NULL;

	createMesh(&mGeometries[NoMask]);
	cleanUp();

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: Correction mesh read successfully! Vertices=%u, Indices=%u.\n", numberOfVertices, numberOfIndices);
	
	return true;
}

void sgct_core::CorrectionMesh::setupSimpleMesh()
{
	unsigned int numberOfVertices = 4;
	unsigned int numberOfIndices = 4;
	
	mGeometries[NoMask].mNumberOfVertices = numberOfVertices;
	mGeometries[NoMask].mNumberOfIndices = numberOfIndices;
	mGeometries[NoMask].mGeometryType = GL_TRIANGLE_STRIP;

	mTempVertices = new CorrectionMeshVertex[ numberOfVertices ];
	memset(mTempVertices, 0, numberOfVertices * sizeof(CorrectionMeshVertex));

	mTempIndices = new unsigned int[numberOfIndices];
	memset(mTempIndices, 0, numberOfIndices * sizeof(unsigned int));

	mTempIndices[0] = 0;
	mTempIndices[1] = 3;
	mTempIndices[2] = 1;
	mTempIndices[3] = 2;

	mTempVertices[0].r = 1.0f;
	mTempVertices[0].g = 1.0f;
	mTempVertices[0].b = 1.0f;
	mTempVertices[0].a = 1.0f;
	mTempVertices[0].s = 0.0f * mXSize + mXOffset;
	mTempVertices[0].t = 0.0f * mYSize + mYOffset;
	mTempVertices[0].x = 2.0f*(0.0f * mXSize + mXOffset) - 1.0f;
	mTempVertices[0].y = 2.0f*(0.0f * mYSize + mYOffset) -1.0f;

	mTempVertices[1].r = 1.0f;
	mTempVertices[1].g = 1.0f;
	mTempVertices[1].b = 1.0f;
	mTempVertices[1].a = 1.0f;
	mTempVertices[1].s = 1.0f * mXSize + mXOffset;
	mTempVertices[1].t = 0.0f * mYSize + mYOffset;
	mTempVertices[1].x = 2.0f*(1.0f * mXSize + mXOffset) - 1.0f;
	mTempVertices[1].y = 2.0f*(0.0f * mYSize + mYOffset) - 1.0f;

	mTempVertices[2].r = 1.0f;
	mTempVertices[2].g = 1.0f;
	mTempVertices[2].b = 1.0f;
	mTempVertices[2].a = 1.0f;
	mTempVertices[2].s = 1.0f * mXSize + mXOffset;
	mTempVertices[2].t = 1.0f * mYSize + mYOffset;
	mTempVertices[2].x = 2.0f*(1.0f * mXSize + mXOffset) - 1.0f;
	mTempVertices[2].y = 2.0f*(1.0f * mYSize + mYOffset) - 1.0f;

	mTempVertices[3].r = 1.0f;
	mTempVertices[3].g = 1.0f;
	mTempVertices[3].b = 1.0f;
	mTempVertices[3].a = 1.0f;
	mTempVertices[3].s = 0.0f * mXSize + mXOffset;
	mTempVertices[3].t = 1.0f * mYSize + mYOffset;
	mTempVertices[3].x = 2.0f*(0.0f * mXSize + mXOffset) - 1.0f;
	mTempVertices[3].y = 2.0f*(1.0f * mYSize + mYOffset) - 1.0f;
}

void sgct_core::CorrectionMesh::setupMaskMesh()
{
	unsigned int numberOfVertices = 4;
	unsigned int numberOfIndices = 4;

	mGeometries[Mask].mNumberOfVertices = numberOfVertices;
	mGeometries[Mask].mNumberOfIndices = numberOfIndices;
	mGeometries[Mask].mGeometryType = GL_TRIANGLE_STRIP;

	mTempVertices = new CorrectionMeshVertex[numberOfVertices];
	memset(mTempVertices, 0, numberOfVertices * sizeof(CorrectionMeshVertex));

	mTempIndices = new unsigned int[numberOfIndices];
	memset(mTempIndices, 0, numberOfIndices * sizeof(unsigned int));

	mTempIndices[0] = 0;
	mTempIndices[1] = 3;
	mTempIndices[2] = 1;
	mTempIndices[3] = 2;

	mTempVertices[0].r = 1.0f;
	mTempVertices[0].g = 1.0f;
	mTempVertices[0].b = 1.0f;
	mTempVertices[0].a = 1.0f;
	mTempVertices[0].s = 0.0f;
	mTempVertices[0].t = 0.0f;
	mTempVertices[0].x = 2.0f*(0.0f * mXSize + mXOffset) - 1.0f;
	mTempVertices[0].y = 2.0f*(0.0f * mYSize + mYOffset) - 1.0f;

	mTempVertices[1].r = 1.0f;
	mTempVertices[1].g = 1.0f;
	mTempVertices[1].b = 1.0f;
	mTempVertices[1].a = 1.0f;
	mTempVertices[1].s = 1.0f;
	mTempVertices[1].t = 0.0f;
	mTempVertices[1].x = 2.0f*(1.0f * mXSize + mXOffset) - 1.0f;
	mTempVertices[1].y = 2.0f*(0.0f * mYSize + mYOffset) - 1.0f;

	mTempVertices[2].r = 1.0f;
	mTempVertices[2].g = 1.0f;
	mTempVertices[2].b = 1.0f;
	mTempVertices[2].a = 1.0f;
	mTempVertices[2].s = 1.0f;
	mTempVertices[2].t = 1.0f;
	mTempVertices[2].x = 2.0f*(1.0f * mXSize + mXOffset) - 1.0f;
	mTempVertices[2].y = 2.0f*(1.0f *mYSize + mYOffset) - 1.0f;

	mTempVertices[3].r = 1.0f;
	mTempVertices[3].g = 1.0f;
	mTempVertices[3].b = 1.0f;
	mTempVertices[3].a = 1.0f;
	mTempVertices[3].s = 0.0f;
	mTempVertices[3].t = 1.0f;
	mTempVertices[3].x = 2.0f*(0.0f * mXSize + mXOffset) - 1.0f;
	mTempVertices[3].y = 2.0f*(1.0f * mYSize + mYOffset) - 1.0f;
}

void sgct_core::CorrectionMesh::createMesh(sgct_core::CorrectionMeshGeometry * geomPtr)
{
	/*sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Uploading mesh data (type=%d)...\n",
		ClusterManager::instance()->getMeshImplementation());*/
	
	if(ClusterManager::instance()->getMeshImplementation() == ClusterManager::BUFFER_OBJECTS)
	{
		if(!sgct::Engine::instance()->isOGLPipelineFixed())
		{
			glGenVertexArrays(1, &(geomPtr->mMeshData[Array]));
			glBindVertexArray(geomPtr->mMeshData[Array]);

			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Generating VAO: %d\n", geomPtr->mMeshData[Array]);
		}

		glGenBuffers(2, &(geomPtr->mMeshData[0]));
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Generating VBOs: %d %d\n", geomPtr->mMeshData[0], geomPtr->mMeshData[1]);

		glBindBuffer(GL_ARRAY_BUFFER, geomPtr->mMeshData[Vertex]);
		glBufferData(GL_ARRAY_BUFFER, geomPtr->mNumberOfVertices * sizeof(CorrectionMeshVertex), &mTempVertices[0], GL_STATIC_DRAW);

		if(!sgct::Engine::instance()->isOGLPipelineFixed())
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
				4,                           // size
				GL_FLOAT,					// type
				GL_FALSE,                    // normalized?
				sizeof(CorrectionMeshVertex),// stride
				reinterpret_cast<void*>(16)  // array buffer offset
			);
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geomPtr->mMeshData[Index]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, geomPtr->mNumberOfIndices * sizeof(unsigned int), &mTempIndices[0], GL_STATIC_DRAW);

		//unbind
		if(!sgct::Engine::instance()->isOGLPipelineFixed())
			glBindVertexArray(0);
		
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else //display lists
	{
		geomPtr->mMeshData[Vertex] = glGenLists(1);
		glNewList(geomPtr->mMeshData[Vertex], GL_COMPILE);

		glBegin(geomPtr->mGeometryType);
		CorrectionMeshVertex vertex;
		
		for (unsigned int i = 0; i < geomPtr->mNumberOfIndices; i++)
		{
			vertex = mTempVertices[mTempIndices[i]];

			glColor4f(vertex.r, vertex.g, vertex.b, vertex.a);
			glTexCoord2f(vertex.s, vertex.t);
			glVertex2f(vertex.x, vertex.y);
		}
		glEnd();

		glEndList();
        
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Generating display list: %d\n", geomPtr->mMeshData[Vertex]);
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
\param mask to enable mask texture mode
*/
void sgct_core::CorrectionMesh::render(bool mask)
{
	CorrectionMeshGeometry * geomPtr = mask ? &mGeometries[NoMask] : &mGeometries[Mask];
	
	if( ClusterManager::instance()->getMeshImplementation() == ClusterManager::BUFFER_OBJECTS )
	{
		if(sgct::Engine::instance()->isOGLPipelineFixed())
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
			glColorPointer(4, GL_FLOAT, sizeof(CorrectionMeshVertex), reinterpret_cast<void*>(16));

			glDrawElements(geomPtr->mGeometryType, geomPtr->mNumberOfIndices, GL_UNSIGNED_INT, NULL);
		
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			glPopClientAttrib();
		}
		else
		{
			glBindVertexArray(geomPtr->mMeshData[Array]);
			glDrawElements(geomPtr->mGeometryType, geomPtr->mNumberOfIndices, GL_UNSIGNED_INT, NULL);
			glBindVertexArray(0);
		}
	}
	else
	{
		glCallList(geomPtr->mMeshData[Vertex]);
	}
}
