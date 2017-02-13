/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel, Linköping University.
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/ogl_headers.h>
#include <sgct/freetype.h>
#include <sgct/FontManager.h>
#include <sgct/ShaderManager.h>
#include <sgct/MessageHandler.h>
#include <sgct/Engine.h>
#include <sgct/helpers/SGCTPortedFunctions.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace sgct_text
{

inline void setupViewport()
{
	sgct::SGCTWindow * cWin = sgct::Engine::instance()->getCurrentWindowPtr();

	int x, y, xSize, ySize;
	x		= static_cast<int>(cWin->getCurrentViewport()->getX() * static_cast<float>(cWin->getXFramebufferResolution()));
	y		= static_cast<int>(cWin->getCurrentViewport()->getY() * static_cast<float>(cWin->getYFramebufferResolution()));
	xSize	= static_cast<int>(cWin->getCurrentViewport()->getXSize() * static_cast<float>(cWin->getXFramebufferResolution()));
	ySize	= static_cast<int>(cWin->getCurrentViewport()->getYSize() * static_cast<float>(cWin->getYFramebufferResolution()));

	sgct::SGCTWindow::StereoMode sm = cWin->getStereoMode();
	if( sm >= sgct::SGCTWindow::Side_By_Side_Stereo )
	{
		if( sgct::Engine::instance()->getCurrentFrustumMode() == sgct_core::Frustum::StereoLeftEye )
		{
			switch(sm)
			{
			case sgct::SGCTWindow::Side_By_Side_Stereo:
				x = x >> 1; //x offset
				xSize = xSize >> 1; //x size
				break;

			case sgct::SGCTWindow::Side_By_Side_Inverted_Stereo:
				x = (x >> 1) + (xSize >> 1); //x offset
				xSize = xSize >> 1; //x size
				break;

			case sgct::SGCTWindow::Top_Bottom_Stereo:
				y = (y >> 1) + (ySize >> 1); //y offset
				ySize = ySize >> 1; //y size
				break;

			case sgct::SGCTWindow::Top_Bottom_Inverted_Stereo:
				y = y >> 1; //y offset
				ySize = ySize >> 1; //y size
				break;

            default:
                break;
			}
		}
		else
		{
			switch(sm)
			{
			case sgct::SGCTWindow::Side_By_Side_Stereo:
				x = (x >> 1) + (xSize >> 1); //x offset
				xSize = xSize >> 1; //x size
				break;

			case sgct::SGCTWindow::Side_By_Side_Inverted_Stereo:
				x = x >> 1; //x offset
				xSize = xSize >> 1; //x size
				break;

			case sgct::SGCTWindow::Top_Bottom_Stereo:
				y = y >> 1; //y offset
				ySize = ySize >> 1; //y size
				break;

			case sgct::SGCTWindow::Top_Bottom_Inverted_Stereo:
				y = (y >> 1) + (ySize >> 1); //y offset
				ySize = ySize >> 1; //y size
				break;

            default:
                break;
			}
		}
	}

	glViewport( x, y, xSize, ySize );
}

inline glm::mat4 setupOrthoMat()
{
	glm::mat4 orthoMat;
	sgct::SGCTWindow * cWin = sgct::Engine::instance()->getCurrentWindowPtr();

	orthoMat = glm::ortho(0.0f,
		cWin->getCurrentViewport()->getXSize() *
		static_cast<float>(cWin->getXResolution())*
        cWin->getXScale(),
		0.0f,
		cWin->getCurrentViewport()->getYSize() *
		static_cast<float>(cWin->getYResolution())*
        cWin->getYScale());

	return orthoMat;
}

inline void pushScreenCoordinateMatrix()
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	setupViewport();
	glLoadMatrixf( glm::value_ptr( setupOrthoMat() ) );
}

/// Pops the projection matrix without changing the current
/// MatrixMode.
inline void pop_projection_matrix()
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

