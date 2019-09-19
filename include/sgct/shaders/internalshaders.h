/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__INTERNAL_SHADERS__H__
#define __SGCT__INTERNAL_SHADERS__H__

namespace sgct::core::shaders {
/*
    All shaders are in GLSL 1.2 for compability with Mac OS X
*/

constexpr const char* BaseVert = R"(
    **glsl_version**
    
    void main() {
        gl_TexCoord[0] = gl_MultiTexCoord0;
        gl_Position = gl_Vertex;
        gl_FrontColor = gl_Color;
    }
)";

constexpr const char* BaseFrag = R"(
    **glsl_version**
    
    uniform sampler2D Tex;
    
    void main() {
        gl_FragColor = gl_Color * texture2D(Tex, gl_TexCoord[0].st);
    }
)";
   
constexpr const char* OverlayVert = R"(
    **glsl_version**
    
    void main() {
        gl_TexCoord[0] = gl_MultiTexCoord0;
        gl_Position = gl_Vertex;
        gl_FrontColor = gl_Color;
    }
)";
   
constexpr const char* OverlayFrag = R"(
    **glsl_version**
    
    uniform sampler2D Tex;
    
    void main() {
        gl_FragColor = texture2D(Tex, gl_TexCoord[0].st);
    }
)";
   
constexpr const char* AnaglyphVert = R"(
    **glsl_version**
    
    void main() {
        gl_TexCoord[0] = gl_MultiTexCoord0;
        gl_Position = gl_Vertex;
        gl_FrontColor = gl_Color;
    }
)";

constexpr const char* AnaglyphRedCyanFrag = R"(
    **glsl_version**
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;

    void main() {
        vec4 leftVals = texture2D(LeftTex, gl_TexCoord[0].st);
        float leftLum = 0.3 * leftVals.r + 0.59 * leftVals.g + 0.11 * leftVals.b;
        vec4 rightVals = texture2D(RightTex, gl_TexCoord[0].st);
        float rightLum = 0.3 * rightVals.r + 0.59 * rightVals.g + 0.11 * rightVals.b;

        gl_FragColor.r = gl_Color.r * leftLum;
        gl_FragColor.g = gl_Color.g * rightLum;
        gl_FragColor.b = gl_Color.b * rightLum;
        gl_FragColor.a = gl_Color.a * max(leftVals.a, rightVals.a);
    }
)";

constexpr const char* AnaglyphRedCyanWimmerFrag = R"(
    **glsl_version**
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;
    
    void main() {
        vec4 leftVals = texture2D(LeftTex, gl_TexCoord[0].st);
        vec4 rightVals = texture2D(RightTex, gl_TexCoord[0].st);
        gl_FragColor.r = gl_Color.r * (0.7 * leftVals.g + 0.3 * leftVals.b);
        gl_FragColor.g = gl_Color.g * rightVals.r;
        gl_FragColor.b = gl_Color.b * rightVals.b;
        gl_FragColor.a = gl_Color.a * max(leftVals.a, rightVals.a);
    }
)";

constexpr const char* AnaglyphAmberBlueFrag = R"(
    **glsl_version**
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;

    void main() {
        vec4 leftVals = texture2D(LeftTex, gl_TexCoord[0].st);
        vec4 rightVals = texture2D(RightTex, gl_TexCoord[0].st);
        vec3 coef = vec3(0.15, 0.15, 0.70);
        float rightMix = dot(rightVals.rbg, coef);
        gl_FragColor.r = gl_Color.r * leftVals.r;
        gl_FragColor.g = gl_Color.g * leftVals.g;
        gl_FragColor.b = gl_Color.b * rightMix;
        gl_FragColor.a = gl_Color.a * max(leftVals.a, rightVals.a);
    }
)";

constexpr const char* CheckerBoardFrag = R"(
    **glsl_version**
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;

    void main() {
        float fval = (gl_FragCoord.x + gl_FragCoord.y) * 0.5;
        if ((fval - floor(fval)) == 0.0) {
            gl_FragColor = gl_Color * texture2D(RightTex, gl_TexCoord[0].st);
        }
        else {
            gl_FragColor = gl_Color * texture2D( LeftTex, gl_TexCoord[0].st);
        }
    }
)";

constexpr const char* CheckerBoardInvertedFrag = R"(
    **glsl_version**
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;
    
    void main() {
        float fval = (gl_FragCoord.x + gl_FragCoord.y) * 0.5;
        if ((fval - floor(fval)) == 0.0) {
            gl_FragColor = gl_Color * texture2D(LeftTex, gl_TexCoord[0].st);
        }
        else {
            gl_FragColor = gl_Color * texture2D(RightTex, gl_TexCoord[0].st);
        }
    }
)";

constexpr const char* VerticalInterlacedFrag = R"(
    **glsl_version**
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;

    void main() {
        float fval = gl_FragCoord.y * 0.5;
        if ((fval - floor(fval)) > 0.5) {
            gl_FragColor = gl_Color * texture2D(RightTex, gl_TexCoord[0].st);
        }
        else {
            gl_FragColor = gl_Color * texture2D(LeftTex, gl_TexCoord[0].st);
        }
    }
)";

constexpr const char* DummyStereoFrag = R"(
    **glsl_version**
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;

    void main() {
        gl_FragColor = gl_Color * (0.5 * texture2D(LeftTex, gl_TexCoord[0].st) +
                       0.5 * texture2D(RightTex, gl_TexCoord[0].st));
    }
)";

constexpr const char* VerticalInterlacedInvertedFrag = R"(
    **glsl_version**
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;

    void main() {
        float fval = gl_FragCoord.y * 0.5;
        if ((fval - floor(fval)) > 0.5) {
            gl_FragColor = gl_Color * texture2D(LeftTex, gl_TexCoord[0].st);
        }
        else {
            gl_FragColor = gl_Color * texture2D(RightTex, gl_TexCoord[0].st);
        }
    }
)";

