/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <glm/gtc/constants.hpp>
#include "../include/sgct/utils/SGCTDome.h"
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/Engine.h"

/*!
	This constructor requires a valid openGL contex 
*/
sgct_utils::SGCTDome::SGCTDome(float radius, float FOV, unsigned int segments, unsigned int rings, float tilt, unsigned int resolution)
{
	init(radius, FOV, segments, rings, tilt, resolution);
}

/*!
	This constructor requires a valid openGL contex
*/
sgct_utils::SGCTDome::SGCTDome(float radius, float FOV, unsigned int segments, unsigned int rings, unsigned int resolution)
{
	init(radius, FOV, segments, rings, 0.0f, resolution);
}

void sgct_utils::SGCTDome::init(float radius, float FOV, unsigned int segments, unsigned int rings, float tilt, unsigned int resolution)
{
	mVerts = NULL;
	mResolution = resolution;
	mRings = rings;
	mSegments = segments;
	mVBO = GL_FALSE;
	mVAO = GL_FALSE;

	mInternalDrawFn = &SGCTDome::drawVBO;

	if(mResolution < 4) //must be four or higher
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Warning: Dome geometry resolution must be higher than 4.\n");
		mResolution = 4;
	}

	mNumberOfVertices = (mSegments * ((mResolution/4)+1) + mRings * mResolution)*6;

	mVerts = new float[mNumberOfVertices];
	memset(mVerts, 0, mNumberOfVertices * sizeof(float));

	float elevationAngle, theta;
	glm::vec3 vertex, transformedVertex;
	unsigned int pos = 0;

	glm::mat3 rotMat = glm::mat3( glm::rotate( glm::mat4(1.0f), -tilt, glm::vec3(1.0f, 0.0f, 0.0f) ) );

	//create rings
	for(unsigned int r = 1; r <= mRings; r++)
	{
		elevationAngle = glm::radians<float>((FOV/2.0f) * (static_cast<float>(r)/static_cast<float>(mRings)));
		vertex.y = radius * cosf( elevationAngle );

		for(unsigned int i = 0; i < mResolution; i++)
		{
			theta = glm::pi<float>() * 2.0f * (static_cast<float>(i)/static_cast<float>(mResolution)); 
			
			vertex.x = radius * sinf( elevationAngle ) * cosf(theta);
			vertex.z = radius * sinf( elevationAngle ) * sinf(theta);
			transformedVertex = rotMat * vertex;
			
			mVerts[pos] = transformedVertex.x;
			mVerts[pos + 1] = transformedVertex.y;
			mVerts[pos + 2] = transformedVertex.z;

			pos += 3;
		}
	}

	//create segments
	for(unsigned int s = 0; s < mSegments; s++)
	{
		theta = (glm::pi<float>() * 2.0f) * (static_cast<float>(s)/static_cast<float>(mSegments)); 

		for(unsigned int i = 0; i < (mResolution/4)+1; i++)
		{
			elevationAngle = glm::radians<float>(FOV/2.0f) * (static_cast<float>(i)/static_cast<float>(mResolution/4));
			vertex.x = radius * sinf( elevationAngle ) * cosf(theta);
			vertex.y = radius * cosf( elevationAngle );
			vertex.z = radius * sinf( elevationAngle ) * sinf(theta);
			transformedVertex = rotMat * vertex;

			mVerts[pos] = transformedVertex.x;
			mVerts[pos + 1] = transformedVertex.y;
			mVerts[pos + 2] = transformedVertex.z;

			pos += 3;
		}
	}

	createVBO();

	if( !sgct::Engine::checkForOGLErrors() ) //if error occured
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "SGCT Utils: Dome creation error!\n");
		void cleanup();
	}

	//free data
	if(mVerts != NULL)
	{
		delete [] mVerts;
		mVerts = NULL;
	}
}

sgct_utils::SGCTDome::~SGCTDome()
{
	cleanup();
}

void sgct_utils::SGCTDome::cleanup()
{
	//cleanup
	if(mVBO != 0)
	{
		glDeleteBuffers(1, &mVBO);
		mVBO = 0;
	}

	if(mVAO != 0)
	{
		glDeleteBuffers(1, &mVAO);
		mVAO = 0;
	}
}

/*!
	If openGL 3.3+ is used layout 0 contains vertex positions (vec3).
*/
void sgct_utils::SGCTDome::draw()
{
	(this->*mInternalDrawFn)();
}

void sgct_utils::SGCTDome::drawVBO()
{
	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	glEnableClientState(GL_VERTEX_ARRAY);

	glBindBuffer(GL_ARRAY_BUFFER, mVBO);

	glVertexPointer(3, GL_FLOAT, 0, NULL);

	for(unsigned int r=0; r<mRings; r++)
		glDrawArrays(GL_LINE_LOOP, r * mResolution, mResolution);
	for(unsigned int s=0; s<mSegments; s++)
		glDrawArrays(GL_LINE_STRIP, mRings * mResolution + s * ((mResolution/4)+1), (mResolution/4)+1);

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glPopClientAttrib();
}

void sgct_utils::SGCTDome::drawVAO()
{
	glBindVertexArray( mVAO );

	for(unsigned int r=0; r<mRings; r++)
		glDrawArrays(GL_LINE_LOOP, r * mResolution, mResolution);
	for(unsigned int s=0; s<mSegments; s++)
		glDrawArrays(GL_LINE_STRIP, mRings * mResolution + s * ((mResolution/4)+1), (mResolution/4)+1);

	//unbind
	glBindVertexArray(0);
}

void sgct_utils::SGCTDome::createVBO()
{
	if( !sgct::Engine::instance()->isOGLPipelineFixed() )
	{
		mInternalDrawFn = &SGCTDome::drawVAO;
		
		glGenVertexArrays(1, &mVAO);
		glBindVertexArray( mVAO );
		glEnableVertexAttribArray(0);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "SGCTDome: Generating VAO: %d\n", mVAO);
	}
	
	glGenBuffers(1, &mVBO);
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "SGCTDome: Generating VBO: %d\n", mVBO);

	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, mNumberOfVertices * sizeof(float), mVerts, GL_STATIC_DRAW);

	if( !sgct::Engine::instance()->isOGLPipelineFixed() )
	{
		glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0) ); //vert positions
	}

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if( !sgct::Engine::instance()->isOGLPipelineFixed() )
	{
		glBindVertexArray( 0 );
	}
}
