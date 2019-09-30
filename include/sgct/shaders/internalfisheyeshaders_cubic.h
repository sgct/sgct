/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__INTERNAL_FISHEYE_SHADERS_CUBIC__H__
#define __SGCT__INTERNAL_FISHEYE_SHADERS_CUBIC__H__

namespace sgct::core::shaders_fisheye_cubic {

constexpr const char* BSplineFun = R"(
  vec4 cubic(float x) {
    float x2 = x * x;
    float x3 = x2 * x;
    vec4 w = vec4(-x3 + 3*x2 - 3*x + 1, 3*x3 - 6*x2 + 4, -3*x3 + 3*x2 + 3*x + 1, x3);
    return w / 6.0;
  }
)";

constexpr const char* catmullRomFun = R"(
  vec4 cubic(float x) {
    float x2 = x * x;
    float x3 = x2 * x;
    vec4 w = vec4(-x + 2*x2 - x3, 2 - 5*x2 + 3*x3, x + 4*x2 - 3*x3, -x2 + x3);
    return w / 2.0;
  }
)";

constexpr const char* weightedMultisample_f = R"(
  float filterf(vec2 tc, samplerCube map, vec4 bg) {
    const vec2 size = vec2(**size**, **size**);
    float sample0 = getCubeSample(tc, map, bg).x;
    float sample1 = getCubeSample(tc + vec2(0.5, 0.5) / size, map, bg).x;
    float sample2 = getCubeSample(tc + vec2(-0.5, 0.5) / size, map, bg).x;
    float sample3 = getCubeSample(tc + vec2(-0.5, -0.5) / size, map, bg).x;
    float sample4 = getCubeSample(tc + vec2(0.5, -0.5) / size, map, bg).x;
    return (4.0 * sample0 + sample1 + sample2 + sample3 + sample4) / 8.0;
  }
)";

constexpr const char* weightedMultisample_3f = R"(
  vec3 filter3f(vec2 tc, samplerCube map, vec4 bg) {
    const vec2 size = vec2(**size**, **size**);
    vec3 sample0 = getCubeSample(tc, map, bg).xyz;
    vec3 sample1 = getCubeSample(tc + vec2(0.5, 0.5) / size, map, bg).xyz;
    vec3 sample2 = getCubeSample(tc + vec2(-0.5, 0.5) / size, map, bg).xyz;
    vec3 sample3 = getCubeSample(tc + vec2(-0.5, -0.5) / size, map, bg).xyz;
    vec3 sample4 = getCubeSample(tc + vec2(0.5, -0.5) / size, map, bg).xyz;
    return (4.0 * sample0 + sample1 + sample2 + sample3 + sample4) / 8.0;
  }
)";

constexpr const char* weightedMultisample_4f = R"(
  vec4 filter4f(vec2 tc, samplerCube map, vec4 bg) {
    const vec2 size = vec2(**size**, **size**);
    vec4 sample0 = getCubeSample(tc, map, bg);
    vec4 sample1 = getCubeSample(tc + vec2(0.5, 0.5) / size, map, bg);
    vec4 sample2 = getCubeSample(tc + vec2(-0.5, 0.5) / size, map, bg);
    vec4 sample3 = getCubeSample(tc + vec2(-0.5, -0.5) / size, map, bg);
    vec4 sample4 = getCubeSample(tc + vec2(0.5, -0.5) / size, map, bg);
    return (4.0 * sample0 + sample1 + sample2 + sample3 + sample4) / 8.0;
  }
)";

