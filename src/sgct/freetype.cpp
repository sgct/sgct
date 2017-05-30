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
#include <sgct/helpers/SGCTStringFunctions.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace sgct_text
{

inline void setupViewport()
{
    sgct::SGCTWindow * cWin = sgct::Engine::instance()->getCurrentWindowPtr();

    int x, y, xSize, ySize;
    x        = static_cast<int>(cWin->getCurrentViewport()->getX() * static_cast<float>(cWin->getXFramebufferResolution()));
    y        = static_cast<int>(cWin->getCurrentViewport()->getY() * static_cast<float>(cWin->getYFramebufferResolution()));
    xSize    = static_cast<int>(cWin->getCurrentViewport()->getXSize() * static_cast<float>(cWin->getXFramebufferResolution()));
    ySize    = static_cast<int>(cWin->getCurrentViewport()->getYSize() * static_cast<float>(cWin->getYFramebufferResolution()));

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

char * parseArgList(va_list args, const char *format)
{
	int size = 1 + vscprintf(format, args);
	char * buffer = new (std::nothrow) char[size];
	if (buffer == NULL)
		return NULL;

	memset(buffer, 0, size);

#if (_MSC_VER >= 1400) //visual studio 2005 or later
	vsprintf_s(buffer, size, format, args);
#else
	vsprintf(buffer, format, args);
#endif

	return buffer;
}

wchar_t * parseArgList(va_list args, const wchar_t *format)
{
	int size = 1 + vscwprintf(format, args);
	wchar_t * buffer = new (std::nothrow) wchar_t[size];
	if (buffer == NULL)
		return NULL;

	memset(buffer, 0, size * sizeof(wchar_t));

#if (_MSC_VER >= 1400) //visual studio 2005 or later
	vswprintf_s(buffer, size, format, args);
#else
	vswprintf(buffer, size, format, args);
#endif

	return buffer;
}

float getLineWidth(sgct_text::Font * ft_font, const std::wstring & line)
{
	FontFaceData * ffdPtr;
	
	//figure out width
	float lineWidth = 0.0f;
	for (size_t j = 0; j < line.length() - 1; ++j)
	{
		auto c = line.c_str()[j];
		ffdPtr = ft_font->getFontFaceData(c);
		lineWidth += ffdPtr->mDistToNextChar;
	}
	//add last char width
	auto c = line.c_str()[line.length() - 1];
	ffdPtr = ft_font->getFontFaceData(c);
	lineWidth += ffdPtr->mSize.x;

	return lineWidth;
}

void render2d(const std::vector<std::wstring> & lines, sgct_text::Font * ft_font, const TextAlignMode & mode, const float & x, const float & y, const glm::vec4 & color)
{
	FontFaceData * ffdPtr;
	float h = ft_font->getHeight() * 1.59f;
	
	if (sgct::Engine::instance()->isOGLPipelineFixed())
	{
		glPushAttrib(GL_LIST_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TRANSFORM_BIT);
		pushScreenCoordinateMatrix();
		glMatrixMode(GL_MODELVIEW);
		glDisable(GL_LIGHTING);
		glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		FontManager::instance()->getShader().bind();
		glUniform4f(FontManager::instance()->getColLoc(), color.r, color.g, color.b, color.a);
		glm::vec4 strokeColor = FontManager::instance()->getStrokeColor();
		glUniform4f(FontManager::instance()->getStkLoc(), strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a);

		for (size_t i = 0; i<lines.size(); ++i)
		{
			glm::vec3 offset(x, y - h*static_cast<float>(i), 0.0f);
			
			if (mode == TOP_CENTER)
				offset.x -= getLineWidth(ft_font, lines[i]) / 2.0f;
			else if(mode == TOP_RIGHT)
				offset.x -= getLineWidth(ft_font, lines[i]);

			for (size_t j = 0; j < lines[i].length(); ++j)
			{
				auto c = lines[i].c_str()[j];
				ffdPtr = ft_font->getFontFaceData(c);

				glPushMatrix();
				glLoadIdentity();
				glTranslatef(offset.x + ffdPtr->mPos.x, offset.y + ffdPtr->mPos.y, offset.z);
				glScalef(ffdPtr->mSize.x, ffdPtr->mSize.y, 1.0f);

				glBindTexture(GL_TEXTURE_2D, ffdPtr->mTexId);
				//disable interpolation for 2D rendering
				if (ffdPtr->mInterpolated)
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					ffdPtr->mInterpolated = false;
				}
				glUniform1i(FontManager::instance()->getTexLoc(), 0);

				glCallList(ft_font->getDisplayList());
				glPopMatrix();

				//iterate
				offset += glm::vec3(ffdPtr->mDistToNextChar, 0.0f, 0.0f);
			}
		}

		sgct::ShaderProgram::unbind();

		pop_projection_matrix();
		glPopAttrib();
	}
	else
	{
		setupViewport();
		glm::mat4 projectionMat(setupOrthoMat());

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		FontManager::instance()->getShader().bind();

		glBindVertexArray(ft_font->getVAO());

		glActiveTexture(GL_TEXTURE0);

		glUniform4f(FontManager::instance()->getColLoc(), color.r, color.g, color.b, color.a);
		glm::vec4 strokeColor = FontManager::instance()->getStrokeColor();
		glUniform4f(FontManager::instance()->getStkLoc(), strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a);

		for (size_t i = 0; i<lines.size(); i++)
		{
			glm::vec3 offset(x, y - h*static_cast<float>(i), 0.0f);
			glm::mat4 trans;
			glm::mat4 scale;

			if (mode == TOP_CENTER)
				offset.x -= getLineWidth(ft_font, lines[i]) / 2.0f;
			else if (mode == TOP_RIGHT)
				offset.x -= getLineWidth(ft_font, lines[i]);

			for (size_t j = 0; j < lines[i].length(); j++)
			{
				auto c = lines[i].c_str()[j];
				ffdPtr = ft_font->getFontFaceData(c);

				trans = glm::translate(projectionMat, glm::vec3(offset.x + ffdPtr->mPos.x, offset.y + ffdPtr->mPos.y, offset.z));
				scale = glm::scale(trans, glm::vec3(ffdPtr->mSize.x, ffdPtr->mSize.y, 1.0f));

				glBindTexture(GL_TEXTURE_2D, ffdPtr->mTexId);
				//disable interpolation for 2D rendering
				if (ffdPtr->mInterpolated)
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					ffdPtr->mInterpolated = false;
				}
				glUniform1i(FontManager::instance()->getTexLoc(), 0);

				glUniformMatrix4fv(FontManager::instance()->getMVPLoc(), 1, GL_FALSE, &scale[0][0]);

				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

				//iterate
				offset += glm::vec3(ffdPtr->mDistToNextChar, 0.0f, 0.0f);
			}//end for chars
		}//end for lines

		 //unbind
		glBindVertexArray(0);
		sgct::ShaderProgram::unbind();
	}
}

