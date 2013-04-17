/*************************************************************************
Copyright (c) 2012 Miroslav Andel
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
#include <memory.h>

sgct_core::Statistics::Statistics()
{
	mAvgFPS = 0.0;
	mAvgDrawTime = 0.0;
	mAvgSyncTime = 0.0;
	mAvgFrameTime = 0.0;

	for(unsigned int i=0; i<4; i++)
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
		glDeleteBuffers(5, &mVboPtrs[0]);
}

void sgct_core::Statistics::initVBO()
{
	glGenBuffers(5, &mVboPtrs[0]);

	glBindBuffer(GL_ARRAY_BUFFER, mVboPtrs[FRAME_TIME]);
	glBufferData(GL_ARRAY_BUFFER, STATS_HISTORY_LENGTH * sizeof(StatsVertex), mFrameTime, GL_STREAM_DRAW );

	glBindBuffer(GL_ARRAY_BUFFER, mVboPtrs[DRAW_TIME]);
	glBufferData(GL_ARRAY_BUFFER, STATS_HISTORY_LENGTH * sizeof(StatsVertex), mDrawTime, GL_STREAM_DRAW );

	glBindBuffer(GL_ARRAY_BUFFER, mVboPtrs[SYNC_TIME]);
	glBufferData(GL_ARRAY_BUFFER, STATS_HISTORY_LENGTH * sizeof(StatsVertex), mSyncTime, GL_STREAM_DRAW );

	glBindBuffer(GL_ARRAY_BUFFER, mVboPtrs[LOOP_TIME_MAX]);
	glBufferData(GL_ARRAY_BUFFER, STATS_HISTORY_LENGTH * sizeof(StatsVertex), mLoopTimeMax, GL_STREAM_DRAW );

	glBindBuffer(GL_ARRAY_BUFFER, mVboPtrs[LOOP_TIME_MIN]);
	glBufferData(GL_ARRAY_BUFFER, STATS_HISTORY_LENGTH * sizeof(StatsVertex), mLoopTimeMin, GL_STREAM_DRAW );

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
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
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glLoadIdentity();

	//draw background (1024x1024 canvas)
	glColor4f(0.0f,0.0f,0.0f,0.5f);
	glBegin(GL_QUADS);
		glVertex2i(0, 0);
		glVertex2i(0, STATS_HISTORY_LENGTH);
		glVertex2i(STATS_HISTORY_LENGTH*2, STATS_HISTORY_LENGTH);
		glVertex2i(STATS_HISTORY_LENGTH*2, 0);
	glEnd();

	glTranslatef(0.0f, 32.0f, 0.0f);
	//glPushMatrix();
	glScalef(1.0f, VERT_SCALE, 1.0f);

	//draw graphs
	glLineWidth(0.9f); //in os X 1.0f was interpreted as 2. Which is a bit weird..

	//zero line, 60hz & 30hz
	glColor4f(1.0f,0.0f,0.0f,1.0f);
	glBegin(GL_LINES);
		glVertex2f(0.0f, 0.0f);
		glVertex2f(static_cast<float>(STATS_HISTORY_LENGTH*2), 0.0f);
		glVertex2f(0.0f, 1.0f/60.0f);
		glVertex2f(static_cast<float>(STATS_HISTORY_LENGTH*2), 1.0f/60.0f);
		glVertex2f(0.0f, 1.0f/30.0f);
		glVertex2f(static_cast<float>(STATS_HISTORY_LENGTH*2), 1.0f/30.0f);
	glEnd();

	//1 ms lines
	glColor4f(1.0f,1.0f,1.0f,0.2f);
	glBegin(GL_LINES);
	for(float f = 0.001f; f < (1.0f/30.0f); f += 0.001f )
	{
		glVertex2f(0.0f, f);
		glVertex2f(static_cast<float>(STATS_HISTORY_LENGTH*2), f);
	}
	glEnd();

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY); //osg enables this which messes up the rendering

	GLvoid* PositionBuffer;

	//frame time (yellow)
	glColor4f(1.0f,1.0f,0.0f,0.8f);
	glBindBuffer(GL_ARRAY_BUFFER, mVboPtrs[FRAME_TIME]);
	if( updateGPU )
	{
		PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		memcpy(PositionBuffer, mFrameTime, STATS_HISTORY_LENGTH * sizeof(StatsVertex));
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
	glVertexPointer(2, GL_DOUBLE, 0, NULL);
	glDrawArrays(GL_LINE_STRIP, 0, STATS_HISTORY_LENGTH);

	//draw time (magenta)
	glColor4f(1.0f,0.0f,1.0f,0.8f);
	glBindBuffer(GL_ARRAY_BUFFER, mVboPtrs[DRAW_TIME]);
	if( updateGPU )
	{
		PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		memcpy(PositionBuffer, mDrawTime, STATS_HISTORY_LENGTH * sizeof(StatsVertex));
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
	glVertexPointer(2, GL_DOUBLE, 0, NULL);
	glDrawArrays(GL_LINE_STRIP, 0, STATS_HISTORY_LENGTH);

	//sync time (cyan)
	glColor4f(0.0f,1.0f,1.0f,0.8f);
	glBindBuffer(GL_ARRAY_BUFFER, mVboPtrs[SYNC_TIME]);
	if( updateGPU )
	{
		PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		memcpy(PositionBuffer, mSyncTime, STATS_HISTORY_LENGTH * sizeof(StatsVertex));
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
	glVertexPointer(2, GL_DOUBLE, 0, NULL);
	glDrawArrays(GL_LINE_STRIP, 0, STATS_HISTORY_LENGTH);

	//loop time max
	glColor4f(0.2f,0.2f,1.0f,0.8f);
	glBindBuffer(GL_ARRAY_BUFFER, mVboPtrs[LOOP_TIME_MAX]);
	if( updateGPU )
	{
		PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		memcpy(PositionBuffer, mLoopTimeMax, STATS_HISTORY_LENGTH * sizeof(StatsVertex));
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
	glVertexPointer(2, GL_DOUBLE, 0, NULL);
	glDrawArrays(GL_LINE_STRIP, 0, STATS_HISTORY_LENGTH);

	glColor4f(0.2f,0.2f,1.0f,0.8f);
	glBindBuffer(GL_ARRAY_BUFFER, mVboPtrs[LOOP_TIME_MIN]);
	if( updateGPU )
	{
		PositionBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		memcpy(PositionBuffer, mLoopTimeMin, STATS_HISTORY_LENGTH * sizeof(StatsVertex));
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}
	glVertexPointer(2, GL_DOUBLE, 0, NULL);
	glDrawArrays(GL_LINE_STRIP, 0, STATS_HISTORY_LENGTH);

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glPopClientAttrib();
	glPopAttrib();

	//exit ortho mode
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}
