/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__INTERNALSHADERS__H__
#define __SGCT__INTERNALSHADERS__H__

#include <string_view>

namespace sgct::shaders {

constexpr std::string_view BaseVert = R"(
  #version 460 core

  layout (location = 0) in vec2 in_position;
  layout (location = 1) in vec2 in_texCoords;
  layout (location = 2) in vec4 in_color;

  out Data {
    vec2 texCoords;
    vec4 color;
  } out_data;


  void main() {
    gl_Position = vec4(in_position, 0.0, 1.0);
    out_data.texCoords = in_texCoords;
    out_data.color = in_color;
  }
)";

constexpr std::string_view BaseFrag = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
    vec4 color;
  } in_data;

  out vec4 out_color;

  uniform sampler2D tex;
  uniform int flipX = 0;
  uniform int flipY = 0;


  void main() {
    vec2 uv = in_data.texCoords;
    if (flipX != 0) {
      uv.x = 1.0 - uv.x;
    }
    if (flipY != 0) {
      uv.y = 1.0 - uv.y;
    }

    out_color = in_data.color * texture(tex, uv);
  }
)";

constexpr std::string_view OverlayFrag = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
    vec4 color;
  } in_data;

  out vec4 out_color;

  uniform sampler2D tex;


  void main() {
    out_color = texture(tex, in_data.texCoords);
  }
)";

constexpr std::string_view AnaglyphRedCyanFrag = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
    vec4 color;
  } in_data;

  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;


  void main() {
    vec4 left = texture(leftTex, in_data.texCoords);
    float leftLum = 0.3 * left.r + 0.59 * left.g + 0.11 * left.b;
    vec4 right = texture(rightTex, in_data.texCoords);
    float rightLum = 0.3 * right.r + 0.59 * right.g + 0.11 * right.b;

    out_color = in_data.color * vec4(leftLum, rightLum, rightLum, max(left.a, right.a));
  }
)";

constexpr std::string_view AnaglyphRedCyanWimmerFrag = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
    vec4 color;
  } in_data;

  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;


  void main() {
    vec4 left = texture(leftTex, in_data.texCoords);
    vec4 right = texture(rightTex, in_data.texCoords);
    vec4 fact = vec4(0.7 * left.g + 0.3 * left.b, right.r, right.b, max(left.a, right.a));
    out_color = in_data.color * fact;
  }
)";

constexpr std::string_view AnaglyphAmberBlueFrag = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
    vec4 color;
  } in_data;

  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;


  void main() {
    vec4 left = texture(leftTex, in_data.texCoords);
    vec4 right = texture(rightTex, in_data.texCoords);
    const vec3 coef = vec3(0.15, 0.15, 0.70);
    float rightMix = dot(right.rbg, coef);
    out_color = in_data.color * vec4(left.r, left.g, rightMix, max(left.a, right.a));
  }
)";

constexpr std::string_view CheckerBoardFrag = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
    vec4 color;
  } in_data;

  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;


  void main() {
    float v = mix(gl_FragCoord.x, gl_FragCoord.y, 0.5);
    if ((v - floor(v)) == 0.0) {
      out_color = in_data.color * texture(rightTex, in_data.texCoords);
    }
    else {
      out_color = in_data.color * texture(leftTex, in_data.texCoords);
    }
  }
)";

constexpr std::string_view CheckerBoardInvertedFrag = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
    vec4 color;
  } in_data;

  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;


  void main() {
    float v = mix(gl_FragCoord.x, gl_FragCoord.y, 0.5);
    if ((v - floor(v)) == 0.0) {
      out_color = in_data.color * texture(leftTex, in_data.texCoords);
    }
    else {
      out_color = in_data.color * texture(rightTex, in_data.texCoords);
    }
  }
)";

constexpr std::string_view VerticalInterlacedFrag = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
    vec4 color;
  } in_data;

  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;


  void main() {
    float v = gl_FragCoord.y * 0.5;
    if ((v - floor(v)) > 0.5) {
      out_color = in_data.color * texture(rightTex, in_data.texCoords);
    }
    else {
      out_color = in_data.color * texture(leftTex, in_data.texCoords);
    }
  }
)";

