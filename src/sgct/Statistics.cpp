/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <GL/glew.h>
#if __WIN32__
#include <GL/wglew.h>
#elif __LINUX__
#include <GL/glext.h>
#else
#include <OpenGL/glext.h>
#endif
#include <GL/glfw3.h>

#include "../include/sgct/Statistics.h"
#include "../include/sgct/MessageHandler.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <memory.h>

const static std::string Stats_Vert_Shader = "\
#version 330 core\n\
\n\
layout (location = 0) in vec2 Position;\n\
\n\
uniform mat4 MVP;\n\
\n\
void main()\n\
{\n\
	gl_Position = MVP * vec4(Position, 0.0, 1.0);\n\
}\n";

const static std::string Stats_Frag_Shader = "\
#version 330 core\n\
\n\
uniform vec4 Col;\n\
out vec4 Color;\n\
\n\
void main()\n\
{\n\
	Color = Col;\n\
}\n";

const static std::string Stats_Vert_Shader_Legacy = "\
#version 120\n\
\n\
void main()\n\
{\n\
	gl_TexCoord[0] = gl_MultiTexCoord0;\n\
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n\
}\n";

const static std::string Stats_Frag_Shader_Legacy = "\
#version 120\n\
\n\
uniform vec4 Col;\n\
\n\
void main()\n\
{\n\
	gl_FragColor = Col;\n\
}\n";

sgct_core::Statistics::Statistics()
{
	mAvgFPS = 0.0;
	mAvgDrawTime = 0.0;
	mAvgSyncTime = 0.0;
	mAvgFrameTime = 0.0;

	mFixedPipeline = true;
	mMVPLoc = -1;
	mColLoc = -1;
	mNumberOfLines = 0;

	mDynamicColors[ FRAME_TIME ]	= glm::vec4( 1.0f,1.0f,0.0f,0.8f );
	mDynamicColors[ DRAW_TIME ]		= glm::vec4( 1.0f,0.0f,1.0f,0.8f );
	mDynamicColors[ SYNC_TIME ]		= glm::vec4( 0.0f,1.0f,1.0f,0.8f );
	mDynamicColors[ LOOP_TIME_MAX ] = glm::vec4( 0.4f,0.4f,1.0f,0.8f );
	mDynamicColors[ LOOP_TIME_MIN ] = glm::vec4( 0.0f,0.0f,0.8f,0.8f );

	mStaticColors[ GRID ]	= glm::vec4( 1.0f,1.0f,1.0f,0.2f );
	mStaticColors[ FREQ ]	= glm::vec4( 1.0f,0.0f,0.0f,1.0f );
	mStaticColors[ BG ]		= glm::vec4( 0.0f,0.0f,0.0f,0.5f );

    mVBOIndex = 0;
	mDynamicVBO[0]	= GL_FALSE;
	mDynamicVAO[0]	= GL_FALSE;
    mDynamicVBO[1]	= GL_FALSE;
	mDynamicVAO[1]	= GL_FALSE;
	mStaticVBO		= GL_FALSE;
	mStaticVAO		= GL_FALSE;

	for(unsigned int i=0; i<STATS_HISTORY_LENGTH; i++)
	{
		mDynamicVertexList[i + FRAME_TIME * STATS_HISTORY_LENGTH].x = static_cast<double>(i);
		mDynamicVertexList[i + DRAW_TIME * STATS_HISTORY_LENGTH].x = static_cast<double>(i);
		mDynamicVertexList[i + SYNC_TIME * STATS_HISTORY_LENGTH].x = static_cast<double>(i);
		mDynamicVertexList[i + LOOP_TIME_MAX * STATS_HISTORY_LENGTH].x = static_cast<double>(i);
		mDynamicVertexList[i + LOOP_TIME_MIN * STATS_HISTORY_LENGTH].x = static_cast<double>(i);

		mDynamicVertexList[i + FRAME_TIME * STATS_HISTORY_LENGTH].y = 0.0;
		mDynamicVertexList[i + DRAW_TIME * STATS_HISTORY_LENGTH].y = 0.0;
		mDynamicVertexList[i + SYNC_TIME * STATS_HISTORY_LENGTH].y = 0.0;
		mDynamicVertexList[i + LOOP_TIME_MAX * STATS_HISTORY_LENGTH].y = 0.0;
		mDynamicVertexList[i + LOOP_TIME_MIN * STATS_HISTORY_LENGTH].y = 0.0;
	}
}

