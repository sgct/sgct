/*************************************************************************
Copyright (c) 2012 Miroslav Andel, Linköping University.
All rights reserved.

Original Authors:
Miroslav Andel, Alexander Fridlund

For any questions or information about the SGCT project please contact: miroslav.andel@liu.se

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to
Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************/

#define BUFFER_OFFSET(i) (reinterpret_cast<void*>(i))
#define MAX_LINE_LENGTH 256

#include <stdio.h>
#include <fstream>
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/CorrectionMesh.h"

core_sgct::CorrectionMesh::CorrectionMesh()
{
	mVertices = NULL;
	mFaces = NULL;

	mXSize = 1.0;
	mYSize = 1.0;
	mXOffset = 0.0;
	mYOffset = 0.0;

	mOrthoCoords[0] = 0.0;
	mOrthoCoords[1] = 1.0;
	mOrthoCoords[2] = 0.0;
	mOrthoCoords[3] = 1.0;

	mMeshData[0] = 0;
	mMeshData[1] = 0;

	hasMesh = false;
}

core_sgct::CorrectionMesh::~CorrectionMesh()
{
	if( hasMesh )
		glDeleteBuffers(2, &mMeshData[0]);
}

void core_sgct::CorrectionMesh::setViewportPointers(double vpXSize, double vpYSize, double vpXPos, double vpYPos)
{
	mXSize = vpXSize;
	mYSize = vpYSize;
	mXOffset = vpXPos;
	mYOffset = vpYPos;
}

bool core_sgct::CorrectionMesh::readAndGenerateMesh(const char * meshPath)
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

	double x,y,s,t;
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
			if( sscanf_s(lineBuffer, "%lf %lf %u %lf %lf", &x, &y, &intensity, &s, &t) == 5 )
#else
			if( sscanf(lineBuffer, "%lf %lf %u %lf %lf", &x, &y, &intensity, &s, &t) == 5 )
#endif
			{
				if( mVertices != NULL && mResolution[0] != 0 && mResolution[1] != 0 )
				{
					mVertices[ numOfVerticesRead ].x = (x/static_cast<double>(mResolution[0])) * mXSize + mXOffset;
					mVertices[ numOfVerticesRead ].y = (y/static_cast<double>(mResolution[1])) * mYSize + mYOffset;
					mVertices[ numOfVerticesRead ].r = static_cast<unsigned char>(intensity);
					mVertices[ numOfVerticesRead ].g = static_cast<unsigned char>(intensity);
					mVertices[ numOfVerticesRead ].b = static_cast<unsigned char>(intensity);
					mVertices[ numOfVerticesRead ].s0 = (1.0 - t) * mXSize + mXOffset;
					mVertices[ numOfVerticesRead ].t0 = (1.0 - s) * mYSize + mYOffset;
					mVertices[ numOfVerticesRead ].s1 = (1.0 - t) * mXSize + mXOffset;
					mVertices[ numOfVerticesRead ].t1 = (1.0 - s) * mYSize + mYOffset;

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
					mFaces[ numOfFacesRead * 3] = static_cast<unsigned short>(a);
					mFaces[ numOfFacesRead * 3 + 1 ] = static_cast<unsigned short>(b);
					mFaces[ numOfFacesRead * 3 + 2 ] = static_cast<unsigned short>(c);
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
				}

#if (_MSC_VER >= 1400) //visual studio 2005 or later
				else if( sscanf_s(lineBuffer, "FACES %u", &mNumberOfFaces) == 1 )
#else
				else if( sscanf(lineBuffer, "FACES %u", &mNumberOfFaces) == 1 )
#endif
					mFaces = new unsigned short[ mNumberOfFaces * 3 ];

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

	createMesh();
	
	cleanUp();

	//indicate that a mesh is loaded and can be used
	hasMesh = true;

	sgct::MessageHandler::Instance()->print("Correction mesh read successfully! Vertices=%u, Faces=%u.\n", numOfVerticesRead, numOfFacesRead);

	return true;
}

