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
	mMVPLoc = -1;
	mColLoc = -1;
	mNumberOfLineVerts = 0;

	mDynamicColors[ FRAME_TIME ]	= glm::vec4( 1.0f,1.0f,0.0f,0.8f );
	mDynamicColors[ DRAW_TIME ]		= glm::vec4( 1.0f,0.0f,1.0f,0.8f );
	mDynamicColors[ SYNC_TIME ]		= glm::vec4( 0.0f,1.0f,1.0f,0.8f );
	mDynamicColors[ LOOP_TIME_MAX ] = glm::vec4( 0.4f,0.4f,1.0f,0.8f );
	mDynamicColors[ LOOP_TIME_MIN ] = glm::vec4( 0.0f,0.0f,0.8f,0.8f );

	mStaticColors[ GRID ]	= glm::vec4( 1.0f,1.0f,1.0f,0.2f );
	mStaticColors[ FREQ ]	= glm::vec4( 1.0f,0.0f,0.0f,1.0f );
	mStaticColors[ BG ]		= glm::vec4( 0.0f,0.0f,0.0f,0.5f );

	for(unsigned int i=0; i<STATS_NUMBER_OF_DYNAMIC_OBJS; i++)
	{
		mDynamicVBOs[i] = GL_FALSE;
		mDynamicVAOs[i] = GL_FALSE;
	}

	for(unsigned int i=0; i<STATS_NUMBER_OF_STATIC_OBJS; i++)
	{
		mStaticVBOs[i] = GL_FALSE;
		mStaticVAOs[i] = GL_FALSE;
	}

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
	if(mDynamicVBOs[0])
		glDeleteBuffers(STATS_NUMBER_OF_DYNAMIC_OBJS, &mDynamicVBOs[0]);
	if(mDynamicVAOs[0])
		glDeleteVertexArrays(STATS_NUMBER_OF_DYNAMIC_OBJS, &mDynamicVAOs[0]);
	if(mStaticVBOs[0])
		glDeleteBuffers(STATS_NUMBER_OF_STATIC_OBJS, &mStaticVBOs[0]);
	if(mStaticVAOs[0])
		glDeleteBuffers(STATS_NUMBER_OF_STATIC_OBJS, &mStaticVAOs[0]);
}