constexpr std::string_view VerticalInterlacedInvertedFrag = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
    vec4 color;
  } in_data;

  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;


  void main() {
    float v = gl_FragCoord.y * 0.5;
    if ((v - floor(v)) > 0.5) {
      out_color = in_data.color * texture(leftTex, in_data.texCoords);
    }
    else {
      out_color = in_data.color * texture(rightTex, in_data.texCoords);
    }
  }
)";

constexpr std::string_view SBSFrag = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
    vec4 color;
  } in_data;

  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;


  void main() {
    vec2 uv = in_data.texCoords * vec2(2.0, 1.0);
    if (in_data.texCoords.s < 0.5) {
      out_color = in_data.color * texture(leftTex, uv);
    }
    else {
      uv -= vec2(1.0, 0.0);
      out_color = in_data.color * texture(rightTex, uv);
    }
  }
)";

constexpr std::string_view SBSInvertedFrag = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
    vec4 color;
  } in_data;

  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;


  void main() {
    vec2 uv = in_data.texCoords * vec2(2.0, 1.0);
    if (in_data.texCoords.s >= 0.5) {
      uv.s -= 1.0;
      out_color = in_data.color * texture(leftTex, uv);
    }
    else {
      out_color = in_data.color * texture(rightTex, uv);
    }
  }
)";

constexpr std::string_view TBFrag = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
    vec4 color;
  } in_data;

  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;


  void main() {
    vec2 uv = in_data.texCoords * vec2(2.0, 1.0);
    if (in_data.texCoords.t >= 0.5) {
      uv.t -= 1.0;
      out_color = in_data.color * texture(leftTex, uv);
    }
    else {
      out_color = in_color.color * texture(rightTex, uv);
    }
  }
)";

constexpr std::string_view TBInvertedFrag = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
    vec4 color;
  } in_data;

  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;


  void main() {
    vec2 uv = in_data.texCoords * vec2(1.0, 2.0);
    if (tr_uv.t >= 0.5) {
      uv.t -= 1.0;
      out_color = in_data.color * texture(rightTex, uv);
    }
    else {
      out_color = in_data.color * texture(leftTex, uv);
    }
  }
)";

constexpr std::string_view DummyStereoFrag = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
    vec4 color;
  } in_data;

  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;


  void main() {
    vec4 left = texture(leftTex, in_data.texCoords);
    vec4 right = texture(rightTex, in_data.texCoords);
    out_color = in_data.color * mix(left, right, 0.5);
  }
)";

constexpr std::string_view FXAAVert = R"(
  #version 460 core

  layout (location = 0) in vec3 in_position;
  layout (location = 1) in vec2 in_texCoords;

  out Data {
    vec2 texCoords;
    vec2 texCoordsOffset[4];
  } out_data;

  uniform float rt_w;
  uniform float rt_h;
  uniform float FxaaSubpixOffset;


  void main() {
    gl_Position = vec4(in_position, 1.0);
    out_data.texCoords = in_texCoords;

    out_data.texCoordsOffset[0] =
      out_data.texCoords + FxaaSubpixOffset * vec2(-1.0 / rt_w,  -1.0 / rt_h);
    out_data.texCoordsOffset[1] =
      out_data.texCoords + FxaaSubpixOffset * vec2( 1.0 / rt_w,  -1.0 / rt_h);
    out_data.texCoordsOffset[2] =
      out_data.texCoords + FxaaSubpixOffset * vec2(-1.0 / rt_w,   1.0 / rt_h);
    out_data.texCoordsOffset[3] =
      out_data.texCoords + FxaaSubpixOffset * vec2( 1.0 / rt_w,   1.0 / rt_h);
  }
)";

