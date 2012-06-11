/*************************************************************************
Copyright (c) 2012 Miroslav Andel, Linköping University.
All rights reserved.

Original Authors:
Miroslav Andel, Alexander Fridlund

For any questions or information about the SGCT project please contact: miroslav.andel@liu.se

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to
Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
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

core_sgct::Statistics::Statistics()
{
	AvgFPS = 0.0;
	for(int unsigned i=0; i<STATS_HISTORY_length; i++)
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
	for(int i=STATS_HISTORY_length-2; i>=0; i--)
	{
		FrameTime[i+1] = FrameTime[i];
	}
	FrameTime[0] = t;
}

void core_sgct::Statistics::setDrawTime(double t)
{
	for(int i=STATS_HISTORY_length-2; i>=0; i--)
	{
		DrawTime[i+1] = DrawTime[i];
	}
	DrawTime[0] = t;
}

void core_sgct::Statistics::setSyncTime(double t)
{
	for(int i=STATS_HISTORY_length-2; i>=0; i--)
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
	gluOrtho2D(0.0,STATS_HISTORY_length*2.0,
		0.0,STATS_HISTORY_length);

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
		glVertex2i(0, STATS_HISTORY_length);
		glVertex2i(STATS_HISTORY_length*2, STATS_HISTORY_length);
		glVertex2i(STATS_HISTORY_length*2, 0);
	glEnd();

	//draw graphs
	glLineWidth(1.0f);
	glColor4f(1.0f,0.0f,0.0f,1.0f);

	const double vertScale = 10000.0;

	//zero line, 60hz & 30hz
	glBegin(GL_LINES);
		glVertex2i(0, 32);
		glVertex2i(STATS_HISTORY_length*2, 32);
		glVertex2i(0, 32 + static_cast<int>(vertScale/60.0));
		glVertex2i(STATS_HISTORY_length*2, 32 + static_cast<int>(vertScale/60.0));
		glVertex2i(0, 32 + static_cast<int>(vertScale/30.0));
		glVertex2i(STATS_HISTORY_length*2, 32 + static_cast<int>(vertScale/30.0));
	glEnd();

	//frame time (yellow)
	glColor4f(1.0f,1.0f,0.0f,0.8f);
	glBegin(GL_LINE_STRIP);
	for(int i=0; i<STATS_HISTORY_length; i++)
		glVertex2i(i*2, 32 + static_cast<int>(FrameTime[i]*vertScale));
	glEnd();

	//draw time (magenta)
	glColor4f(1.0f,0.0f,1.0f,0.8f);
	glBegin(GL_LINE_STRIP);
	for(int i=0; i<STATS_HISTORY_length; i++)
		glVertex2i(i*2, 32 + static_cast<int>(DrawTime[i]*vertScale));
	glEnd();

	//sync time (cyan)
	glColor4f(0.0f,1.0f,1.0f,0.8f);
	glBegin(GL_LINE_STRIP);
	for(int i=0; i<STATS_HISTORY_length; i++)
		glVertex2i(i*2, 32 + static_cast<int>(SyncTime[i]*vertScale));
	glEnd();

    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
	glPopAttrib();

	//exit ortho mode
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}
