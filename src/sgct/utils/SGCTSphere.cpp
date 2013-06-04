/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <glm/gtc/constants.hpp>
#include "../include/sgct/utils/SGCTSphere.h"
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/Engine.h"

/*!
	This constructor requires a valid openGL contex 
*/
sgct_utils::SGCTSphere::SGCTSphere(float radius, unsigned int segments)
{
	mVerts = NULL;
	mIndices = NULL;
	mVBO[Vertex] = GL_FALSE;
	mVBO[Index] = GL_FALSE;
	mVAO = GL_FALSE;

	mInternalDrawFn = &SGCTSphere::drawVBO;

	unsigned int i, j;
	float x, y, z, R;
	double theta, phi;
	unsigned int vsegs, hsegs;
	
	vsegs = segments;
	if (vsegs < 2)
		vsegs = 2;
	
	hsegs = vsegs*2;
	mNumberOfVertices = 1 + (vsegs-1)*(hsegs+1) + 1; // top + middle + bottom
	mNumberOfFaces = hsegs + (vsegs-2)*hsegs*2 + hsegs; // top + middle + bottom
	
	mVerts = new sgct_helpers::SGCTVertexData[mNumberOfVertices];
	memset(mVerts, 0, mNumberOfVertices * sizeof(sgct_helpers::SGCTVertexData));

	mIndices = new unsigned int[mNumberOfFaces * 3];
	memset(mIndices, 0, mNumberOfFaces * 3 * sizeof(unsigned int));

	// First vertex: top pole (+y is "up" in object local coords)
	addVertexData( 0, 
		0.5f, 1.0f,
		0.0f, 1.0f, 0.0f, //normal
		0.0f, radius, 0.0f);

	// Last vertex: bottom pole
	addVertexData( mNumberOfVertices-1, 
		0.5f, 0.0f,
		0.0f, -1.0f, 0.0f, //normal
		0.0f, -radius, 0.0f);

	// All other vertices:
    // vsegs-1 latitude rings of hsegs+1 vertices each (duplicates at texture seam s=0 / s=1)
    for(j=0; j<vsegs-1; j++)
	{ // vsegs-1 latitude rings of vertices
        theta = (static_cast<double>(j+1) / static_cast<double>(vsegs)) * glm::pi<double>();
        y = static_cast<float>( cos(theta) );
        R = static_cast<float>( sin(theta) );
        
		for (i=0; i<=hsegs; i++)
		{ // hsegs+1 vertices in each ring (duplicate for texcoords)
            phi = (static_cast<double>(i) / static_cast<double>(hsegs)) * 2.0 * glm::pi<double>();
            x = R * static_cast<float>( cos(phi) );
            z = R * static_cast<float>( sin(phi) );

			addVertexData( 1+j*(hsegs+1)+i, 
				static_cast<float>( i )/static_cast<float>( hsegs ), //s
				1.0f - static_cast<float>( j+1 )/static_cast<float>( vsegs ), //t
				x, y, z, //normals
				radius * x, radius * y, radius * z);
        }
    }

	// The index array: triplets of integers, one for each triangle
    // Top cap
    for(i=0; i<hsegs; i++)
	{
        mIndices[3*i]=0;
        mIndices[3*i+1]=1+i;
        mIndices[3*i+2]=2+i;
    }
    // Middle part (possibly empty if vsegs=2)
    for(j=0; j<vsegs-2; j++)
	{
        for(i=0; i<hsegs; i++)
		{
            unsigned int base = 3*(hsegs + 2*(j*hsegs + i));
            unsigned int i0 = 1 + j*(hsegs+1) + i;
            mIndices[base] = i0;
            mIndices[base+1] = i0+hsegs+1;
            mIndices[base+2] = i0+1;
            mIndices[base+3] = i0+1;
            mIndices[base+4] = i0+hsegs+1;
            mIndices[base+5] = i0+hsegs+2;
        }
    }
    // Bottom cap
    for(i=0; i<hsegs; i++)
	{
        unsigned int base = 3*(hsegs + 2*(vsegs-2)*hsegs);
        mIndices[base+3*i] = mNumberOfVertices-1;
        mIndices[base+3*i+1] = mNumberOfVertices-2-i;
        mIndices[base+3*i+2] = mNumberOfVertices-3-i;
    }

	//create mesh
	createVBO();

	if( !sgct::Engine::checkForOGLErrors() ) //if error occured
	{
		sgct::MessageHandler::Instance()->print("SGCT Utils: Sphere creation error!\n");
		void cleanup();
	}

	//free data
	if( mVerts != NULL )
	{
		delete [] mVerts;
		mVerts = NULL;
	}

	if( mIndices != NULL )
	{
		delete [] mIndices;
		mIndices = NULL;
	}
}

