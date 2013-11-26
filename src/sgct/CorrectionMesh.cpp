/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

//#define SGCT_SHOW_CORRECTION_MESH_WIREFRAME

#define MAX_LINE_LENGTH 256

#include <stdio.h>
#include <fstream>
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/CorrectionMesh.h"
#include "../include/sgct/ClusterManager.h"
#include "../include/sgct/Engine.h"
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

sgct_core::CorrectionMesh::CorrectionMesh()
{
	mVertices = NULL;
	mFaces = NULL;

	mUseTriangleStrip = false;

	mXSize = 1.0f;
	mYSize = 1.0f;
	mXOffset = 0.0f;
	mYOffset = 0.0f;

	mOrthoCoords[0] = 0.0;
	mOrthoCoords[1] = 1.0;
	mOrthoCoords[2] = 0.0;
	mOrthoCoords[3] = 1.0;

	mMeshData[0] = GL_FALSE;
	mMeshData[1] = GL_FALSE;
	mMeshData[2] = GL_FALSE;
}

sgct_core::CorrectionMesh::~CorrectionMesh()
{	
	if(ClusterManager::instance()->getMeshImplementation() == ClusterManager::DISPLAY_LIST && mMeshData[0] != GL_FALSE)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Releasing correction mesh OpenGL data...\n");
		glDeleteLists(mMeshData[0], 1);
	}
	else if(mMeshData[0] != 0)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Releasing correction mesh OpenGL data...\n");
		if(ClusterManager::instance()->getMeshImplementation() == ClusterManager::VAO && mMeshData[2] != GL_FALSE)
			glDeleteVertexArrays(1, &mMeshData[2]);
		glDeleteBuffers(2, &mMeshData[0]);
	}
}

void sgct_core::CorrectionMesh::setViewportCoords(float vpXSize, float vpYSize, float vpXPos, float vpYPos)
{
	mXSize = vpXSize;
	mYSize = vpYSize;
	mXOffset = vpXPos;
	mYOffset = vpYPos;
}

bool sgct_core::CorrectionMesh::readAndGenerateMesh(const char * meshPath)
{
	if (meshPath == NULL)
	{
		setupSimpleMesh();
		return false;
	}

	int length = 0;

	while (meshPath[length] != '\0')
		length++;

	if (length > 4 && (strcmp(".sgc", &meshPath[length - 4]) == 0 || strcmp(".SGC", &meshPath[length - 4]) == 0))
	{
		if (!readAndGenerateScissMesh(meshPath))
		{
			setupSimpleMesh();
			return false;
		}
	}
	else if (length > 3 && (strcmp(".ol", &meshPath[length - 3]) == 0 || strcmp(".OL", &meshPath[length - 3]) == 0))
	{
		if( !readAndGenerateScalableMesh(meshPath))
		{
			setupSimpleMesh();
			return false;
		}
	}
	else
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Image error: Loading failed (bad filename: %s)\n", meshPath);
		setupSimpleMesh();
		return false;
	}

	return true;
}

bool sgct_core::CorrectionMesh::readAndGenerateScalableMesh(const char * meshPath)
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
				if( mVertices != NULL && mResolution[0] != 0 && mResolution[1] != 0 )
				{
					mVertices[ numOfVerticesRead ].x = (x/static_cast<float>(mResolution[0])) * mXSize + mXOffset;
					mVertices[ numOfVerticesRead ].y = (y/static_cast<float>(mResolution[1])) * mYSize + mYOffset;
					mVertices[ numOfVerticesRead ].r = static_cast<unsigned char>(intensity);
					mVertices[ numOfVerticesRead ].g = static_cast<unsigned char>(intensity);
					mVertices[ numOfVerticesRead ].b = static_cast<unsigned char>(intensity);
					mVertices[ numOfVerticesRead ].s = (1.0f - t) * mXSize + mXOffset;
					mVertices[ numOfVerticesRead ].t = (1.0f - s) * mYSize + mYOffset;

					numOfVerticesRead++;
				}
			}
#if (_MSC_VER >= 1400) //visual studio 2005 or later
			else if( sscanf_s(lineBuffer, "[ %u %u %u ]", &a, &b, &c) == 3 )
#else
			else if( sscanf(lineBuffer, "[ %u %u %u ]", &a, &b, &c) == 3 )