void core_sgct::CorrectionMesh::createMesh()
{
	//sgct::TextureManager::Instance()->loadTexure(mTexId, "Alignment Grid", "grid2.png", true, 0);

	// generate a new VBO and get the associated ID
	glGenBuffers(2, &mMeshData[0]);

	// bind VBO & upload
	glBindBuffer(GL_ARRAY_BUFFER, mMeshData[Vertex]);
	glBufferData(GL_ARRAY_BUFFER, mNumberOfVertices * sizeof(CorrectionMeshVertex), mVertices, GL_STATIC_DRAW);
	
	//glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_DOUBLE, sizeof(CorrectionMeshVertex), BUFFER_OFFSET(0));
	
	glClientActiveTexture(GL_TEXTURE0);
	//glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_DOUBLE, sizeof(CorrectionMeshVertex), BUFFER_OFFSET(16));
	
	glClientActiveTexture(GL_TEXTURE1);
	//glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_DOUBLE, sizeof(CorrectionMeshVertex), BUFFER_OFFSET(32));
	
	//glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(3, GL_UNSIGNED_BYTE, sizeof(CorrectionMeshVertex), BUFFER_OFFSET(48));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mMeshData[Index]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mNumberOfFaces*3*sizeof(unsigned short), mFaces, GL_STATIC_DRAW);

	/*glDisableClientState(GL_COLOR_ARRAY);
	glClientActiveTexture(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);*/
	
	/*
	glBindBuffer(GL_ARRAY_BUFFER, mMeshData[Color]);
	glBufferData(GL_ARRAY_BUFFER, mNumberOfVertices*3*sizeof(unsigned char), mColors, GL_STATIC_DRAW);
	glColorPointer(3,GL_UNSIGNED_BYTE,0,0);

	glBindBuffer(GL_ARRAY_BUFFER, mMeshData[Texture0]);
	glBufferData(GL_ARRAY_BUFFER, mNumberOfVertices*2*sizeof(double), mTexCoords, GL_STATIC_DRAW);
	glClientActiveTexture(GL_TEXTURE0);
	glTexCoordPointer(2,GL_DOUBLE,0,0);
	
	glBindBuffer(GL_ARRAY_BUFFER, mMeshData[Texture1]);
	glBufferData(GL_ARRAY_BUFFER, mNumberOfVertices*2*sizeof(double), mTexCoords, GL_STATIC_DRAW);
	glClientActiveTexture(GL_TEXTURE1);
	glTexCoordPointer(2,GL_DOUBLE,0,0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mMeshData[Index]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mNumberOfFaces*3*sizeof(unsigned short), mFaces, GL_STATIC_DRAW);
	*/

	//unbind
	glBindBufferARB(GL_ARRAY_BUFFER, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);

	//mMeshData = glGenLists(1);
	//glNewList(mMeshData, GL_COMPILE);

	/*glEnable(GL_TEXTURE_2D);
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByIndex(texId) );
	
	//draw triangles
	unsigned int colorIndex;
	unsigned int vertexIndex;
	for(unsigned int i=0; i<mNumberOfFaces; i++)
	{
		glBegin(GL_TRIANGLES);
			colorIndex = mFaces[i*3] * 3;
			vertexIndex = mFaces[i*3] * 2;
			glColor3ub( mColors[ colorIndex ], mColors[ colorIndex ], mColors[ colorIndex ]);
			glTexCoord2d( mTexCoords[ vertexIndex ], mTexCoords[ vertexIndex + 1] );
			glVertex2f( mVertices[ vertexIndex ], mVertices[ vertexIndex + 1] );
			
			colorIndex = mFaces[i*3 + 1] * 3;
			vertexIndex = mFaces[i*3 + 1] * 2;
			glColor3ub( mColors[ colorIndex ], mColors[ colorIndex ], mColors[ colorIndex ]);
			glTexCoord2d( mTexCoords[ vertexIndex ], mTexCoords[ vertexIndex + 1] );
			glVertex2f( mVertices[ vertexIndex ], mVertices[ vertexIndex + 1] );
			
			colorIndex = mFaces[i*3 + 2] * 3;
			vertexIndex = mFaces[i*3 + 2] * 2;
			glColor3ub( mColors[ colorIndex ], mColors[ colorIndex ], mColors[ colorIndex ]);
			glTexCoord2d( mTexCoords[ vertexIndex ], mTexCoords[ vertexIndex + 1] );
			glVertex2f( mVertices[ vertexIndex ], mVertices[ vertexIndex + 1] );
		glEnd();
	}

	glDisable(GL_TEXTURE_2D);

	//draw lines
	unsigned int colorIndex;
	unsigned int vertexIndex;

	glLineWidth(1.0);
	for(unsigned int i=0; i<mNumberOfFaces; i++)
	{
		glBegin(GL_LINE_LOOP);
			colorIndex = mFaces[i*3] * 3;
			vertexIndex = mFaces[i*3] * 2;
			glColor3ub( mColors[ colorIndex ], mColors[ colorIndex ], mColors[ colorIndex ]);
			glVertex2f( mVertices[ vertexIndex ], mVertices[ vertexIndex + 1] );
			
			colorIndex = mFaces[i*3 + 1] * 3;
			vertexIndex = mFaces[i*3 + 1] * 2;
			glColor3ub( mColors[ colorIndex ], mColors[ colorIndex ], mColors[ colorIndex ]);
			glVertex2f( mVertices[ vertexIndex ], mVertices[ vertexIndex + 1] );
			
			colorIndex = mFaces[i*3 + 2] * 3;
			vertexIndex = mFaces[i*3 + 2] * 2;
			glColor3ub( mColors[ colorIndex ], mColors[ colorIndex ], mColors[ colorIndex ]);
			glVertex2f( mVertices[ vertexIndex ], mVertices[ vertexIndex + 1] );
		glEnd();
	}

	
	//draw points
	glPointSize(1.0);
	glBegin(GL_POINTS);
	for(unsigned int i=0; i<mNumberOfVertices; i++)
	{
		glColor3ub( mColors[ i*3 ], mColors[ i*3 + 1 ], mColors[ i*3 + 2 ]);
		glColor3ub( 255, 0, 0);
		glVertex2f( mVertices[ i*2 ], mVertices[ i*2 + 1] );
	}
	glEnd();
	
	*/
	//glEndList();
}

