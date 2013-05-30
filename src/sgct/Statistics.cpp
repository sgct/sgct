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
#include <GL/glfw.h>

#include "../include/sgct/Statistics.h"
#include "../include/sgct/ShaderManager.h"
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
};\n";

const static std::string Stats_Frag_Shader = "\
#version 330 core\n\
\n\
uniform vec4 Col;\n\
out vec4 Color;\n\
\n\
void main()\n\
{\n\
	Color = Col;\n\
};\n";

sgct_core::Statistics::Statistics()
{
	mAvgFPS = 0.0;
	mAvgDrawTime = 0.0;
	mAvgSyncTime = 0.0;
	mAvgFrameTime = 0.0;

	mFixedPipeline = true;
	mMVPLoc = 0;
	mColLoc = 0;
	mNumberOfLineVerts = 0;

	colors[0] = glm::vec4( 1.0f,1.0f,0.0f,0.8f );
	colors[1] = glm::vec4( 1.0f,0.0f,1.0f,0.8f );
	colors[2] = glm::vec4( 0.0f,1.0f,1.0f,0.8f );
	colors[3] = glm::vec4( 0.2f,0.2f,1.0f,0.8f );
	colors[4] = glm::vec4( 0.2f,0.2f,1.0f,0.8f );

	mVAO = 0;
	for(unsigned int i=0; i<STATS_NUMBER_OF_VBOs; i++)
		mVboPtrs[i] = 0;

	for(unsigned int i=0; i<STATS_HISTORY_LENGTH; i++)
	{
		mFrameTime[i].x = static_cast<double>(i*2);
		mDrawTime[i].x = static_cast<double>(i*2);
		mSyncTime[i].x = static_cast<double>(i*2);
		mLoopTimeMax[i].x = static_cast<double>(i*2);
		mLoopTimeMin[i].x = static_cast<double>(i*2);

		mFrameTime[i].y = 0.0;
		mDrawTime[i].y = 0.0;
		mSyncTime[i].y = 0.0;
		mLoopTimeMax[i].y = 0.0;
		mLoopTimeMin[i].y = 0.0;
	}
}

sgct_core::Statistics::~Statistics()
{
	if(mVboPtrs[0] != 0)
		glDeleteBuffers(STATS_NUMBER_OF_VBOs, &mVboPtrs[0]);
	if(mVAO != 0)
		glDeleteVertexArrays(1, &mVAO);
	if(mGridVBO !=0)
		glDeleteBuffers(1, &mGridVBO);
	if(mFreqLinesVBO !=0)
		glDeleteBuffers(1, &mFreqLinesVBO);
	if(mBackgroundVBO !=0)
		glDeleteBuffers(1, &mBackgroundVBO);
}

void sgct_core::Statistics::initVBO(bool fixedPipeline)
{
	mFixedPipeline = fixedPipeline;
	
	if(!mFixedPipeline)
	{
		sgct::ShaderManager::Instance()->addShader( mShader, "StatisticsShader",
			Stats_Vert_Shader,
			Stats_Frag_Shader, sgct::ShaderManager::SHADER_SRC_STRING );
		mShader.bind();

		mMVPLoc = mShader.getUniformLocation( "MVP" );
		mColLoc = mShader.getUniformLocation( "Col" );

		sgct::ShaderManager::Instance()->unBindShader();
		
		glGenVertexArrays(1, &mVAO);
		glBindVertexArray(mVAO);
	}

	glGenBuffers(STATS_NUMBER_OF_VBOs, &mVboPtrs[0]);
	for(unsigned int i=0; i<STATS_NUMBER_OF_VBOs; i++)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mVboPtrs[i]);
		glBufferData(GL_ARRAY_BUFFER, STATS_HISTORY_LENGTH * sizeof(StatsVertex), getVerts(i), GL_STREAM_DRAW );
	}

	std::vector<float> gridVerts;
	for(float f = 0.001f; f < (1.0f/30.0f); f += 0.001f )
	{
		gridVerts.push_back( 0.0f );
		gridVerts.push_back( f );
		gridVerts.push_back( static_cast<float>(STATS_HISTORY_LENGTH*2) );
		gridVerts.push_back( f );
	}
	glGenBuffers(1, &mGridVBO);
	glBindBuffer(GL_ARRAY_BUFFER, mGridVBO);
	glBufferData(GL_ARRAY_BUFFER, gridVerts.size() * sizeof(float), &gridVerts[0], GL_STATIC_DRAW );
	mNumberOfLineVerts = gridVerts.size() / 2;

	float lineVerts[] = { 0.0f, 0.0f,
		static_cast<float>(STATS_HISTORY_LENGTH*2), 0.0f,
		0.0f, 1.0f/60.0f,
		static_cast<float>(STATS_HISTORY_LENGTH*2), 1.0f/60.0f,
		0.0f, 1.0f/30.0f,
		static_cast<float>(STATS_HISTORY_LENGTH*2), 1.0f/30.0f };
	glGenBuffers(1, &mFreqLinesVBO);
	glBindBuffer(GL_ARRAY_BUFFER, mFreqLinesVBO);
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), &lineVerts[0], GL_STATIC_DRAW );
	
	float bgVerts[] = { 0.0f, 0.0f,
		static_cast<float>(STATS_HISTORY_LENGTH*2), 0.0f,
		0.0f, static_cast<float>(STATS_HISTORY_LENGTH),
		static_cast<float>(STATS_HISTORY_LENGTH*2), static_cast<float>(STATS_HISTORY_LENGTH) };
	glGenBuffers(1, &mBackgroundVBO);
	glBindBuffer(GL_ARRAY_BUFFER, mBackgroundVBO);
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), &bgVerts[0], GL_STATIC_DRAW );

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if(!mFixedPipeline)
		glBindVertexArray(0);
}

