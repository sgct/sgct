/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__INTERNALSHADERS__H__
#define __SGCT__INTERNALSHADERS__H__

namespace sgct::shaders {

constexpr std::string_view BaseVert = R"(
  #version 330 core

  layout (location = 0) in vec2 in_position;
  layout (location = 1) in vec2 in_texCoords;
  layout (location = 2) in vec4 in_color;
  out vec2 tr_uv;
  out vec4 tr_color;

  void main() {
    gl_Position = vec4(in_position, 0.0, 1.0);
    tr_uv = in_texCoords;
    tr_color = in_color;
  }
)";

constexpr std::string_view BaseFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D tex;

  void main() {
    out_color = tr_color * texture(tex, tr_uv);
  }
)";

constexpr std::string_view OverlayFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D tex;

  void main() {
    out_color = texture(tex, tr_uv);
  }
)";

constexpr std::string_view AnaglyphRedCyanFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;

  void main() {
    vec4 left = texture(leftTex, tr_uv);
    float leftLum = 0.3 * left.r + 0.59 * left.g + 0.11 * left.b;
    vec4 right = texture(rightTex, tr_uv);
    float rightLum = 0.3 * right.r + 0.59 * right.g + 0.11 * right.b;

    out_color = tr_color * vec4(leftLum, rightLum, rightLum, max(left.a, right.a));
  }
)";

constexpr std::string_view AnaglyphRedCyanWimmerFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;

  void main() {
    vec4 left = texture(leftTex, tr_uv);
    vec4 right = texture(rightTex, tr_uv);
    vec4 fact = vec4(0.7 * left.g + 0.3 * left.b, right.r, right.b, max(left.a, right.a));
    out_color = tr_color * fact;
  }
)";

constexpr std::string_view AnaglyphAmberBlueFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;

  void main() {
    vec4 left = texture(leftTex, tr_uv);
    vec4 right = texture(rightTex, tr_uv);
    const vec3 coef = vec3(0.15, 0.15, 0.70);
    float rightMix = dot(right.rbg, coef);
    out_color = tr_color * vec4(left.r, left.g, rightMix, max(left.a, right.a));
  }
)";

constexpr std::string_view CheckerBoardFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;

  void main() {
    float v = (gl_FragCoord.x + gl_FragCoord.y) * 0.5;
    out_color = tr_color * texture((v - floor(v)) == 0.0 ? rightTex : leftTex, tr_uv);
  }
)";

constexpr std::string_view CheckerBoardInvertedFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;

  void main() {
    float v = (gl_FragCoord.x + gl_FragCoord.y) * 0.5;
    out_color = tr_color * texture((v - floor(v)) == 0.0 ? leftTex : rightTex, tr_uv);
  }
)";

constexpr std::string_view VerticalInterlacedFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;

  void main() {
    float v = gl_FragCoord.y * 0.5;
    out_color = tr_color * texture((v - floor(v)) > 0.5 ? rightTex : leftTex, tr_uv);
  }
)";

constexpr std::string_view VerticalInterlacedInvertedFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;

  void main() {
    float v = gl_FragCoord.y * 0.5;
    out_color = tr_color * texture((v - floor(v)) > 0.5 ? leftTex : rightTex, tr_uv);
  }
)";

constexpr std::string_view SBSFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;

  void main() {
    vec2 uv = tr_uv * vec2(2.0, 1.0);
    if (tr_uv.s < 0.5) {
      out_color = tr_color * texture(leftTex, uv);
    }
    else {
      uv -= vec2(1.0, 0.0);
      out_color = tr_color * texture(rightTex, uv);
    }
  }
)";

constexpr std::string_view SBSInvertedFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;

  void main() {
    vec2 uv = tr_uv * vec2(2.0, 1.0);
    if (tr_uv.s >= 0.5) {
      uv.s -= 1.0;
    }
    out_color = tr_color * texture(tr_uv.s >= 0.5 ? leftTex : rightTex, uv);
  }
)";

constexpr std::string_view TBFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;

  void main() {
    vec2 uv = tr_uv * vec2(2.0, 1.0);
    if (tr_uv.t >= 0.5) {
      uv.t -= 1.0;
    }
    out_color = tr_color * texture(tr_uv.s >= 0.5 ? leftTex : rightTex, uv);
  }
)";

