#include <GL/glew.h>
#if __WIN32__
#include <GL/wglew.h>
#else
#include <OpenGL/glext.h>
#endif
#include <GL/glfw.h>

#include "../include/sgct/Statistics.h"

core_sgct::Statistics::Statistics()
{
	AvgFPS = 0.0;
	for(int unsigned i=0; i<STATS_HISTORY_LENGHT; i++)
	{
		FrameTime[i] = 0.0;
		DrawTime[i] = 0.0;
		SyncTime[i] = 0.0;
	}
}

void core_sgct::Statistics::setAvgFPS(double afps)
{
	AvgFPS = afps;
}

void core_sgct::Statistics::setFrameTime(double t)
{
	for(int i=STATS_HISTORY_LENGHT-2; i>=0; i--)
	{
		FrameTime[i+1] = FrameTime[i];
	}
	FrameTime[0] = t;
}

void core_sgct::Statistics::setDrawTime(double t)
{
	for(int i=STATS_HISTORY_LENGHT-2; i>=0; i--)
	{
		DrawTime[i+1] = DrawTime[i];
	}
	DrawTime[0] = t;
}

void core_sgct::Statistics::setSyncTime(double t)
{
	for(int i=STATS_HISTORY_LENGHT-2; i>=0; i--)
	{
		SyncTime[i+1] = SyncTime[i];
	}
	SyncTime[0] = t;
}

void core_sgct::Statistics::addSyncTime(double t)
{
	SyncTime[0] += t;
}

void core_sgct::Statistics::draw()
{
	glDrawBuffer(GL_BACK); //draw into both back buffers

	//enter ortho mode
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glPushMatrix();
	gluOrtho2D(0.0,STATS_HISTORY_LENGHT*2.0,
		0.0,STATS_HISTORY_LENGHT);

	glPushAttrib(GL_LIST_BIT | GL_CURRENT_BIT  | GL_ENABLE_BIT | GL_TRANSFORM_BIT);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glLoadIdentity();

	//draw background (1000x1000 canvas)
	glColor4f(0.0f,0.0f,0.0f,0.5f);
	glBegin(GL_QUADS);
		glVertex2i(0, 0);
		glVertex2i(0, STATS_HISTORY_LENGHT);
		glVertex2i(STATS_HISTORY_LENGHT*2, STATS_HISTORY_LENGHT);
		glVertex2i(STATS_HISTORY_LENGHT*2, 0);
	glEnd();

	//draw graphs
	glLineWidth(1.0f);
	glColor4f(1.0f,0.0f,0.0f,1.0f);

	const double vertScale = 10000.0;

	//zero line, 60hz & 30hz
	glBegin(GL_LINES);
		glVertex2i(0, 32);
		glVertex2i(STATS_HISTORY_LENGHT*2, 32);
		glVertex2i(0, 32 + static_cast<int>(vertScale/60.0));
		glVertex2i(STATS_HISTORY_LENGHT*2, 32 + static_cast<int>(vertScale/60.0));
		glVertex2i(0, 32 + static_cast<int>(vertScale/30.0));
		glVertex2i(STATS_HISTORY_LENGHT*2, 32 + static_cast<int>(vertScale/30.0));
	glEnd();

	//frame time (yellow)
	glColor4f(1.0f,1.0f,0.0f,0.8f);
	glBegin(GL_LINE_STRIP);
	for(int i=0; i<STATS_HISTORY_LENGHT; i++)
		glVertex2i(i*2, 32 + static_cast<int>(FrameTime[i]*vertScale));
	glEnd();

	//draw time (magenta)
	glColor4f(1.0f,0.0f,1.0f,0.8f);
	glBegin(GL_LINE_STRIP);
	for(int i=0; i<STATS_HISTORY_LENGHT; i++)
		glVertex2i(i*2, 32 + static_cast<int>(DrawTime[i]*vertScale));
	glEnd();

	//sync time (cyan)
	glColor4f(0.0f,1.0f,1.0f,0.8f);
	glBegin(GL_LINE_STRIP);
	for(int i=0; i<STATS_HISTORY_LENGHT; i++)
		glVertex2i(i*2, 32 + static_cast<int>(SyncTime[i]*vertScale));
	glEnd();

	glPopAttrib();

	//exit ortho mode
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}
