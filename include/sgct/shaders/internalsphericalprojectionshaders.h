/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__INTERNAL_SPHERICAL_PROJECTION_SHADERS__H__
#define __SGCT__INTERNAL_SPHERICAL_PROJECTION_SHADERS__H__

namespace sgct::core::shaders {

constexpr const char* SphericalProjectionVert = R"(
  #version 330 core

  layout (location = 0) in vec2 Position;
  layout (location = 1) in vec2 TexCoords;
  layout (location = 2) in vec4 VertColor;

  uniform mat4 MVP;

  out vec2 UV;
  out vec4 Col;

  void main() {
    gl_Position = MVP * vec4(Position, 0.0, 1.0);
    UV = TexCoords;
    Col = VertColor;
  }
)";

constexpr const char* SphericalProjectionFrag = R"(
  #version 330 core

  in vec2 UV;
  in vec4 Col;
  out vec4 Color;

  uniform sampler2D Tex;

  void main() {
    Color = Col * texture(Tex, UV);
  }
)";

} // sgct::core::shaders

#endif // __SGCT__INTERNAL_SPHERICAL_PROJECTION_SHADERS__H__
