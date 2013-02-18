/*************************************************************************
Copyright (c) 2012 Miroslav Andel
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
	mVertexList = NULL;
	mFaces = NULL;

	mXSize = 1.0f;
	mYSize = 1.0f;
	mXOffset = 0.0f;
	mYOffset = 0.0f;

	mOrthoCoords[0] = 0.0;
	mOrthoCoords[1] = 1.0;
	mOrthoCoords[2] = 0.0;
	mOrthoCoords[3] = 1.0;

	mMeshData[0] = 0;
	mMeshData[1] = 0;

	hasMesh = false;
}

sgct_core::CorrectionMesh::~CorrectionMesh()
{
	if( hasMesh )
	{
		if(ClusterManager::Instance()->getMeshImplementation() == ClusterManager::VBO_INDEX)
			glDeleteBuffers(2, &mMeshData[0]);
		else if(ClusterManager::Instance()->getMeshImplementation() == ClusterManager::VBO_ARRAY)
			glDeleteBuffers(1, &mMeshData[0]);
		else
			glDeleteLists(mMeshData[0], 1);
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
	    sgct::MessageHandler::Instance()->print("Error: cannot read mesh path (string is null)!\n", meshPath);
		return false;
	}

	sgct::MessageHandler::Instance()->print("Reading mesh data from '%s'.\n", meshPath);

	FILE * meshFile;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	if( fopen_s(&meshFile, meshPath, "r") != 0 )
	{
		sgct::MessageHandler::Instance()->print("Failed to open mesh file!\n");
		return false;
	}
#else
	meshFile = fopen(meshPath, "r");
	if( meshFile == NULL )
	{
		sgct::MessageHandler::Instance()->print("Failed to open mesh file!\n");
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
					mVertices[ numOfVerticesRead ].s0 = (1.0f - t) * mXSize + mXOffset;
					mVertices[ numOfVerticesRead ].t0 = (1.0f - s) * mYSize + mYOffset;
					mVertices[ numOfVerticesRead ].s1 = (1.0f - t) * mXSize + mXOffset;
					mVertices[ numOfVerticesRead ].t1 = (1.0f - s) * mYSize + mYOffset;

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
					if(ClusterManager::Instance()->getMeshImplementation() == ClusterManager::VBO_ARRAY)
					{
						mVertexList = new CorrectionMeshVertex[ mNumberOfFaces * 3 ];
						memset(mVertexList, 0, mNumberOfFaces * 3 * sizeof(CorrectionMeshVertex));
					}
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
		sgct::MessageHandler::Instance()->print("Mesh data geometry incorrect!");
		return false;
	}

	fclose( meshFile );

	if(ClusterManager::Instance()->getMeshImplementation() == ClusterManager::VBO_ARRAY)
	{
		for(unsigned int i=0; i<mNumberOfFaces; i++)
			for(unsigned int j=0; j<3; j++)
			{
				mVertexList[i*3 + j].x = mVertices[mFaces[i*3 + j]].x;
				mVertexList[i*3 + j].y = mVertices[mFaces[i*3 + j]].y;
				mVertexList[i*3 + j].s0 = mVertices[mFaces[i*3 + j]].s0;
				mVertexList[i*3 + j].t0 = mVertices[mFaces[i*3 + j]].t0;
				mVertexList[i*3 + j].s1 = mVertices[mFaces[i*3 + j]].s1;
				mVertexList[i*3 + j].t1 = mVertices[mFaces[i*3 + j]].t1;
				mVertexList[i*3 + j].r = mVertices[mFaces[i*3 + j]].r;
				mVertexList[i*3 + j].g = mVertices[mFaces[i*3 + j]].g;
				mVertexList[i*3 + j].b = mVertices[mFaces[i*3 + j]].b;
			}
	}

	createMesh();

	cleanUp();

	//indicate that a mesh is loaded and can be used
	hasMesh = true;

	sgct::MessageHandler::Instance()->print("Correction mesh read successfully! Vertices=%u, Faces=%u.\n", numOfVerticesRead, numOfFacesRead);

	return true;
}

void sgct_core::CorrectionMesh::createMesh()
{
	//sgct::MessageHandler::Instance()->print("Uploading mesh data...\n");

	if( ClusterManager::Instance()->getMeshImplementation() == ClusterManager::VBO_INDEX )
	{
		glGenBuffers(2, &mMeshData[0]);

		glBindBuffer(GL_ARRAY_BUFFER, mMeshData[Vertex]);
		glBufferData(GL_ARRAY_BUFFER, mNumberOfVertices * sizeof(CorrectionMeshVertex), mVertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mMeshData[Index]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mNumberOfFaces*3*sizeof(unsigned int), mFaces, GL_STATIC_DRAW);

		//unbind
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else if( ClusterManager::Instance()->getMeshImplementation() == ClusterManager::VBO_ARRAY )
	{
		glGenBuffers(1, &mMeshData[0]);
		
		glBindBuffer(GL_ARRAY_BUFFER, mMeshData[Vertex]);
		glBufferData(GL_ARRAY_BUFFER, mNumberOfFaces * 3 * sizeof(CorrectionMeshVertex), mVertexList, GL_STATIC_DRAW);

		//unbind
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	else
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
				glMultiTexCoord2f( GL_TEXTURE0, mVertices[mFaces[i*3 + j]].s0, mVertices[mFaces[i*3 + j]].t0 );
				glMultiTexCoord2f( GL_TEXTURE1, mVertices[mFaces[i*3 + j]].s1, mVertices[mFaces[i*3 + j]].t1 );
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
				glMultiTexCoord2f( GL_TEXTURE0, mVertices[mFaces[i*3 + j]].s0, mVertices[mFaces[i*3 + j]].t0 );
				glMultiTexCoord2f( GL_TEXTURE1, mVertices[mFaces[i*3 + j]].s1, mVertices[mFaces[i*3 + j]].t1 );
				glVertex2f( mVertices[mFaces[i*3 + j]].x, mVertices[mFaces[i*3 + j]].y );
			}
		glEnd();
#endif
		glEndList();
	}
}

void sgct_core::CorrectionMesh::render()
{
	if(hasMesh)
		renderMesh();
	else
	{
		/*
			counter clock wise winding
		*/
		
		glColor4f(1.0f,1.0f,1.0f,1.0f);
		glBegin(GL_QUADS);
		glMultiTexCoord2f(GL_TEXTURE0, 0.0f*mXSize + mXOffset, 0.0f*mYSize + mYOffset);
		glMultiTexCoord2f(GL_TEXTURE1, 0.0f*mXSize + mXOffset, 0.0f*mYSize + mYOffset);
		//glTexCoord2f(0.0f*mXSize + mXOffset, 0.0f*mYSize + mYOffset);
		glVertex2f(0.0f*mXSize + mXOffset, 0.0f*mYSize + mYOffset);

		glMultiTexCoord2f(GL_TEXTURE0, 1.0f*mXSize + mXOffset, 0.0f*mYSize + mYOffset);
		glMultiTexCoord2f(GL_TEXTURE1, 1.0f*mXSize + mXOffset, 0.0f*mYSize + mYOffset);
		//glTexCoord2f(1.0f*mXSize + mXOffset, 0.0f*mYSize + mYOffset);
		glVertex2f(1.0f*mXSize + mXOffset, 0.0f*mYSize + mYOffset);

		glMultiTexCoord2f(GL_TEXTURE0, 1.0f*mXSize + mXOffset, 1.0f*mYSize + mYOffset);
		glMultiTexCoord2f(GL_TEXTURE1, 1.0f*mXSize + mXOffset, 1.0f*mYSize + mYOffset);
		//glTexCoord2f(1.0f*mXSize + mXOffset, 1.0f*mYSize + mYOffset);
		glVertex2f(1.0f*mXSize + mXOffset, 1.0f*mYSize + mYOffset);

		glMultiTexCoord2f(GL_TEXTURE0, 0.0f*mXSize + mXOffset, 1.0f*mYSize + mYOffset);
		glMultiTexCoord2f(GL_TEXTURE1, 0.0f*mXSize + mXOffset, 1.0f*mYSize + mYOffset);
		//glTexCoord2f(0.0f*mXSize + mXOffset, 1.0f*mYSize + mYOffset);
		glVertex2f(0.0f*mXSize + mXOffset, 1.0f*mYSize + mYOffset);
		glEnd();
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

	if( mVertexList != NULL )
	{
		delete [] mVertexList;
		mVertexList = NULL;
	}

	if( mFaces != NULL )
	{
		delete [] mFaces;
		mFaces = NULL;
	}
}

void sgct_core::CorrectionMesh::renderMesh()
{
	if( ClusterManager::Instance()->getMeshImplementation() != ClusterManager::DISPLAY_LIST )
	{
		glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
		glBindBuffer(GL_ARRAY_BUFFER, mMeshData[Vertex]);
	
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, sizeof(CorrectionMeshVertex), BUFFER_OFFSET(0));
	
		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(CorrectionMeshVertex), BUFFER_OFFSET(8));
	
		glClientActiveTexture(GL_TEXTURE1);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(CorrectionMeshVertex), BUFFER_OFFSET(16));

		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(3, GL_UNSIGNED_BYTE, sizeof(CorrectionMeshVertex), BUFFER_OFFSET(24));
		
		if(ClusterManager::Instance()->getMeshImplementation() == ClusterManager::VBO_INDEX)
		{
			glEnableClientState(GL_INDEX_ARRAY);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mMeshData[Index]);

			glDrawElements(GL_TRIANGLES, mNumberOfFaces*3, GL_UNSIGNED_INT, NULL);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
		else
		{
			glDrawArrays(GL_TRIANGLES, 0, mNumberOfFaces*3);
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glPopClientAttrib();
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