void sgct_core::Statistics::initVBO(bool fixedPipeline)
{
	mFixedPipeline = fixedPipeline;

	if(!mFixedPipeline)
	{
		glGenVertexArrays(STATS_NUMBER_OF_DYNAMIC_OBJS, &mDynamicVAOs[0]);
		glGenVertexArrays(STATS_NUMBER_OF_STATIC_OBJS, &mStaticVAOs[0]);
	}

	glGenBuffers(STATS_NUMBER_OF_DYNAMIC_OBJS, &mDynamicVBOs[0]);
	glGenBuffers(STATS_NUMBER_OF_STATIC_OBJS, &mStaticVBOs[0]);
		
	for(unsigned int i=0; i<STATS_NUMBER_OF_DYNAMIC_OBJS; i++)
	{
		if(!mFixedPipeline)
		{
			glBindVertexArray(mDynamicVAOs[i]);
			glEnableVertexAttribArray(0);
		}
		glBindBuffer(GL_ARRAY_BUFFER, mDynamicVBOs[i]);
		glBufferData(GL_ARRAY_BUFFER, STATS_HISTORY_LENGTH * sizeof(StatsVertex), getVerts(i), GL_DYNAMIC_DRAW );
		if(!mFixedPipeline)
			glVertexAttribPointer( 0, 2, GL_DOUBLE, GL_FALSE, 0, NULL );
	}

	//static data
	std::vector<float> gridVerts;
	for(float f = 0.001f; f < (1.0f/30.0f); f += 0.001f )
	{
		gridVerts.push_back( 0.0f );
		gridVerts.push_back( f );
		gridVerts.push_back( static_cast<float>(STATS_HISTORY_LENGTH*2) );
		gridVerts.push_back( f );
	}
	
	if(!mFixedPipeline)
	{
		glBindVertexArray(mStaticVAOs[ GRID ]);
		glEnableVertexAttribArray(0);
	}
	glBindBuffer(GL_ARRAY_BUFFER, mStaticVBOs[ GRID ]);
	glBufferData(GL_ARRAY_BUFFER, gridVerts.size() * sizeof(float), &gridVerts[0], GL_STATIC_DRAW );
	if(!mFixedPipeline)
		glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0) );

	mNumberOfLineVerts = gridVerts.size() / 2;

	float lineVerts[] = { 0.0f, 0.0f,
		static_cast<float>(STATS_HISTORY_LENGTH*2), 0.0f,
		0.0f, 1.0f/60.0f,
		static_cast<float>(STATS_HISTORY_LENGTH*2), 1.0f/60.0f,
		0.0f, 1.0f/30.0f,
		static_cast<float>(STATS_HISTORY_LENGTH*2), 1.0f/30.0f };
	
	if(!mFixedPipeline)
	{
		glBindVertexArray(mStaticVAOs[ FREQ ]);
		glEnableVertexAttribArray(0);
	}
	glBindBuffer(GL_ARRAY_BUFFER, mStaticVBOs[ FREQ ]);
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), &lineVerts[0], GL_STATIC_DRAW );
	if(!mFixedPipeline)
		glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0) );
	
	float bgVerts[] = { 0.0f, 0.0f,
		static_cast<float>(STATS_HISTORY_LENGTH*2), 0.0f,
		0.0f, 1.0f/30.0f,
		static_cast<float>(STATS_HISTORY_LENGTH*2), 1.0f/30.0f };
	
	if(!mFixedPipeline)
	{
		glBindVertexArray(mStaticVAOs[ BG ]);
		glEnableVertexAttribArray(0);
	}
	glBindBuffer(GL_ARRAY_BUFFER, mStaticVBOs[ BG ]);
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), &bgVerts[0], GL_STATIC_DRAW );
	if(!mFixedPipeline)
		glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0) );

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if(!mFixedPipeline)
	{
		glBindVertexArray(0);

		sgct::ShaderManager::Instance()->addShader( mShader, "StatisticsShader",
			Stats_Vert_Shader,
			Stats_Frag_Shader, sgct::ShaderManager::SHADER_SRC_STRING );
		mShader.bind();

		mMVPLoc = mShader.getUniformLocation( "MVP" );
		mColLoc = mShader.getUniformLocation( "Col" );

		sgct::ShaderManager::Instance()->unBindShader();
	}
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
	mLoopTimeMin[0].y = min;
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

	//update buffers if needed
	if( updateGPU )
	{
		GLvoid* PositionBuffer;

		for(unsigned int i=0; i<STATS_NUMBER_OF_DYNAMIC_OBJS; i++)
		{
			glBindBuffer(GL_ARRAY_BUFFER, mDynamicVBOs[i]);
			PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
			if( PositionBuffer != NULL )
				memcpy(PositionBuffer, getVerts(i), STATS_HISTORY_LENGTH * sizeof(StatsVertex));
			//This shouldn't happen.. but in case
			if( glUnmapBuffer(GL_ARRAY_BUFFER) == GL_FALSE )
			{
				//re-init
				glBufferData(GL_ARRAY_BUFFER, STATS_HISTORY_LENGTH * sizeof(StatsVertex), getVerts(i), GL_DYNAMIC_DRAW );
				if(!mFixedPipeline)
					glVertexAttribPointer( 0, 2, GL_DOUBLE, GL_FALSE, 0, NULL );
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

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
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glLoadIdentity();

		glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
		glEnableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY); //osg enables this which messes up the rendering

		glTranslatef(0.0f, 32.0f, 0.0f);
		//glPushMatrix();
		glScalef(1.0f, VERT_SCALE, 1.0f);

		glLineWidth(0.9f); //in os X 1.0f was interpreted as 2. Which is a bit weird..

		//draw background (1024x1024 canvas)
		glColor4fv( glm::value_ptr(mStaticColors[ BG ]) );
		glBindBuffer(GL_ARRAY_BUFFER, mStaticVBOs[ BG ]);
		glVertexPointer(2, GL_FLOAT, 0, NULL);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		//zero line, 60hz & 30hz
		glColor4fv( glm::value_ptr(mStaticColors[ FREQ ]) );
		glBindBuffer(GL_ARRAY_BUFFER, mStaticVBOs[ FREQ ]);
		glVertexPointer(2, GL_FLOAT, 0, NULL);
		glDrawArrays(GL_LINES, 0, 6);

		//1 ms lines
		glColor4fv( glm::value_ptr(mStaticColors[ GRID ]) );
		glBindBuffer(GL_ARRAY_BUFFER, mStaticVBOs[ GRID ]);
		glVertexPointer(2, GL_FLOAT, 0, NULL);
		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mNumberOfLineVerts));

		for(unsigned int i=0; i<STATS_NUMBER_OF_DYNAMIC_OBJS; i++)
		{
			glColor4fv( glm::value_ptr(mDynamicColors[i]) );
			glBindBuffer(GL_ARRAY_BUFFER, mDynamicVBOs[i]);
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
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glLineWidth(0.9f); //in os X 1.0f was interpreted as 2. Which is a bit weird..
		
		glm::mat4 orthoMat = glm::ortho( 0.0f, static_cast<float>(STATS_HISTORY_LENGTH)*2.0f,
			0.0f, static_cast<float>(STATS_HISTORY_LENGTH) );
		orthoMat = glm::translate( orthoMat, glm::vec3(0.0f, 32.0f, 0.0f) );
		orthoMat = glm::scale( orthoMat, glm::vec3(1.0f, static_cast<float>(VERT_SCALE), 1.0f) );

		mShader.bind();
		glUniformMatrix4fv( mMVPLoc, 1, GL_FALSE, &orthoMat[0][0]);
		
		//draw background (1024x1024 canvas)
		glUniform4f( mColLoc, mStaticColors[ BG ].r, mStaticColors[ BG ].g, mStaticColors[ BG ].b, mStaticColors[ BG ].a );
		glBindVertexArray( mStaticVAOs[ BG ] );
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		//1 ms lines
		glUniform4f( mColLoc, mStaticColors[ GRID ].r, mStaticColors[ GRID ].g, mStaticColors[ GRID ].b, mStaticColors[ GRID ].a );
		glBindVertexArray( mStaticVAOs[ GRID ] );
		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mNumberOfLineVerts));
		
		//zero line, 60hz & 30hz
		glUniform4f( mColLoc, mStaticColors[ FREQ ].r, mStaticColors[ FREQ ].g, mStaticColors[ FREQ ].b, mStaticColors[ FREQ ].a );
		glBindVertexArray( mStaticVAOs[ FREQ ] );
		glDrawArrays( GL_LINES, 0, 6 );
		
		for(unsigned int i=0; i<STATS_NUMBER_OF_DYNAMIC_OBJS; i++)
		{
			glUniform4f( mColLoc, mDynamicColors[i].r, mDynamicColors[i].g, mDynamicColors[i].b, mDynamicColors[i].a );
			glBindVertexArray( mDynamicVAOs[ i ] );
			glDrawArrays(GL_LINE_STRIP, 0, STATS_HISTORY_LENGTH);
		}
		
		//unbind
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