constexpr std::string_view FXAAFrag = R"(
  #version 460 core

  // FxaaEdgeThreshold: The minimum amount of local contrast required to apply algorithm
  //   1/3 - too little
  //   1/4 - low quality
  //   1/8 - high quality
  //   1/16 - overkill
  const float FxaaEdgeThreshold = 1.0 / 8.0;

  // FxaaEdgeThresholdMin: Trims the algorithm from processing dark.
  //   1/32 - visible limit
  //   1/16 - high quality
  //   1/12 - upper limit (start of visible unfiltered edges)
  const float FxaaEdgeThresholdMin = 1.0 / 16.0;

  const float FxaaSpanMax = 8.0;

  // FxaaSubPixTrim: Controls removal of sub-pixel aliasing
  //   1/2 - low removal
  //   1/3 - medium removal
  //   1/4 - default removal
  //   1/8 - high removal
  //     0 - complete removal
  uniform float FxaaSubPixTrim; // 1.0 / 8.0;

  in Data {
    vec2 texCoords;
    vec2 texcoordsOffset[4];
  } in_data;

  out vec4 out_color;

  uniform float rt_w;
  uniform float rt_h;
  uniform sampler2D tex;

  void main() {
    vec3 rgbNW = textureLod(tex, in_data.texcoordsOffset[0], 0.0).rgb;
    vec3 rgbNE = textureLod(tex, in_data.texcoordsOffset[1], 0.0).rgb;
    vec3 rgbSW = textureLod(tex, in_data.texcoordsOffset[2], 0.0).rgb;
    vec3 rgbSE = textureLod(tex, in_data.texcoordsOffset[3], 0.0).rgb;
    vec3 rgbM  = textureLod(tex, in_data.texCoords, 0.0).rgb;

    const vec3 Luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, Luma);
    float lumaNE = dot(rgbNE, Luma);
    float lumaSW = dot(rgbSW, Luma);
    float lumaSE = dot(rgbSE, Luma);
    float lumaM  = dot( rgbM, Luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    float range = lumaMax - lumaMin;
    // local contrast check, for not processing homogeneous areas
    if (range < max(FxaaEdgeThresholdMin, lumaMax * FxaaEdgeThreshold)) {
      out_color = vec4(rgbM, 1.0);
      return;
    }

    vec2 dir = vec2(
      -((lumaNW + lumaNE) - (lumaSW + lumaSE)),
      (lumaNW + lumaSW) - (lumaNE + lumaSE)
    );

    const float FxaaReduceMin = 1.0 / 128.0;
    float dirReduce = max(
      (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FxaaSubPixTrim),
      FxaaReduceMin
    );

    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(
      vec2(FxaaSpanMax,  FxaaSpanMax),
      max(vec2(-FxaaSpanMax, -FxaaSpanMax), dir * rcpDirMin)
    ) / vec2(rt_w, rt_h);

    vec3 rgbA = 0.5 * (
      textureLod(tex, in_data.texCoords + dir * (1.0 / 3.0 - 0.5), 0.0).rgb +
      textureLod(tex, in_data.texCoords + dir * (2.0 / 3.0 - 0.5), 0.0).rgb
    );
    vec3 rgbB = rgbA * 0.5 + (1.0 / 4.0) * (
      textureLod(tex, in_data.texCoords + dir * (0.0 / 3.0 - 0.5), 0.0).rgb +
      textureLod(tex, in_data.texCoords + dir * (3.0 / 3.0 - 0.5), 0.0).rgb
    );
    float lumaB = dot(rgbB, Luma);

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
  #version 460 core

  vec3 rotate(vec3 dir) {
    const float Angle = 0.7071067812;
    float x = Angle * dir.x + Angle * dir.z;
    float y = dir.y;
    float z = -Angle * dir.x + Angle * dir.z;
    return vec3(x, y, z);
  }
)";

constexpr std::string_view RotationFiveSixFaceCubeFun = R"(
  #version 460 core

  vec3 rotate(vec3 dir) {
    const float Angle = 0.7071067812;
    float x = Angle * dir.x - Angle * dir.y;
    float y = Angle * dir.x + Angle * dir.y;
    float z = dir.z;
    return vec3(x, y, z);
  }
)";

