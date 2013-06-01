/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef FREE__TYPE_H
#define FREE__TYPE_H

//FreeType Headers
#include "external/ft2build.h"
#include FT_FREETYPE_H
#include "external/freetype/ftglyph.h"
#include "external/freetype/ftoutln.h"
#include "external/freetype/fttrigon.h"

#include "Font.h"

//Some STL headers
#include <string>
#include <vector>
#include <glm/glm.hpp>

//Using the STL exception library increases the
//chances that someone else using our code will corretly
//catch any exceptions that we throw.
#include <stdexcept>

namespace sgct_text
{

void print(const sgct_text::Font * ft_font, float x, float y, const char *fmt, ...);
void print(const sgct_text::Font * ft_font, float x, float y, glm::vec4 color, const char *fmt, ...);
void print3d(const sgct_text::Font * ft_font, glm::mat4 mvp, const char *fmt, ...);
void print3d(const sgct_text::Font * ft_font, glm::mat4 mvp, glm::vec4 color, const char *fmt, ...);

}

#endif