void sgct_core::Statistics::setAvgFPS(double afps)
{
	mAvgFPS = afps;
}

void sgct_core::Statistics::setFrameTime(double t)
{
	mAvgFrameTime = 0.0;
	for(int i=STATS_HISTORY_LENGTH-2; i>=0; i--)
	{
		mFrameTime[i+1].y = mFrameTime[i].y;

		if(i < STATS_AVERAGE_LENGTH)
			mAvgFrameTime += mFrameTime[i].y;
	}
	mFrameTime[0].y = t;
	mAvgFrameTime += mFrameTime[0].y;
	
	mAvgFrameTime /= static_cast<double>(STATS_AVERAGE_LENGTH);
}

void sgct_core::Statistics::setDrawTime(double t)
{
	mAvgDrawTime = 0.0;
	for(int i=STATS_HISTORY_LENGTH-2; i>=0; i--)
	{
		mDrawTime[i+1].y = mDrawTime[i].y;

		if(i < STATS_AVERAGE_LENGTH)
			mAvgDrawTime += mDrawTime[i].y;
	}
	mDrawTime[0].y = t;
	mAvgDrawTime += mDrawTime[0].y;

	mAvgDrawTime /= static_cast<double>(STATS_AVERAGE_LENGTH);
}

void sgct_core::Statistics::setSyncTime(double t)
{
	mAvgSyncTime = 0.0;
	for(int i=STATS_HISTORY_LENGTH-2; i>=0; i--)
	{
		mSyncTime[i+1].y = mSyncTime[i].y;

		if(i < STATS_AVERAGE_LENGTH)
			mAvgSyncTime += mSyncTime[i].y;
	}
	mSyncTime[0].y = t;
	mAvgSyncTime += mSyncTime[0].y;

	mAvgSyncTime /= static_cast<double>(STATS_AVERAGE_LENGTH);
}

/*!
	Set the minimum and maximum time it takes for a sync message from send to receive
*/
void sgct_core::Statistics::setLoopTime(double min, double max)
{
	for(int i=STATS_HISTORY_LENGTH-2; i>=0; i--)
	{
		mLoopTimeMax[i+1].y = mLoopTimeMax[i].y;
		mLoopTimeMin[i+1].y = mLoopTimeMin[i].y;
	}
	mLoopTimeMax[0].y = max;
	mLoopTimeMax[0].y = min;
}


void sgct_core::Statistics::addSyncTime(double t)
{
	mSyncTime[0].y += t;
	mAvgSyncTime += (t/static_cast<double>(STATS_AVERAGE_LENGTH));
}

