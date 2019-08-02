/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__FREE_TYPE__H__
#define __SGCT__FREE_TYPE__H__

#include <glm/glm.hpp>
#include <string>
#include <wchar.h>

namespace sgct_text {

class Font;

enum TextAlignMode { TOP_LEFT, TOP_CENTER, TOP_RIGHT};

void print(Font* ft_font, TextAlignMode mode, float x, float y, const char* format, ...);
void print(Font* ft_font, TextAlignMode mode, float x, float y,
    const wchar_t* format, ...);
void print3d(Font* ft_font, TextAlignMode mode, glm::mat4 mvp, const char* format, ...);
void print3d(Font* ft_font, TextAlignMode mode, glm::mat4 mvp,
    const wchar_t* format, ...);

// with color
void print(Font* ft_font, TextAlignMode mode, float x, float y, const glm::vec4& color,
    const char* format, ...);
void print(Font* ft_font, TextAlignMode mode, float x, float y, const glm::vec4& color,
    const wchar_t* format, ...);
void print3d(Font* ft_font, TextAlignMode mode, glm::mat4 mvp, const glm::vec4& color,
    const char* format, ...);
void print3d(Font* ft_font, TextAlignMode mode, glm::mat4 mvp, const glm::vec4& color,
    const wchar_t* format, ...);

} // namespace sgct_text

#endif // __SGCT__FREE_TYPE__H__
