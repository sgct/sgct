/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef FREE__TYPE_H
#define FREE__TYPE_H

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

void print(const sgct_text::Font * ft_font, float x, float y, const char *format, ...);
void print(const sgct_text::Font * ft_font, float x, float y, glm::vec4 color, const char *format, ...);
void print3d(const sgct_text::Font * ft_font, glm::mat4 mvp, const char *format, ...);
void print3d(const sgct_text::Font * ft_font, glm::mat4 mvp, glm::vec4 color, const char *format, ...);

}

#endif