constexpr std::string_view TBInvertedFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;

  void main() {
    vec2 uv = tr_uv * vec2(1.0, 2.0);
    if (tr_uv.t >= 0.5) {
      uv.t -= 1.0;
    }
    out_color = tr_color * texture(tr_uv.t >= 0.5 ? rightTex : leftTex, uv);
  }
)";

constexpr std::string_view DummyStereoFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;

  void main() {
    out_color = tr_color * mix(texture(leftTex, tr_uv), texture(rightTex, tr_uv), 0.5);
  }
)";

constexpr std::string_view FXAAVert = R"(
  #version 330 core

  layout (location = 0) in vec3 in_position;
  layout (location = 1) in vec2 in_texCoords;
  out vec2 tr_uv;
  out vec2 tr_texcoordOffset[4];

  uniform float rt_w;
  uniform float rt_h;
  uniform float FXAA_SUBPIX_OFFSET;

  void main() {
    gl_Position = vec4(in_position, 1.0);
    tr_uv = in_texCoords;

    tr_texcoordOffset[0] = tr_uv + FXAA_SUBPIX_OFFSET * vec2(-1.0 / rt_w,  -1.0 / rt_h);
    tr_texcoordOffset[1] = tr_uv + FXAA_SUBPIX_OFFSET * vec2( 1.0 / rt_w,  -1.0 / rt_h);
    tr_texcoordOffset[2] = tr_uv + FXAA_SUBPIX_OFFSET * vec2(-1.0 / rt_w,   1.0 / rt_h);
    tr_texcoordOffset[3] = tr_uv + FXAA_SUBPIX_OFFSET * vec2( 1.0 / rt_w,   1.0 / rt_h);
  }
)";

constexpr std::string_view FXAAFrag = R"(
  #version 330 core

  // FXAA_EDGE_THRESHOLD: The minimum amount of local contrast required to apply algorithm
  //   1/3 - too little
  //   1/4 - low quality
  //   1/8 - high quality
  //   1/16 - overkill
  const float FXAA_EDGE_THRESHOLD_MIN = 1.0 / 16.0;
  const float FXAA_EDGE_THRESHOLD = 1.0 / 8.0;

  // FXAA_EDGE_THRESHOLD_MIN: Trims the algorithm from processing dark.
  //   1/32 - visible limit
  //   1/16 - high quality
  //   1/12 - upper limit (start of visible unfiltered edges)
  const float FXAA_SPAN_MAX = 8.0;

  // FXAA_SUBPIX_TRIM: Controls removal of sub-pixel aliasing
  //   1/2 - low removal
  //   1/3 - medium removal
  //   1/4 - default removal
  //   1/8 - high removal
  //     0 - complete removal
  uniform float FXAA_SUBPIX_TRIM; // 1.0 / 8.0;

  in vec2 tr_texcoordOffset[4];
  in vec2 tr_uv;
  out vec4 out_color;

  uniform float rt_w;
  uniform float rt_h;
  uniform sampler2D tex;

  void main() {
    vec3 rgbNW = textureLod(tex, tr_texcoordOffset[0], 0.0).xyz;
    vec3 rgbNE = textureLod(tex, tr_texcoordOffset[1], 0.0).xyz;
    vec3 rgbSW = textureLod(tex, tr_texcoordOffset[2], 0.0).xyz;
    vec3 rgbSE = textureLod(tex, tr_texcoordOffset[3], 0.0).xyz;
    vec3 rgbM  = textureLod(tex, tr_uv, 0.0).xyz;

    const vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot( rgbM, luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    float range = lumaMax - lumaMin;
    // local contrast check, for not processing homogeneous areas
    if (range < max(FXAA_EDGE_THRESHOLD_MIN, lumaMax * FXAA_EDGE_THRESHOLD)) {
      out_color = vec4(rgbM, 1.0);
      return;
    }

    vec2 dir = vec2(
      -((lumaNW + lumaNE) - (lumaSW + lumaSE)),
      ((lumaNW + lumaSW) - (lumaNE + lumaSE))
    );

    const float FXAA_REDUCE_MIN = 1.0 / 128.0;
    float dirReduce = max(
      (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_SUBPIX_TRIM),
      FXAA_REDUCE_MIN
    );

    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(
      vec2(FXAA_SPAN_MAX,  FXAA_SPAN_MAX),
      max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)
    ) / vec2(rt_w, rt_h);

    vec3 rgbA = 0.5 * (
      textureLod(tex, tr_uv + dir * (1.0 / 3.0 - 0.5), 0.0).xyz +
      textureLod(tex, tr_uv + dir * (2.0 / 3.0 - 0.5), 0.0).xyz
    );
    vec3 rgbB = rgbA * 0.5 + (1.0/4.0) * (
      textureLod(tex, tr_uv + dir * (0.0 / 3.0 - 0.5), 0.0).xyz +
      textureLod(tex, tr_uv + dir * (3.0 / 3.0 - 0.5), 0.0).xyz
    );
    float lumaB = dot(rgbB, luma);

    if ((lumaB < lumaMin) || (lumaB > lumaMax))  {
      out_color = vec4(rgbA, 1.0);
    }
    else {
      out_color = vec4(rgbB, 1.0);
    }
  }
)";

} // namespace sgct::shaders

