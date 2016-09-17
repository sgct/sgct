/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#define MAX_LINE_LENGTH 1024

#include <stdio.h>
#include <fstream>
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/CorrectionMesh.h"
#include "../include/sgct/ClusterManager.h"
#include "../include/sgct/Engine.h"
#include "../include/sgct/Viewport.h"
#include "../include/sgct/SGCTSettings.h"
#include <cstring>
#include <algorithm>

#if (_MSC_VER >= 1400) //visual studio 2005 or later
	#define _sscanf sscanf_s
#else
	#define _sscanf sscanf
#endif

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
}

sgct_core::CorrectionMesh::~CorrectionMesh()
{	
	
}

/*!
This function finds a suitible parser for warping meshes and loads them into memory.

@param meshPath the path to the mesh data
@param meshHint a hint to pass to the parser selector
@param parent the pointer to parent viewport
@return true if mesh found and loaded successfully
*/
bool sgct_core::CorrectionMesh::readAndGenerateMesh(std::string meshPath, sgct_core::Viewport * parent, MeshHint hint)
{	
	//generate unwarped mask
	setupSimpleMesh(&mGeometries[QUAD_MESH], parent);
	createMesh(&mGeometries[QUAD_MESH]);
	cleanUp();
	
	//generate unwarped mesh for mask
	if(parent->hasBlendMaskTexture() || parent->hasBlackLevelMaskTexture())
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Creating mask mesh\n");
        
		bool flip_x = false;
		bool flip_y = false;
		//if (hint == DOMEPROJECTION_HINT)
		//	flip_x = true;

		setupMaskMesh(parent, flip_x, flip_y);
		createMesh(&mGeometries[MASK_MESH]);
        cleanUp();
    }

	if (meshPath.empty())
	{
		setupSimpleMesh(&mGeometries[WARP_MESH], parent);
		createMesh(&mGeometries[WARP_MESH]);
		cleanUp();
		
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Empty mesh path!\n");
		return false;
	}
	
	//transform to lowercase
	std::string path(meshPath);
	std::transform(path.begin(), path.end(), path.begin(), ::tolower);

	MeshFormat meshFmt = NO_FMT;
	//find a suitible format
	if (path.find(".sgc") != std::string::npos)
		meshFmt = SCISS_FMT;
	else if (path.find(".ol") != std::string::npos)
		meshFmt = SCALEABLE_FMT;
	else if (path.find(".skyskan") != std::string::npos)
		meshFmt = SKYSKAN_FMT;
	else if (path.find(".txt") != std::string::npos)
	{
		if (hint == NO_HINT || hint == SKYSKAN_HINT)//default for this suffix
			meshFmt = SKYSKAN_FMT;
	}
	else if (path.find(".csv") != std::string::npos)
	{
		if (hint == NO_HINT || hint == DOMEPROJECTION_HINT)//default for this suffix
			meshFmt = DOMEPROJECTION_FMT;
	}
	else if (path.find(".data") != std::string::npos)
	{
		if (hint == NO_HINT || hint == PAULBOURKE_HINT)//default for this suffix
			meshFmt = PAULBOURKE_FMT;
	}

	//select parser
	bool loadStatus = false;
	switch (meshFmt)
	{
	case DOMEPROJECTION_FMT:
		loadStatus = readAndGenerateDomeProjectionMesh(meshPath, parent);
		break;

	case SCALEABLE_FMT:
		loadStatus = readAndGenerateScalableMesh(meshPath, parent);
		break;

	case SCISS_FMT:
		loadStatus = readAndGenerateScissMesh(meshPath, parent);
		break;

	case SKYSKAN_FMT:
		loadStatus = readAndGenerateSkySkanMesh(meshPath, parent);
		break;

	case PAULBOURKE_FMT:
		loadStatus = readAndGeneratePaulBourkeMesh(meshPath, parent);
		break;
	}

	if( !loadStatus )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh error: Loading mesh '%s' failed!\n", meshPath.c_str());
		
		setupSimpleMesh(&mGeometries[WARP_MESH], parent);
		createMesh(&mGeometries[WARP_MESH]);
		cleanUp();
		return false;
	}

	return true;
}

/*!
Parse data from domeprojection's camera based calibration system. Domeprojection.com
*/
bool sgct_core::CorrectionMesh::readAndGenerateDomeProjectionMesh(const std::string & meshPath, Viewport * parent)
{
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO,
		"CorrectionMesh: Reading DomeProjection mesh data from '%s'.\n", meshPath.c_str());

	FILE * meshFile = NULL;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if (fopen_s(&meshFile, meshPath.c_str(), "r") != 0 || !meshFile)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Failed to open warping mesh file!\n");
		return false;
	}
#else
	meshFile = fopen(meshPath.c_str(), "r");
	if (meshFile == NULL)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Failed to open warping mesh file!\n");
		return false;
	}