sgct_utils::SGCTSphere::~SGCTSphere()
{
	cleanUp();
}

void sgct_utils::SGCTSphere::addVertexData(unsigned int pos,
		const float &s, const float &t,
		const float &nx, const float &ny, const float &nz,
		const float &x, const float &y, const float &z)
{
	mVerts[ pos ].set(s,t,nx,ny,nz,x,y,z);
}

void sgct_utils::SGCTSphere::cleanUp()
{
	//cleanup
	if(mVBO[0] != 0)
	{
		glDeleteBuffers(2, &mVBO[0]);
		mVBO[Vertex] = 0;
		mVBO[Index] = 0;
	}

	if(mVAO != 0)
	{
		glDeleteBuffers(1, &mVAO);
		mVAO = 0;
	}
}

/*!
	If openGL 3.3+ is used:
	layout 0 contains texture coordinates (vec2)
	layout 1 contains vertex normals (vec3)
	layout 2 contains vertex positions (vec3).
*/
void sgct_utils::SGCTSphere::draw()
{
	(this->*mInternalDrawFn)();
}

void sgct_utils::SGCTSphere::drawVBO()
{
	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_INDEX_ARRAY);
	
	glBindBuffer(GL_ARRAY_BUFFER, mVBO[Vertex]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVBO[Index]);
	
	glInterleavedArrays(GL_T2F_N3F_V3F, 0, 0);
    glDrawElements(GL_TRIANGLES, mNumberOfFaces * 3, GL_UNSIGNED_INT, 0);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glPopClientAttrib();
}

void sgct_utils::SGCTSphere::drawVAO()
{
	glBindVertexArray( mVAO );
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVBO[Index]);

	glDrawElements(GL_TRIANGLES, mNumberOfFaces * 3, GL_UNSIGNED_INT, 0);

	//unbind
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void sgct_utils::SGCTSphere::createVBO()
{
	if( !sgct::Engine::Instance()->isOGLPipelineFixed() )
	{
		mInternalDrawFn = &SGCTSphere::drawVAO;
		
		glGenVertexArrays(1, &mVAO);
		glBindVertexArray( mVAO );
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
	}
	
	glGenBuffers(2, &mVBO[0]);

	glBindBuffer(GL_ARRAY_BUFFER, mVBO[Vertex]);
	glBufferData(GL_ARRAY_BUFFER, mNumberOfVertices * sizeof(sgct_helpers::SGCTVertexData), mVerts, GL_STATIC_DRAW);

	if( !sgct::Engine::Instance()->isOGLPipelineFixed() )
	{
		glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof(sgct_helpers::SGCTVertexData), reinterpret_cast<void*>(0) ); //texcoords
		glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof(sgct_helpers::SGCTVertexData), reinterpret_cast<void*>(8) ); //normals
		glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, sizeof(sgct_helpers::SGCTVertexData), reinterpret_cast<void*>(20) ); //vert positions
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVBO[Index]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mNumberOfFaces * 3 * sizeof(unsigned int), mIndices, GL_STATIC_DRAW);

	//unbind
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if( !sgct::Engine::Instance()->isOGLPipelineFixed() )
	{
		glBindVertexArray( 0 );
	}
}