constexpr const char* interpolate4_f = R"(
  float filterf(vec2 tc, samplerCube map, vec4 bg)  {
    vec2 transTex = tc * vec2(**size**, **size**);
    float fx = fract(transTex.x);
    float fy = fract(transTex.y);
    transTex.x -= fx;
    transTex.y -= fy;

    vec4 xcubic = cubic(fx);
    vec4 ycubic = cubic(fy);

    vec4 c = vec4(
      transTex.x - **step**,
      transTex.x + **step**,
      transTex.y - **step**,
      transTex.y + **step**
    );
    vec4 s = vec4(
      xcubic.x + xcubic.y,
      xcubic.z + xcubic.w,
      ycubic.x + ycubic.y,
      ycubic.z + ycubic.w
    );
    vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;

    const vec2 size = vec2(**size**, **size**);
    float sample0 = getCubeSample(vec2(offset.x, offset.z) / size, map, bg).x;
    float sample1 = getCubeSample(vec2(offset.y, offset.z) / size, map, bg).x;
    float sample2 = getCubeSample(vec2(offset.x, offset.w) / size, map, bg).x;
    float sample3 = getCubeSample(vec2(offset.y, offset.w) / size, map, bg).x;

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    return mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
  }
)";

constexpr const char* interpolate4_3f = R"(
  vec3 filter3f(vec2 tc, samplerCube map, vec4 bg) {
    const vec2 size = vec2(**size**, **size**);
    vec2 transTex = tc * size;
    float fx = fract(transTex.x);
    float fy = fract(transTex.y);
    transTex.x -= fx;
    transTex.y -= fy;

    vec4 xcubic = cubic(fx);
    vec4 ycubic = cubic(fy);

    vec4 c = vec4(
      transTex.x - **step**,
      transTex.x + **step**,
      transTex.y - **step**,
      transTex.y + **step**
    );
    vec4 s = vec4(
      xcubic.x + xcubic.y,
      xcubic.z + xcubic.w,
      ycubic.x + ycubic.y,
      ycubic.z + ycubic.w
    );
    vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;

    vec3 sample0 = getCubeSample(vec2(offset.x, offset.z) / size, map, bg).xyz;
    vec3 sample1 = getCubeSample(vec2(offset.y, offset.z) / size, map, bg).xyz;
    vec3 sample2 = getCubeSample(vec2(offset.x, offset.w) / size, map, bg).xyz;
    vec3 sample3 = getCubeSample(vec2(offset.y, offset.w) / size, map, bg).xyz;

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    return mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
  }
)";

constexpr const char* interpolate4_4f = R"(
  vec4 filter4f(vec2 tc, samplerCube map, vec4 bg) {
    const vec2 size = vec2(**size**, **size**);
    vec2 transTex = tc * size;
    float fx = fract(transTex.x);
    float fy = fract(transTex.y);
    transTex.x -= fx;
    transTex.y -= fy;

    vec4 xcubic = cubic(fx);
    vec4 ycubic = cubic(fy);

    vec4 c = vec4(
      transTex.x - **step**,
      transTex.x + **step**,
      transTex.y - **step**,
      transTex.y + **step**
    );
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

constexpr const char* interpolate16_f = R"(
  float filterf(vec2 tc, samplerCube map, vec4 bg) {
    const vec2 size = vec2(**size**, **size**);
    vec2 transTex = tc * size;
    vec2 xy = floor(transTex);
    vec2 normxy = transTex - xy;

    vec2 st0 = ((2.0 - normxy) * normxy - 1.0) * normxy;
    vec2 st1 = (3.0 * normxy - 5.0) * normxy * normxy + 2.0;
    vec2 st2 = ((4.0 - 3.0 * normxy) * normxy + 1.0) * normxy;
    vec2 st3 = (normxy - 1.0) * normxy * normxy;

    float row0 =
      st0.s * getCubeSample((xy + vec2(-1.0, -1.0)) / size, map, bg).x +
      st1.s * getCubeSample((xy + vec2(0.0, -1.0)) / size, map, bg).x +
      st2.s * getCubeSample((xy + vec2(1.0, -1.0)) / size, map, bg).x +
      st3.s * getCubeSample((xy + vec2(2.0, -1.0)) / size, map, bg).x;

    float row1 =
      st0.s * getCubeSample((xy + vec2(-1.0, 0.0)) / size, map, bg).x +
      st1.s * getCubeSample((xy + vec2(0.0, 0.0)) / size, map, bg).x +
      st2.s * getCubeSample((xy + vec2(1.0, 0.0)) / size, map, bg).x +
      st3.s * getCubeSample((xy + vec2(2.0, 0.0)) / size, map, bg).x;

    float row2 =
      st0.s * getCubeSample((xy + vec2(-1.0, 1.0)) / size, map, bg).x +
      st1.s * getCubeSample((xy + vec2(0.0, 1.0)) / size, map, bg).x +
      st2.s * getCubeSample((xy + vec2(1.0, 1.0)) / size, map, bg).x +
      st3.s * getCubeSample((xy + vec2(2.0, 1.0)) / size, map, bg).x;

    float row3 =
      st0.s * getCubeSample((xy + vec2(-1.0, 2.0)) / size, map, bg).x +
      st1.s * getCubeSample((xy + vec2(0.0, 2.0)) / size, map, bg).x +
      st2.s * getCubeSample((xy + vec2(1.0, 2.0)) / size, map, bg).x +
      st3.s * getCubeSample((xy + vec2(2.0, 2.0)) / size, map, bg).x;

    return 0.25 * ((st0.t * row0) + (st1.t * row1) + (st2.t * row2) + (st3.t * row3));
  }
)";