#endif

	char lineBuffer[MAX_LINE_LENGTH];
	float x, y, u, v;
	unsigned int col, row;
	unsigned int numberOfCols = 0;
	unsigned int numberOfRows = 0;

	CorrectionMeshVertex vertex;
	std::vector<CorrectionMeshVertex> vertices;

	//init to max intencity (opaque white)
	vertex.r = 1.0f;
	vertex.g = 1.0f;
	vertex.b = 1.0f;
	vertex.a = 1.0f;

	while (!feof(meshFile))
	{
		if (fgets(lineBuffer, MAX_LINE_LENGTH, meshFile) != NULL)
		{
#if (_MSC_VER >= 1400) //visual studio 2005 or later
			if (sscanf_s(lineBuffer, "%f;%f;%f;%f;%u;%u", &x, &y, &u, &v, &col, &row) == 6)
#else
			if (sscanf(lineBuffer, "%f;%f;%f;%f;%u;%u", &x, &y, &u, &v, &col, &row) == 6)
#endif
			{
				//find dimensions of meshdata
				if (col > numberOfCols)
					numberOfCols = col;

				if (row > numberOfRows)
					numberOfRows = row;

				//clamp
				clamp(x, 1.0f, 0.0f);
				clamp(y, 1.0f, 0.0f);
				//clamp(u, 1.0f, 0.0f);
				//clamp(v, 1.0f, 0.0f);

				//convert to [-1, 1]
				vertex.x = 2.0f * (x * parent->getXSize() + parent->getX()) - 1.0f;
				vertex.y = 2.0f * ((1.0f-y) * parent->getYSize() + parent->getY()) - 1.0f;

				//scale to viewport coordinates
				vertex.s = u * parent->getXSize() + parent->getX();
				vertex.t = (1.0f-v) * parent->getYSize() + parent->getY();

				vertices.push_back(vertex);
			}
			
		}

	}

	fclose(meshFile);

	//add one to actually store the dimensions instread of largest index
	numberOfCols++;
	numberOfRows++;

	//copy vertices
	unsigned int numberOfVertices = numberOfCols * numberOfRows;
	mTempVertices = new CorrectionMeshVertex[numberOfVertices];
	memcpy(mTempVertices, vertices.data(), numberOfVertices * sizeof(CorrectionMeshVertex));
	mGeometries[WARP_MESH].mNumberOfVertices = numberOfVertices;
	vertices.clear();

	std::vector<unsigned int> indices;
	unsigned int i0, i1, i2, i3;
	for (unsigned int c = 0; c < (numberOfCols -1); c++)
		for (unsigned int r = 0; r < (numberOfRows-1); r++)
		{
			i0 = r * numberOfCols + c;
			i1 = r * numberOfCols + (c + 1);
			i2 = (r + 1) * numberOfCols + (c + 1);
			i3 = (r + 1) * numberOfCols + c;

			//fprintf(stderr, "Indexes: %u %u %u %u\n", i0, i1, i2, i3);

			/*

			3      2
			 x____x
			 |   /|
			 |  / |
			 | /  |
			 |/   |
			 x----x
			0      1
			
			*/

			//triangle 1
			indices.push_back(i0);
			indices.push_back(i1);
			indices.push_back(i2);

			//triangle 2
			indices.push_back(i0);
			indices.push_back(i2);
			indices.push_back(i3);
		}

	//allocate and copy indices
	mGeometries[WARP_MESH].mNumberOfIndices = static_cast<unsigned int>(indices.size());
	mTempIndices = new unsigned int[mGeometries[WARP_MESH].mNumberOfIndices];
	memcpy(mTempIndices, indices.data(), mGeometries[WARP_MESH].mNumberOfIndices * sizeof(unsigned int));
	indices.clear();

	mGeometries[WARP_MESH].mGeometryType = GL_TRIANGLES;

	createMesh(&mGeometries[WARP_MESH]);
	cleanUp();

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Correction mesh read successfully! Vertices=%u, Indices=%u.\n", mGeometries[WARP_MESH].mNumberOfVertices, mGeometries[WARP_MESH].mNumberOfIndices);
	
	return true;
}

bool sgct_core::CorrectionMesh::readAndGenerateScalableMesh(const std::string & meshPath, Viewport * parent)
{
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO,
		"CorrectionMesh: Reading scalable mesh data from '%s'.\n", meshPath.c_str());

	FILE * meshFile = NULL;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if( fopen_s(&meshFile, meshPath.c_str(), "r") != 0 || !meshFile )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Failed to open warping mesh file!\n");
		return false;
	}
