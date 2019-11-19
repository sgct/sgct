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

  layout (location = 0) in vec2 in_position;
  layout (location = 1) in vec2 in_texCoords;
  layout (location = 2) in vec4 in_vertColor;
  out vec2 tr_uv;
  out vec4 tr_color;

  uniform mat4 MVP;

  void main() {
    gl_Position = MVP * vec4(in_position, 0.0, 1.0);
    tr_uv = in_texCoords;
    tr_color = in_vertColor;
  }
)";

constexpr const char* SphericalProjectionFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D tex;

  void main() {
    out_color = tr_color * texture(tex, tr_uv);
  }
)";

} // namespace sgct::core::shaders

#endif // __SGCT__INTERNAL_SPHERICAL_PROJECTION_SHADERS__H__