constexpr const char* interpolate16_3f = R"(
  vec3 filter3f(vec2 tc, samplerCube map, vec4 bg) {
    const vec2 size = vec2(**size**, **size**);
    vec2 transTex = tc * size;
    vec2 xy = floor(transTex);
    vec2 normxy = transTex - xy;

    vec2 st0 = ((2.0 - normxy) * normxy - 1.0) * normxy;
    vec2 st1 = (3.0 * normxy - 5.0) * normxy * normxy + 2.0;
    vec2 st2 = ((4.0 - 3.0 * normxy) * normxy + 1.0) * normxy;
    vec2 st3 = (normxy - 1.0) * normxy * normxy;

    vec3 row0 =
      st0.s * getCubeSample((xy + vec2(-1.0, -1.0)) / size, map, bg).xyz +
      st1.s * getCubeSample((xy + vec2(0.0, -1.0)) / size, map, bg).xyz +
      st2.s * getCubeSample((xy + vec2(1.0, -1.0)) / size, map, bg).xyz +
      st3.s * getCubeSample((xy + vec2(2.0, -1.0)) / size, map, bg).xyz;

    vec3 row1 =
      st0.s * getCubeSample((xy + vec2(-1.0, 0.0)) / size, map, bg).xyz +
      st1.s * getCubeSample((xy + vec2(0.0, 0.0)) / size, map, bg).xyz +
      st2.s * getCubeSample((xy + vec2(1.0, 0.0)) / size, map, bg).xyz +
      st3.s * getCubeSample((xy + vec2(2.0, 0.0)) / size, map, bg).xyz;

    vec3 row2 =
      st0.s * getCubeSample((xy + vec2(-1.0, 1.0)) / size, map, bg).xyz +
      st1.s * getCubeSample((xy + vec2(0.0, 1.0)) / size, map, bg).xyz +
      st2.s * getCubeSample((xy + vec2(1.0, 1.0)) / size, map, bg).xyz +
      st3.s * getCubeSample((xy + vec2(2.0, 1.0)) / size, map, bg).xyz;

    vec3 row3 =
      st0.s * getCubeSample((xy + vec2(-1.0, 2.0)) / size, map, bg).xyz +
      st1.s * getCubeSample((xy + vec2(0.0, 2.0)) / size, map, bg).xyz +
      st2.s * getCubeSample((xy + vec2(1.0, 2.0)) / size, map, bg).xyz +
      st3.s * getCubeSample((xy + vec2(2.0, 2.0)) / size, map, bg).xyz;

    return 0.25 * ((st0.t * row0) + (st1.t * row1) + (st2.t * row2) + (st3.t * row3));
  }
)";