void sgct_core::Statistics::draw(unsigned int frameNumber)
{
	//make sure to only update the VBOs once per frame
	static unsigned int lastFrameNumber = 0;
	bool updateGPU = true;
	if( lastFrameNumber == frameNumber )
		updateGPU = false;
	lastFrameNumber = frameNumber;

	glDrawBuffer(GL_BACK); //draw into both back buffers

	if(mFixedPipeline)
	{
		//enter ortho mode
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glPushMatrix();
		gluOrtho2D(0.0,STATS_HISTORY_LENGTH*2.0,
			0.0,STATS_HISTORY_LENGTH);

		glMatrixMode(GL_MODELVIEW);

		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glLoadIdentity();

		glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY); //osg enables this which messes up the rendering

		//draw background (1024x1024 canvas)
		glColor4f(0.0f,0.0f,0.0f,0.5f);
		glBindBuffer(GL_ARRAY_BUFFER, mBackgroundVBO);
		glVertexPointer(2, GL_FLOAT, 0, NULL);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glTranslatef(0.0f, 32.0f, 0.0f);
		//glPushMatrix();
		glScalef(1.0f, VERT_SCALE, 1.0f);

		//draw graphs
		glLineWidth(0.9f); //in os X 1.0f was interpreted as 2. Which is a bit weird..

		//zero line, 60hz & 30hz
		glColor4f(1.0f,0.0f,0.0f,1.0f);
		glBindBuffer(GL_ARRAY_BUFFER, mFreqLinesVBO);
		glVertexPointer(2, GL_FLOAT, 0, NULL);
		glDrawArrays(GL_LINES, 0, 6);

		//1 ms lines
		glColor4f(1.0f,1.0f,1.0f,0.2f);
		glBindBuffer(GL_ARRAY_BUFFER, mGridVBO);
		glVertexPointer(2, GL_FLOAT, 0, NULL);
		glDrawArrays(GL_LINES, 0, mNumberOfLineVerts);

		GLvoid* PositionBuffer;

		for(unsigned int i=0; i<STATS_NUMBER_OF_VBOs; i++)
		{
			glColor4fv( glm::value_ptr(colors[i]) );
			glBindBuffer(GL_ARRAY_BUFFER, mVboPtrs[i]);
			if( updateGPU )
			{
				PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
				memcpy(PositionBuffer, getVerts(i), STATS_HISTORY_LENGTH * sizeof(StatsVertex));
				glUnmapBuffer(GL_ARRAY_BUFFER);
			}
			glVertexPointer(2, GL_DOUBLE, 0, NULL);
			glDrawArrays(GL_LINE_STRIP, 0, STATS_HISTORY_LENGTH);
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
		GLvoid* PositionBuffer;
		
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glLineWidth(0.9f); //in os X 1.0f was interpreted as 2. Which is a bit weird..
		
		glm::mat4 orthoMat = glm::ortho( 0.0f, static_cast<float>(STATS_HISTORY_LENGTH)*2.0f,
			0.0f, static_cast<float>(STATS_HISTORY_LENGTH) );

		mShader.bind();
		glBindVertexArray( mVAO );
		glEnableVertexAttribArray(0);

		//draw background (1024x1024 canvas)
		glUniform4f( mColLoc, 0.0f,0.0f,0.0f,0.5f );
		glUniformMatrix4fv( mMVPLoc, 1, GL_FALSE, &orthoMat[0][0]);
		glBindBuffer(GL_ARRAY_BUFFER, mBackgroundVBO);
		glVertexAttribPointer(
				0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
				2,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,				    // stride
				reinterpret_cast<void*>(0) // array buffer offset
			);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		orthoMat = glm::translate( orthoMat, glm::vec3(0.0f, 32.0f, 0.0f) );
		orthoMat = glm::scale( orthoMat, glm::vec3(1.0f, static_cast<float>(VERT_SCALE), 1.0f) );
		
		glUniformMatrix4fv( mMVPLoc, 1, GL_FALSE, &orthoMat[0][0]);

		//1 ms lines
		glUniform4f( mColLoc, 1.0f,1.0f,1.0f,0.2f );
		glBindBuffer(GL_ARRAY_BUFFER, mGridVBO);
		glVertexAttribPointer(
				0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
				2,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,				    // stride
				reinterpret_cast<void*>(0) // array buffer offset
			);
		glDrawArrays(GL_LINES, 0, mNumberOfLineVerts);

		//zero line, 60hz & 30hz
		glUniform4f( mColLoc, 1.0f,0.0f,0.0f,1.0f );
		glBindBuffer(GL_ARRAY_BUFFER, mFreqLinesVBO);
		glVertexAttribPointer(
				0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
				2,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,				    // stride
				reinterpret_cast<void*>(0) // array buffer offset
			);
		glDrawArrays(GL_LINES, 0, 6);
		
		for(unsigned int i=0; i<STATS_NUMBER_OF_VBOs; i++)
		{
			glUniform4f( mColLoc, colors[i].r, colors[i].g, colors[i].b, colors[i].a );
			glBindBuffer(GL_ARRAY_BUFFER, mVboPtrs[i]);
			if( updateGPU )
			{
				PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
				memcpy(PositionBuffer, getVerts(i), STATS_HISTORY_LENGTH * sizeof(StatsVertex));
				glUnmapBuffer(GL_ARRAY_BUFFER);
			}

			glVertexAttribPointer(
				0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
				2,                  // size
				GL_DOUBLE,          // type
				GL_FALSE,           // normalized?
				0,				    // stride
				reinterpret_cast<void*>(0) // array buffer offset
			);

			glDrawArrays(GL_LINE_STRIP, 0, STATS_HISTORY_LENGTH);
		}
		
		//unbind
		glDisableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		sgct::ShaderManager::Instance()->unBindShader();
	}
}

StatsVertex * sgct_core::Statistics::getVerts( unsigned int index )
{
	StatsVertex * retVal = NULL;
	switch(index)
	{
	case 1:
		retVal = &mDrawTime[0];
		break;

	case 2:
		retVal = &mSyncTime[0];
		break;

	case 3:
		retVal = &mLoopTimeMax[0];
		break;

	case 4:
		retVal = &mLoopTimeMin[0];
		break;

	default:
		retVal = &mFrameTime[0];
		break;
	}

	return retVal;
}