#endif
			{
				if( mFaces != NULL )
				{
					mFaces[ numOfFacesRead * 3] = a;
					mFaces[ numOfFacesRead * 3 + 1 ] = b;
					mFaces[ numOfFacesRead * 3 + 2 ] = c;
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
				if( sscanf_s(lineBuffer, "VERTICES %u", &mNumberOfVertices) == 1 )
#else
				if( sscanf(lineBuffer, "VERTICES %u", &mNumberOfVertices) == 1 )
#endif
				{
					mVertices = new CorrectionMeshVertex[ mNumberOfVertices ];
					memset(mVertices, 0, mNumberOfVertices * sizeof(CorrectionMeshVertex));
				}

#if (_MSC_VER >= 1400) //visual studio 2005 or later
				else if( sscanf_s(lineBuffer, "FACES %u", &mNumberOfFaces) == 1 )
#else
				else if( sscanf(lineBuffer, "FACES %u", &mNumberOfFaces) == 1 )
#endif
				{
					mFaces = new unsigned int[ mNumberOfFaces * 3 ];
					memset(mFaces, 0, mNumberOfFaces * 3 * sizeof(unsigned int));
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

	if( mNumberOfVertices != numOfVerticesRead || mNumberOfFaces != numOfFacesRead )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Incorrect mesh data geometry!");
		return false;
	}

	fclose( meshFile );

	mUseTriangleStrip = false;

	createMesh();

	cleanUp();

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: Correction mesh read successfully! Vertices=%u, Faces=%u.\n", numOfVerticesRead, numOfFacesRead);

	return true;
}

bool sgct_core::CorrectionMesh::readAndGenerateScissMesh(const char * meshPath)
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
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Corrupt file!\n");
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
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Corrupt file!\n");
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
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Corrupt file!\n");
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
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Corrupt file!\n");
		fclose(meshFile);
		return false;
	}
	else
	{
		mNumberOfVertices = size[0]*size[1];
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: Number of vertices = %u (%ux%u)\n", mNumberOfVertices, size[0], size[1]);
	}
	//read vertices
	SCISSTexturedVertex * texturedVertexList = new SCISSTexturedVertex[mNumberOfVertices];
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	retval = fread_s(texturedVertexList, sizeof(SCISSTexturedVertex)* mNumberOfVertices, sizeof(SCISSTexturedVertex), mNumberOfVertices, meshFile);
#else
	retval = fread(texturedVertexList, sizeof(SCISSTexturedVertex), mNumberOfVertices, meshFile);
#endif
	if (retval != mNumberOfVertices)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Corrupt file!\n");
		fclose(meshFile);
		return false;
	}

	//read number of indices
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	retval = fread_s(&mNumberOfFaces, sizeof(unsigned int), sizeof(unsigned int), 1, meshFile);
#else
	retval = fread(&mNumberOfFaces, sizeof(unsigned int), 1, meshFile);
#endif

	if (retval != 1)
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Corrupt file!\n");
		fclose(meshFile);
		return false;
	}
	else
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: Number of faces = %u\n", mNumberOfFaces);

	//read faces
	if (mNumberOfFaces > 0)
	{
		mFaces = new unsigned int[mNumberOfFaces];
#if (_MSC_VER >= 1400) //visual studio 2005 or later
		retval = fread_s(mFaces, sizeof(unsigned int)* mNumberOfFaces, sizeof(unsigned int), mNumberOfFaces, meshFile);
#else
		retval = fread(texturedVertexList, sizeof(unsigned int), mNumberOfFaces, meshFile);
#endif
		if (retval != mNumberOfFaces)
		{
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "CorrectionMesh: Corrupt file!\n");
			fclose(meshFile);
			return false;
		}
	}

	fclose(meshFile);

	//store all verts in sgct format
	mVertices = new CorrectionMeshVertex[mNumberOfVertices];
	for (unsigned int i = 0; i < mNumberOfVertices; i++)
	{
		mVertices[i].r = 255;
		mVertices[i].g = 255;
		mVertices[i].b = 255;

		mVertices[i].x = texturedVertexList[i].x * mXSize + mXOffset;
		mVertices[i].y = texturedVertexList[i].y * mYSize + mYOffset;

		mVertices[i].s = texturedVertexList[i].tx * mXSize + mXOffset;
		mVertices[i].t = 1.0f - texturedVertexList[i].ty * mYSize + mYOffset;

		/*fprintf(stderr, "Coords: %f %f %f\tTex: %f %f %f\n",
			texturedVertexList[i].x, texturedVertexList[i].y, texturedVertexList[i].z,
			texturedVertexList[i].tx, texturedVertexList[i].ty, texturedVertexList[i].tz);*/
	}

	//clean up
	delete [] texturedVertexList;
	texturedVertexList = NULL;

	mUseTriangleStrip = true;

	createMesh();
	cleanUp();

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: Correction mesh read successfully! Vertices=%u, Faces=%u.\n", mNumberOfVertices, mNumberOfFaces);
	
	return true;
}