sgct_core::Statistics::~Statistics()
{
	mShader.deleteProgram();
	
	if(mDynamicVBO[0])
		glDeleteBuffers(2, &mDynamicVBO[0]);
	if(mDynamicVAO[0])
		glDeleteVertexArrays(2, &mDynamicVAO[0]);
	if(mStaticVBO)
		glDeleteBuffers(1, &mStaticVBO);
	if(mStaticVAO)
		glDeleteBuffers(1, &mStaticVAO);
}

void sgct_core::Statistics::initVBO(bool fixedPipeline)
{
	mFixedPipeline = fixedPipeline;

	if(!mFixedPipeline)
	{
		glGenVertexArrays(2, &mDynamicVAO[0]);
		glGenVertexArrays(1, &mStaticVAO);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Statistics: Generating VAOs:\n");
		for(unsigned int i=0; i<2; i++)
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "\t%d\n", mDynamicVAO[i]);
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "\t%d\n\n", mStaticVAO);
	}

	glGenBuffers(2, &mDynamicVBO[0]);
	glGenBuffers(1, &mStaticVBO);

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Statistics: Generating VBOs:\n");
    for(unsigned int i=0; i<2; i++)
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "\t%d\n", mDynamicVBO[i]);
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "\t%d\n\n", mStaticVBO);
	
	//double buffered VBOs
    for(unsigned int i=0; i<2; i++)
    {
        if(!mFixedPipeline)
        {
            glBindVertexArray(mDynamicVAO[i]);
            glEnableVertexAttribArray(0);
        }
        glBindBuffer(GL_ARRAY_BUFFER, mDynamicVBO[i]);
        glBufferData(GL_ARRAY_BUFFER, STATS_HISTORY_LENGTH * sizeof(StatsVertex) * STATS_NUMBER_OF_DYNAMIC_OBJS, &mDynamicVertexList[0], GL_STREAM_DRAW);
	
        if(!mFixedPipeline)
            glVertexAttribPointer( 0, 2, GL_DOUBLE, GL_FALSE, 0, NULL );
    }

	std::vector<float> verts;

	/*
		============================================
		         STATIC BACKGROUND QUAD
		============================================
	*/
	verts.push_back( 0.0f ); //x0
	verts.push_back( 0.0f ); //y0
	verts.push_back( static_cast<float>(STATS_HISTORY_LENGTH*2) ); //x1
	verts.push_back( 0.0f ); //y1
	verts.push_back( 0.0f ); //x2
	verts.push_back( 1.0f/30.0f ); //y2
	verts.push_back( static_cast<float>(STATS_HISTORY_LENGTH*2) ); //x3
	verts.push_back( 1.0f/30.0f ); //y3;
	
	/*
		============================================
		             STATIC 1 ms LINES
		============================================
	*/
	mNumberOfLines = 0;
	for(float f = 0.001f; f < (1.0f/30.0f); f += 0.001f )
	{
		verts.push_back( 0.0f ); //x0
		verts.push_back( f ); //y0
		verts.push_back( static_cast<float>(STATS_HISTORY_LENGTH*2) ); //x1
		verts.push_back( f ); //y1

		mNumberOfLines++;
	}

	/*
		============================================
		         STATIC 0, 30 & 60 FPS LINES
		============================================
	*/
	verts.push_back( 0.0f ); //x0
	verts.push_back( 0.0f ); //y0
	verts.push_back( static_cast<float>(STATS_HISTORY_LENGTH*2) ); //0x1
	verts.push_back( 0.0f ); //y1

	verts.push_back( 0.0f ); //x0
	verts.push_back( 1.0f/30.0f ); //y0
	verts.push_back( static_cast<float>(STATS_HISTORY_LENGTH*2) ); //x1
	verts.push_back( 1.0f/30.0f ); //y1

	verts.push_back( 0.0f ); //x0
	verts.push_back( 1.0f/60.0f ); //y0
	verts.push_back( static_cast<float>(STATS_HISTORY_LENGTH*2) ); //x1
	verts.push_back( 1.0f/60.0f ); //y1

	if(!mFixedPipeline)
	{
		glBindVertexArray(mStaticVAO);
		glEnableVertexAttribArray(0);
	}
	glBindBuffer(GL_ARRAY_BUFFER, mStaticVBO);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), &verts[0], GL_STATIC_DRAW );
	if(!mFixedPipeline)
		glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0) );
	
	/*
		============================================
		              SETUP SHADERS
		============================================
	*/
	if(mFixedPipeline)
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		mShader.setName("StatsShader");
		mShader.addShaderSrc( Stats_Vert_Shader_Legacy, GL_VERTEX_SHADER, sgct::ShaderProgram::SHADER_SRC_STRING );
		mShader.addShaderSrc( Stats_Frag_Shader_Legacy, GL_FRAGMENT_SHADER, sgct::ShaderProgram::SHADER_SRC_STRING );
		mShader.createAndLinkProgram();
		mShader.bind();

		mColLoc = mShader.getUniformLocation( "Col" );

		mShader.unbind();
	}
	else
	{
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		mShader.setName("StatsShader");
		mShader.addShaderSrc( Stats_Vert_Shader, GL_VERTEX_SHADER, sgct::ShaderProgram::SHADER_SRC_STRING );
		mShader.addShaderSrc( Stats_Frag_Shader, GL_FRAGMENT_SHADER, sgct::ShaderProgram::SHADER_SRC_STRING );
		mShader.createAndLinkProgram();
		mShader.bind();

		mMVPLoc = mShader.getUniformLocation( "MVP" );
		mColLoc = mShader.getUniformLocation( "Col" );

		mShader.unbind();
	}
}