constexpr const char* SBSFrag = R"(
    **glsl_version**
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;

    void main() {
        vec2 mul = vec2(2.0, 1.0); 
        if (gl_TexCoord[0].s < 0.5) {
            gl_FragColor = gl_Color * texture2D(LeftTex, gl_TexCoord[0].st * mul); 
        }
        else {
            gl_FragColor = gl_Color *
                           texture2D(RightTex, gl_TexCoord[0].st * mul - vec2(1.0, 0.0)); 
        }
    }
)";

constexpr const char* SBSInvertedFrag = R"(
    **glsl_version**
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;

    void main() {
        vec2 mul = vec2(2.0, 1.0); 
        if (gl_TexCoord[0].s < 0.5) { 
            gl_FragColor = gl_Color * texture2D(RightTex, gl_TexCoord[0].st * mul); 
        }
        else {
            gl_FragColor = gl_Color *
                           texture2D(LeftTex, gl_TexCoord[0].st * mul - vec2(1.0, 0.0)); 
        }
    }
)";

constexpr const char* TBFrag = R"(
    **glsl_version**
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;

    void main() {
        vec2 mul = vec2(1.0, 2.0); 
        if (gl_TexCoord[0].t < 0.5) {
            gl_FragColor = gl_Color * texture2D(RightTex, gl_TexCoord[0].st * mul); 
        }
        else {
            gl_FragColor = gl_Color *
                           texture2D( LeftTex, gl_TexCoord[0].st * mul - vec2(0.0, 1.0));
        }
    }
)";

constexpr const char* TBInvertedFrag = R"(
    **glsl_version**
    uniform sampler2D LeftTex;
    uniform sampler2D RightTex;

    void main() {
        vec2 mul = vec2(1.0, 2.0); 
        if (gl_TexCoord[0].t < 0.5) { 
            gl_FragColor = gl_Color * texture2D(LeftTex, gl_TexCoord[0].st * mul); 
        }
        else {
            gl_FragColor = gl_Color *
                           texture2D(RightTex, gl_TexCoord[0].st * mul - vec2(0.0, 1.0)); 
        }
    }
)";

constexpr const char* FXAAVert = R"(
    **glsl_version**
    uniform float rt_w;
    uniform float rt_h;
    uniform float FXAA_SUBPIX_OFFSET; 
    
    varying vec2 texcoordOffset[4];
    
    void main() {
        gl_Position = gl_Vertex;
        gl_TexCoord[0] = gl_MultiTexCoord0;
        
        texcoordOffset[0] = gl_TexCoord[0].st +
                            FXAA_SUBPIX_OFFSET * vec2(-1.0 / rt_w,  -1.0 / rt_h);
        texcoordOffset[1] = gl_TexCoord[0].st +
                            FXAA_SUBPIX_OFFSET * vec2( 1.0 / rt_w,  -1.0 / rt_h);
        texcoordOffset[2] = gl_TexCoord[0].st +
                            FXAA_SUBPIX_OFFSET * vec2(-1.0 / rt_w,   1.0 / rt_h);
        texcoordOffset[3] = gl_TexCoord[0].st +
                            FXAA_SUBPIX_OFFSET * vec2( 1.0 / rt_w,   1.0 / rt_h);
    }
)";

constexpr const char* FXAAFrag = R"(
    **glsl_version**
    #extension GL_EXT_gpu_shader4 : enable // For NVIDIA cards.
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
    
    varying vec2 texcoordOffset[4];
    
    vec3 antialias()  { 
        float FXAA_REDUCE_MIN = 1.0 / 128.0; 
        vec3 rgbNW = texture2DLod(tex, texcoordOffset[0], 0.0).xyz; 
        vec3 rgbNE = texture2DLod(tex, texcoordOffset[1], 0.0).xyz; 
        vec3 rgbSW = texture2DLod(tex, texcoordOffset[2], 0.0).xyz; 
        vec3 rgbSE = texture2DLod(tex, texcoordOffset[3], 0.0).xyz; 
        vec3 rgbM  = texture2DLod(tex, gl_TexCoord[0].st, 0.0).xyz;
        
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
            
        float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
        
        dir = min(
            vec2(FXAA_SPAN_MAX,  FXAA_SPAN_MAX), 
            max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) / vec2(rt_w, rt_h
        );
            
        vec3 rgbA = 0.5 * (
                    texture2DLod(tex, gl_TexCoord[0].st + dir * (1.0/3.0 - 0.5), 0.0).xyz +
                    texture2DLod(tex, gl_TexCoord[0].st + dir * (2.0/3.0 - 0.5), 0.0).xyz);
        vec3 rgbB = rgbA * 0.5 + (1.0 / 4.0) * (
                    texture2DLod(tex, gl_TexCoord[0].st + dir * (0.0/3.0 - 0.5), 0.0).xyz +
                    texture2DLod(tex, gl_TexCoord[0].st + dir * (3.0/3.0 - 0.5), 0.0).xyz);
        float lumaB = dot(rgbB, luma);
        
        if((lumaB < lumaMin) || (lumaB > lumaMax))  { 
            return rgbA; 
            //return vec3(1.0, 0.0, 0.0); 
        } 
        else {
            return rgbB; 
            //return vec3(0.0, 1.0, 0.0); 
        } 
    }
    
    void main(void) {
        gl_FragColor = vec4(antialias(), 1.0); 
    }
)";

} // namespace sgct::core::shaders

#endif // __SGCT__INTERNAL_SHADERS__H__