void sgct_core::CorrectionMesh::setupSimpleMesh()
{
	mNumberOfVertices = 4;
	mNumberOfFaces = 2;

	mVertices = new CorrectionMeshVertex[ mNumberOfVertices ];
	memset(mVertices, 0, mNumberOfVertices * sizeof(CorrectionMeshVertex));

	mFaces = new unsigned int[ mNumberOfFaces * 3 ];
	memset(mFaces, 0, mNumberOfFaces * 3 * sizeof(unsigned int));

	mFaces[0] = 0;
	mFaces[1] = 1;
	mFaces[2] = 2;
	mFaces[3] = 2;
	mFaces[4] = 3;
	mFaces[5] = 0;

	mVertices[0].r = 255;
	mVertices[0].g = 255;
	mVertices[0].b = 255;
	mVertices[0].s = 0.0f * mXSize + mXOffset;
	mVertices[0].t = 0.0f * mYSize + mYOffset;
	mVertices[0].x = 0.0f * mXSize + mXOffset;
	mVertices[0].y = 0.0f * mYSize + mYOffset;

	mVertices[1].r = 255;
	mVertices[1].g = 255;
	mVertices[1].b = 255;
	mVertices[1].s = 1.0f * mXSize + mXOffset;
	mVertices[1].t = 0.0f * mYSize + mYOffset;
	mVertices[1].x = 1.0f * mXSize + mXOffset;
	mVertices[1].y = 0.0f * mYSize + mYOffset;

	mVertices[2].r = 255;
	mVertices[2].g = 255;
	mVertices[2].b = 255;
	mVertices[2].s = 1.0f * mXSize + mXOffset;
	mVertices[2].t = 1.0f * mYSize + mYOffset;
	mVertices[2].x = 1.0f*mXSize + mXOffset;
	mVertices[2].y = 1.0f*mYSize + mYOffset;

	mVertices[3].r = 255;
	mVertices[3].g = 255;
	mVertices[3].b = 255;
	mVertices[3].s = 0.0f * mXSize + mXOffset;
	mVertices[3].t = 1.0f * mYSize + mYOffset;
	mVertices[3].x = 0.0f*mXSize + mXOffset;
	mVertices[3].y = 1.0f*mYSize + mYOffset;

	createMesh();

	cleanUp();
}