constexpr const char* interpolate16_4f = R"(
  vec4 filter4f(vec2 tc, samplerCube map, vec4 bg) {
    const vec2 size = vec2(**size**, **size**);
    vec2 transTex = tc * size;
    vec2 xy = floor(transTex);
    vec2 normxy = transTex - xy;

    vec2 st0 = ((2.0 - normxy) * normxy - 1.0) * normxy;
    vec2 st1 = (3.0 * normxy - 5.0) * normxy * normxy + 2.0;
    vec2 st2 = ((4.0 - 3.0 * normxy) * normxy + 1.0) * normxy;
    vec2 st3 = (normxy - 1.0) * normxy * normxy;

    vec4 row0 =
      st0.s * getCubeSample((xy + vec2(-1.0, -1.0)) / size, map, bg) +
      st1.s * getCubeSample((xy + vec2(0.0, -1.0)) / size, map, bg) +
      st2.s * getCubeSample((xy + vec2(1.0, -1.0)) / size, map, bg) +
      st3.s * getCubeSample((xy + vec2(2.0, -1.0)) / size, map, bg);

    vec4 row1 =
      st0.s * getCubeSample((xy + vec2(-1.0, 0.0)) / size, map, bg) +
      st1.s * getCubeSample((xy + vec2(0.0, 0.0)) / size, map, bg) +
      st2.s * getCubeSample((xy + vec2(1.0, 0.0)) / size, map, bg) +
      st3.s * getCubeSample((xy + vec2(2.0, 0.0)) / size, map, bg);

    vec4 row2 =
      st0.s * getCubeSample((xy + vec2(-1.0, 1.0)) / size, map, bg) +
      st1.s * getCubeSample((xy + vec2(0.0, 1.0)) / size, map, bg) +
      st2.s * getCubeSample((xy + vec2(1.0, 1.0)) / size, map, bg) +
      st3.s * getCubeSample((xy + vec2(2.0, 1.0)) / size, map, bg);

    vec4 row3 =
      st0.s * getCubeSample((xy + vec2(-1.0, 2.0)) / size, map, bg) +
      st1.s * getCubeSample((xy + vec2(0.0, 2.0)) / size, map, bg) +
      st2.s * getCubeSample((xy + vec2(1.0, 2.0)) / size, map, bg) +
      st3.s * getCubeSample((xy + vec2(2.0, 2.0)) / size, map, bg);

    return 0.25 * ((st0.t * row0) + (st1.t * row1) + (st2.t * row2) + (st3.t * row3));
  }
)";
   
constexpr const char* FisheyeVert = R"(
  **glsl_version**

  layout (location = 0) in vec2 TexCoords;
  layout (location = 1) in vec3 Position;

  out vec2 UV;

  void main() {
    gl_Position = vec4(Position, 1.0);
    UV = TexCoords;
  }
)";

constexpr const char* FisheyeFrag = R"(
  **glsl_version**

  in vec2 UV;
  out vec4 diffuse;

  uniform samplerCube cubemap;
  uniform float halfFov;
  const float angle45Factor = 0.7071067812;

  **sample_fun**
  **cubic_fun**
  **interpolate4f**

  void main() {
    diffuse = filter4f(UV, cubemap, **bgColor**);
  }
)";

constexpr const char* FisheyeFragNormal = R"(
  **glsl_version**

  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 normal;

  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform float halfFov;
  const float angle45Factor = 0.7071067812;

  **sample_fun**
  **cubic_fun**
  **interpolate3f**
  **interpolate4f**

  void main() {
    diffuse = filter4f(UV, cubemap, **bgColor**);
    normal = filter3f(UV, normalmap, vec4(0.0, 0.0, 0.0, 0.0));
  }
)";

constexpr const char* FisheyeFragPosition = R"(
  **glsl_version**

  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 position;

  uniform samplerCube cubemap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  const float angle45Factor = 0.7071067812;

  **sample_fun**
  **cubic_fun**
  **interpolate3f**
  **interpolate4f**

  void main() {
    diffuse = filter4f(UV, cubemap, **bgColor**);
    position = filter3f(UV, positionmap, vec4(0.0, 0.0, 0.0, 0.0));
  }
)";