void sgct_core::Statistics::setAvgFPS(double afps)
{
	mAvgFPS = afps;
}

void sgct_core::Statistics::setFrameTime(double t)
{
	mAvgFrameTime = 0.0;
	int start = FRAME_TIME * STATS_HISTORY_LENGTH;

	for (int i = start + STATS_HISTORY_LENGTH - 2; i >= start; i--)
	{
		mDynamicVertexList[i + 1].y = mDynamicVertexList[i].y;
		if (i < (STATS_AVERAGE_LENGTH + start))
			mAvgFrameTime += mDynamicVertexList[i].y;
	}
	mDynamicVertexList[start].y = t;
    
	mAvgFrameTime += mDynamicVertexList[start].y;
	mAvgFrameTime /= static_cast<double>(STATS_AVERAGE_LENGTH);
}

void sgct_core::Statistics::setDrawTime(double t)
{
	mAvgDrawTime = 0.0;
	int start = DRAW_TIME * STATS_HISTORY_LENGTH;

	for (int i = start + STATS_HISTORY_LENGTH - 2; i >= start; i--)
	{
		mDynamicVertexList[i + 1].y = mDynamicVertexList[i].y;
		if (i < (STATS_AVERAGE_LENGTH + start))
			mAvgDrawTime += mDynamicVertexList[i].y;
	}
	mDynamicVertexList[start].y = t;
    
	mAvgDrawTime += mDynamicVertexList[start].y;
	mAvgDrawTime /= static_cast<double>(STATS_AVERAGE_LENGTH);
}

void sgct_core::Statistics::setSyncTime(double t)
{
	mAvgSyncTime = 0.0;
    int start = SYNC_TIME * STATS_HISTORY_LENGTH;
    
	for (int i = start + STATS_HISTORY_LENGTH - 2; i >= start; i--)
	{
		mDynamicVertexList[i + 1].y = mDynamicVertexList[i].y;
		if (i < (STATS_AVERAGE_LENGTH + SYNC_TIME * STATS_NUMBER_OF_DYNAMIC_OBJS))
			mAvgSyncTime += mDynamicVertexList[i].y;
	}
	mDynamicVertexList[start].y = t;
	
    mAvgSyncTime += mDynamicVertexList[start].y;
	mAvgSyncTime /= static_cast<double>(STATS_AVERAGE_LENGTH);
}

/*!
	Set the minimum and maximum time it takes for a sync message from send to receive
*/
void sgct_core::Statistics::setLoopTime(double min, double max)
{
	int start = LOOP_TIME_MAX * STATS_HISTORY_LENGTH;
    for (int i = start + STATS_HISTORY_LENGTH - 2; i >= start; i--)
		mDynamicVertexList[i + 1].y = mDynamicVertexList[i].y;
    mDynamicVertexList[start].y = max;
    
	start = LOOP_TIME_MIN * STATS_HISTORY_LENGTH;
    for (int i = start + STATS_HISTORY_LENGTH - 2; i >= start; i--)
		mDynamicVertexList[i + 1].y = mDynamicVertexList[i].y;
	mDynamicVertexList[start].y = min;
}


void sgct_core::Statistics::addSyncTime(double t)
{
	mDynamicVertexList[SYNC_TIME * STATS_HISTORY_LENGTH].y += t;
	mAvgSyncTime += (t/static_cast<double>(STATS_AVERAGE_LENGTH));
}

