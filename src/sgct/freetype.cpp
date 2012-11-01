/*************************************************************************
Copyright (c) 2012 Miroslav Andel, Linköping University.
All rights reserved.

Original Authors:
Freetype (TM), Miroslav Andel, Alexander Fridlund

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


#if defined(__APPLE__)
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#elif __LINUX__
#  include <GL/gl.h>
#  include <GL/glu.h>
#else
#include <windows.h>
#  include <GL/gl.h>
#  include <GL/glu.h>
#endif

//Include our header file.
#include "../include/sgct/freetype.h"
#include "../include/sgct/ClusterManager.h"

#define TEXT_RENDER_BUFFER_SIZE 512

namespace sgct_text
{

inline void pushScreenCoordinateMatrix()
{
	glPushAttrib(GL_TRANSFORM_BIT);
	//GLint	viewport[4];
	//glGetIntegerv(GL_VIEWPORT, viewport);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	sgct_core::SGCTNode * tmpNode = sgct_core::ClusterManager::Instance()->getThisNodePtr();
	//set current viewport
	//user or external scenegraph may change the viewport so it's important to reset it.
	glViewport(
		static_cast<int>(tmpNode->getCurrentViewport()->getX() * static_cast<double>(tmpNode->getWindowPtr()->getHResolution())),
		static_cast<int>(tmpNode->getCurrentViewport()->getY() * static_cast<double>(tmpNode->getWindowPtr()->getVResolution())),
		static_cast<int>(tmpNode->getCurrentViewport()->getXSize() * static_cast<double>(tmpNode->getWindowPtr()->getHResolution())),
		static_cast<int>(tmpNode->getCurrentViewport()->getYSize() * static_cast<double>(tmpNode->getWindowPtr()->getVResolution())));
	
	gluOrtho2D(
		0.0,
		tmpNode->getCurrentViewport()->getXSize() *
		static_cast<double>(tmpNode->getWindowPtr()->getHResolution()),
		0.0,
		tmpNode->getCurrentViewport()->getYSize() *
		static_cast<double>(tmpNode->getWindowPtr()->getVResolution()));
	glPopAttrib();
}

/// Pops the projection matrix without changing the current
/// MatrixMode.
inline void pop_projection_matrix() {
	glPushAttrib(GL_TRANSFORM_BIT);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
}

///Much like Nehe's glPrint function, but modified to work
///with freetype fonts.
void print(const sgct_text::Font * ft_font, float x, float y, const char *fmt, ...)
{
	if( ft_font == NULL ) { return; }

	// We want a coordinate system where things coresponding to window pixels.
	pushScreenCoordinateMatrix();

	GLuint font = ft_font->getListBase();
	float h = ft_font->getHeight() * 1.59f;

	char		text[TEXT_RENDER_BUFFER_SIZE];			// Holds Our String
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		*text=0;											// Do Nothing

	else {
	va_start(ap, fmt);									// Parses The String For Variables
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	    vsprintf_s(text, TEXT_RENDER_BUFFER_SIZE, fmt, ap);	// And Converts Symbols To Actual Numbers
#else
		vsprintf(text, fmt, ap);
#endif
	va_end(ap);											// Results Are Stored In Text
	}


	//Here is some code to split the text that we have been
	//given into a set of lines.
	//This could be made much neater by using
	//a regular expression library such as the one avliable from
	//boost.org (I've only done it out by hand to avoid complicating
	//this tutorial with unnecessary library dependencies).
	const char *start_line=text;
	std::vector<std::string> lines;
	char *c;
	for(c=text;*c;c++)
	{
		if(*c=='\n')
		{
			std::string line;
			for(const char *n=start_line;n<c;n++) line.append(1,*n);
			lines.push_back(line);
			start_line=c+1;
		}
	}
	if(start_line)
	{
		std::string line;
		for(const char *n=start_line;n<c;n++) line.append(1,*n);
		lines.push_back(line);
	}

	glPushAttrib(GL_LIST_BIT | GL_CURRENT_BIT  | GL_ENABLE_BIT | GL_TRANSFORM_BIT);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glListBase(font);

	/*float modelview_matrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, modelview_matrix);*/

	//This is where the text display actually happens.
	//For each line of text we reset the modelview matrix
	//so that the line's text will start in the correct position.
	//Notice that we need to reset the matrix, rather than just translating
	//down by h. This is because when each character is
	//draw it modifies the current matrix so that the next character
	//will be drawn immediatly after it.
	for(unsigned int i=0;i<lines.size();i++) {


		glPushMatrix();
		glLoadIdentity();
		glTranslatef(x,y-h*i,0);
		//glMultMatrixf(modelview_matrix); we don't want to get affected by current scene's transformation when printing in 2d.

	//  The commented out raster position stuff can be useful if you need to
	//  know the length of the text that you are creating.
	//  If you decide to use it make sure to also uncomment the glBitmap command
	//  in make_dlist().
	//	glRasterPos2f(0,0);
		glCallLists(lines[i].length(), GL_UNSIGNED_BYTE, lines[i].c_str());
	//	float rpos[4];
	//	glGetFloatv(GL_CURRENT_RASTER_POSITION ,rpos);
	//	float len=x-rpos[0];

		glPopMatrix();
	}

    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
	glPopAttrib();

	pop_projection_matrix();
}

void print3d(const sgct_text::Font * ft_font, float x, float y, float z, float scale, const char *fmt, ...)
{
	if( ft_font == NULL ) { return; }

	GLuint font = ft_font->getListBase();
	float h = ft_font->getHeight();

	char		text[TEXT_RENDER_BUFFER_SIZE];			// Holds Our String
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		*text=0;										// Do Nothing

	else {
	va_start(ap, fmt);									// Parses The String For Variables
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	    vsprintf_s(text, TEXT_RENDER_BUFFER_SIZE, fmt, ap);	// And Converts Symbols To Actual Numbers
#else
		vsprintf(text, fmt, ap);
#endif
	va_end(ap);											// Results Are Stored In Text
	}


	//Here is some code to split the text that we have been
	//given into a set of lines.
	//This could be made much neater by using
	//a regular expression library such as the one avliable from
	//boost.org (I've only done it out by hand to avoid complicating
	//this tutorial with unnecessary library dependencies).
	const char *start_line=text;
	char *c;
	std::vector<std::string> lines;
	for(c=text;*c;c++) {
		if(*c=='\n') {
			std::string line;
			for(const char *n=start_line;n<c;n++) line.append(1,*n);
			lines.push_back(line);
			start_line=c+1;
		}
	}
	if(start_line) {
		std::string line;
		for(const char *n=start_line;n<c;n++) line.append(1,*n);
		lines.push_back(line);
	}

	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glListBase(font);

	for(unsigned int i=0;i<lines.size();i++)
	{
		float textScale = scale / ft_font->getHeight();
		glPushMatrix();
		glTranslatef(x,y-h*i,z);
		glScalef( textScale, textScale, textScale);
		glCallLists(lines[i].length(), GL_UNSIGNED_BYTE, lines[i].c_str());
		glPopMatrix();
	}

	glEnable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
}

}