///Much like Nehe's glPrint function, but modified to work
///with freetype fonts.
void print(const sgct_text::Font * ft_font, float x, float y, const char *format, ...)
{
	if (ft_font == NULL || format == NULL)
		return;

	float h = ft_font->getHeight() * 1.59f;

	va_list		args;	 // Pointer To List Of Arguments
	va_start(args, format); // Parses The String For Variables
    
    int size = 1 + vscprintf(format, args);
	char * buffer = new (std::nothrow) char[size];
	if (buffer == NULL)
		return;

    memset(buffer, 0, size);
    
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	vsprintf_s(buffer, size, format, args);
#else
    vsprintf(buffer, format, args);
#endif
	va_end(args);										// Results Are Stored In Text

	//Here is some code to split the text that we have been
	//given into a set of lines.
	//This could be made much neater by using
	//a regular expression library such as the one avliable from
	//boost.org (I've only done it out by hand to avoid complicating
	//this tutorial with unnecessary library dependencies).
	const char *start_line = buffer;
	std::vector<std::string> lines;
	char *c;
	for (c = buffer; *c; c++)
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

	glm::vec4 color( 1.0f, 1.0f, 1.0f, 1.0f );
	if( sgct::Engine::instance()->isOGLPipelineFixed() )
	{
		GLuint font = ft_font->getListBase();

		glPushAttrib(GL_LIST_BIT | GL_CURRENT_BIT  | GL_ENABLE_BIT | GL_TRANSFORM_BIT);
		pushScreenCoordinateMatrix();
		glMatrixMode(GL_MODELVIEW);
		glDisable(GL_LIGHTING);
		glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		FontManager::instance()->getShader().bind();
		glUniform4f( FontManager::instance()->getColLoc(), color.r, color.g, color.b, color.a );
		glm::vec4 strokeColor = FontManager::instance()->getStrokeColor();
		glUniform4f( FontManager::instance()->getStkLoc(), strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

		for(size_t i=0;i<lines.size();i++)
		{
			glm::vec3 trans(x, y-h*static_cast<float>(i), 0.0f);
			for(size_t j=0; j < lines[i].length(); j++)
			{
				char c = lines[i].c_str()[j];

				glPushMatrix();
				glLoadIdentity();
				glTranslatef( trans.x, trans.y, trans.z );
				trans += glm::vec3( ft_font->getCharWidth(c), 0.0f, 0.0f );

				glBindTexture(GL_TEXTURE_2D, ft_font->getTexture(c) );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
				glUniform1i( FontManager::instance()->getTexLoc(), 0);

				glCallList( font + c );
				glPopMatrix();
			}
		}

		sgct::ShaderProgram::unbind();

		pop_projection_matrix();
		glPopAttrib();
	}
	else
	{
		setupViewport();
		glm::mat4 projectionMat( setupOrthoMat() );

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		FontManager::instance()->getShader().bind();

		glBindVertexArray( ft_font->getVAO() );

		glActiveTexture(GL_TEXTURE0);

		glUniform4f( FontManager::instance()->getColLoc(), color.r, color.g, color.b, color.a );
		glm::vec4 strokeColor = FontManager::instance()->getStrokeColor();
		glUniform4f( FontManager::instance()->getStkLoc(), strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

		for(size_t i=0;i<lines.size();i++)
		{
			glm::mat4 trans = glm::translate( projectionMat, glm::vec3(x, y-h*static_cast<float>(i), 0.0f));

			for(size_t j=0; j < lines[i].length(); j++)
			{
				char c = lines[i].c_str()[j];

				glBindTexture(GL_TEXTURE_2D, ft_font->getTexture(c) );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
				glUniform1i( FontManager::instance()->getTexLoc(), 0);

				glUniformMatrix4fv( FontManager::instance()->getMVPLoc(), 1, GL_FALSE, &trans[0][0]);
				trans = glm::translate( trans, glm::vec3( ft_font->getCharWidth(lines[i].c_str()[j]), 0.0f, 0.0f ));

				glDrawArrays(GL_TRIANGLE_STRIP, static_cast<GLint>(c)*4, 4);
			}//end for chars
		}//end for lines

		//unbind
		glBindVertexArray(0);
		sgct::ShaderProgram::unbind();
	}

	delete[] buffer;
}

///Much like Nehe's glPrint function, but modified to work
///with freetype fonts.
void print(const sgct_text::Font * ft_font, float x, float y, glm::vec4 color, const char *format, ...)
{
	if (ft_font == NULL || format == NULL)
		return;

	float h = ft_font->getHeight() * 1.59f;

	va_list		args;	 // Pointer To List Of Arguments
	va_start(args, format); // Parses The String For Variables
    
    int size = 1 + vscprintf(format, args);
	char * buffer = new (std::nothrow) char[size];
	if (buffer == NULL)
		return;

    memset(buffer, 0, size);
    
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	vsprintf_s(buffer, size, format, args);
#else
    vsprintf(buffer, format, args);
#endif
	va_end(args);


	//Here is some code to split the text that we have been
	//given into a set of lines.
	//This could be made much neater by using
	//a regular expression library such as the one avliable from
	//boost.org (I've only done it out by hand to avoid complicating
	//this tutorial with unnecessary library dependencies).
	const char *start_line = buffer;
	std::vector<std::string> lines;
	char *c;
	for (c = buffer; *c; c++)
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

	if( sgct::Engine::instance()->isOGLPipelineFixed() )
	{
		GLuint font = ft_font->getListBase();

		glPushAttrib(GL_LIST_BIT | GL_CURRENT_BIT  | GL_ENABLE_BIT | GL_TRANSFORM_BIT);
		pushScreenCoordinateMatrix();
		glMatrixMode(GL_MODELVIEW);
		glDisable(GL_LIGHTING);
		glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		FontManager::instance()->getShader().bind();
		glUniform4f( FontManager::instance()->getColLoc(), color.r, color.g, color.b, color.a );
		glm::vec4 strokeColor = FontManager::instance()->getStrokeColor();
		glUniform4f( FontManager::instance()->getStkLoc(), strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

		for(size_t i=0;i<lines.size();i++)
		{
			glm::vec3 trans(x, y-h*static_cast<float>(i), 0.0f);
			for(size_t j=0; j < lines[i].length(); j++)
			{
				char c = lines[i].c_str()[j];

				glPushMatrix();
				glLoadIdentity();
				glTranslatef( trans.x, trans.y, trans.z );
				trans += glm::vec3( ft_font->getCharWidth(c), 0.0f, 0.0f );

				glBindTexture(GL_TEXTURE_2D, ft_font->getTexture(c) );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
				glUniform1i( FontManager::instance()->getTexLoc(), 0);

				glCallList( font + c );
				glPopMatrix();
			}
		}

		sgct::ShaderProgram::unbind();

		pop_projection_matrix();
		glPopAttrib();
	}
	else
	{
		setupViewport();
        glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		FontManager::instance()->getShader().bind();

		glBindVertexArray( ft_font->getVAO() );

		glActiveTexture(GL_TEXTURE0);

		glUniform4f( FontManager::instance()->getColLoc(), color.r, color.g, color.b, color.a );
		glm::vec4 strokeColor = FontManager::instance()->getStrokeColor();
		glUniform4f( FontManager::instance()->getStkLoc(), strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

		for(size_t i=0;i<lines.size();i++)
		{
			glm::mat4 trans = glm::translate( setupOrthoMat(), glm::vec3(x, y-h*static_cast<float>(i), 0.0f));

			for(size_t j=0; j < lines[i].length(); j++)
			{
				char c = lines[i].c_str()[j];

				glBindTexture(GL_TEXTURE_2D, ft_font->getTexture(c) );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
				glUniform1i( FontManager::instance()->getTexLoc(), 0);

				glUniformMatrix4fv( FontManager::instance()->getMVPLoc(), 1, GL_FALSE, &trans[0][0]);
				trans = glm::translate( trans, glm::vec3( ft_font->getCharWidth(lines[i].c_str()[j]), 0.0f, 0.0f ));

				glDrawArrays(GL_TRIANGLE_STRIP, static_cast<GLint>(c)*4, 4);
			}//end for chars
		}//end for lines

		//unbind
		glBindVertexArray(0);
		sgct::ShaderProgram::unbind();
	}

    delete[] buffer;
}

void print3d(const sgct_text::Font * ft_font, glm::mat4 mvp, const char *format, ...)
{
	if (ft_font == NULL || format == NULL)
		return;

	GLuint font = ft_font->getListBase();
	float h = ft_font->getHeight() * 1.59f;

    va_list		args;	 // Pointer To List Of Arguments
	va_start(args, format); // Parses The String For Variables
    
    int size = 1 + vscprintf(format, args);
	char * buffer = new (std::nothrow) char[size];
	if (buffer == NULL)
		return;

    memset(buffer, 0, size);
    
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	vsprintf_s(buffer, size, format, args);
#else
    vsprintf(buffer, format, args);
#endif
	va_end(args);

	//Here is some code to split the text that we have been
	//given into a set of lines.
	//This could be made much neater by using
	//a regular expression library such as the one avliable from
	//boost.org (I've only done it out by hand to avoid complicating
	//this tutorial with unnecessary library dependencies).
	const char *start_line = buffer;
	char *c;
	std::vector<std::string> lines;
	for (c = buffer; *c; c++) {
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

	glm::vec4 color( 1.0f, 1.0f, 1.0f, 1.0f );
	if( sgct::Engine::instance()->isOGLPipelineFixed() )
	{
		glPushAttrib(GL_LIST_BIT | GL_CURRENT_BIT  | GL_ENABLE_BIT | GL_TRANSFORM_BIT);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_LIGHTING);
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);

		float textScale = 1.0f / ft_font->getHeight();
		glm::mat4 scaleMat = glm::scale( mvp, glm::vec3(textScale) );

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		FontManager::instance()->getShader().bind();
		glUniform4f( FontManager::instance()->getColLoc(), color.r, color.g, color.b, color.a );
		glm::vec4 strokeColor = FontManager::instance()->getStrokeColor();
		glUniform4f( FontManager::instance()->getStkLoc(), strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

		for(size_t i=0;i<lines.size();i++)
		{
			glm::mat4 trans = glm::translate( scaleMat, glm::vec3(0.0f, -h*static_cast<float>(i), 0.0f));
			for(size_t j=0; j < lines[i].length(); j++)
			{
				char c = lines[i].c_str()[j];

				glLoadMatrixf( glm::value_ptr( trans ) );
				trans = glm::translate( trans, glm::vec3( ft_font->getCharWidth(c), 0.0f, 0.0f ));

				glBindTexture(GL_TEXTURE_2D, ft_font->getTexture(c) );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				glUniform1i( FontManager::instance()->getTexLoc(), 0);
				glCallList( font + c );
			}
		}

		sgct::ShaderProgram::unbind();
		glPopMatrix();
		glPopAttrib();
	}
	else
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		FontManager::instance()->getShader().bind();

		glBindVertexArray( ft_font->getVAO() );
		glActiveTexture(GL_TEXTURE0);

		glUniform4f( FontManager::instance()->getColLoc(), color.r, color.g, color.b, color.a );
		glm::vec4 strokeColor = FontManager::instance()->getStrokeColor();
		glUniform4f( FontManager::instance()->getStkLoc(), strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

		float textScale = 1.0f / ft_font->getHeight();
		glm::mat4 scaleMat = glm::scale( mvp, glm::vec3(textScale) );

		for(size_t i=0;i<lines.size();i++)
		{
			glm::mat4 trans = glm::translate( scaleMat, glm::vec3(0.0f, -h*static_cast<float>(i), 0.0f));

			for(size_t j=0; j < lines[i].length(); j++)
			{
				char c = lines[i].c_str()[j];

				glBindTexture(GL_TEXTURE_2D, ft_font->getTexture(c) );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				glUniform1i( FontManager::instance()->getTexLoc(), 0);

				glUniformMatrix4fv( FontManager::instance()->getMVPLoc(), 1, GL_FALSE, &trans[0][0]);
				trans = glm::translate( trans, glm::vec3( ft_font->getCharWidth(lines[i].c_str()[j]), 0.0f, 0.0f ));

				glDrawArrays(GL_TRIANGLE_STRIP, static_cast<GLint>(c)*4, 4);
			}//end for chars
		}//end for lines

		//unbind
		glBindVertexArray(0);
		sgct::ShaderProgram::unbind();
	}

    delete[] buffer;
}

void print3d(const sgct_text::Font * ft_font, glm::mat4 mvp, glm::vec4 color, const char *format, ...)
{
	if (ft_font == NULL || format == NULL)
		return;

	GLuint font = ft_font->getListBase();
	float h = ft_font->getHeight() * 1.59f;

	va_list		args;	 // Pointer To List Of Arguments
	va_start(args, format); // Parses The String For Variables
    
    int size = 1 + vscprintf(format, args);
	char * buffer = new (std::nothrow) char[size];
	if (buffer == NULL)
		return;

    memset(buffer, 0, size);
    
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	vsprintf_s(buffer, size, format, args);
#else
    vsprintf(buffer, format, args);
#endif

	//Here is some code to split the text that we have been
	//given into a set of lines.
	//This could be made much neater by using
	//a regular expression library such as the one avliable from
	//boost.org (I've only done it out by hand to avoid complicating
	//this tutorial with unnecessary library dependencies).
	const char *start_line = buffer;
	char *c;
	std::vector<std::string> lines;
	for (c = buffer; *c; c++) {
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

	if( sgct::Engine::instance()->isOGLPipelineFixed() )
	{
		glPushAttrib(GL_LIST_BIT | GL_CURRENT_BIT  | GL_ENABLE_BIT | GL_TRANSFORM_BIT);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_LIGHTING);
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);

		float textScale = 1.0f / ft_font->getHeight();
		glm::mat4 scaleMat = glm::scale( mvp, glm::vec3(textScale) );

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		FontManager::instance()->getShader().bind();
		glUniform4f( FontManager::instance()->getColLoc(), color.r, color.g, color.b, color.a );
		glm::vec4 strokeColor = FontManager::instance()->getStrokeColor();
		glUniform4f( FontManager::instance()->getStkLoc(), strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

		for(size_t i=0;i<lines.size();i++)
		{
			glm::mat4 trans = glm::translate( scaleMat, glm::vec3(0.0f, -h*static_cast<float>(i), 0.0f));
			for(size_t j=0; j < lines[i].length(); j++)
			{
				char c = lines[i].c_str()[j];

				glLoadMatrixf( glm::value_ptr( trans ) );
				trans = glm::translate( trans, glm::vec3( ft_font->getCharWidth(c), 0.0f, 0.0f ));

				glBindTexture(GL_TEXTURE_2D, ft_font->getTexture(c));
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				glUniform1i( FontManager::instance()->getTexLoc(), 0);
				glCallList( font + c );
			}
		}

		sgct::ShaderProgram::unbind();
		glPopMatrix();
		glPopAttrib();
	}
	else
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		FontManager::instance()->getShader().bind();

		glBindVertexArray( ft_font->getVAO() );
		glActiveTexture(GL_TEXTURE0);

		glUniform4f( FontManager::instance()->getColLoc(), color.r, color.g, color.b, color.a );
		glm::vec4 strokeColor = FontManager::instance()->getStrokeColor();
		glUniform4f( FontManager::instance()->getStkLoc(), strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a );

		float textScale = 1.0f / ft_font->getHeight();
		glm::mat4 scaleMat = glm::scale( mvp, glm::vec3(textScale) );

		for(size_t i=0;i<lines.size();i++)
		{
			glm::mat4 trans = glm::translate( scaleMat, glm::vec3(0.0f, -h*static_cast<float>(i), 0.0f));

			for(size_t j=0; j < lines[i].length(); j++)
			{
				char c = lines[i].c_str()[j];

				glBindTexture(GL_TEXTURE_2D, ft_font->getTexture(c) );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				glUniform1i( FontManager::instance()->getTexLoc(), 0);

				glUniformMatrix4fv( FontManager::instance()->getMVPLoc(), 1, GL_FALSE, &trans[0][0]);
				trans = glm::translate( trans, glm::vec3( ft_font->getCharWidth(lines[i].c_str()[j]), 0.0f, 0.0f ));

				glDrawArrays(GL_TRIANGLE_STRIP, static_cast<GLint>(c)*4, 4);
			}//end for chars
		}//end for lines

		//unbind
		glBindVertexArray(0);
		sgct::ShaderProgram::unbind();
	}

    delete[] buffer;
}

}