#else
	meshFile = fopen(meshPath.c_str(), "r");
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
					vertexPtr->x = (x / static_cast<float>(resolution[0])) * parent->getXSize() + parent->getX();
					vertexPtr->y = (y / static_cast<float>(resolution[1])) * parent->getYSize() + parent->getY();
					vertexPtr->r = static_cast<float>(intensity)/255.0f;
					vertexPtr->g = static_cast<float>(intensity)/255.0f;
					vertexPtr->b = static_cast<float>(intensity)/255.0f;
					vertexPtr->a = 1.0f;
					vertexPtr->s = (1.0f - t) * parent->getXSize() + parent->getX();
					vertexPtr->t = (1.0f - s) * parent->getYSize() + parent->getY();

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

	mGeometries[WARP_MESH].mNumberOfVertices = numberOfVertices;
	mGeometries[WARP_MESH].mNumberOfIndices = numberOfIndices;
	mGeometries[WARP_MESH].mGeometryType = GL_TRIANGLES;

	createMesh(&mGeometries[WARP_MESH]);

	cleanUp();

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: Correction mesh read successfully! Vertices=%u, Faces=%u.\n", numOfVerticesRead, numOfFacesRead);

	return true;
}

bool sgct_core::CorrectionMesh::readAndGenerateScissMesh(const std::string & meshPath, sgct_core::Viewport * parent)
{	
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO,
		"CorrectionMesh: Reading sciss mesh data from '%s'.\n", meshPath.c_str());

	FILE * meshFile = NULL;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if (fopen_s(&meshFile, meshPath.c_str(), "rb") != 0 || !meshFile)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Failed to open warping mesh file!\n");
		return false;
	}
#else
	meshFile = fopen(meshPath.c_str(), "rb");
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
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: file version %u\n", fileVersion);

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
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Mapping type = %s (%u)\n", mappingType == 0 ? "planar" : "cube", mappingType);

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
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Rotation quat = [%f %f %f %f]\n",
			viewData.qx, viewData.qy, viewData.qz, viewData.qw);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Position = [%f %f %f]\n",
			viewData.x, viewData.y, viewData.z);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: FOV up = %f\n",
			viewData.fovUp);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: FOV down = %f\n",
			viewData.fovDown);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: FOV left = %f\n",
			viewData.fovLeft);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: FOV right = %f\n",
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
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Number of vertices = %u (%ux%u)\n", numberOfVertices, size[0], size[1]);
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
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Number of indices = %u\n", numberOfIndices);

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

	parent->getUser()->setPos(
		viewData.x, viewData.y, viewData.z);

	parent->setViewPlaneCoordsUsingFOVs(
		viewData.fovUp,
		viewData.fovDown,
		viewData.fovLeft,
		viewData.fovRight,
		glm::quat(viewData.qw, viewData.qx, viewData.qy, viewData.qz)
		);

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
		clamp(scissVertexPtr->x, 1.0f, 0.0f);
		clamp(scissVertexPtr->y, 1.0f, 0.0f);
		clamp(scissVertexPtr->tx, 1.0f, 0.0f);
		clamp(scissVertexPtr->ty, 1.0f, 0.0f);

		//convert to [-1, 1]
		vertexPtr->x = 2.0f*(scissVertexPtr->x * parent->getXSize() + parent->getX()) - 1.0f;
		vertexPtr->y = 2.0f*((1.0f - scissVertexPtr->y) * parent->getYSize() + parent->getY()) - 1.0f;

		vertexPtr->s = scissVertexPtr->tx * parent->getXSize() + parent->getX();
		vertexPtr->t = scissVertexPtr->ty * parent->getYSize() + parent->getY();

		/*fprintf(stderr, "Coords: %f %f %f\tTex: %f %f %f\n",
			scissVertexPtr->x, scissVertexPtr->y, scissVertexPtr->z,
			scissVertexPtr->tx, scissVertexPtr->ty, scissVertexPtr->tz);*/
	}

	mGeometries[WARP_MESH].mNumberOfVertices = numberOfVertices;
	mGeometries[WARP_MESH].mNumberOfIndices = numberOfIndices;
	
	//GL_QUAD_STRIP removed in OpenGL 3.3+
	//mGeometries[WARP_MESH].mGeometryType = GL_QUAD_STRIP;
	mGeometries[WARP_MESH].mGeometryType = GL_TRIANGLE_STRIP;

	//clean up
	delete [] texturedVertexList;
	texturedVertexList = NULL;

	createMesh(&mGeometries[WARP_MESH]);
	cleanUp();

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Correction mesh read successfully! Vertices=%u, Indices=%u.\n", numberOfVertices, numberOfIndices);
	
	return true;
}

bool sgct_core::CorrectionMesh::readAndGenerateSkySkanMesh(const std::string & meshPath, Viewport * parent)
{
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO,
		"CorrectionMesh: Reading SkySkan mesh data from '%s'.\n", meshPath.c_str());

	FILE * meshFile = NULL;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if (fopen_s(&meshFile, meshPath.c_str(), "r") != 0 || !meshFile)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Failed to open warping mesh file!\n");
		return false;
	}
