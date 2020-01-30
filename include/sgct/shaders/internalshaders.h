/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__INTERNALSHADERS__H__
#define __SGCT__INTERNALSHADERS__H__

namespace sgct::shaders {

constexpr const char* BaseVert = R"(
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

constexpr const char* BaseFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D tex;

  void main() {
    out_color = tr_color * texture(tex, tr_uv);
  }
)";

constexpr const char* OverlayVert = R"(
  #version 330 core

  layout (location = 0) in vec2 in_texCoords;
  layout (location = 1) in vec3 in_position;
  out vec2 tr_uv;

  void main() {
    gl_Position = vec4(in_position, 1.0);
    tr_uv = in_texCoords;
  }
)";

constexpr const char* OverlayFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  out vec4 out_color;

  uniform sampler2D tex;

  void main() {
    out_color = texture(tex, tr_uv);
  }
)";

constexpr const char* AnaglyphVert = R"(
  #version 330 core

  layout (location = 0) in vec2 in_position;
  layout (location = 1) in vec2 in_texCoords;
  layout (location = 2) in vec3 in_color;
  out vec2 tr_uv;
  out vec4 tr_color;

  void main() {
    gl_Position = vec4(in_position, 0.0, 1.0);
    tr_uv = in_texCoords;
    tr_color = vec4(in_color, 1.0);
  }
)";

constexpr const char* AnaglyphRedCyanFrag = R"(
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

constexpr const char* AnaglyphRedCyanWimmerFrag = R"(
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

constexpr const char* AnaglyphAmberBlueFrag = R"(
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

constexpr const char* CheckerBoardFrag = R"(
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

constexpr const char* CheckerBoardInvertedFrag = R"(
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

constexpr const char* VerticalInterlacedFrag = R"(
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

constexpr const char* VerticalInterlacedInvertedFrag = R"(
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

constexpr const char* SBSFrag = R"(
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

constexpr const char* SBSInvertedFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;

  void main() {
    vec2 uv = tr_uv * vec2(2.0, 1.0);
    if (tr_uv.s >= 0.5) {
      uv -= vec2(1.0, 0.0);
    }
    out_color = tr_color * texture(tr_uv.s >= 0.5 ? leftTex : rightTex, uv);
  }
)";

constexpr const char* TBFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;

  void main() {
    vec2 uv = tr_uv * vec2(2.0, 1.0);
    if (tr_uv.t >= 0.5) {
      uv -= vec2(0.0, 1.0);
    }
    out_color = tr_color * texture(tr_uv.s >= 0.5 ? leftTex : rightTex, uv);
  }
)";

constexpr const char* TBInvertedFrag = R"(
  #version 330 core

  in vec2 tr_uv;
  in vec4 tr_color;
  out vec4 out_color;

  uniform sampler2D leftTex;
  uniform sampler2D rightTex;

  void main() {
    vec2 uv = tr_uv * vec2(1.0, 2.0);
    if (tr_uv.t >= 0.5) {
      uv -= vec2(0.0, 1.0);
    }
    out_color = tr_color * texture(tr_uv.t >= 0.5 ? rightTex : leftTex, uv);
  }
)";

constexpr const char* DummyStereoFrag = R"(
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

constexpr const char* FXAAVert = R"(
  #version 330 core

  layout (location = 0) in vec2 in_texCoords;
  layout (location = 1) in vec3 in_position;
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

constexpr const char* FXAAFrag = R"(
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

#endif // __SGCT__INTERNALSHADERS__H__