constexpr const char* FisheyeFragNormalPosition = R"(
  **glsl_version**

  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 normal;
  layout(location = 2) out vec3 position;

  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  const float angle45Factor = 0.7071067812;

  **sample_fun**
  **cubic_fun**
  **interpolate3f**
  **interpolate4f**

  void main() {
    diffuse = filter4f(UV, cubemap, **bgColor**);
    normal = filter3f(UV, normalmap, vec4(0.0, 0.0, 0.0, 0.0));
    position = filter3f(UV, positionmap, vec4(0.0, 0.0, 0.0, 0.0));
  }
)";

constexpr const char* FisheyeFragDepth = R"(
  **glsl_version**

  in vec2 UV;
  out vec4 diffuse;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform float halfFov;
  const float angle45Factor = 0.7071067812;

  **sample_fun**
  **cubic_fun**
  **interpolate4f**
  **interpolatef**

  void main() {
    diffuse = filter4f(UV, cubemap, **bgColor**);
    gl_FragDepth = filterf(UV, depthmap, vec4(1.0, 1.0, 1.0, 1.0));
  }
)";

constexpr const char* FisheyeFragDepthNormal = R"(
  **glsl_version**

  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 normal;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube normalmap;
  uniform float halfFov;
  const float angle45Factor = 0.7071067812;

  **sample_fun**
  **cubic_fun**
  **interpolate4f**
  **interpolate3f**
  **interpolatef**

  void main() {
    diffuse = filter4f(UV, cubemap, **bgColor**);
    normal = filter3f(UV, normalmap, vec4(0.0, 0.0, 0.0, 0.0));
    gl_FragDepth = filterf(UV, depthmap, vec4(1.0, 1.0, 1.0, 1.0));
  }
)";

constexpr const char* FisheyeFragDepthPosition = R"(
  **glsl_version**

  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 position;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  const float angle45Factor = 0.7071067812;

  **sample_fun**
  **cubic_fun**
  **interpolate4f**
  **interpolate3f**
  **interpolatef**

  void main() {
    diffuse = filter4f(UV, cubemap, **bgColor**);
    position = filter3f(UV, positionmap, vec4(0.0, 0.0, 0.0, 0.0));
    gl_FragDepth = filterf(UV, depthmap, vec4(1.0, 1.0, 1.0, 1.0));
  }
)";

constexpr const char* FisheyeFragDepthNormalPosition = R"(
  **glsl_version**

  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 normal;
  layout(location = 2) out vec3 position;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube normalmap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  const float angle45Factor = 0.7071067812;

  **sample_fun**
  **cubic_fun**
  **interpolate4f**
  **interpolate3f**
  **interpolatef**

  void main() {
    diffuse = filter4f(UV, cubemap, **bgColor**);
    normal = filter3f(UV, normalmap, vec4(0.0, 0.0, 0.0, 0.0));
    position = filter3f(UV, positionmap, vec4(0.0, 0.0, 0.0, 0.0));
    gl_FragDepth = filterf(UV, depthmap, vec4(1.0, 1.0, 1.0, 1.0));
  }
)";

constexpr const char* FisheyeFragOffAxis = R"(
  **glsl_version**

  in vec2 UV;
  out vec4 diffuse;

  uniform samplerCube cubemap;
  uniform float halfFov;
  uniform vec3 offset;
  const float angle45Factor = 0.7071067812;

  **sample_fun**
  **cubic_fun**
  **interpolate4f**

  void main() {
    diffuse = filter4f(UV, cubemap, **bgColor**);
  }
)";

constexpr const char* FisheyeFragOffAxisNormal = R"(
  **glsl_version**

  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 normal;

  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform float halfFov;
  uniform vec3 offset;
  const float angle45Factor = 0.7071067812;

  **sample_fun**
  **cubic_fun**
  **interpolate3f**
  **interpolate4f**

  void main() {
    diffuse = filter4f(UV, cubemap, **bgColor**);
    normal = filter3f(UV, normalmap, vec4(0.0, 0.0, 0.0, 0.0));
  }
)";

