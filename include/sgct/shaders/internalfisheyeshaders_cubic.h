/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__INTERNALFISHEYESHADERS_CUBIC__H__
#define __SGCT__INTERNALFISHEYESHADERS_CUBIC__H__

namespace sgct::shaders_fisheye_cubic {

constexpr const char* interpolate = R"(
  vec4 cubic(float x) {
    float x2 = x * x;
    float x3 = x2 * x;
    vec4 w = vec4(-x + 2*x2 - x3, 2 - 5*x2 + 3*x3, x + 4*x2 - 3*x3, -x2 + x3);
    return w / 2.0;
  }

  vec4 filter(vec2 tc, samplerCube map, vec4 bg) {
    const vec2 size = vec2(**size**, **size**);
    vec2 transTex = tc * size;
    vec2 frac = fract(transTex);
    transTex -= frac;

    vec4 xcubic = cubic(frac.x);
    vec4 ycubic = cubic(frac.y);

    const float h = 1.0;
    vec4 c = transTex.xxyy + vec4(-h, +h, -h, +h);
    vec4 s = vec4(
      xcubic.x + xcubic.y,
      xcubic.z + xcubic.w,
      ycubic.x + ycubic.y,
      ycubic.z + ycubic.w
    );
    vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;

    vec4 sample0 = getCubeSample(vec2(offset.x, offset.z) / size, map, bg);
    vec4 sample1 = getCubeSample(vec2(offset.y, offset.z) / size, map, bg);
    vec4 sample2 = getCubeSample(vec2(offset.x, offset.w) / size, map, bg);
    vec4 sample3 = getCubeSample(vec2(offset.y, offset.w) / size, map, bg);

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    return mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
  }
)";

constexpr const char* FisheyeFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  out vec4 out_diffuse;

  uniform samplerCube cubemap;
  uniform float halfFov;
  uniform vec4 bgColor;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  **interpolate**

  void main() {
    out_diffuse = filter(tr_uv, cubemap, bgColor);
  }
)";

constexpr const char* FisheyeFragNormal = R"(
  #version 330 core

  in vec2 tr_uv;
  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;

  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform float halfFov;
  uniform vec4 bgColor;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  **interpolate**

  void main() {
    out_diffuse = filter(tr_uv, cubemap, bgColor);
    out_normal = filter(tr_uv, normalmap, vec4(0.0, 0.0, 0.0, 0.0)).xyz;
  }
)";

constexpr const char* FisheyeFragPosition = R"(
  #version 330 core

  in vec2 tr_uv;
  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  uniform vec4 bgColor;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  **interpolate**

  void main() {
    out_diffuse = filter(tr_uv, cubemap, bgColor);
    out_position = filter(tr_uv, positionmap, vec4(0.0, 0.0, 0.0, 0.0)).xyz;
  }
)";

constexpr const char* FisheyeFragNormalPosition = R"(
  #version 330 core

  in vec2 tr_uv;
  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;
  layout(location = 2) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  uniform vec4 bgColor;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  **interpolate**

  void main() {
    out_diffuse = filter(tr_uv, cubemap, bgColor);
    out_normal = filter(tr_uv, normalmap, vec4(0.0, 0.0, 0.0, 0.0)).xyz;
    out_position = filter(tr_uv, positionmap, vec4(0.0, 0.0, 0.0, 0.0)).xyz;
  }
)";

constexpr const char* FisheyeFragDepth = R"(
  #version 330 core

  in vec2 tr_uv;
  out vec4 out_diffuse;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform float halfFov;
  uniform vec4 bgColor;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  **interpolate**

  void main() {
    out_diffuse = filter(tr_uv, cubemap, bgColor);
    gl_FragDepth = filter(tr_uv, depthmap, vec4(1.0, 1.0, 1.0, 1.0)).x;
  }
)";

constexpr const char* FisheyeFragDepthNormal = R"(
  #version 330 core

  in vec2 tr_uv;
  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube normalmap;
  uniform float halfFov;
  uniform vec4 bgColor;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  **interpolate**

  void main() {
    out_diffuse = filter(tr_uv, cubemap, bgColor);
    out_normal = filter(tr_uv, normalmap, vec4(0.0, 0.0, 0.0, 0.0)).xyz;
    gl_FragDepth = filter(tr_uv, depthmap, vec4(1.0, 1.0, 1.0, 1.0)).x;
  }
)";

constexpr const char* FisheyeFragDepthPosition = R"(
  #version 330 core

  in vec2 tr_uv;
  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  uniform vec4 bgColor;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  **interpolate**

  void main() {
    out_diffuse = filter(tr_uv, cubemap, bgColor);
    out_position = filter(tr_uv, positionmap, vec4(0.0, 0.0, 0.0, 0.0)).xyz;
    gl_FragDepth = filter(tr_uv, depthmap, vec4(1.0, 1.0, 1.0, 1.0)).x;
  }
)";