#else
	meshFile = fopen(meshPath.c_str(), "r");
	if (meshFile == NULL)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Failed to open warping mesh file!\n");
		return false;
	}
#endif

	float azimuth = 0.0f;
	float elevation = 0.0f;
	float horizontal_fov = 0.0f;
	float vertical_fov = 0.0f;
	float fovTweeks[2] = { -1.0f, -1.0f };
	float UVTweeks[2] = { -1.0f, -1.0f };
	bool dimensionsSet = false;
	bool azimuthSet = false;
	bool elevationSet = false;
	bool hFovSet = false;
	bool vFovSet = false;
	float x, y, u, v;

	unsigned int size[2];
	unsigned int counter = 0;

	char lineBuffer[MAX_LINE_LENGTH];
	while (!feof(meshFile))
	{
		if (fgets(lineBuffer, MAX_LINE_LENGTH, meshFile) != NULL)
		{

#if (_MSC_VER >= 1400) //visual studio 2005 or later
			if (sscanf_s(lineBuffer, "Dome Azimuth=%f", &azimuth) == 1)
#else
			if (sscanf(lineBuffer, "Dome Azimuth=%f", &azimuth) == 1)
#endif
			{
				azimuthSet = true;
			}

#if (_MSC_VER >= 1400) //visual studio 2005 or later
			else if (sscanf_s(lineBuffer, "Dome Elevation=%f", &elevation) == 1)
#else
			else if (sscanf(lineBuffer, "Dome Elevation=%f", &elevation) == 1)
#endif
			{
				elevationSet = true;
			}

#if (_MSC_VER >= 1400) //visual studio 2005 or later
			else if (sscanf_s(lineBuffer, "Horizontal FOV=%f", &horizontal_fov) == 1)
#else
			else if (sscanf(lineBuffer, "Horizontal FOV=%f", &horizontal_fov) == 1)
#endif
			{
				hFovSet = true;
			}

#if (_MSC_VER >= 1400) //visual studio 2005 or later
			else if (sscanf_s(lineBuffer, "Vertical FOV=%f", &vertical_fov) == 1)
#else
			else if (sscanf(lineBuffer, "Vertical FOV=%f", &vertical_fov) == 1)
#endif
			{
				vFovSet = true;
			}

#if (_MSC_VER >= 1400) //visual studio 2005 or later
			else if (sscanf_s(lineBuffer, "Horizontal Tweek=%f", &fovTweeks[0]) == 1)
#else
			else if (sscanf(lineBuffer, "Horizontal Tweek=%f", &fovTweeks[0]) == 1)
#endif
			{
				;
			}

#if (_MSC_VER >= 1400) //visual studio 2005 or later
			else if (sscanf_s(lineBuffer, "Vertical Tweek=%f", &fovTweeks[1]) == 1)
#else
			else if (sscanf(lineBuffer, "Vertical Tweek=%f", &fovTweeks[1]) == 1)
#endif
			{
				;
			}

#if (_MSC_VER >= 1400) //visual studio 2005 or later
			else if (sscanf_s(lineBuffer, "U Tweek=%f", &UVTweeks[0]) == 1)
#else
			else if (sscanf(lineBuffer, "U Tweek=%f", &UVTweeks[0]) == 1)
#endif
			{
				;
			}

#if (_MSC_VER >= 1400) //visual studio 2005 or later
			else if (sscanf_s(lineBuffer, "V Tweek=%f", &UVTweeks[1]) == 1)
#else
			else if (sscanf(lineBuffer, "V Tweek=%f", &UVTweeks[1]) == 1)
#endif
			{
				;
			}

#if (_MSC_VER >= 1400) //visual studio 2005 or later
			else if (!dimensionsSet && sscanf_s(lineBuffer, "%u %u", &size[0], &size[1]) == 2)
#else
			else if (!dimensionsSet && sscanf(lineBuffer, "%u %u", &size[0], &size[1]) == 2)
#endif
			{
				dimensionsSet = true;
				mTempVertices = new CorrectionMeshVertex[size[0] * size[1]];
				mGeometries[WARP_MESH].mNumberOfVertices = size[0] * size[1];
			}

#if (_MSC_VER >= 1400) //visual studio 2005 or later
			else if (dimensionsSet && sscanf_s(lineBuffer, "%f %f %f %f", &x, &y, &u, &v) == 4)
#else
			else if (dimensionsSet && sscanf(lineBuffer, "%f %f %f %f", &x, &y, &u, &v) == 4)
#endif
			{
				if (UVTweeks[0] > -1.0f)
					u *= UVTweeks[0];

				if (UVTweeks[1] > -1.0f)
					v *= UVTweeks[1];
				
				mTempVertices[counter].x = x;
				mTempVertices[counter].y = y;
				mTempVertices[counter].s = u;
				//mTempVertices[counter].t = v;
				//mTempVertices[counter].s = 1.0f - u;
				mTempVertices[counter].t = 1.0f - v;

				mTempVertices[counter].r = 1.0f;
				mTempVertices[counter].g = 1.0f;
				mTempVertices[counter].b = 1.0f;
				mTempVertices[counter].a = 1.0f;

				//fprintf(stderr, "Adding vertex: %u %.3f %.3f %.3f %.3f\n", counter, x, y, u, v);

				counter++;
			}

			//fprintf(stderr, "Row text: %s", lineBuffer);
		}

	}

	fclose(meshFile);

	if (!dimensionsSet ||
		!azimuthSet ||
		!elevationSet ||
		!hFovSet ||
		horizontal_fov <= 0.0f)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Data reading error!\n");
		return false;
	}

	//create frustums and projection matrices
	if (!vFovSet || vertical_fov <= 0.0f)
	{
		//half the width (radius is one unit, cancels it self out)
		float hw = tanf(glm::radians<float>(horizontal_fov) / 2.0f);
		//half height
		float hh = (1200.0f / 2048.0f) * hw;
		
		vertical_fov = 2.0f * glm::degrees<float>(atanf(hh));

		fprintf(stderr, "HFOV: %f VFOV: %f\n", horizontal_fov, vertical_fov);
		
		//vertical_fov = (1200.0f / 2048.0f) * horizontal_fov;
		//vertical_fov = horizontal_fov;
	}

	//correct fovs ??
	//horizontal_fov *= 0.948f;
	//vertical_fov *= 1.51f;

	if (fovTweeks[0] > 0.0f)
		horizontal_fov *= fovTweeks[0];
	if (fovTweeks[1] > 0.0f)
		vertical_fov *= fovTweeks[1];

	glm::quat rotQuat;
	rotQuat = glm::rotate(rotQuat, glm::radians(-azimuth), glm::vec3(0.0f, 1.0f, 0.0f));
	rotQuat = glm::rotate(rotQuat, glm::radians(elevation), glm::vec3(1.0f, 0.0f, 0.0f));

	parent->getUser()->setPos(0.0f, 0.0f, 0.0f);
	parent->setViewPlaneCoordsUsingFOVs(
		vertical_fov / 2.0f,
		-vertical_fov / 2.0f,
		-horizontal_fov / 2.0f,
		horizontal_fov / 2.0f,
		rotQuat
		);
	
	sgct::Engine::instance()->updateFrustums();

	std::vector<unsigned int> indices;
	unsigned int i0, i1, i2, i3;
	for (unsigned int c = 0; c < (size[0]-1); c++)
		for (unsigned int r = 0; r < (size[1]-1); r++)
		{
			i0 = r * size[0] + c;
			i1 = r * size[0] + (c + 1);
			i2 = (r + 1) * size[0] + (c + 1);
			i3 = (r + 1) * size[0] + c;

			//fprintf(stderr, "Indexes: %u %u %u %u\n", i0, i1, i2, i3);

			/*

			3      2
			 x____x
			 |   /|
			 |  / |
			 | /  |
			 |/   |
			 x----x
			0      1
			
			*/

			//triangle 1
			if (mTempVertices[i0].x != -1.0f && mTempVertices[i0].y != -1.0f &&
				mTempVertices[i1].x != -1.0f && mTempVertices[i1].y != -1.0f &&
				mTempVertices[i2].x != -1.0f && mTempVertices[i2].y != -1.0f)
			{
				indices.push_back(i0);
				indices.push_back(i1);
				indices.push_back(i2);
			}

			//triangle 2
			if (mTempVertices[i0].x != -1.0f && mTempVertices[i0].y != -1.0f &&
				mTempVertices[i2].x != -1.0f && mTempVertices[i2].y != -1.0f &&
				mTempVertices[i3].x != -1.0f && mTempVertices[i3].y != -1.0f)
			{
				indices.push_back(i0);
				indices.push_back(i2);
				indices.push_back(i3);
			}
		}

	for (unsigned int i = 0; i < mGeometries[WARP_MESH].mNumberOfVertices; i++)
	{
		//clamp
		/*if (mTempVertices[i].x > 1.0f)
			mTempVertices[i].x = 1.0f;
		else if (mTempVertices[i].x < 0.0f)
			mTempVertices[i].x = 0.0f;

		if (mTempVertices[i].y > 1.0f)
			mTempVertices[i].y = 1.0f;
		else if (mTempVertices[i].y < 0.0f)
			mTempVertices[i].y = 0.0f;

		if (mTempVertices[i].s > 1.0f)
			mTempVertices[i].s = 1.0f;
		else if (mTempVertices[i].s < 0.0f)
			mTempVertices[i].s = 0.0f;

		if (mTempVertices[i].t > 1.0f)
			mTempVertices[i].t = 1.0f;
		else if (mTempVertices[i].t < 0.0f)
			mTempVertices[i].t = 0.0f;*/

		//convert to [-1, 1]
		mTempVertices[i].x = 2.0f*(mTempVertices[i].x * parent->getXSize() + parent->getX()) - 1.0f;
		//mTempVertices[i].x = 2.0f*((1.0f - mTempVertices[i].x) * parent->getXSize() + parent->getX()) - 1.0f;
		//mTempVertices[i].y = 2.0f*(mTempVertices[i].y * parent->getYSize() + parent->getY()) - 1.0f;
		mTempVertices[i].y = 2.0f*((1.0f - mTempVertices[i].y) * parent->getYSize() + parent->getY()) - 1.0f;
		//test code
		//mTempVertices[i].x /= 1.5f;
		//mTempVertices[i].y /= 1.5f;

		mTempVertices[i].s = mTempVertices[i].s * parent->getXSize() + parent->getX();
		mTempVertices[i].t = mTempVertices[i].t * parent->getYSize() + parent->getY();
	}

	//allocate and copy indices
	mGeometries[WARP_MESH].mNumberOfIndices = static_cast<unsigned int>(indices.size());
	mTempIndices = new unsigned int[mGeometries[WARP_MESH].mNumberOfIndices];
	memcpy(mTempIndices, indices.data(), mGeometries[WARP_MESH].mNumberOfIndices * sizeof(unsigned int));

	mGeometries[WARP_MESH].mGeometryType = GL_TRIANGLES;

	createMesh(&mGeometries[WARP_MESH]);
	cleanUp();

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Correction mesh read successfully! Vertices=%u, Indices=%u.\n", mGeometries[WARP_MESH].mNumberOfVertices, mGeometries[WARP_MESH].mNumberOfIndices);

	return true;
}

