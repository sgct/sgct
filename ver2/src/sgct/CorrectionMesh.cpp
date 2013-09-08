/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

//#define SGCT_SHOW_CORRECTION_MESH_WIREFRAME

#define BUFFER_OFFSET(i) (reinterpret_cast<void*>(i))
#define MAX_LINE_LENGTH 256

#include <stdio.h>
#include <fstream>
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/CorrectionMesh.h"
#include "../include/sgct/ClusterManager.h"
#include <cstring>

sgct_core::CorrectionMesh::CorrectionMesh()
{
	mVertices = NULL;
	mFaces = NULL;

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
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "CorrectionMesh: Releasing correction mesh OpenGL data...\n");
	
	if(ClusterManager::instance()->getMeshImplementation() == ClusterManager::DISPLAY_LIST && mMeshData[0] != GL_FALSE)
		glDeleteLists(mMeshData[0], 1);
	else if(mMeshData[0] != 0)
	{
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
	if( meshPath == NULL )
	{
	    setupSimpleMesh();
		return false;
	}

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO,
		"CorrectionMesh: Reading mesh data from '%s'.\n", meshPath);

	FILE * meshFile;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if( fopen_s(&meshFile, meshPath, "r") != 0 )
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

	createMesh();

	cleanUp();

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "CorrectionMesh: Correction mesh read successfully! Vertices=%u, Faces=%u.\n", numOfVerticesRead, numOfFacesRead);

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
	mVertices[0].x = 0.0f*mXSize + mXOffset;
	mVertices[0].y = 0.0f*mYSize + mYOffset;

	mVertices[1].r = 255;
	mVertices[1].g = 255;
	mVertices[1].b = 255;
	mVertices[1].s = 1.0f * mXSize + mXOffset;
	mVertices[1].t = 0.0f * mYSize + mYOffset;
	mVertices[1].x = 1.0f*mXSize + mXOffset;
	mVertices[1].y = 0.0f*mYSize + mYOffset;

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
	//sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Uploading mesh data...\n");

	if( ClusterManager::instance()->getMeshImplementation() == ClusterManager::DISPLAY_LIST )
	{
		mMeshData[Vertex] = glGenLists(1);
		glNewList(mMeshData[Vertex], GL_COMPILE);

#ifdef SGCT_SHOW_CORRECTION_MESH_WIREFRAME
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
#else
		glBegin(GL_TRIANGLES);
		for(unsigned int i=0; i<mNumberOfFaces; i++)
			for(unsigned int j=0; j<3; j++)
			{
				glColor3ub( mVertices[mFaces[i*3 + j]].r, mVertices[mFaces[i*3 + j]].g, mVertices[mFaces[i*3 + j]].b );
				glTexCoord2f( mVertices[mFaces[i*3 + j]].s, mVertices[mFaces[i*3 + j]].t );
				glVertex2f( mVertices[mFaces[i*3 + j]].x, mVertices[mFaces[i*3 + j]].y );
			}
		glEnd();
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
		glBufferData(GL_ARRAY_BUFFER, mNumberOfVertices * sizeof(CorrectionMeshVertex), mVertices, GL_STATIC_DRAW);

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
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mNumberOfFaces*3*sizeof(unsigned int), mFaces, GL_STATIC_DRAW);

		//unbind
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		if(ClusterManager::instance()->getMeshImplementation() == ClusterManager::VAO)
			glBindVertexArray(0);
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
		glBindBuffer(GL_ARRAY_BUFFER, mMeshData[Vertex]);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, sizeof(CorrectionMeshVertex), BUFFER_OFFSET(0));

		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(CorrectionMeshVertex), BUFFER_OFFSET(8));

		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(3, GL_UNSIGNED_BYTE, sizeof(CorrectionMeshVertex), BUFFER_OFFSET(16));

		glEnableClientState(GL_INDEX_ARRAY);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mMeshData[Index]);

		glDrawElements(GL_TRIANGLES, mNumberOfFaces*3, GL_UNSIGNED_INT, NULL);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glPopClientAttrib();
	}
	else if( ClusterManager::instance()->getMeshImplementation() == ClusterManager::VAO )
	{
		glBindVertexArray(mMeshData[Array]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mMeshData[Index]);

		glDrawElements(GL_TRIANGLES, mNumberOfFaces*3, GL_UNSIGNED_INT, NULL);

		//unbind
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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