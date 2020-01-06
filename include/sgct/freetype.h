/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__FREETYPE__H__
#define __SGCT__FREETYPE__H__

#include <glm/fwd.hpp>

namespace sgct {
    class BaseViewport;
    class Window;
} // namespace sgct

namespace sgct::text {

class Font;

enum class Alignment { TopLeft, TopCenter, TopRight};

void print(const Window& window, const BaseViewport& viewport, Font& font,
    Alignment mode, float x, float y, const glm::vec4& color,
    const char* format, ...);

} // namespace sgct::text

#endif // __SGCT__FREETYPE__H__
