/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__INTERNAL_SHADERS__H__
#define __SGCT__INTERNAL_SHADERS__H__

namespace sgct::core::shaders {

constexpr const char* BaseVert = R"(
    **glsl_version**
    
    layout (location = 0) in vec2 Position;
    layout (location = 1) in vec2 TexCoords;
    layout (location = 2) in vec4 VertColor;
    
    out vec2 UV;
    out vec4 Col;
    
    void main() {
        gl_Position = vec4(Position, 0.0, 1.0);
        UV = TexCoords;
        Col = VertColor;
    }
)";

constexpr const char* BaseFrag = R"(
    **glsl_version**
    
    in vec2 UV;
    in vec4 Col;
    out vec4 Color;
    
    uniform sampler2D Tex;
    
    void main() {
        Color = Col * texture(Tex, UV);
    }
)";

constexpr const char* OverlayVert = R"(
    **glsl_version**
    
    layout (location = 0) in vec2 TexCoords;
    layout (location = 1) in vec3 Position;
    
    out vec2 UV;
    
    void main() {
        gl_Position = vec4(Position, 1.0);
        UV = TexCoords;
    }
)";

constexpr const char* OverlayFrag = R"(
    **glsl_version**
    
    in vec2 UV;
    out vec4 Color;
    
    uniform sampler2D Tex;
    
    void main() {
        Color = texture(Tex, UV);
    }
)";

constexpr const char* AnaglyphVert = R"(
    **glsl_version**
    
    layout (location = 0) in vec2 Position;
    layout (location = 1) in vec2 TexCoords;
    layout (location = 2) in vec3 VertColor;
    
    out vec2 UV;
    out vec4 Col;
    
    void main() {
        gl_Position = vec4(Position, 0.0, 1.0);
        UV = TexCoords;
        Col = vec4(VertColor, 1.0);
    }
)";

constexpr const char* AnaglyphRedCyanFrag = R"(
    **glsl_version**
    
    in vec2 UV;
    in vec4 Col;
    out vec4 Color;
    
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;
    
    void main() {
        vec4 leftVals = texture(LeftTex, UV);
        float leftLum = 0.3 * leftVals.r + 0.59 * leftVals.g + 0.11 * leftVals.b;
        vec4 rightVals = texture(RightTex, UV);
        float rightLum = 0.3 * rightVals.r + 0.59 * rightVals.g + 0.11 * rightVals.b;

        Color = Col * vec4(leftLum, rightLum, rightLum, max(leftVals.a, rightVals.a));
    }
)";

constexpr const char* AnaglyphRedCyanWimmerFrag = R"(
    **glsl_version**
    
    in vec2 UV;
    in vec4 Col;
    out vec4 Color;
    
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;
    
    void main() {
        vec4 leftVals = texture(LeftTex, UV);
        vec4 rightVals = texture(RightTex, UV);
        vec4 fact = vec4(
            0.7 * leftVals.g + 0.3 * leftVals.b,
            rightVals.r,
            rightVals.b,
            max(leftVals.a, rightVals.a)
        );
        Color = Col * fact;
    }
)";

constexpr const char* AnaglyphAmberBlueFrag = R"(
    **glsl_version**
    
    in vec2 UV;
    in vec4 Col;
    out vec4 Color;
    
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;
    
    void main() {
        vec4 leftVals = texture(LeftTex, UV);
        vec4 rightVals = texture(RightTex, UV);
        vec3 coef = vec3(0.15, 0.15, 0.70);
        float rightMix = dot(rightVals.rbg, coef);
        Color = Col * vec4(leftVals.r, leftVals.g, rightMix, max(leftVals.a, rightVals.a));
    }
)";

constexpr const char* CheckerBoardFrag = R"(
    **glsl_version**
    
    in vec2 UV;
    in vec4 Col;
    out vec4 Color;
    
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;
    
    void main() {
        float fval = (gl_FragCoord.x + gl_FragCoord.y) * 0.5;
        if ((fval - floor(fval)) == 0.0) {
            Color = Col * texture(RightTex, UV);
        }
        else {
            Color = Col * texture(LeftTex, UV);
        }
    }
)";

