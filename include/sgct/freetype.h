/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__FREETYPE__H__
#define __SGCT__FREETYPE__H__

#include <sgct/sgctexports.h>
#include <sgct/fmt.h>
#include <sgct/math.h>

namespace sgct {
    class BaseViewport;
    class Window;
} // namespace sgct

namespace sgct::text {

class Font;

enum class Alignment { TopLeft, TopCenter, TopRight};

SGCT_EXPORT void print(const Window& window, const BaseViewport& viewport, Font& font,
    Alignment mode, float x, float y, const vec4& color, std::string text);

} // namespace sgct::text

#endif // __SGCT__FREETYPE__H__