constexpr const char* FisheyeFragOffAxisPosition = R"(
  **glsl_version**

  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 position;

  uniform samplerCube cubemap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  uniform vec3 offset;
  const float angle45Factor = 0.7071067812;

  **sample_fun**
  **cubic_fun**
  **interpolate3f**
  **interpolate4f**

  void main() {
    diffuse = filter4f(UV, cubemap, **bgColor**);
    position = filter3f(UV, positionmap, vec4(0.0, 0.0, 0.0, 0.0));
  }
)";

constexpr const char* FisheyeFragOffAxisNormalPosition = R"(
  **glsl_version**

  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 normal;
  layout(location = 2) out vec3 position;

  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  uniform vec3 offset;
  const float angle45Factor = 0.7071067812;

  **sample_fun**
  **cubic_fun**
  **interpolate3f**
  **interpolate4f**

  void main() {
    diffuse = filter4f(UV, cubemap, **bgColor**);
    normal = filter3f(UV, normalmap, vec4(0.0, 0.0, 0.0, 0.0));
    position = filter3f(UV, positionmap, vec4(0.0, 0.0, 0.0, 0.0));
  }
)";

constexpr const char* FisheyeFragOffAxisDepth = R"(
  **glsl_version**

  in vec2 UV;
  out vec4 diffuse;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform float halfFov;
  uniform vec3 offset;
  const float angle45Factor = 0.7071067812;

  **sample_fun**
  **cubic_fun**
  **interpolate4f**
  **interpolatef**

  void main() {
    diffuse = filter4f(UV, cubemap, **bgColor**);
    gl_FragDepth = filterf(UV, depthmap, vec4(1.0, 1.0, 1.0, 1.0));
  }
)";

constexpr const char* FisheyeFragOffAxisDepthNormal = R"(
  **glsl_version**

  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 normal;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube normalmap;
  uniform float halfFov;
  uniform vec3 offset;
  const float angle45Factor = 0.7071067812;

  **sample_fun**
  **cubic_fun**
  **interpolate4f**
  **interpolate3f**
  **interpolatef**

  void main() {
    diffuse = filter4f(UV, cubemap, **bgColor**);
    normal = filter3f(UV, normalmap, vec4(0.0, 0.0, 0.0, 0.0));
    gl_FragDepth = filterf(UV, depthmap, vec4(1.0, 1.0, 1.0, 1.0));
  }
)";

constexpr const char* FisheyeFragOffAxisDepthPosition = R"(
  **glsl_version**

  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 position;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  uniform vec3 offset;
  const float angle45Factor = 0.7071067812;

  **sample_fun**
  **cubic_fun**
  **interpolate4f**
  **interpolate3f**
  **interpolatef**

  void main() {
    diffuse = filter4f(UV, cubemap, **bgColor**);
    position = filter3f(UV, positionmap, vec4(0.0, 0.0, 0.0, 0.0));
    gl_FragDepth = filterf(UV, depthmap, vec4(1.0, 1.0, 1.0, 1.0));
  }
)";

constexpr const char* FisheyeFragOffAxisDepthNormalPosition = R"(
  **glsl_version**

  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 normal;
  layout(location = 2) out vec3 position;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube normalmap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  uniform vec3 offset;
  const float angle45Factor = 0.7071067812;

  **sample_fun**
  **cubic_fun**
  **interpolate4f**
  **interpolate3f**
  **interpolatef**

  void main() {
    diffuse = filter4f(UV, cubemap, **bgColor**);
    normal = filter3f(UV, normalmap, vec4(0.0, 0.0, 0.0, 0.0));
    position = filter3f(UV, positionmap, vec4(0.0, 0.0, 0.0, 0.0));
    gl_FragDepth = filterf(UV, depthmap, vec4(1.0, 1.0, 1.0, 1.0));
  }
)";

} // sgct::core::shaders_fisheye_cubic

#endif // __SGCT__INTERNAL_FISHEYE_SHADERS_CUBIC__H__