constexpr std::string_view RotationFun = R"(
  #version 460 core

  uniform mat4 rotMatrix;

  vec3 rotate(vec3 dir) {
    return (rotMatrix * vec4(dir, 0.0)).xyz;
  }
)";

constexpr std::string_view SampleFun = R"(
  #version 460 core

  uniform float halfFov;

  vec3 rotate(vec3 dir);

  vec4 sampleCubeTexture(vec2 texel, samplerCube map, vec4 background) {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s * s + t * t;
    if (r2 <= 1.0) {
      float phi = sqrt(r2) * halfFov;
      float theta = atan(s, t);
      vec3 dir = vec3(sin(phi) * sin(theta), -sin(phi) * cos(theta), cos(phi));
      return texture(map, rotate(dir));
    }
    else {
      return background;
    }
  }
)";

constexpr std::string_view SampleLatlonFun = R"(
  #version 460 core

  vec3 rotate(vec3 dir);

  const float M_PI = 3.14159265359;

  vec4 sampleCubeTexture(vec2 texel, samplerCube map, vec4 background) {
    float phi = M_PI * (1.0 - texel.t);
    float theta = 2.0 * M_PI * (texel.s - 0.5);
    vec3 dir = vec3(sin(phi) * sin(theta), sin(phi) * cos(theta), cos(phi));
    return texture(map, rotate(dir));
  }
)";

constexpr std::string_view SampleOffsetFun = R"(
  #version 460 core

  uniform float halfFov;
  uniform vec3 offset;

  vec3 rotate(vec3 dir);

  vec4 sampleCubeTexture(vec2 texel, samplerCube map, vec4 background) {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s * s + t * t;
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
      return background;
    }
  }
)";

constexpr std::string_view InterpolateLinearFun = R"(
  #version 460 core

  vec4 sampleCubeTexture(vec2 texel, samplerCube map, vec4 background);

  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background) {
    return sampleCubeTexture(texCoords, map, background);
  }
)";

constexpr std::string_view InterpolateCubicFun = R"(
  #version 460 core

  uniform float size;

  vec4 sampleCubeTexture(vec2 texel, samplerCube map, vec4 background);

  vec4 cubic(float x) {
    float x2 = x * x;
    float x3 = x2 * x;
    vec4 w = vec4(-x + 2 * x2 - x3, 2 - 5 * x2 + 3 * x3, x + 4 * x2 - 3 * x3, -x2 + x3);
    return w / 2.0;
  }

  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background) {
    vec2 transTex = texCoords * vec2(size, size);
    vec2 frac = fract(transTex);
    transTex -= frac;

    vec4 xcubic = cubic(frac.x);
    vec4 ycubic = cubic(frac.y);

    const float h = 1.0;
    vec4 c = transTex.xxyy + vec4(-h, +h, -h, +h);
    vec4 s = vec4(xcubic.xz + xcubic.yw, ycubic.xz + ycubic.yw);
    vec4 offset = c + vec4(xcubic.yw, ycubic.yw) / s;

    vec4 sample0 = sampleCubeTexture(offset.xz / size, map, background);
    vec4 sample1 = sampleCubeTexture(offset.yz / size, map, background);
    vec4 sample2 = sampleCubeTexture(offset.xw / size, map, background);
    vec4 sample3 = sampleCubeTexture(offset.yw / size, map, background);

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    return mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
  }
)";

constexpr std::string_view BaseVert = R"(
  #version 460 core

  layout (location = 0) in vec3 in_position;
  layout (location = 1) in vec2 in_texCoords;

  out Data {
    vec2 texCoords;
  } out_data;


  void main() {
    gl_Position = vec4(in_position, 1.0);
    out_data.texCoords = in_texCoords;
  }
)";