void core_sgct::CorrectionMesh::render()
{
	if(hasMesh)
		renderMesh();
	else
	{
		glColor4f(1.0f,1.0f,1.0f,1.0f);
		glBegin(GL_QUADS);
		glMultiTexCoord2d(GL_TEXTURE0, 0.0*mXSize + mXOffset, 0.0*mYSize + mYOffset);
		glMultiTexCoord2d(GL_TEXTURE1, 0.0*mXSize + mXOffset, 0.0*mYSize + mYOffset);
		//glTexCoord2d(0.0*mXSize + mXOffset, 0.0*mYSize + mYOffset);
		glVertex2d(0.0*mXSize + mXOffset, 0.0*mYSize + mYOffset);

		glMultiTexCoord2d(GL_TEXTURE0, 0.0*mXSize + mXOffset, 1.0*mYSize + mYOffset);
		glMultiTexCoord2d(GL_TEXTURE1, 0.0*mXSize + mXOffset, 1.0*mYSize + mYOffset);
		//glTexCoord2d(0.0*mXSize + mXOffset, 1.0*mYSize + mYOffset);
		glVertex2d(0.0*mXSize + mXOffset, 1.0*mYSize + mYOffset);

		glMultiTexCoord2d(GL_TEXTURE0, 1.0*mXSize + mXOffset, 1.0*mYSize + mYOffset);
		glMultiTexCoord2d(GL_TEXTURE1, 1.0*mXSize + mXOffset, 1.0*mYSize + mYOffset);
		//glTexCoord2d(1.0*mXSize + mXOffset, 1.0*mYSize + mYOffset);
		glVertex2d(1.0*mXSize + mXOffset, 1.0*mYSize + mYOffset);

		glMultiTexCoord2d(GL_TEXTURE0, 1.0*mXSize + mXOffset, 0.0*mYSize + mYOffset);
		glMultiTexCoord2d(GL_TEXTURE1, 1.0*mXSize + mXOffset, 0.0*mYSize + mYOffset);
		//glTexCoord2d(1.0*mXSize + mXOffset, 0.0*mYSize + mYOffset);
		glVertex2d(1.0*mXSize + mXOffset, 0.0*mYSize + mYOffset);
		glEnd();
	}
}

void core_sgct::CorrectionMesh::cleanUp()
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

void core_sgct::CorrectionMesh::renderMesh()
{
	//glCallList( mMeshData );

	/*glActiveTexture(GL_TEXTURE0);
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByIndex(mTexId) );
	glEnable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture( GL_TEXTURE_2D, sgct::TextureManager::Instance()->getTextureByIndex(mTexId) );
	glEnable(GL_TEXTURE_2D);*/

	glBindBuffer(GL_ARRAY_BUFFER, mMeshData[Vertex]);

	glEnableClientState(GL_VERTEX_ARRAY);
	//glVertexPointer(2, GL_DOUBLE, sizeof(CorrectionMeshVertex), BUFFER_OFFSET(0));
	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	//glTexCoordPointer(2, GL_DOUBLE, sizeof(CorrectionMeshVertex), BUFFER_OFFSET(16));
	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	//glTexCoordPointer(2, GL_DOUBLE, sizeof(CorrectionMeshVertex), BUFFER_OFFSET(32));
	glEnableClientState(GL_COLOR_ARRAY);
	//glColorPointer(3, GL_UNSIGNED_BYTE, sizeof(CorrectionMeshVertex), BUFFER_OFFSET(48));
	glEnableClientState(GL_INDEX_ARRAY);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mMeshData[Index]);

	glDrawElements(GL_TRIANGLES, mNumberOfFaces*3, GL_UNSIGNED_SHORT, NULL);

	//according to nvidia this function can be optimized but it makes no difference
	//glDrawRangeElements(GL_TRIANGLES, 0, mNumberOfFaces*3, mNumberOfFaces*3, GL_UNSIGNED_SHORT, NULL);

	glDisableClientState(GL_INDEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glClientActiveTexture(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}