void sgct_core::Statistics::update()
{
    GLvoid* PositionBuffer;

    mVBOIndex = 1 - mVBOIndex; //ping-pong
	glBindBuffer(GL_ARRAY_BUFFER, mDynamicVBO[mVBOIndex]);
   
    size_t size = STATS_HISTORY_LENGTH * sizeof(StatsVertex) * STATS_NUMBER_OF_DYNAMIC_OBJS;

	//glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STREAM_DRAW);
	
	//PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	PositionBuffer = glMapBufferRange(GL_ARRAY_BUFFER, 0, STATS_HISTORY_LENGTH * sizeof(StatsVertex), GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT);

	if (PositionBuffer != NULL)
		memcpy(PositionBuffer, &mDynamicVertexList[0], size);
	glUnmapBuffer(GL_ARRAY_BUFFER);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void sgct_core::Statistics::draw(float lineWidth)
{
    mShader.bind();

	if(mFixedPipeline)
	{
		//enter ortho mode
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glPushMatrix();
		
        double size = static_cast<double>(STATS_HISTORY_LENGTH);
        glOrtho(0.0, size, 0.0, size, -1.0, 1.0);

		glMatrixMode(GL_MODELVIEW);

		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glDisable(GL_LIGHTING);

		glLoadIdentity();

		glTranslatef(0.0f, static_cast<float>(size)/4.0f, 0.0f);
		glScalef(1.0f, VERT_SCALE, 1.0f);

		glLineWidth( lineWidth );

		glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
		glEnableClientState(GL_VERTEX_ARRAY);

		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_EDGE_FLAG_ARRAY);
		glDisableClientState(GL_FOG_COORD_ARRAY);
		glDisableClientState(GL_INDEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_SECONDARY_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		
		glBindBuffer(GL_ARRAY_BUFFER, mStaticVBO);
		glVertexPointer(2, GL_FLOAT, 0, reinterpret_cast<void*>(0));

		//draw background (1024x1024 canvas)
		glUniform4fv( mColLoc, 1, glm::value_ptr(mStaticColors[ BG ]) );
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		//1 ms lines
		glUniform4fv( mColLoc, 1, glm::value_ptr(mStaticColors[ GRID ]) );
		glDrawArrays( GL_LINES, 4, mNumberOfLines*2);

		//zero line, 60hz & 30hz
		glUniform4fv( mColLoc, 1, glm::value_ptr(mStaticColors[ FREQ ]) );
		glDrawArrays( GL_LINES, 4+mNumberOfLines*2, 6 );

		glBindBuffer(GL_ARRAY_BUFFER, mDynamicVBO[mVBOIndex]);
		glVertexPointer(2, GL_DOUBLE, 0, reinterpret_cast<void*>(0));

		for(unsigned int i=0; i<STATS_NUMBER_OF_DYNAMIC_OBJS; i++)
		{
			glUniform4fv( mColLoc, 1, glm::value_ptr(mDynamicColors[ i ]) );
			glDrawArrays(GL_LINE_STRIP, i * STATS_HISTORY_LENGTH, STATS_HISTORY_LENGTH);
		}

		//unbind
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glPopClientAttrib();
		glPopAttrib();

		//exit ortho mode
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	}
	else //programmable pipeline
	{
        //gives an opengl error in mac os x (intel iris)
        glLineWidth( lineWidth );
        
        float size = static_cast<float>(STATS_HISTORY_LENGTH);
		
		glm::mat4 orthoMat = glm::ortho( 0.0f, size, 0.0f, size );
		orthoMat = glm::translate( orthoMat, glm::vec3(0.0f, size/4.0f, 0.0f) );
		orthoMat = glm::scale( orthoMat, glm::vec3(1.0f, VERT_SCALE, 1.0f) );
		
		glUniformMatrix4fv( mMVPLoc, 1, GL_FALSE, &orthoMat[0][0]);
		
		glBindVertexArray( mStaticVAO );

		//draw background (1024x1024 canvas)
		glUniform4fv( mColLoc, 1, glm::value_ptr(mStaticColors[ BG ]) );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4);

		//1 ms lines
		glUniform4fv( mColLoc, 1, glm::value_ptr(mStaticColors[ GRID ]) );
		glDrawArrays( GL_LINES, 4, mNumberOfLines*2);
		
		//zero line, 60hz & 30hz
		glUniform4fv( mColLoc, 1, glm::value_ptr(mStaticColors[ FREQ ]) );
		glDrawArrays( GL_LINES, 4+mNumberOfLines*2, 6 );
		
		glBindVertexArray(mDynamicVAO[mVBOIndex]);
		for (unsigned int i = 0; i<STATS_NUMBER_OF_DYNAMIC_OBJS; i++)
		{
			glUniform4fv( mColLoc, 1, glm::value_ptr(mDynamicColors[i]) );
			glDrawArrays(GL_LINE_STRIP, i * STATS_HISTORY_LENGTH, STATS_HISTORY_LENGTH);
		}
		
		//unbind
		glBindVertexArray(0);
	}

	mShader.unbind();
}
