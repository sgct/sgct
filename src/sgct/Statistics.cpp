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
	for(unsigned int i=0; i<4; i++)
		mVboPtrs[i] = 0;

	for(unsigned int i=0; i<STATS_HISTORY_LENGTH; i++)
	{
		mFrameTime[i].x = static_cast<double>(i*2);
		mDrawTime[i].x = static_cast<double>(i*2);
		mSyncTime[i].x = static_cast<double>(i*2);

		mFrameTime[i].y = 0.0;
		mDrawTime[i].y = 0.0;
		mSyncTime[i].y = 0.0;
	}
}

sgct_core::Statistics::~Statistics()
{
	if(mVboPtrs[0] != 0)
		glDeleteBuffers(3, &mVboPtrs[0]);
}

void sgct_core::Statistics::initVBO()
{
	glGenBuffers(3, &mVboPtrs[0]);

	glBindBuffer(GL_ARRAY_BUFFER, mVboPtrs[FRAME_TIME]);
	glBufferData(GL_ARRAY_BUFFER, STATS_HISTORY_LENGTH * sizeof(StatsVertex), mFrameTime, GL_STREAM_DRAW );

	glBindBuffer(GL_ARRAY_BUFFER, mVboPtrs[DRAW_TIME]);
	glBufferData(GL_ARRAY_BUFFER, STATS_HISTORY_LENGTH * sizeof(StatsVertex), mDrawTime, GL_STREAM_DRAW );

	glBindBuffer(GL_ARRAY_BUFFER, mVboPtrs[SYNC_TIME]);
	glBufferData(GL_ARRAY_BUFFER, STATS_HISTORY_LENGTH * sizeof(StatsVertex), mSyncTime, GL_STREAM_DRAW );

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void sgct_core::Statistics::setAvgFPS(double afps)
{
	mAvgFPS = afps;
}

void sgct_core::Statistics::setFrameTime(double t)
{
	for(int i=STATS_HISTORY_LENGTH-2; i>=0; i--)
	{
		mFrameTime[i+1].y = mFrameTime[i].y;
	}
	mFrameTime[0].y = t;
}

void sgct_core::Statistics::setDrawTime(double t)
{
	for(int i=STATS_HISTORY_LENGTH-2; i>=0; i--)
	{
		mDrawTime[i+1].y = mDrawTime[i].y;
	}
	mDrawTime[0].y = t;
}

void sgct_core::Statistics::setSyncTime(double t)
{
	for(int i=STATS_HISTORY_LENGTH-2; i>=0; i--)
	{
		mSyncTime[i+1].y = mSyncTime[i].y;
	}
	mSyncTime[0].y = t;
}

void sgct_core::Statistics::addSyncTime(double t)
{
	mSyncTime[0].y += t;
}

void sgct_core::Statistics::draw(unsigned long long frameNumber)
{
	//make sure to only update the VBOs once per frame
	static unsigned long long lastFrameNumber = 0;
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

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glPopClientAttrib();
	glPopAttrib();

	//exit ortho mode
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}