constexpr std::string_view FisheyeFrag = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  out vec4 out_diffuse;

  uniform samplerCube cubemap;
  uniform vec4 bgColor;


  vec4 cubeSample(vec2 texel, samplerCube map, vec4 background);


  void main() {
    out_diffuse = cubeSample(in_data.texCoords, cubemap, bgColor);
  }
)";

constexpr std::string_view FisheyeFragNormal = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;

  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform vec4 bgColor;


  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background);


  void main() {
    out_diffuse = cubeSample(in_data.texCoords, cubemap, bgColor);
    out_normal = cubeSample(in_data.texCoords, normalmap, vec4(0.0)).xyz;
  }
)";

constexpr std::string_view FisheyeFragPosition = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube positionmap;
  uniform vec4 bgColor;


  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background);


  void main() {
    out_diffuse = cubeSample(in_data.texCoords, cubemap, bgColor);
    out_position = cubeSample(in_data.texCoords, positionmap, vec4(0.0)).xyz;
  }
)";

constexpr std::string_view FisheyeFragNormalPosition = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;
  layout(location = 2) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform samplerCube positionmap;
  uniform vec4 bgColor;


  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background);


  void main() {
    out_diffuse = cubeSample(in_data.texCoords, cubemap, bgColor);
    out_normal = cubeSample(in_data.texCoords, normalmap, vec4(0.0)).xyz;
    out_position = cubeSample(in_data.texCoords, positionmap, vec4(0.0)).xyz;
  }
)";

constexpr std::string_view FisheyeFragDepth = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  out vec4 out_diffuse;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform vec4 bgColor;


  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background);


  void main() {
    out_diffuse = cubeSample(in_data.texCoords, cubemap, bgColor);
    gl_FragDepth = cubeSample(in_data.texCoords, depthmap, vec4(1.0)).x;
  }
)";

constexpr std::string_view FisheyeFragDepthNormal = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube normalmap;
  uniform vec4 bgColor;


  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background);


  void main() {
    out_diffuse = cubeSample(in_data.texCoords, cubemap, bgColor);
    out_normal = cubeSample(in_data.texCoords, normalmap, vec4(0.0)).xyz;
    gl_FragDepth = cubeSample(in_data.texCoords, depthmap, vec4(1.0)).x;
  }
)";

constexpr std::string_view FisheyeFragDepthPosition = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube positionmap;
  uniform vec4 bgColor;


  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background);


  void main() {
    out_diffuse = cubeSample(in_data.texCoords, cubemap, bgColor);
    out_position = cubeSample(in_data.texCoords, positionmap, vec4(0.0)).xyz;
    gl_FragDepth = cubeSample(in_data.texCoords, depthmap, vec4(1.0)).x;
  }
)";

constexpr std::string_view FisheyeFragDepthNormalPosition = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;
  layout(location = 2) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube normalmap;
  uniform samplerCube positionmap;
  uniform vec4 bgColor;


  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background);


  void main() {
    out_diffuse = cubeSample(in_data.texCoords, cubemap, bgColor);
    out_normal = cubeSample(in_data.texCoords, normalmap, vec4(0.0)).xyz;
    out_position = cubeSample(in_data.texCoords, positionmap, vec4(0.0)).xyz;
    gl_FragDepth = cubeSample(in_data.texCoords, depthmap, vec4(1.0)).x;
  }
)";

constexpr std::string_view FisheyeFragOffAxis = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  out vec4 out_diffuse;

  uniform samplerCube cubemap;
  uniform vec4 bgColor;


  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background);


  void main() {
    out_diffuse = cubeSample(in_data.texCoords, cubemap, bgColor);
  }
)";

constexpr std::string_view FisheyeFragOffAxisNormal = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;

  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform vec4 bgColor;


  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background);


  void main() {
    out_diffuse = cubeSample(in_data.texCoords, cubemap, bgColor);
    out_normal = cubeSample(in_data.texCoords, normalmap, vec4(0.0)).xyz;
  }
)";