bool sgct_core::CorrectionMesh::readAndGeneratePaulBourkeMesh(const std::string & meshPath, Viewport * parent)
{
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO,
		"CorrectionMesh: Reading Paul Bourke spherical mirror mesh data from '%s'.\n", meshPath.c_str());

	FILE * meshFile = NULL;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if (fopen_s(&meshFile, meshPath.c_str(), "r") != 0 || !meshFile)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Failed to open warping mesh file!\n");
		return false;
	}
#else
	meshFile = fopen(meshPath.c_str(), "r");
	if (meshFile == NULL)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Failed to open warping mesh file!\n");
		return false;
	}
#endif

	//variables
	int mappingType = -1;
	int size[2] = {-1, -1};
	unsigned int counter = 0;
	float x, y, s, t, intensity;
	char lineBuffer[MAX_LINE_LENGTH];

	//get the fist line containing the mapping type id
	if (fgets(lineBuffer, MAX_LINE_LENGTH, meshFile) != NULL)
	{
		int tmpi;
		if(_sscanf(lineBuffer, "%d", &tmpi) == 1)
			mappingType = tmpi;
	}

	//get the mesh dimensions
	if (fgets(lineBuffer, MAX_LINE_LENGTH, meshFile) != NULL)
	{
		if (_sscanf(lineBuffer, "%d %d", &size[0], &size[1]) == 2)
		{
			mTempVertices = new CorrectionMeshVertex[size[0] * size[1]];
			mGeometries[WARP_MESH].mNumberOfVertices = static_cast<unsigned int>(size[0] * size[1]);
		}
	}

	//check if everyting useful is set
	if (mappingType == -1 || size[0] == -1 || size[1] == -1)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Invalid data");
		return false;
	}

	//get all data
	while (!feof(meshFile))
	{
		if (fgets(lineBuffer, MAX_LINE_LENGTH, meshFile) != NULL)
		{
			if (_sscanf(lineBuffer, "%f %f %f %f %f", &x, &y, &s, &t, &intensity) == 5)
			{
				mTempVertices[counter].x = x;
				mTempVertices[counter].y = y;
				mTempVertices[counter].s = s;
				mTempVertices[counter].t = t;

				mTempVertices[counter].r = intensity;
				mTempVertices[counter].g = intensity;
				mTempVertices[counter].b = intensity;
				mTempVertices[counter].a = 1.0f;

				//if(counter <= 100)
				//	fprintf(stderr, "Adding vertex: %u %.3f %.3f %.3f %.3f %.3f\n", counter, x, y, s, t, intensity);

				counter++;
			}
		}
	}

	//generate indices
	std::vector<unsigned int> indices;
	int i0, i1, i2, i3;
	for (int c = 0; c < (size[0] - 1); c++)
		for (int r = 0; r < (size[1] - 1); r++)
		{
			i0 = r * size[0] + c;
			i1 = r * size[0] + (c + 1);
			i2 = (r + 1) * size[0] + (c + 1);
			i3 = (r + 1) * size[0] + c;

			//fprintf(stderr, "Indexes: %u %u %u %u\n", i0, i1, i2, i3);

			/*

			3      2
			x____x
			|   /|
			|  / |
			| /  |
			|/   |
			x----x
			0      1

			*/

			//triangle 1
			indices.push_back(i0);
			indices.push_back(i1);
			indices.push_back(i2);

			//triangle 2
			indices.push_back(i0);
			indices.push_back(i2);
			indices.push_back(i3);
		}

	float aspect = sgct::Engine::instance()->getCurrentWindowPtr()->getAspectRatio() * 
		(parent->getXSize() / parent->getYSize());
	
	for (unsigned int i = 0; i < mGeometries[WARP_MESH].mNumberOfVertices; i++)
	{
		//convert to [0, 1] (normalize)
		mTempVertices[i].x /= aspect;
		mTempVertices[i].x = (mTempVertices[i].x + 1.0f) / 2.0f;
		mTempVertices[i].y = (mTempVertices[i].y + 1.0f) / 2.0f;
		
		//scale, re-position and convert to [-1, 1]
		mTempVertices[i].x = (mTempVertices[i].x * parent->getXSize() + parent->getX()) * 2.0f - 1.0f;
		mTempVertices[i].y = (mTempVertices[i].y * parent->getYSize() + parent->getY()) * 2.0f - 1.0f;

		//convert to viewport coordinates
		mTempVertices[i].s = mTempVertices[i].s * parent->getXSize() + parent->getX();
		mTempVertices[i].t = mTempVertices[i].t * parent->getYSize() + parent->getY();
	}

	//allocate and copy indices
	mGeometries[WARP_MESH].mNumberOfIndices = static_cast<unsigned int>(indices.size());
	mTempIndices = new unsigned int[mGeometries[WARP_MESH].mNumberOfIndices];
	memcpy(mTempIndices, indices.data(), mGeometries[WARP_MESH].mNumberOfIndices * sizeof(unsigned int));

	mGeometries[WARP_MESH].mGeometryType = GL_TRIANGLES;
	createMesh(&mGeometries[WARP_MESH]);

	//force regeneration of dome render quad
	if (FisheyeProjection* fishPrj = dynamic_cast<FisheyeProjection*>(parent->getNonLinearProjectionPtr()))
	{
		fishPrj->setIgnoreAspectRatio(true);
		fishPrj->update(1.0f, 1.0f);
	}

	cleanUp();

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Correction mesh read successfully! Vertices=%u, Indices=%u.\n", mGeometries[WARP_MESH].mNumberOfVertices, mGeometries[WARP_MESH].mNumberOfIndices);
	return true;
}