void render3d(const std::vector<std::wstring> & lines, sgct_text::Font * ft_font, const TextAlignMode & mode, const glm::mat4 & mvp, const glm::vec4 & color)
{
	FontFaceData * ffdPtr;
	float h = ft_font->getHeight() * 1.59f;
	
	if (sgct::Engine::instance()->isOGLPipelineFixed())
	{
		glPushAttrib(GL_LIST_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TRANSFORM_BIT);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_LIGHTING);
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);

		float textScale = 1.0f / ft_font->getHeight();
		glm::mat4 textScaleMat = glm::scale(mvp, glm::vec3(textScale));

		//clear sgct projection
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		FontManager::instance()->getShader().bind();
		glUniform4f(FontManager::instance()->getColLoc(), color.r, color.g, color.b, color.a);
		glm::vec4 strokeColor = FontManager::instance()->getStrokeColor();
		glUniform4f(FontManager::instance()->getStkLoc(), strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a);

		for (size_t i = 0; i<lines.size(); i++)
		{
			glm::vec3 offset(0.0f, -h*static_cast<float>(i), 0.0f);
			glm::mat4 trans;
			glm::mat4 scale;

			if (mode == TOP_CENTER)
				offset.x -= getLineWidth(ft_font, lines[i]) / 2.0f;
			else if (mode == TOP_RIGHT)
				offset.x -= getLineWidth(ft_font, lines[i]);
			
			for (size_t j = 0; j < lines[i].length(); j++)
			{
				auto c = lines[i].c_str()[j];
				ffdPtr = ft_font->getFontFaceData(c);

				trans = glm::translate(textScaleMat, glm::vec3(offset.x + ffdPtr->mPos.x, offset.y + ffdPtr->mPos.y, offset.z));
				scale = glm::scale(trans, glm::vec3(ffdPtr->mSize.x, ffdPtr->mSize.y, 1.0f));

				glLoadMatrixf( glm::value_ptr(scale) );

				glBindTexture(GL_TEXTURE_2D, ffdPtr->mTexId);
				//use linear interpolation in 3D
				if (!ffdPtr->mInterpolated)
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					ffdPtr->mInterpolated = true;
				}

				glUniform1i( FontManager::instance()->getTexLoc(), 0);
				glCallList(ft_font->getDisplayList());

				//iterate
				offset += glm::vec3(ffdPtr->mDistToNextChar, 0.0f, 0.0f);
			}
		}

		sgct::ShaderProgram::unbind();
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glPopAttrib();
	}
	else
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		FontManager::instance()->getShader().bind();

		glBindVertexArray(ft_font->getVAO());
		glActiveTexture(GL_TEXTURE0);

		glUniform4f(FontManager::instance()->getColLoc(), color.r, color.g, color.b, color.a);
		glm::vec4 strokeColor = FontManager::instance()->getStrokeColor();
		glUniform4f(FontManager::instance()->getStkLoc(), strokeColor.r, strokeColor.g, strokeColor.b, strokeColor.a);

		float textScale = 1.0f / ft_font->getHeight();
		glm::mat4 textScaleMat = glm::scale(mvp, glm::vec3(textScale));

		for (size_t i = 0; i<lines.size(); i++)
		{
			glm::vec3 offset(0.0f, -h*static_cast<float>(i), 0.0f);
			glm::mat4 trans;
			glm::mat4 scale;

			if (mode == TOP_CENTER)
				offset.x -= getLineWidth(ft_font, lines[i]) / 2.0f;
			else if (mode == TOP_RIGHT)
				offset.x -= getLineWidth(ft_font, lines[i]);

			for (size_t j = 0; j < lines[i].length(); j++)
			{
				auto c = lines[i].c_str()[j];
				ffdPtr = ft_font->getFontFaceData(c);

				trans = glm::translate(textScaleMat, glm::vec3(offset.x + ffdPtr->mPos.x, offset.y + ffdPtr->mPos.y, offset.z));
				scale = glm::scale(trans, glm::vec3(ffdPtr->mSize.x, ffdPtr->mSize.y, 1.0f));

				glBindTexture(GL_TEXTURE_2D, ffdPtr->mTexId);
				//use linear interpolation in 3D
				if (!ffdPtr->mInterpolated)
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					ffdPtr->mInterpolated = true;
				}

				glUniform1i( FontManager::instance()->getTexLoc(), 0);
				glUniformMatrix4fv( FontManager::instance()->getMVPLoc(), 1, GL_FALSE, &scale[0][0]);
				
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

				//iterate
				offset += glm::vec3(ffdPtr->mDistToNextChar, 0.0f, 0.0f);
			}//end for chars
		}//end for lines

		 //unbind
		glBindVertexArray(0);
		sgct::ShaderProgram::unbind();
	}
}

