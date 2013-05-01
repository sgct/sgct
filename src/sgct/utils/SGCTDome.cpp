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
#include "../include/sgct/SGCTSettings.h"

sgct_utils::SGCTDome::SGCTDome(float radius, float FOV, unsigned int segments, unsigned int rings, unsigned int resolution)
{
	mVerts = NULL;
	mResolution = resolution;
	mRings = rings;
	mSegments = segments;
	mVBO = 0;

	if(mResolution < 4) //must be four or higher
	{
		sgct::MessageHandler::Instance()->print("Warning: Dome geometry resolution must be higher than 4.\n");
		mResolution = 4;
	}

	mNumberOfVertices = (mSegments * ((mResolution/4)+1) + mRings * mResolution)*6;

	mVerts = new float[mNumberOfVertices];
	memset(mVerts, 0, mNumberOfVertices * sizeof(float));

	float elevationAngle, theta;
	float x, y, z;
	unsigned int pos = 0;

	//create rings
	for(unsigned int r = 1; r <= mRings; r++)
	{
		elevationAngle = glm::radians<float>((FOV/2.0f) * (static_cast<float>(r)/static_cast<float>(mRings)));
		y = radius * cosf( elevationAngle );

		for(unsigned int i = 0; i < mResolution; i++)
		{
			theta = glm::pi<float>() * 2.0f * (static_cast<float>(i)/static_cast<float>(mResolution)); 
			
			x = radius * sinf( elevationAngle ) * cosf(theta);
			z = radius * sinf( elevationAngle ) * sinf(theta);
			mVerts[pos] = x;
			mVerts[pos + 1] = y;
			mVerts[pos + 2] = z;

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
			x = radius * sinf( elevationAngle ) * cosf(theta);
			y = radius * cosf( elevationAngle );
			z = radius * sinf( elevationAngle ) * sinf(theta);

			mVerts[pos] = x;
			mVerts[pos + 1] = y;
			mVerts[pos + 2] = z;

			pos += 3;
		}
	}

	createVBO();

	if( !sgct::Engine::checkForOGLErrors() ) //if error occured
	{
		sgct::MessageHandler::Instance()->print("SGCT Utils: Dome creation error!\n");
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
}

void sgct_utils::SGCTDome::draw()
{
	//if not set
	if(mVBO == 0)
		return;

	glPushMatrix();
	glRotatef(-sgct_core::SGCTSettings::Instance()->getFisheyeTilt(), 1.0f, 0.0f, 0.0f); 
	
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
	glPopMatrix();
}

void sgct_utils::SGCTDome::createVBO()
{
	glGenBuffers(1, &mVBO);

	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, mNumberOfVertices * sizeof(float), mVerts, GL_STATIC_DRAW);

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