namespace sgct::shaders_fisheye {

constexpr std::string_view RotationFourFaceCubeFun = R"(
  #version 330 core

  vec3 rotate(vec3 dir) {
    const float Angle = 0.7071067812;
    float x = Angle*dir.x + Angle*dir.z;
    float y = dir.y;
    float z = -Angle*dir.x + Angle*dir.z;

    return vec3(x, y, z);
  }
)";

constexpr std::string_view RotationFiveSixFaceCubeFun = R"(
  #version 330 core

  vec3 rotate(vec3 dir) {
    const float Angle = 0.7071067812;
    float x = Angle*dir.x - Angle*dir.y;
    float y = Angle*dir.x + Angle*dir.y;
    float z = dir.z;
    return vec3(x, y, z);
  }
)";

constexpr std::string_view RotationFun = R"(
  #version 330 core

  uniform mat4 rotMatrix;

  vec3 rotate(vec3 dir) {
    return (rotMatrix * vec4(dir, 0.0)).xyz;
  }
)";

constexpr std::string_view SampleFun = R"(
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

constexpr std::string_view SampleLatlonFun = R"(
  #version 330 core

  vec3 rotate(vec3 dir);

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg) {
    float phi = 3.14159265359 * (1.0 - texel.t);
    float theta = 6.28318530718 * (texel.s - 0.5);
    vec3 dir = vec3(sin(phi) * sin(theta), sin(phi) * cos(theta), cos(phi));
    return texture(map, rotate(dir));
  }
)";

constexpr std::string_view SampleOffsetFun = R"(
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

constexpr std::string_view InterpolateLinearFun = R"(
  #version 330 core

  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg);

  vec4 sample(vec2 tc, samplerCube map, vec4 bg) {
    return getCubeSample(tc, map, bg);
  }
)";

constexpr std::string_view InterpolateCubicFun = R"(
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

constexpr std::string_view BaseVert = R"(
  #version 330 core

  layout (location = 0) in vec3 in_position;
  layout (location = 1) in vec2 in_texCoords;
  out vec2 tr_uv;

  void main() {
    gl_Position = vec4(in_position, 1.0);
    tr_uv = in_texCoords;
  }
)";

constexpr std::string_view FisheyeFrag = R"(
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

constexpr std::string_view FisheyeFragNormal = R"(
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

constexpr std::string_view FisheyeFragPosition = R"(
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

constexpr std::string_view FisheyeFragNormalPosition = R"(
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

constexpr std::string_view FisheyeFragDepth = R"(
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

constexpr std::string_view FisheyeFragDepthNormal = R"(
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

constexpr std::string_view FisheyeFragDepthPosition = R"(
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

constexpr std::string_view FisheyeFragDepthNormalPosition = R"(
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

constexpr std::string_view FisheyeFragOffAxis = R"(
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

constexpr std::string_view FisheyeFragOffAxisNormal = R"(
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

constexpr std::string_view FisheyeFragOffAxisPosition = R"(
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

constexpr std::string_view FisheyeFragOffAxisNormalPosition = R"(
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

constexpr std::string_view FisheyeFragOffAxisDepth = R"(
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

constexpr std::string_view FisheyeFragOffAxisDepthNormal = R"(
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

constexpr std::string_view FisheyeFragOffAxisDepthPosition = R"(
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

constexpr std::string_view FisheyeFragOffAxisDepthNormalPosition = R"(
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

constexpr std::string_view FisheyeDepthCorrectionFrag = R"(
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


#endif // __SGCT__INTERNALSHADERS__H__
