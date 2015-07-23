/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/utils/SGCTBox.h"
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/Engine.h"

/*!
	This constructor requires a valid openGL contex 
*/
sgct_utils::SGCTBox::SGCTBox(float size, TextureMappingMode tmm)
{
	//init
	mVBO = GL_FALSE;
	mVAO = GL_FALSE;
	mVerts = NULL;

	mInternalDrawFn = &SGCTBox::drawVBO;

	mVerts = new sgct_helpers::SGCTVertexData[36];
	memset(mVerts, 0, 36 * sizeof(sgct_helpers::SGCTVertexData));

	//populate the array
	if(tmm == Regular)
	{
		//A (front/+z)
		mVerts[0].set(0.0f, 1.0f, 0.0f, 0.0f, 1.0f, -size/2.0f, size/2.0f, size/2.0f);
		mVerts[1].set(0.0f, 0.0f, 0.0f, 0.0f, 1.0f, -size/2.0f, -size/2.0f, size/2.0f);
		mVerts[2].set(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, size/2.0f, -size/2.0f, size/2.0f);
		mVerts[3].set(0.0f, 1.0f, 0.0f, 0.0f, 1.0f, -size/2.0f, size/2.0f, size/2.0f);
		mVerts[4].set(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, size/2.0f, -size/2.0f, size/2.0f);
		mVerts[5].set(1.0f, 1.0f, 0.0f, 0.0f, 1.0f, size/2.0f, size/2.0f, size/2.0f);

		//B (right/+x)
		mVerts[6].set(0.0f, 1.0f, 1.0f, 0.0f, 0.0f, size/2.0f, size/2.0f, size/2.0f);
		mVerts[7].set(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, size/2.0f, -size/2.0f, size/2.0f);
		mVerts[8].set(1.0f, 0.0f, 1.0f, 0.0f, 0.0f, size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[9].set(0.0f, 1.0f, 1.0f, 0.0f, 0.0f, size/2.0f, size/2.0f, size/2.0f);
		mVerts[10].set(1.0f, 0.0f, 1.0f, 0.0f, 0.0f, size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[11].set(1.0f, 1.0f, 1.0f, 0.0f, 0.0f, size/2.0f, size/2.0f, -size/2.0f);

		//C (Back/-z)
		mVerts[12].set(0.0f, 1.0f, 0.0f, 0.0f, -1.0f, size/2.0f, size/2.0f, -size/2.0f);
		mVerts[13].set(0.0f, 0.0f, 0.0f, 0.0f, -1.0f, size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[14].set(1.0f, 0.0f, 0.0f, 0.0f, -1.0f, -size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[15].set(0.0f, 1.0f, 0.0f, 0.0f, -1.0f, size/2.0f, size/2.0f, -size/2.0f);
		mVerts[16].set(1.0f, 0.0f, 0.0f, 0.0f, -1.0f, -size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[17].set(1.0f, 1.0f, 0.0f, 0.0f, -1.0f, -size/2.0f, size/2.0f, -size/2.0f);

		//D (Left/-x)
		mVerts[18].set(0.0f, 1.0f, -1.0f, 0.0f, 0.0f, -size/2.0f, size/2.0f, -size/2.0f);
		mVerts[19].set(0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[20].set(1.0f, 0.0f, -1.0f, 0.0f, 0.0f, -size/2.0f, -size/2.0f, size/2.0f);
		mVerts[21].set(0.0f, 1.0f, -1.0f, 0.0f, 0.0f, -size/2.0f, size/2.0f, -size/2.0f);
		mVerts[22].set(1.0f, 0.0f, -1.0f, 0.0f, 0.0f, -size/2.0f, -size/2.0f, size/2.0f);
		mVerts[23].set(1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -size/2.0f, size/2.0f, size/2.0f);

		//E (Top/+y)
		mVerts[24].set(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, -size/2.0f, size/2.0f, -size/2.0f);
		mVerts[25].set(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, -size/2.0f, size/2.0f, size/2.0f);
		mVerts[26].set(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, size/2.0f, size/2.0f, size/2.0f);
		mVerts[27].set(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, -size/2.0f, size/2.0f, -size/2.0f);
		mVerts[28].set(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, size/2.0f, size/2.0f, size/2.0f);
		mVerts[29].set(1.0f, 1.0f, 0.0f, 1.0f, 0.0f, size/2.0f, size/2.0f, -size/2.0f);

		//F (Bottom/-y)
		mVerts[30].set(0.0f, 1.0f, 0.0f, -1.0f, 0.0f, -size/2.0f, -size/2.0f, size/2.0f);
		mVerts[31].set(0.0f, 0.0f, 0.0f, -1.0f, 0.0f, -size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[32].set(1.0f, 0.0f, 0.0f, -1.0f, 0.0f, size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[33].set(0.0f, 1.0f, 0.0f, -1.0f, 0.0f, -size/2.0f, -size/2.0f, size/2.0f);
		mVerts[34].set(1.0f, 0.0f, 0.0f, -1.0f, 0.0f, size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[35].set(1.0f, 1.0f, 0.0f, -1.0f, 0.0f, size/2.0f, -size/2.0f, size/2.0f);
	}
	else if(tmm == CubeMap)
	{
		//A (front/+z)
		mVerts[0].set(0.0f, 1.0f, 0.0f, 0.0f, 1.0f, -size/2.0f, size/2.0f, size/2.0f);
		mVerts[1].set(0.0f, 0.5f, 0.0f, 0.0f, 1.0f, -size/2.0f, -size/2.0f, size/2.0f);
		mVerts[2].set(0.333333f, 0.5f, 0.0f, 0.0f, 1.0f, size/2.0f, -size/2.0f, size/2.0f);
		mVerts[3].set(0.0f, 1.0f, 0.0f, 0.0f, 1.0f, -size/2.0f, size/2.0f, size/2.0f);
		mVerts[4].set(0.333333f, 0.5f, 0.0f, 0.0f, 1.0f, size/2.0f, -size/2.0f, size/2.0f);
		mVerts[5].set(0.333333f, 1.0f, 0.0f, 0.0f, 1.0f, size/2.0f, size/2.0f, size/2.0f);

		//B (right/+x)
		mVerts[6].set(0.333334f, 1.0f, 1.0f, 0.0f, 0.0f, size/2.0f, size/2.0f, size/2.0f);
		mVerts[7].set(0.333334f, 0.5f, 1.0f, 0.0f, 0.0f, size/2.0f, -size/2.0f, size/2.0f);
		mVerts[8].set(0.666666f, 0.5f, 1.0f, 0.0f, 0.0f, size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[9].set(0.333334f, 1.0f, 1.0f, 0.0f, 0.0f, size/2.0f, size/2.0f, size/2.0f);
		mVerts[10].set(0.666666f, 0.5f, 1.0f, 0.0f, 0.0f, size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[11].set(0.666666f, 1.0f, 1.0f, 0.0f, 0.0f, size/2.0f, size/2.0f, -size/2.0f);

		//C (Back/-z)
		mVerts[12].set(0.666667f, 1.0f, 0.0f, 0.0f, -1.0f, size/2.0f, size/2.0f, -size/2.0f);
		mVerts[13].set(0.666667f, 0.5f, 0.0f, 0.0f, -1.0f, size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[14].set(1.0f, 0.5f, 0.0f, 0.0f, -1.0f, -size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[15].set(0.666667f, 1.0f, 0.0f, 0.0f, -1.0f, size/2.0f, size/2.0f, -size/2.0f);
		mVerts[16].set(1.0f, 0.5f, 0.0f, 0.0f, -1.0f, -size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[17].set(1.0f, 1.0f, 0.0f, 0.0f, -1.0f, -size/2.0f, size/2.0f, -size/2.0f);

		//D (Left/-x)
		mVerts[18].set(0.0f, 0.5f, -1.0f, 0.0f, 0.0f, -size/2.0f, size/2.0f, -size/2.0f);
		mVerts[19].set(0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[20].set(0.333333f, 0.0f, -1.0f, 0.0f, 0.0f, -size/2.0f, -size/2.0f, size/2.0f);
		mVerts[21].set(0.0f, 0.5f, -1.0f, 0.0f, 0.0f, -size/2.0f, size/2.0f, -size/2.0f);
		mVerts[22].set(0.333333f, 0.0f, -1.0f, 0.0f, 0.0f, -size/2.0f, -size/2.0f, size/2.0f);
		mVerts[23].set(0.333333f, 0.5f, -1.0f, 0.0f, 0.0f, -size/2.0f, size/2.0f, size/2.0f);

		//E (Top/+y)
		mVerts[24].set(0.333334f, 0.5f, 0.0f, 1.0f, 0.0f, -size/2.0f, size/2.0f, -size/2.0f);
		mVerts[25].set(0.333334f, 0.0f, 0.0f, 1.0f, 0.0f, -size/2.0f, size/2.0f, size/2.0f);
		mVerts[26].set(0.666666f, 0.0f, 0.0f, 1.0f, 0.0f, size/2.0f, size/2.0f, size/2.0f);
		mVerts[27].set(0.333334f, 0.5f, 0.0f, 1.0f, 0.0f, -size/2.0f, size/2.0f, -size/2.0f);
		mVerts[28].set(0.666666f, 0.0f, 0.0f, 1.0f, 0.0f, size/2.0f, size/2.0f, size/2.0f);
		mVerts[29].set(0.666666f, 0.5f, 0.0f, 1.0f, 0.0f, size/2.0f, size/2.0f, -size/2.0f);

		//F (Bottom/-y)
		mVerts[30].set(0.666667f, 0.5f, 0.0f, -1.0f, 0.0f, -size/2.0f, -size/2.0f, size/2.0f);
		mVerts[31].set(0.666667f, 0.0f, 0.0f, -1.0f, 0.0f, -size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[32].set(1.0f, 0.0f, 0.0f, -1.0f, 0.0f, size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[33].set(0.666667f, 0.5f, 0.0f, -1.0f, 0.0f, -size/2.0f, -size/2.0f, size/2.0f);
		mVerts[34].set(1.0f, 0.0f, 0.0f, -1.0f, 0.0f, size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[35].set(1.0f, 0.5f, 0.0f, -1.0f, 0.0f, size/2.0f, -size/2.0f, size/2.0f);
	}
	else //skybox
	{
		//A (front/+z)
		mVerts[0].set(1.000f, 0.666666f, 0.0f, 0.0f, 1.0f, -size/2.0f, size/2.0f, size/2.0f);
		mVerts[1].set(1.000f, 0.333334f, 0.0f, 0.0f, 1.0f, -size/2.0f, -size/2.0f, size/2.0f);
		mVerts[2].set(0.751f, 0.333334f, 0.0f, 0.0f, 1.0f, size/2.0f, -size/2.0f, size/2.0f);
		mVerts[3].set(1.000f, 0.666666f, 0.0f, 0.0f, 1.0f, -size/2.0f, size/2.0f, size/2.0f);
		mVerts[4].set(0.751f, 0.333334f, 0.0f, 0.0f, 1.0f, size/2.0f, -size/2.0f, size/2.0f);
		mVerts[5].set(0.751f, 0.666666f, 0.0f, 0.0f, 1.0f, size/2.0f, size/2.0f, size/2.0f);

		//B (right/+x)
		mVerts[6].set(0.749f, 0.666666f, 1.0f, 0.0f, 0.0f, size/2.0f, size/2.0f, size/2.0f);
		mVerts[7].set(0.749f, 0.333334f, 1.0f, 0.0f, 0.0f, size/2.0f, -size/2.0f, size/2.0f);
		mVerts[8].set(0.501f, 0.333334f, 1.0f, 0.0f, 0.0f, size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[9].set(0.749f, 0.666666f, 1.0f, 0.0f, 0.0f, size/2.0f, size/2.0f, size/2.0f);
		mVerts[10].set(0.501f, 0.333334f, 1.0f, 0.0f, 0.0f, size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[11].set(0.501f, 0.666666f, 1.0f, 0.0f, 0.0f, size/2.0f, size/2.0f, -size/2.0f);

		//C (Back/-z)
		mVerts[12].set(0.499f, 0.666666f, 0.0f, 0.0f, -1.0f, size/2.0f, size/2.0f, -size/2.0f);
		mVerts[13].set(0.499f, 0.333334f, 0.0f, 0.0f, -1.0f, size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[14].set(0.251f, 0.333334f, 0.0f, 0.0f, -1.0f, -size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[15].set(0.499f, 0.666666f, 0.0f, 0.0f, -1.0f, size/2.0f, size/2.0f, -size/2.0f);
		mVerts[16].set(0.251f, 0.333334f, 0.0f, 0.0f, -1.0f, -size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[17].set(0.251f, 0.666666f, 0.0f, 0.0f, -1.0f, -size/2.0f, size/2.0f, -size/2.0f);

		//D (Left/-x)
		mVerts[18].set(0.249f, 0.666666f, -1.0f, 0.0f, 0.0f, -size/2.0f, size/2.0f, -size/2.0f);
		mVerts[19].set(0.249f, 0.333334f, -1.0f, 0.0f, 0.0f, -size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[20].set(0.000f, 0.333334f, -1.0f, 0.0f, 0.0f, -size/2.0f, -size/2.0f, size/2.0f);
		mVerts[21].set(0.249f, 0.666666f, -1.0f, 0.0f, 0.0f, -size/2.0f, size/2.0f, -size/2.0f);
		mVerts[22].set(0.000f, 0.333334f, -1.0f, 0.0f, 0.0f, -size/2.0f, -size/2.0f, size/2.0f);
		mVerts[23].set(0.000f, 0.666666f, -1.0f, 0.0f, 0.0f, -size/2.0f, size/2.0f, size/2.0f);

		//E (Top/+y)
		mVerts[24].set(0.251f, 0.666667f, 0.0f, 1.0f, 0.0f, -size/2.0f, size/2.0f, -size/2.0f);
		mVerts[25].set(0.251f, 1.000000f, 0.0f, 1.0f, 0.0f, -size/2.0f, size/2.0f, size/2.0f);
		mVerts[26].set(0.499f, 1.000000f, 0.0f, 1.0f, 0.0f, size/2.0f, size/2.0f, size/2.0f);
		mVerts[27].set(0.251f, 0.666667f, 0.0f, 1.0f, 0.0f, -size/2.0f, size/2.0f, -size/2.0f);
		mVerts[28].set(0.499f, 1.000000f, 0.0f, 1.0f, 0.0f, size/2.0f, size/2.0f, size/2.0f);
		mVerts[29].set(0.499f, 0.666667f, 0.0f, 1.0f, 0.0f, size/2.0f, size/2.0f, -size/2.0f);

		//F (Bottom/-y)
		mVerts[30].set(0.251f, 0.000000f, 0.0f, -1.0f, 0.0f, -size/2.0f, -size/2.0f, size/2.0f);
		mVerts[31].set(0.251f, 0.333333f, 0.0f, -1.0f, 0.0f, -size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[32].set(0.499f, 0.333333f, 0.0f, -1.0f, 0.0f, size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[33].set(0.251f, 0.000000f, 0.0f, -1.0f, 0.0f, -size/2.0f, -size/2.0f, size/2.0f);
		mVerts[34].set(0.499f, 0.333333f, 0.0f, -1.0f, 0.0f, size/2.0f, -size/2.0f, -size/2.0f);
		mVerts[35].set(0.499f, 0.000000f, 0.0f, -1.0f, 0.0f, size/2.0f, -size/2.0f, size/2.0f);
	}

	createVBO();

	if( !sgct::Engine::checkForOGLErrors() ) //if error occured
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "SGCT Utils: Box creation error!\n");
		void cleanup();
	}

	//free data
	if( mVerts != NULL )
	{
		delete [] mVerts;
		mVerts = NULL;
	}
}

sgct_utils::SGCTBox::~SGCTBox()
{
	cleanUp();
}

/*!
	If openGL 3.3+ is used:
	layout 0 contains texture coordinates (vec2)
	layout 1 contains vertex normals (vec3)
	layout 2 contains vertex positions (vec3).
*/
void sgct_utils::SGCTBox::draw()
{
	(this->*mInternalDrawFn)();
}

void sgct_utils::SGCTBox::drawVBO()
{
	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	
	glInterleavedArrays(GL_T2F_N3F_V3F, 0, 0);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glPopClientAttrib();
}

void sgct_utils::SGCTBox::drawVAO()
{
	glBindVertexArray( mVAO );
	glDrawArrays(GL_TRIANGLES, 0, 36);

	//unbind
	glBindVertexArray(0);
}

void sgct_utils::SGCTBox::cleanUp()
{
	//cleanup
	if(mVBO != 0)
	{
		glDeleteBuffers(1, &mVBO);
		mVBO = 0;
	}

	if(mVAO != 0)
	{
		glDeleteVertexArrays(1, &mVAO);
		mVAO = 0;
	}
}

void sgct_utils::SGCTBox::createVBO()
{
	if( !sgct::Engine::instance()->isOGLPipelineFixed() )
	{
		mInternalDrawFn = &SGCTBox::drawVAO;
		
		glGenVertexArrays(1, &mVAO);
		glBindVertexArray( mVAO );
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "SGCTBox: Generating VAO: %d\n", mVAO);
	}
	
	glGenBuffers(1, &mVBO);
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "SGCTBox: Generating VBO: %d\n", mVBO);

	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, 36 * sizeof(sgct_helpers::SGCTVertexData), mVerts, GL_STATIC_DRAW);

	if( !sgct::Engine::instance()->isOGLPipelineFixed() )
	{
		glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof(sgct_helpers::SGCTVertexData), reinterpret_cast<void*>(0) ); //texcoords
		glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof(sgct_helpers::SGCTVertexData), reinterpret_cast<void*>(8) ); //normals
		glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, sizeof(sgct_helpers::SGCTVertexData), reinterpret_cast<void*>(20) ); //vert positions
	}

	//unbind
	if( !sgct::Engine::instance()->isOGLPipelineFixed() )
		glBindVertexArray( 0 );
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