constexpr const char* FisheyeFragDepthNormalPosition = R"(
  #version 330 core

  in vec2 tr_uv;
  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;
  layout(location = 2) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube normalmap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  uniform vec4 bgColor;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  **interpolate**

  void main() {
    out_diffuse = filter(tr_uv, cubemap, bgColor);
    out_normal = filter(tr_uv, normalmap, vec4(0.0)).xyz;
    out_position = filter(tr_uv, positionmap, vec4(0.0)).xyz;
    gl_FragDepth = filter(tr_uv, depthmap, vec4(1.0)).x;
  }
)";

constexpr const char* FisheyeFragOffAxis = R"(
  #version 330 core

  in vec2 tr_uv;
  out vec4 out_diffuse;

  uniform samplerCube cubemap;
  uniform float halfFov;
  uniform vec3 offset;
  uniform vec4 bgColor;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  **interpolate**

  void main() {
    out_diffuse = filter(tr_uv, cubemap, bgColor);
  }
)";

constexpr const char* FisheyeFragOffAxisNormal = R"(
  #version 330 core

  in vec2 tr_uv;
  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;

  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform float halfFov;
  uniform vec3 offset;
  uniform vec4 bgColor;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  **interpolate**

  void main() {
    out_diffuse = filter(tr_uv, cubemap, bgColor);
    out_normal = filter(tr_uv, normalmap, vec4(0.0, 0.0, 0.0, 0.0)).xyz;
  }
)";

constexpr const char* FisheyeFragOffAxisPosition = R"(
  #version 330 core

  in vec2 tr_uv;
  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  uniform vec3 offset;
  uniform vec4 bgColor;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  **interpolate**

  void main() {
    out_diffuse = filter(tr_uv, cubemap, bgColor);
    out_position = filter(tr_uv, positionmap, vec4(0.0, 0.0, 0.0, 0.0)).xyz;
  }
)";

constexpr const char* FisheyeFragOffAxisNormalPosition = R"(
  #version 330 core

  in vec2 tr_uv;
  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;
  layout(location = 2) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  uniform vec3 offset;
  uniform vec4 bgColor;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  **interpolate**

  void main() {
    out_diffuse = filter(tr_uv, cubemap, bgColor);
    out_normal = filter(tr_uv, normalmap, vec4(0.0)).xyz;
    out_position = filter(tr_uv, positionmap, vec4(0.0)).xyz;
  }
)";

constexpr const char* FisheyeFragOffAxisDepth = R"(
  #version 330 core

  in vec2 tr_uv;
  out vec4 out_diffuse;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform float halfFov;
  uniform vec3 offset;
  uniform vec4 bgColor;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  **interpolate**

  void main() {
    out_diffuse = filter(tr_uv, cubemap, bgColor);
    gl_FragDepth = filter(tr_uv, depthmap, vec4(1.0)).x;
  }
)";

constexpr const char* FisheyeFragOffAxisDepthNormal = R"(
  #version 330 core

  in vec2 tr_uv;
  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube normalmap;
  uniform float halfFov;
  uniform vec3 offset;
  uniform vec4 bgColor;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  **interpolate**

  void main() {
    out_diffuse = filter(tr_uv, cubemap, bgColor);
    out_normal = filter(tr_uv, normalmap, vec4(0.0)).xyz;
    gl_FragDepth = filter(tr_uv, depthmap, vec4(1.0)).x;
  }
)";

constexpr const char* FisheyeFragOffAxisDepthPosition = R"(
  #version 330 core

  in vec2 tr_uv;
  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  uniform vec3 offset;
  uniform vec4 bgColor;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  **interpolate**

  void main() {
    out_diffuse = filter(tr_uv, cubemap, bgColor);
    out_position = filter(tr_uv, positionmap, vec4(0.0)).xyz;
    gl_FragDepth = filter(tr_uv, depthmap, vec4(1.0)).x;
  }
)";

constexpr const char* FisheyeFragOffAxisDepthNormalPosition = R"(
  #version 330 core

  in vec2 tr_uv;
  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;
  layout(location = 2) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube normalmap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  uniform vec3 offset;
  uniform vec4 bgColor;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  **interpolate**

  void main() {
    out_diffuse = filter(tr_uv, cubemap, bgColor);
    out_normal = filter(tr_uv, normalmap, vec4(0.0)).xyz;
    out_position = filter(tr_uv, positionmap, vec4(0.0)).xyz;
    gl_FragDepth = filter(tr_uv, depthmap, vec4(1.0)).x;
  }
)";

} // namespace sgct::shaders_fisheye_cubic

#endif // __SGCT__INTERNALFISHEYESHADERS_CUBIC__H__