void sgct_core::CorrectionMesh::setupSimpleMesh(CorrectionMeshGeometry * geomPtr, Viewport * parent)
{
	unsigned int numberOfVertices = 4;
	unsigned int numberOfIndices = 4;
	
	geomPtr->mNumberOfVertices = numberOfVertices;
	geomPtr->mNumberOfIndices = numberOfIndices;
	geomPtr->mGeometryType = GL_TRIANGLE_STRIP;

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
	mTempVertices[0].s = 0.0f * parent->getXSize() + parent->getX();
	mTempVertices[0].t = 0.0f * parent->getYSize() + parent->getY();
	mTempVertices[0].x = 2.0f*(0.0f * parent->getXSize() + parent->getX()) - 1.0f;
	mTempVertices[0].y = 2.0f*(0.0f * parent->getYSize() + parent->getY()) -1.0f;

	mTempVertices[1].r = 1.0f;
	mTempVertices[1].g = 1.0f;
	mTempVertices[1].b = 1.0f;
	mTempVertices[1].a = 1.0f;
	mTempVertices[1].s = 1.0f * parent->getXSize() + parent->getX();
	mTempVertices[1].t = 0.0f * parent->getYSize() + parent->getY();
	mTempVertices[1].x = 2.0f*(1.0f * parent->getXSize() + parent->getX()) - 1.0f;
	mTempVertices[1].y = 2.0f*(0.0f * parent->getYSize() + parent->getY()) - 1.0f;

	mTempVertices[2].r = 1.0f;
	mTempVertices[2].g = 1.0f;
	mTempVertices[2].b = 1.0f;
	mTempVertices[2].a = 1.0f;
	mTempVertices[2].s = 1.0f * parent->getXSize() + parent->getX();
	mTempVertices[2].t = 1.0f * parent->getYSize() + parent->getY();
	mTempVertices[2].x = 2.0f*(1.0f * parent->getXSize() + parent->getX()) - 1.0f;
	mTempVertices[2].y = 2.0f*(1.0f * parent->getYSize() + parent->getY()) - 1.0f;

	mTempVertices[3].r = 1.0f;
	mTempVertices[3].g = 1.0f;
	mTempVertices[3].b = 1.0f;
	mTempVertices[3].a = 1.0f;
	mTempVertices[3].s = 0.0f * parent->getXSize() + parent->getX();
	mTempVertices[3].t = 1.0f * parent->getYSize() + parent->getY();
	mTempVertices[3].x = 2.0f*(0.0f * parent->getXSize() + parent->getX()) - 1.0f;
	mTempVertices[3].y = 2.0f*(1.0f * parent->getYSize() + parent->getY()) - 1.0f;
}