constexpr std::string_view FisheyeFragOffAxisPosition = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube positionmap;
  uniform vec4 bgColor;


  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background);


  void main() {
    out_diffuse = cubeSample(in_data.texCoords, cubemap, bgColor);
    out_position = cubeSample(in_data.texCoords, positionmap, vec4(0.0)).xyz;
  }
)";

constexpr std::string_view FisheyeFragOffAxisNormalPosition = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;
  layout(location = 2) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform samplerCube positionmap;
  uniform vec4 bgColor;


  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background);


  void main() {
    out_diffuse = cubeSample(in_data.texCoords, cubemap, bgColor);
    out_normal = cubeSample(in_data.texCoords, normalmap, vec4(0.0)).xyz;
    out_position = cubeSample(in_data.texCoords, positionmap, vec4(0.0)).xyz;
  }
)";

constexpr std::string_view FisheyeFragOffAxisDepth = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  out vec4 out_diffuse;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform vec4 bgColor;


  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background);


  void main() {
    out_diffuse = cubeSample(in_data.texCoords, cubemap, bgColor);
    gl_FragDepth = cubeSample(in_data.texCoords, depthmap, vec4(1.0)).x;
  }
)";

constexpr std::string_view FisheyeFragOffAxisDepthNormal = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube normalmap;
  uniform vec4 bgColor;


  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background);


  void main() {
    out_diffuse = cubeSample(in_data.texCoords, cubemap, bgColor);
    out_normal = cubeSample(in_data.texCoords, normalmap, vec4(0.0)).xyz;
    gl_FragDepth = cubeSample(in_data.texCoords, depthmap, vec4(1.0)).x;
  }
)";

constexpr std::string_view FisheyeFragOffAxisDepthPosition = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube positionmap;
  uniform vec4 bgColor;


  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background);


  void main() {
    out_diffuse = cubeSample(in_data.texCoords, cubemap, bgColor);
    out_position = cubeSample(in_data.texCoords, positionmap, vec4(0.0)).xyz;
    gl_FragDepth = cubeSample(in_data.texCoords, depthmap, vec4(1.0)).x;
  }
)";

constexpr std::string_view FisheyeFragOffAxisDepthNormalPosition = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  layout(location = 0) out vec4 out_diffuse;
  layout(location = 1) out vec3 out_normal;
  layout(location = 2) out vec3 out_position;

  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform samplerCube normalmap;
  uniform samplerCube positionmap;
  uniform vec4 bgColor;


  vec4 cubeSample(vec2 texCoords, samplerCube map, vec4 background);


  void main() {
    out_diffuse = cubeSample(in_data.texCoords, cubemap, bgColor);
    out_normal = cubeSample(in_data.texCoords, normalmap, vec4(0.0)).xyz;
    out_position = cubeSample(in_data.texCoords, positionmap, vec4(0.0)).xyz;
    gl_FragDepth = cubeSample(in_data.texCoords, depthmap, vec4(1.0)).x;
  }
)";

constexpr std::string_view FisheyeDepthCorrectionFrag = R"(
  #version 460 core

  in Data {
    vec2 texCoords;
  } in_data;

  out vec4 out_color;

  uniform sampler2D cTex;
  uniform sampler2D dTex;
  uniform float near;
  uniform float far;


  void main() {
    float a = far / (far - near);
    float b = far * near / (near - far);
    float z = b / (texture(dTex, in_data.texCoords).r - a);

    // get angle from -45 to 45 degrees (-pi/4 to +pi/4)
    vec2 angle = 1.57079632679 * in_data.texCoords - 0.5;

    float xNorm = tan(angle.x);
    float yNorm = tan(angle.y);
    float r = z * sqrt(xNorm * xNorm + yNorm * yNorm + 1.0);

    out_color = texture(cTex, in_data.texCoords);
    gl_FragDepth = a + b / r;
  }
)";

} // namespace sgct::shaders_fisheye


#endif // __SGCT__INTERNALSHADERS__H__
