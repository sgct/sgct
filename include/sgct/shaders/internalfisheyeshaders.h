/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__INTERNALFISHEYESHADERS__H__
#define __SGCT__INTERNALFISHEYESHADERS__H__

namespace sgct::shaders_fisheye {

constexpr const char* RotationFourFaceCubeFun = R"(
  #version 330 core

  vec3 rotate(vec3 dir) {
    const float Angle = 0.7071067812;
    float x = Angle*dir.x + Angle*dir.z;
    float y = dir.y;
    float z = -Angle*dir.x + Angle*dir.z;

    return vec3(x, y, z);
  }
)";

constexpr const char* RotationFiveSixFaceCubeFun = R"(
  #version 330 core

  vec3 rotate(vec3 dir) {
    const float Angle = 0.7071067812;
    float x = Angle*dir.x - Angle*dir.y;
    float y = Angle*dir.x + Angle*dir.y;
    float z = dir.z;
    return vec3(x, y, z);
  }
)";

constexpr const char* RotationFun = R"(
  #version 330 core

  uniform mat4 rotMatrix;

  vec3 rotate(vec3 dir) {
    return (rotMatrix * vec4(dir, 0.0)).xyz;
  }
)";

constexpr const char* SampleFun = R"(
  #version 330 core

  uniform float halfFov;

  vec3 rotate(vec3 dir);

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg) {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s*s + t*t;
    if (r2 <= 1.0) {
      float phi = sqrt(r2) * halfFov;
      float theta = atan(s, t);
      vec3 dir = vec3(sin(phi) * sin(theta), -sin(phi) * cos(theta), cos(phi));
      return texture(map, rotate(dir));
    }
    else {
      return bg;
    }
  }
)";

constexpr const char* SampleLatlonFun = R"(
  #version 330 core

  vec3 rotate(vec3 dir);

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg) {
    float phi = 3.14159265359 * (1.0 - texel.t);
    float theta = 6.28318530718 * (texel.s - 0.5);
    vec3 dir = vec3(sin(phi) * sin(theta), sin(phi) * cos(theta), cos(phi));
    return texture(map, rotate(dir));
  }
)";

constexpr const char* SampleOffsetFun = R"(
  #version 330 core

  uniform float halfFov;
  uniform vec3 offset;

  vec3 rotate(vec3 dir);

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg) {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s*s + t*t;
    if (r2 <= 1.0) {
      float phi = sqrt(r2) * halfFov;
      float theta = atan(s, t);
      float x = sin(phi) * sin(theta) - offset.x;
      float y = -sin(phi) * cos(theta) - offset.y;
      float z = cos(phi) - offset.z;
      vec3 dir = vec3(x, y, z);
      return texture(map, rotate(dir));
    }
    else {
      return bg;
    }
  }
)";

constexpr const char* InterpolateLinearFun = R"(
  #version 330 core

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  vec4 sample(vec2 tc, samplerCube map, vec4 bg) {
    return getCubeSample(tc, map, bg);
  }
)";

constexpr const char* InterpolateCubicFun = R"(
  #version 330 core

  uniform float size;

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  vec4 cubic(float x) {
    float x2 = x * x;
    float x3 = x2 * x;
    vec4 w = vec4(-x + 2*x2 - x3, 2 - 5*x2 + 3*x3, x + 4*x2 - 3*x3, -x2 + x3);
    return w / 2.0;
  }

  vec4 sample(vec2 tc, samplerCube map, vec4 bg) {
    vec2 transTex = tc * vec2(size, size);
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

constexpr const char* BaseVert = R"(
  #version 330 core

  layout (location = 0) in vec2 in_texCoords;
  layout (location = 1) in vec3 in_position;
  out vec2 tr_uv;

  void main() {
    gl_Position = vec4(in_position, 1.0);
    tr_uv = in_texCoords;
  }
)";

constexpr const char* FisheyeFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  out vec4 out_diffuse;

  uniform samplerCube cubemap;
  uniform vec4 bgColor;

  vec4 sample(vec2 texel, samplerCube map, vec4 bg);

  void main() {
    out_diffuse = sample(tr_uv, cubemap, bgColor);
  }
)";

constexpr const char* FisheyeFragNormal = R"(
  #version 330 core

  in vec2 tr_uv;
  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;

  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform vec4 bgColor;

  vec4 sample(vec2 tc, samplerCube map, vec4 bg);

  void main() {
    out_diffuse = sample(tr_uv, cubemap, bgColor);
    out_normal = sample(tr_uv, normalmap, vec4(0.0)).xyz;
  }
)";

constexpr const char* FisheyeFragPosition = R"(
  #version 330 core

  in vec2 tr_uv;
  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube positionmap;
  uniform vec4 bgColor;

  vec4 sample(vec2 tc, samplerCube map, vec4 bg);

  void main() {
    out_diffuse = sample(tr_uv, cubemap, bgColor);
    out_position = sample(tr_uv, positionmap, vec4(0.0)).xyz;
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
  uniform vec4 bgColor;

  vec4 sample(vec2 tc, samplerCube map, vec4 bg);

  void main() {
    out_diffuse = sample(tr_uv, cubemap, bgColor);
    out_normal = sample(tr_uv, normalmap, vec4(0.0)).xyz;
    out_position = sample(tr_uv, positionmap, vec4(0.0)).xyz;
  }
)";