void sgct_core::CorrectionMesh::createMesh()
{
	/*sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Uploading mesh data (type=%d)...\n",
		ClusterManager::instance()->getMeshImplementation());*/

	if (ClusterManager::instance()->getMeshImplementation() == ClusterManager::DISPLAY_LIST)
	{
		mMeshData[Vertex] = glGenLists(1);
		glNewList(mMeshData[Vertex], GL_COMPILE);

#ifdef SGCT_SHOW_CORRECTION_MESH_WIREFRAME
		if(mUseTriangleStrip)
		{
			glBegin(GL_LINE_STRIP);
			for(unsigned int i=0; i<mNumberOfFaces; i++)
			{
				glColor3ub( mVertices[mFaces[i]].r, 0, 255 - mVertices[mFaces[i]].r );
				glTexCoord2f( mVertices[mFaces[i]].s, mVertices[mFaces[i]].t );
				glVertex2f( mVertices[mFaces[i]].x, mVertices[mFaces[i]].y );
			}
			glEnd();
		}
		else
		{
			for(unsigned int i=0; i<mNumberOfFaces; i++)
			{
				glBegin(GL_LINE_LOOP);
				for(unsigned int j=0; j<3; j++)
				{
					glColor3ub( mVertices[mFaces[i*3 + j]].r, 0, 255 - mVertices[mFaces[i*3 + j]].r );
					glTexCoord2f( mVertices[mFaces[i*3 + j]].s, mVertices[mFaces[i*3 + j]].t );
					glVertex2f( mVertices[mFaces[i*3 + j]].x, mVertices[mFaces[i*3 + j]].y );
				}
				glEnd();
			}
		}
#else
		if (mUseTriangleStrip)
		{
			glBegin(GL_TRIANGLE_STRIP);
			for (unsigned int i = 0; i < mNumberOfFaces; i++)
			{
				glColor3ub(mVertices[mFaces[i]].r, mVertices[mFaces[i]].g, mVertices[mFaces[i]].b);
				glTexCoord2f(mVertices[mFaces[i]].s, mVertices[mFaces[i]].t);
				glVertex2f(mVertices[mFaces[i]].x, mVertices[mFaces[i]].y);
			}
			glEnd();
		}
		else
		{
			glBegin(GL_TRIANGLES);
			for (unsigned int i = 0; i < mNumberOfFaces; i++)
			for (unsigned int j = 0; j < 3; j++)
			{
				glColor3ub(mVertices[mFaces[i * 3 + j]].r, mVertices[mFaces[i * 3 + j]].g, mVertices[mFaces[i * 3 + j]].b);
				glTexCoord2f(mVertices[mFaces[i * 3 + j]].s, mVertices[mFaces[i * 3 + j]].t);
				glVertex2f(mVertices[mFaces[i * 3 + j]].x, mVertices[mFaces[i * 3 + j]].y);
			}
			glEnd();
		}
#endif
		glEndList();
	}
	else
	{
		if(ClusterManager::instance()->getMeshImplementation() == ClusterManager::VAO)
		{
			glGenVertexArrays(1, &mMeshData[Array]);
			glBindVertexArray(mMeshData[Array]);

			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Generating VAO: %d\n", mMeshData[Array]);
		}

		glGenBuffers(2, &mMeshData[0]);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Generating VBOs: %d %d\n", mMeshData[0], mMeshData[1]);

		glBindBuffer(GL_ARRAY_BUFFER, mMeshData[Vertex]);
		glBufferData(GL_ARRAY_BUFFER, mNumberOfVertices * sizeof(CorrectionMeshVertex), &mVertices[0], GL_STATIC_DRAW);

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

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mMeshData[Index]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mNumberOfFaces*3*sizeof(unsigned int), &mFaces[0], GL_STATIC_DRAW);

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
	if( mVertices != NULL )
	{
		delete [] mVertices;
		mVertices = NULL;
	}

	if( mFaces != NULL )
	{
		delete [] mFaces;
		mFaces = NULL;
	}
}

void sgct_core::CorrectionMesh::render()
{
	if( ClusterManager::instance()->getMeshImplementation() == ClusterManager::VBO )
	{
		glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

		glEnableClientState(GL_VERTEX_ARRAY);
		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);

		glBindBuffer(GL_ARRAY_BUFFER, mMeshData[Vertex]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mMeshData[Index]);
		
		glVertexPointer(2, GL_FLOAT, sizeof(CorrectionMeshVertex), reinterpret_cast<void*>(0));		
		glTexCoordPointer(2, GL_FLOAT, sizeof(CorrectionMeshVertex), reinterpret_cast<void*>(8));
		glColorPointer(3, GL_UNSIGNED_BYTE, sizeof(CorrectionMeshVertex), reinterpret_cast<void*>(16));

		mUseTriangleStrip ?
			glDrawElements(GL_TRIANGLE_STRIP, mNumberOfFaces, GL_UNSIGNED_INT, NULL) :
			glDrawElements(GL_TRIANGLES, mNumberOfFaces * 3, GL_UNSIGNED_INT, NULL);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glPopClientAttrib();
	}
	else if( ClusterManager::instance()->getMeshImplementation() == ClusterManager::VAO )
	{
		glBindVertexArray(mMeshData[Array]);
		mUseTriangleStrip ? 
			glDrawElements(GL_TRIANGLE_STRIP, mNumberOfFaces, GL_UNSIGNED_INT, NULL) :
			glDrawElements(GL_TRIANGLES, mNumberOfFaces * 3, GL_UNSIGNED_INT, NULL);
		glBindVertexArray(0);
	}
	else
	{
#ifdef SGCT_SHOW_CORRECTION_MESH_WIREFRAME
		glDisable(GL_TEXTURE_2D);
		glLineWidth(0.9);
#endif
		glCallList(mMeshData[Vertex]);
	}
}