constexpr const char* CheckerBoardInvertedFrag = R"(
    **glsl_version**
    
    in vec2 UV;
    in vec4 Col;
    out vec4 Color;
    
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;
    
    void main() {
        float fval = (gl_FragCoord.x + gl_FragCoord.y) * 0.5;
        if ((fval - floor(fval)) == 0.0) {
            Color = Col * texture(LeftTex, UV);
        }
        else {
            Color = Col * texture(RightTex, UV);
        }
    }
)";

constexpr const char* VerticalInterlacedFrag = R"(
    **glsl_version**
    
    in vec2 UV;
    in vec4 Col;
    out vec4 Color;
    
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;
    
    void main() {
        float fval = gl_FragCoord.y * 0.5;
        if ((fval - floor(fval)) > 0.5) {
            Color = Col * texture(RightTex, UV);
        }
        else {
            Color = Col * texture(LeftTex, UV);
        }
    }
)";

constexpr const char* VerticalInterlacedInvertedFrag = R"(
    **glsl_version**
    
    in vec2 UV;
    in vec4 Col;
    out vec4 Color;
    
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;
    
    void main() {
        float fval = gl_FragCoord.y * 0.5;
        if ((fval - floor(fval)) > 0.5) {
            Color = Col * texture(LeftTex, UV);
        }
        else {
            Color = Col * texture(RightTex, UV);
        }
    }
)";

constexpr const char* SBSFrag = R"(
    **glsl_version**
    
    in vec2 UV;
    in vec4 Col;
    out vec4 Color;
    
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;
    
    void main() {
        if (UV.s < 0.5) {
            Color = Col * texture(LeftTex, UV.st * vec2(2.0, 1.0)); 
        }
        else {
            Color = Col * texture(RightTex, UV.st * vec2(2.0, 1.0) - vec2(1.0, 0.0)); 
        }
    }
)";

constexpr const char* SBSInvertedFrag = R"(
    **glsl_version**
    
    in vec2 UV;
    in vec4 Col;
    out vec4 Color;
    
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;
    
    void main() {
        if (UV.s < 0.5) { 
            Color = Col * texture(RightTex, UV.st * vec2(2.0, 1.0)); 
        }
        else {
            Color = Col * texture(LeftTex, UV.st * vec2(2.0, 1.0) - vec2(1.0, 0.0)); 
        }
    }
)";

constexpr const char* TBFrag = R"(
    **glsl_version**
    
    in vec2 UV;
    in vec4 Col;
    out vec4 Color;
    
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;
    
    void main() {
        if (UV.t < 0.5) {
            Color = Col * texture(RightTex, UV.st * vec2(1.0, 2.0)); 
        }
        else {
            Color = Col * texture(LeftTex, UV.st * vec2(1.0, 2.0) - vec2(0.0, 1.0)); 
        }
    }
)";

constexpr const char* TBInvertedFrag = R"(
    **glsl_version**
    
    in vec2 UV;
    in vec4 Col;
    out vec4 Color;
    
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;
    
    void main() {
        if (UV.t < 0.5) {
            Color = Col * texture(LeftTex, UV.st * vec2(1.0, 2.0)); 
        }
        else {
            Color = Col * texture(RightTex, UV.st * vec2(1.0, 2.0) - vec2(0.0, 1.0)); 
        }
    }
)";

constexpr const char* DummyStereoFrag = R"(
    **glsl_version**
    
    in vec2 UV;
    in vec4 Col;
    out vec4 Color;
    
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;
    
    void main() {
        Color = Col * (0.5 * texture(LeftTex, UV) + 0.5 * texture(RightTex, UV));
    }
)";

constexpr const char* FXAAVert = R"(
    **glsl_version**
    
    layout (location = 0) in vec2 TexCoords;
    layout (location = 1) in vec3 Position;
    
    uniform float rt_w;
    uniform float rt_h;
    uniform float FXAA_SUBPIX_OFFSET; 
    
    out vec2 UVCoord;
    out vec2 texcoordOffset[4];
    
    void main(void) {
        gl_Position = vec4(Position, 1.0);
        UVCoord = TexCoords;
        
        texcoordOffset[0] = UVCoord + FXAA_SUBPIX_OFFSET * vec2(-1.0/rt_w,  -1.0/rt_h);
        texcoordOffset[1] = UVCoord + FXAA_SUBPIX_OFFSET * vec2( 1.0/rt_w,  -1.0/rt_h);
        texcoordOffset[2] = UVCoord + FXAA_SUBPIX_OFFSET * vec2(-1.0/rt_w,  1.0/rt_h);
        texcoordOffset[3] = UVCoord + FXAA_SUBPIX_OFFSET * vec2( 1.0/rt_w,  1.0/rt_h);
    }
)";