constexpr const char* FisheyeFragDepth = R"(
  #version 330 core

  in vec2 tr_uv;
  out vec4 out_diffuse;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform vec4 bgColor;

  vec4 sample(vec2 tc, samplerCube map, vec4 bg);

  void main() {
    out_diffuse = sample(tr_uv, cubemap, bgColor);
    gl_FragDepth = sample(tr_uv, depthmap, vec4(1.0)).x;
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
  uniform vec4 bgColor;

  vec4 sample(vec2 tc, samplerCube map, vec4 bg);

  void main() {
    out_diffuse = sample(tr_uv, cubemap, bgColor);
    out_normal = sample(tr_uv, normalmap, vec4(0.0)).xyz;
    gl_FragDepth = sample(tr_uv, depthmap, vec4(1.0)).x;
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
  uniform vec4 bgColor;

  vec4 sample(vec2 tc, samplerCube map, vec4 bg);

  void main() {
    out_diffuse = sample(tr_uv, cubemap, bgColor);
    out_position = sample(tr_uv, positionmap, vec4(0.0)).xyz;
    gl_FragDepth = sample(tr_uv, depthmap, vec4(1.0)).x;
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
  uniform vec4 bgColor;

  vec4 sample(vec2 tc, samplerCube map, vec4 bg);

  void main() {
    out_diffuse = sample(tr_uv, cubemap, bgColor);
    out_normal = sample(tr_uv, normalmap, vec4(0.0)).xyz;
    out_position = sample(tr_uv, positionmap, vec4(0.0)).xyz;
    gl_FragDepth = sample(tr_uv, depthmap, vec4(1.0)).x;
  }
)";

constexpr const char* FisheyeFragOffAxis = R"(
  #version 330 core

  in vec2 tr_uv;
  out vec4 out_diffuse;

  uniform samplerCube cubemap;
  uniform vec4 bgColor;

  vec4 sample(vec2 tc, samplerCube map, vec4 bg);

  void main() {
    out_diffuse = sample(tr_uv, cubemap, bgColor);
  }
)";

constexpr const char* FisheyeFragOffAxisNormal = R"(
  #version 330 core

  in vec2 tr_uv;
  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;

  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform vec4 bgColor;

  vec4 sample(vec2 tc, samplerCube map, vec4 bg);

  void main() {
    out_diffuse = sample(tr_uv, cubemap, bgColor);
    out_normal = sample(tr_uv, normalmap, vec4(0.0)).xyz;
  }
)";

constexpr const char* FisheyeFragOffAxisPosition = R"(
  #version 330 core

  in vec2 tr_uv;
  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube positionmap;
  uniform vec4 bgColor;

  vec4 sample(vec2 tc, samplerCube map, vec4 bg);

  void main() {
    out_diffuse = sample(tr_uv, cubemap, bgColor);
    out_position = sample(tr_uv, positionmap, vec4(0.0)).xyz;
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
  uniform vec4 bgColor;

  vec4 sample(vec2 tc, samplerCube map, vec4 bg);

  void main() {
    out_diffuse = sample(tr_uv, cubemap, bgColor);
    out_normal = sample(tr_uv, normalmap, vec4(0.0)).xyz;
    out_position = sample(tr_uv, positionmap, vec4(0.0)).xyz;
  }
)";

constexpr const char* FisheyeFragOffAxisDepth = R"(
  #version 330 core

  in vec2 tr_uv;
  out vec4 out_diffuse;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform vec4 bgColor;

  vec4 sample(vec2 tc, samplerCube map, vec4 bg);

  void main() {
    out_diffuse = sample(tr_uv, cubemap, bgColor);
    gl_FragDepth = sample(tr_uv, depthmap, vec4(1.0)).x;
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
  uniform vec4 bgColor;

  vec4 sample(vec2 tc, samplerCube map, vec4 bg);

  void main() {
    out_diffuse = sample(tr_uv, cubemap, bgColor);
    out_normal = sample(tr_uv, normalmap, vec4(0.0)).xyz;
    gl_FragDepth = sample(tr_uv, depthmap, vec4(1.0)).x;
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
  uniform vec4 bgColor;

  vec4 sample(vec2 tc, samplerCube map, vec4 bg);

  void main() {
    out_diffuse = sample(tr_uv, cubemap, bgColor);
    out_position = sample(tr_uv, positionmap, vec4(0.0)).xyz;
    gl_FragDepth = sample(tr_uv, depthmap, vec4(1.0)).x;
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
  uniform vec4 bgColor;

  vec4 sample(vec2 tc, samplerCube map, vec4 bg);

  void main() {
    out_diffuse = sample(tr_uv, cubemap, bgColor);
    out_normal = sample(tr_uv, normalmap, vec4(0.0)).xyz;
    out_position = sample(tr_uv, positionmap, vec4(0.0)).xyz;
    gl_FragDepth = sample(tr_uv, depthmap, vec4(1.0)).x;
  }
)";

constexpr const char* FisheyeDepthCorrectionFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  out vec4 Color;

  uniform sampler2D cTex;
  uniform sampler2D dTex;
  uniform float near;
  uniform float far;

  void main() {
    float a = far / (far - near);
    float b = far * near / (near - far);
    float z = b / (texture(dTex, tr_uv).x - a);

    // get angle from -45 to 45 degrees (-pi/4 to +pi/4)
    vec2 angle = 1.57079632679 * tr_uv - 0.5;

    float xNorm = tan(angle.x);
    float yNorm = tan(angle.y);
    float r = z * sqrt(xNorm * xNorm + yNorm * yNorm + 1.0);

    Color = texture(cTex, tr_uv);
    gl_FragDepth = a + b / r;
  }
)";

} // namespace sgct::shaders_fisheye

#endif // __SGCT__INTERNALFISHEYESHADERS__H__