void print(sgct_text::Font * ft_font, TextAlignMode mode, float x, float y, const char *format, ...)
{
	if (ft_font == NULL || format == NULL)
		return;

	va_list		args;	 // Pointer To List Of Arguments
	va_start(args, format); // Parses The String For Variables
	char * buffer = parseArgList(args, format);
	va_end(args); // Results Are Stored In Text

	std::vector<std::wstring> lines = sgct_helpers::split(buffer, L'\n');

	render2d(lines, ft_font, mode, x, y, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	if (buffer)
		delete[] buffer;
}

void print(sgct_text::Font * ft_font, TextAlignMode mode, float x, float y, const wchar_t *format, ...)
{
	if (ft_font == NULL || format == NULL)
		return;

	va_list		args;	 // Pointer To List Of Arguments
	va_start(args, format); // Parses The String For Variables
	wchar_t * buffer = parseArgList(args, format);
	va_end(args); // Results Are Stored In Text

	std::vector<std::wstring> lines = sgct_helpers::split(buffer, L'\n');

	render2d(lines, ft_font, mode, x, y, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

	if (buffer)
		delete[] buffer;
}

void print(sgct_text::Font * ft_font, TextAlignMode mode, float x, float y, const glm::vec4 & color, const char *format, ...)
{
	if (ft_font == NULL || format == NULL)
		return;

	va_list		args;	 // Pointer To List Of Arguments
	va_start(args, format); // Parses The String For Variables
	char * buffer = parseArgList(args, format);
	va_end(args); // Results Are Stored In Text

	std::vector<std::wstring> lines = sgct_helpers::split(buffer, L'\n');

	render2d(lines, ft_font, mode, x, y, color);

	if (buffer)
		delete[] buffer;
}

void print(sgct_text::Font * ft_font, TextAlignMode mode, float x, float y, const glm::vec4 & color, const wchar_t *format, ...)
{
	if (ft_font == NULL || format == NULL)
		return;

	va_list		args;	 // Pointer To List Of Arguments
	va_start(args, format); // Parses The String For Variables
	wchar_t * buffer = parseArgList(args, format);
	va_end(args); // Results Are Stored In Text

	std::vector<std::wstring> lines = sgct_helpers::split(buffer, L'\n');

	render2d(lines, ft_font, mode, x, y, color);

	if (buffer)
		delete[] buffer;
}

void print3d(sgct_text::Font * ft_font, TextAlignMode mode, glm::mat4 mvp, const char *format, ...)
{
    if (ft_font == NULL || format == NULL)
        return;

    va_list		args;	 // Pointer To List Of Arguments
	va_start(args, format); // Parses The String For Variables
	char * buffer = parseArgList(args, format);
	va_end(args); // Results Are Stored In Text

	std::vector<std::wstring> lines = sgct_helpers::split(buffer, L'\n');

	glm::vec4 color( 1.0f, 1.0f, 1.0f, 1.0f );
	render3d(lines, ft_font, mode, mvp, color);

	if (buffer)
		delete[] buffer;
}

void print3d(sgct_text::Font * ft_font, TextAlignMode mode, glm::mat4 mvp, const wchar_t *format, ...)
{
	if (ft_font == NULL || format == NULL)
		return;

	va_list		args;	 // Pointer To List Of Arguments
	va_start(args, format); // Parses The String For Variables
	wchar_t * buffer = parseArgList(args, format);
	va_end(args); // Results Are Stored In Text

	std::vector<std::wstring> lines = sgct_helpers::split(buffer, L'\n');

	glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
	render3d(lines, ft_font, mode, mvp, color);

	if (buffer)
		delete[] buffer;
}

void print3d(sgct_text::Font * ft_font, TextAlignMode mode, glm::mat4 mvp, const glm::vec4 & color, const char *format, ...)
{
    if (ft_font == NULL || format == NULL)
        return;

	va_list		args;	 // Pointer To List Of Arguments
	va_start(args, format); // Parses The String For Variables
	char * buffer = parseArgList(args, format);
	va_end(args); // Results Are Stored In Text

	std::vector<std::wstring> lines = sgct_helpers::split(buffer, L'\n');

	render3d(lines, ft_font, mode, mvp, color);

	if (buffer)
		delete[] buffer;
}

void print3d(sgct_text::Font * ft_font, TextAlignMode mode, glm::mat4 mvp, const glm::vec4 & color, const wchar_t *format, ...)
{
	if (ft_font == NULL || format == NULL)
		return;

	va_list		args;	 // Pointer To List Of Arguments
	va_start(args, format); // Parses The String For Variables
	wchar_t * buffer = parseArgList(args, format);
	va_end(args); // Results Are Stored In Text

	std::vector<std::wstring> lines = sgct_helpers::split(buffer, L'\n');

	render3d(lines, ft_font, mode, mvp, color);

	if (buffer)
		delete[] buffer;
}

}