constexpr const char* FXAAFrag = R"(
    **glsl_version**
    /* 
    FXAA_EDGE_THRESHOLD 
    The minimum amount of local contrast required to apply algorithm. 
    1/3 - too little 
    1/4 - low quality 
    1/8 - high quality 
    1/16 - overkill 
    
    FXAA_EDGE_THRESHOLD_MIN 
    Trims the algorithm from processing darks. 
    1/32 - visible limit 
    1/16 - high quality 
    1/12 - upper limit (start of visible unfiltered edges) 
    */ 
    #define FXAA_EDGE_THRESHOLD_MIN 1.0/16.0 
    #define FXAA_EDGE_THRESHOLD 1.0/8.0 
    #define FXAA_SPAN_MAX 8.0 
    uniform float rt_w;
    uniform float rt_h;
    uniform sampler2D tex;
    /* 
        FXAA_SUBPIX_TRIM 
        Controls removal of sub-pixel aliasing. 
        1/2 - low removal 
        1/3 - medium removal 
        1/4 - default removal 
        1/8 - high removal 
        0 - complete removal 
    */ 
    uniform float FXAA_SUBPIX_TRIM; //1.0/8.0;
    
    in vec2 texcoordOffset[4];
    in vec2 UVCoord;
    out vec4 Color;
    
    vec3 antialias()  { 
        float FXAA_REDUCE_MIN = 1.0/128.0; 
        vec3 rgbNW = textureLod(tex, texcoordOffset[0], 0.0).xyz; 
        vec3 rgbNE = textureLod(tex, texcoordOffset[1], 0.0).xyz; 
        vec3 rgbSW = textureLod(tex, texcoordOffset[2], 0.0).xyz; 
        vec3 rgbSE = textureLod(tex, texcoordOffset[3], 0.0).xyz; 
        vec3 rgbM  = textureLod(tex, UVCoord, 0.0).xyz;
        
        vec3 luma = vec3(0.299, 0.587, 0.114);
        float lumaNW = dot(rgbNW, luma);
        float lumaNE = dot(rgbNE, luma);
        float lumaSW = dot(rgbSW, luma);
        float lumaSE = dot(rgbSE, luma);
        float lumaM  = dot( rgbM, luma);
        
        float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
        float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
        float range = lumaMax - lumaMin;
        //local contrast check, for not processing homogenius areas 
        if (range < max(FXAA_EDGE_THRESHOLD_MIN, lumaMax * FXAA_EDGE_THRESHOLD)) { 
            return rgbM; 
        } 
        
        vec2 dir = vec2(
            -((lumaNW + lumaNE) - (lumaSW + lumaSE)),
             ((lumaNW + lumaSW) - (lumaNE + lumaSE))
        );
        
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
                    textureLod(tex, UVCoord + dir * (1.0/3.0 - 0.5), 0.0).xyz +
                    textureLod(tex, UVCoord + dir * (2.0/3.0 - 0.5), 0.0).xyz);
        vec3 rgbB = rgbA * 0.5 + (1.0/4.0) * (
                    textureLod(tex, UVCoord + dir * (0.0/3.0 - 0.5), 0.0).xyz +
                    textureLod(tex, UVCoord + dir * (3.0/3.0 - 0.5), 0.0).xyz);
        float lumaB = dot(rgbB, luma);
        
        if ((lumaB < lumaMin) || (lumaB > lumaMax))  { 
            return rgbA; 
        } 
        else {
            return rgbB; 
        } 
    }
    
    void main() { 
        Color = vec4(antialias(), 1.0); 
    }
)";

} // namespace sgct::core::shaders

#endif // __SGCT__INTERNAL_SHADERS__H__