void sgct_core::CorrectionMesh::setupMaskMesh(Viewport * parent, bool flip_x, bool flip_y)
{
	unsigned int numberOfVertices = 4;
	unsigned int numberOfIndices = 4;

	mGeometries[MASK_MESH].mNumberOfVertices = numberOfVertices;
	mGeometries[MASK_MESH].mNumberOfIndices = numberOfIndices;
	mGeometries[MASK_MESH].mGeometryType = GL_TRIANGLE_STRIP;

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
	mTempVertices[0].s = flip_x ? 1.0f : 0.0f;
	mTempVertices[0].t = flip_y ? 1.0f : 0.0f;
	mTempVertices[0].x = 2.0f*(0.0f * parent->getXSize() + parent->getX()) - 1.0f;
	mTempVertices[0].y = 2.0f*(0.0f * parent->getYSize() + parent->getY()) - 1.0f;

	mTempVertices[1].r = 1.0f;
	mTempVertices[1].g = 1.0f;
	mTempVertices[1].b = 1.0f;
	mTempVertices[1].a = 1.0f;
	mTempVertices[1].s = flip_x ? 0.0f : 1.0f;
	mTempVertices[1].t = flip_y ? 1.0f : 0.0f;
	mTempVertices[1].x = 2.0f*(1.0f * parent->getXSize() + parent->getX()) - 1.0f;
	mTempVertices[1].y = 2.0f*(0.0f * parent->getYSize() + parent->getY()) - 1.0f;

	mTempVertices[2].r = 1.0f;
	mTempVertices[2].g = 1.0f;
	mTempVertices[2].b = 1.0f;
	mTempVertices[2].a = 1.0f;
	mTempVertices[2].s = flip_x ? 0.0f : 1.0f;
	mTempVertices[2].t = flip_y ? 0.0f : 1.0f;
	mTempVertices[2].x = 2.0f*(1.0f * parent->getXSize() + parent->getX()) - 1.0f;
	mTempVertices[2].y = 2.0f*(1.0f *parent->getYSize() + parent->getY()) - 1.0f;

	mTempVertices[3].r = 1.0f;
	mTempVertices[3].g = 1.0f;
	mTempVertices[3].b = 1.0f;
	mTempVertices[3].a = 1.0f;
	mTempVertices[3].s = flip_x ? 1.0f : 0.0f;
	mTempVertices[3].t = flip_y ? 0.0f : 1.0f;
	mTempVertices[3].x = 2.0f*(0.0f * parent->getXSize() + parent->getX()) - 1.0f;
	mTempVertices[3].y = 2.0f*(1.0f * parent->getYSize() + parent->getY()) - 1.0f;
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

void sgct_core::CorrectionMesh::clamp(float & val, const float max, const float min)
{
	if (val > max)
		val = max;
	else if (val < min)
		val = min;
}

/*!
Render the final mesh where for mapping the frame buffer to the screen
\param mask to enable mask texture mode
*/
void sgct_core::CorrectionMesh::render(const MeshType & mt)
{
	//for test
	//glDisable(GL_CULL_FACE);

	if(sgct::SGCTSettings::instance()->getShowWarpingWireframe())
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	CorrectionMeshGeometry * geomPtr = &mGeometries[mt];

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

	if (sgct::SGCTSettings::instance()->getShowWarpingWireframe())
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//for test
	//glEnable(GL_CULL_FACE);
}

/*!
Parse hint from string to enum.
*/
sgct_core::CorrectionMesh::MeshHint sgct_core::CorrectionMesh::parseHint(const std::string & hintStr)
{
	if (hintStr.empty())
		return NO_HINT;
	
	//transform to lowercase
	std::string str(hintStr);
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);

	sgct_core::CorrectionMesh::MeshHint hint = NO_HINT;
	if (str.compare("domeprojection") == 0)
		hint = DOMEPROJECTION_HINT;
	else if(str.compare("scalable") == 0)
		hint = SCALEABLE_HINT;
	else if (str.compare("sciss") == 0)
		hint = SCISS_HINT;
	else if (str.compare("skyskan") == 0)
		hint = SKYSKAN_HINT;
	else
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "CorrectionMesh: hint '%s' is invalid!\n", hintStr.c_str());
	}

	return hint;
}
