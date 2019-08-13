/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__INTERNAL_FISHEYE_SHADERS_MODERN__H__
#define __SGCT__INTERNAL_FISHEYE_SHADERS_MODERN__H__

namespace sgct_core::shaders_modern_fisheye {
/*
    Contains GLSL 3.3+ shaders
*/

constexpr const char* sample_fun = R"(
    vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg) {
        float s = 2.0 * (texel.s - 0.5);
        float t = 2.0 * (texel.t - 0.5);
        float r2 = s*s + t*t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s,t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            return texture(map, rotVec);
        }
        else {
            return bg;
        }
    }
)";

constexpr const char* sample_latlon_fun = R"(
    vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg) {
        float phi = 3.14159265359 * (1.0 - texel.t);
        float theta = 6.28318530718 * (texel.s - 0.5);
        float x = sin(phi) * sin(theta);
        float y = sin(phi) * cos(theta);
        float z = cos(phi);
        **rotVec**;
        return texture(map, rotVec);
    }
)";

constexpr const char* sample_offset_fun = R"(
    vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg) {
        float s = 2.0 * (texel.s - 0.5);
        float t = 2.0 * (texel.t - 0.5);
        float r2 = s * s + t * t;
        if( r2 <= 1.0 ) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta) - offset.x;
            float y = -sin(phi) * cos(theta) - offset.y;
            float z = cos(phi) - offset.z;
            **rotVec**;
            return texture(map, rotVec);
        }
        else {
            return bg;
        }
    }
)";
   
constexpr const char* Base_Vert_Shader = R"(
    **glsl_version**
    
    layout (location = 0) in vec2 TexCoords;
    layout (location = 1) in vec3 Position;
    
    out vec2 UV;
    
    void main() {
        gl_Position = vec4(Position, 1.0);
        UV = TexCoords;
    }
)";

constexpr const char* Fisheye_Vert_Shader = R"(
    **glsl_version**
    
    layout (location = 0) in vec2 TexCoords;
    layout (location = 1) in vec3 Position;
    
    out vec2 UV;
    
    void main() {
        gl_Position = vec4(Position, 1.0);
        UV = TexCoords;
    }
)";

constexpr const char* Fisheye_Frag_Shader = R"(
    **glsl_version**
    
    in vec2 UV;
    out vec4 diffuse;
    
    uniform samplerCube cubemap;
    uniform float halfFov;
    float angle45Factor = 0.7071067812;
    
    **sample_fun**
    
    void main() {
        diffuse = getCubeSample(UV, cubemap, **bgColor**);
    }
)";

constexpr const char* Fisheye_Frag_Shader_Normal = R"(
    **glsl_version**
    
    in vec2 UV;
    layout(location = 0) out vec4 diffuse;
    layout(location = 1) out vec3 normal;
    
    uniform samplerCube cubemap;
    uniform samplerCube normalmap;
    uniform float halfFov;
    float angle45Factor = 0.7071067812;
    
    void main() {
        float s = 2.0 * (UV.s - 0.5);
        float t = 2.0 * (UV.t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            diffuse = texture(cubemap, rotVec);
            normal = texture(normalmap, rotVec).xyz;
        }
        else {
            diffuse = **bgColor**;
            normal = vec3(0.0, 0.0, 0.0);
        }
    }
)";

constexpr const char* Fisheye_Frag_Shader_Position = R"(
    **glsl_version**
    
    in vec2 UV;
    layout(location = 0) out vec4 diffuse;
    layout(location = 1) out vec3 position;
    
    uniform samplerCube cubemap;
    uniform samplerCube positionmap;
    uniform float halfFov;
    float angle45Factor = 0.7071067812;
    
    void main() {
        float s = 2.0 * (UV.s - 0.5);
        float t = 2.0 * (UV.t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            diffuse = texture(cubemap, rotVec);
            position = texture(positionmap, rotVec).xyz;
        }
        else {
            diffuse = **bgColor**;
            position = vec3(0.0, 0.0, 0.0);
        }
    }
)";

constexpr const char* Fisheye_Frag_Shader_Normal_Position = R"(
    **glsl_version**
    
    in vec2 UV;
    layout(location = 0) out vec4 diffuse;
    layout(location = 1) out vec3 normal;
    layout(location = 2) out vec3 position;
    
    uniform samplerCube cubemap;
    uniform samplerCube normalmap;
    uniform samplerCube positionmap;
    uniform float halfFov;
    float angle45Factor = 0.7071067812;
    
    void main() {
        float s = 2.0 * (UV.s - 0.5);
        float t = 2.0 * (UV.t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            diffuse = texture(cubemap, rotVec);
            normal = texture(normalmap, rotVec).xyz;
            position = texture(positionmap, rotVec).xyz;
        }
        else {
            diffuse = **bgColor**;
            normal = vec3(0.0, 0.0, 0.0);
            position = vec3(0.0, 0.0, 0.0);
        }
    }
)";

constexpr const char* Fisheye_Frag_Shader_Depth = R"(
    **glsl_version**
    
    in vec2 UV;
    out vec4 diffuse;
    
    uniform samplerCube cubemap;
    uniform samplerCube depthmap;
    uniform float halfFov;
    float angle45Factor = 0.7071067812;
    
    void main() {
        float s = 2.0 * (UV.s - 0.5);
        float t = 2.0 * (UV.t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            diffuse = texture(cubemap, rotVec);
            gl_FragDepth = texture(depthmap, rotVec).x;
        }
        else {
            diffuse = **bgColor**;
            gl_FragDepth = 1.0;
        }
    }
)";

constexpr const char* Fisheye_Frag_Shader_Depth_Normal = R"(
    **glsl_version**
    
    in vec2 UV;
    layout(location = 0) out vec4 diffuse;
    layout(location = 1) out vec3 normal;
    
    uniform samplerCube cubemap;
    uniform samplerCube normalmap;
    uniform samplerCube depthmap;
    uniform float halfFov;
    float angle45Factor = 0.7071067812;
    
    void main() {
        float s = 2.0 * (UV.s - 0.5);
        float t = 2.0 * (UV.t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            diffuse = texture(cubemap, rotVec);
            normal = texture(normalmap, rotVec).xyz;
            gl_FragDepth = texture(depthmap, rotVec).x;
        }
        else {
            diffuse = **bgColor**;
            normal = vec3(0.0, 0.0, 0.0);
            gl_FragDepth = 1.0;
        }
    }
)";

constexpr const char* Fisheye_Frag_Shader_Depth_Position = R"(
    **glsl_version**
    
    in vec2 UV;
    layout(location = 0) out vec4 diffuse;
    layout(location = 1) out vec3 position;
    
    uniform samplerCube cubemap;
    uniform samplerCube positionmap;
    uniform samplerCube depthmap;
    uniform float halfFov;
    float angle45Factor = 0.7071067812;
    
    void main() {
        float s = 2.0 * (UV.s - 0.5);
        float t = 2.0 * (UV.t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            diffuse = texture(cubemap, rotVec);
            position = texture(positionmap, rotVec).xyz;
            gl_FragDepth = texture(depthmap, rotVec).x;
        }
        else {
            diffuse = **bgColor**;
            position = vec3(0.0, 0.0, 0.0);
            gl_FragDepth = 1.0;
        }
    }
)";

constexpr const char* Fisheye_Frag_Shader_Depth_Normal_Position = R"(
    **glsl_version**
    
    in vec2 UV;
    layout(location = 0) out vec4 diffuse;
    layout(location = 1) out vec3 normal;
    layout(location = 2) out vec3 position;
    
    uniform samplerCube cubemap;
    uniform samplerCube normalmap;
    uniform samplerCube positionmap;
    uniform samplerCube depthmap;
    uniform float halfFov;
    float angle45Factor = 0.7071067812;
    
    void main() {
        float s = 2.0 * (UV.s - 0.5);
        float t = 2.0 * (UV.t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            diffuse = texture(cubemap, rotVec);
            normal = texture(normalmap, rotVec).xyz;
            position = texture(positionmap, rotVec).xyz;
            gl_FragDepth = texture(depthmap, rotVec).x;
        }
        else {
            diffuse = **bgColor**;
            normal = vec3(0.0, 0.0, 0.0);
            position = vec3(0.0, 0.0, 0.0);
            gl_FragDepth = 1.0;
        }
    }
)";

constexpr const char* Fisheye_Frag_Shader_OffAxis = R"(
    **glsl_version**
    
    in vec2 UV;
    out vec4 diffuse;
    
    uniform samplerCube cubemap;
    uniform float halfFov;
    uniform vec3 offset;
    float angle45Factor = 0.7071067812;
    
    **sample_fun**
    
    void main() {
        diffuse = getCubeSample(UV, cubemap, **bgColor**);
    }
)";

constexpr const char* Fisheye_Frag_Shader_OffAxis_Normal = R"(
    **glsl_version**
    
    in vec2 UV;
    layout(location = 0) out vec4 diffuse;
    layout(location = 1) out vec3 normal;
    
    uniform samplerCube cubemap;
    uniform samplerCube normalmap;
    uniform float halfFov;
    uniform vec3 offset;
    float angle45Factor = 0.7071067812;
    
    void main() {
        float s = 2.0 * (UV.s - 0.5);
        float t = 2.0 * (UV.t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta) - offset.x;
            float y = -sin(phi) * cos(theta) - offset.y;
            float z = cos(phi) - offset.z;
            **rotVec**;
            diffuse = texture(cubemap, rotVec);
            normal = texture(normalmap, rotVec).xyz;
        }
        else {
            diffuse = **bgColor**;
            normal = vec3(0.0, 0.0, 0.0);
        }
    }
)";

constexpr const char* Fisheye_Frag_Shader_OffAxis_Position = R"(
    **glsl_version**
    
    in vec2 UV;
    layout(location = 0) out vec4 diffuse;
    layout(location = 1) out vec3 position;
    
    uniform samplerCube cubemap;
    uniform samplerCube positionmap;
    uniform float halfFov;
    uniform vec3 offset;
    float angle45Factor = 0.7071067812;
    
    void main() {
        float s = 2.0 * (UV.s - 0.5);
        float t = 2.0 * (UV.t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta) - offset.x;
            float y = -sin(phi) * cos(theta) - offset.y;
            float z = cos(phi) - offset.z;
            **rotVec**;
            diffuse = texture(cubemap, rotVec);
            position = texture(positionmap, rotVec).xyz;
        }
        else {
            diffuse = **bgColor**;
            position = vec3(0.0, 0.0, 0.0);
        }
    }
)";

constexpr const char* Fisheye_Frag_Shader_OffAxis_Normal_Position = R"(
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
    float angle45Factor = 0.7071067812;
    
    void main() {
        float s = 2.0 * (UV.s - 0.5);
        float t = 2.0 * (UV.t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta) - offset.x;
            float y = -sin(phi) * cos(theta) - offset.y;
            float z = cos(phi) - offset.z;
            **rotVec**;
            diffuse = texture(cubemap, rotVec);
            normal = texture(normalmap, rotVec).xyz;
            position = texture(positionmap, rotVec).xyz;
        }
        else {
            diffuse = **bgColor**;
            normal = vec3(0.0, 0.0, 0.0);
            position = vec3(0.0, 0.0, 0.0);
        }
    }
)";

constexpr const char* Fisheye_Frag_Shader_OffAxis_Depth = R"(
    **glsl_version**
    
    in vec2 UV;
    out vec4 diffuse;
    
    uniform samplerCube cubemap;
    uniform samplerCube depthmap;
    uniform float halfFov;
    uniform vec3 offset;
    float angle45Factor = 0.7071067812;
    
    void main() {
        float s = 2.0 * (UV.s - 0.5);
        float t = 2.0 * (UV.t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta) - offset.x;
            float y = -sin(phi) * cos(theta) - offset.y;
            float z = cos(phi) - offset.z;
            **rotVec**;
            diffuse = texture(cubemap, rotVec);
            gl_FragDepth = texture(depthmap, rotVec).x;
        }
        else {
            diffuse = **bgColor**;
            gl_FragDepth = 1.0;
        }
    }
)";

constexpr const char* Fisheye_Frag_Shader_OffAxis_Depth_Normal = R"(
    **glsl_version**
    
    in vec2 UV;
    layout(location = 0) out vec4 diffuse;
    layout(location = 1) out vec3 normal;
    
    uniform samplerCube cubemap;
    uniform samplerCube normalmap;
    uniform samplerCube depthmap;
    uniform float halfFov;
    uniform vec3 offset;
    float angle45Factor = 0.7071067812;
    
    void main()
    {
        float s = 2.0 * (UV.s - 0.5);
        float t = 2.0 * (UV.t - 0.5);
        float r2 = s*s + t*t;
        if( r2 <= 1.0 )
        {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta) - offset.x;
            float y = -sin(phi) * cos(theta) - offset.y;
            float z = cos(phi) - offset.z;
            **rotVec**;
            diffuse = texture(cubemap, rotVec);
            normal = texture(normalmap, rotVec).xyz;
            gl_FragDepth = texture(depthmap, rotVec).x;
        }
        else {
            diffuse = **bgColor**;
            normal = vec3(0.0, 0.0, 0.0);
            gl_FragDepth = 1.0;
        }
    }
)";

constexpr const char* Fisheye_Frag_Shader_OffAxis_Depth_Position = R"(
    **glsl_version**
    
    in vec2 UV;
    layout(location = 0) out vec4 diffuse;
    layout(location = 1) out vec3 position;
    
    uniform samplerCube cubemap;
    uniform samplerCube positionmap;
    uniform samplerCube depthmap;
    uniform float halfFov;
    uniform vec3 offset;
    float angle45Factor = 0.7071067812;
    
    void main() {
        float s = 2.0 * (UV.s - 0.5);
        float t = 2.0 * (UV.t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta) - offset.x;
            float y = -sin(phi) * cos(theta) - offset.y;
            float z = cos(phi) - offset.z;
            **rotVec**;
            diffuse = texture(cubemap, rotVec);
            position = texture(positionmap, rotVec).xyz;
            gl_FragDepth = texture(depthmap, rotVec).x;
        }
        else {
            diffuse = **bgColor**;
            position = vec3(0.0, 0.0, 0.0);
            gl_FragDepth = 1.0;
        }
    }
)";

constexpr const char* Fisheye_Frag_Shader_OffAxis_Depth_Normal_Position = R"(
    **glsl_version**
    
    in vec2 UV;
    layout(location = 0) out vec4 diffuse;
    layout(location = 1) out vec3 normal;
    layout(location = 2) out vec3 position;
    
    uniform samplerCube cubemap;
    uniform samplerCube normalmap;
    uniform samplerCube positionmap;
    uniform samplerCube depthmap;
    uniform float halfFov;
    uniform vec3 offset;
    float angle45Factor = 0.7071067812;
    
    void main() {
        float s = 2.0 * (UV.s - 0.5);
        float t = 2.0 * (UV.t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta) - offset.x;
            float y = -sin(phi) * cos(theta) - offset.y;
            float z = cos(phi) - offset.z;
            **rotVec**;
            diffuse = texture(cubemap, rotVec);
            normal = texture(normalmap, rotVec).xyz;
            position = texture(positionmap, rotVec).xyz;
            gl_FragDepth = texture(depthmap, rotVec).x;
        }
        else {
            diffuse = **bgColor**;
            normal = vec3(0.0, 0.0, 0.0);
            position = vec3(0.0, 0.0, 0.0);
            gl_FragDepth = 1.0;
        }
    }
)";

constexpr const char* Fisheye_Depth_Correction_Frag_Shader = R"(
    **glsl_version**
    
    in vec2 UV;
    out vec4 Color;
    
    uniform sampler2D cTex;
    uniform sampler2D dTex;
    uniform float near;
    uniform float far;
    
    void main() {
        float a = far / (far - near);
        float b = far * near / (near - far);
        float z = b / (texture(dTex, UV).x - a);
        
        // get angle from -45 to 45 degrees (-pi/4 to +pi/4) 
        float xAngle = 1.57079632679 * (UV.s - 0.5);
        float yAngle = 1.57079632679 * (UV.t - 0.5);
        
        float x_norm = tan(xAngle); 
        float y_norm = tan(yAngle); 
        float r = z * sqrt(x_norm * x_norm + y_norm * y_norm + 1.0); 
        
        Color = texture(cTex, UV);
        gl_FragDepth = a + b / r;
        //No correction
        //gl_FragDepth = texture(dTex, UV).x; 
    }
)";

constexpr const char* Fisheye_Frag_Shader_Cubic_Test = R"(
    **glsl_version**
    
    in vec2 UV;
    out vec4 diffuse;
    
    uniform samplerCube cubemap;
    uniform float halfFov;
    uniform vec4 **bgColor**;
    float size = 4096.0;
    float angle45Factor = 0.7071067812;
    
    vec4 getCubeSample(vec2 texel) {
        float s = 2.0 * (texel.s - 0.5);
        float t = 2.0 * (texel.t - 0.5);
        float r2 = s * s + t * t;
        if( r2 <= 1.0 ) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            return texture(cubemap, rotVec);
        }
        else {
            return **bgColor**;
        }
    }
    
    vec4 cubic(float x) {
        float x2 = x * x;
        float x3 = x2 * x;
        vec4 w = vec4(
              -x3 + 3*x2 - 3*x + 1,
             3*x3 - 6*x2      + 4,
            -3*x3 + 3*x2 + 3*x + 1,
               x3
        );
        return w / 6.0;
    }
    
    vec4 catmull_rom(float x) {
        float x2 = x * x;
        float x3 = x2 * x;
        vec4 w = vec4(
            -x + 2*x2 - x3,
             2 - 5*x2 + 3*x3,
             x + 4*x2 - 3*x3,
                  -x2 + x3
        );
        return w / 2.0;
    }
    
    vec4 filter4(vec2 texcoord) {
        float fx = fract(texcoord.x);
        float fy = fract(texcoord.y);
        texcoord.x -= fx;
        texcoord.y -= fy;
        
        vec4 xcubic = catmull_rom(fx);
        vec4 ycubic = catmull_rom(fy);
        
        vec4 c = vec4(texcoord.x - 0.75, texcoord.x + 0.75, texcoord.y - 0.75, texcoord.y + 0.75);
        vec4 s = vec4(xcubic.x + xcubic.y, xcubic.z + xcubic.w, ycubic.x + ycubic.y, ycubic.z + ycubic.w);
        vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;
        
        vec4 sample0 = getCubeSample(vec2(offset.x, offset.z) / vec2(size, size));
        vec4 sample1 = getCubeSample(vec2(offset.y, offset.z) / vec2(size, size));
        vec4 sample2 = getCubeSample(vec2(offset.x, offset.w) / vec2(size, size));
        vec4 sample3 = getCubeSample(vec2(offset.y, offset.w) / vec2(size, size));
        
        float sx = s.x / (s.x + s.y);
        float sy = s.z / (s.z + s.w);
        
        return mix(mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
    }
    vec4 filter16(vec2 texcoord) {
        vec2 xy = floor(texcoord);
        vec2 normxy = texcoord - xy;
        
        vec2 st0 = ((2.0 - normxy) * normxy - 1.0) * normxy;
        vec2 st1 = (3.0 * normxy - 5.0) * normxy * normxy + 2.0;
        vec2 st2 = ((4.0 - 3.0 * normxy) * normxy + 1.0) * normxy;
        vec2 st3 = (normxy - 1.0) * normxy * normxy;
        
        vec4 row0 =
            st0.s * getCubeSample((xy + vec2(-1.0, -1.0)) / vec2(size, size)) +
            st1.s * getCubeSample((xy + vec2(0.0, -1.0)) / vec2(size, size)) +
            st2.s * getCubeSample((xy + vec2(1.0, -1.0)) / vec2(size, size)) +
            st3.s * getCubeSample((xy + vec2(2.0, -1.0)) / vec2(size, size));
        
        vec4 row1 =
            st0.s * getCubeSample((xy + vec2(-1.0, 0.0)) / vec2(size, size)) +
            st1.s * getCubeSample((xy + vec2(0.0, 0.0)) / vec2(size, size)) +
            st2.s * getCubeSample((xy + vec2(1.0, 0.0)) / vec2(size, size)) +
            st3.s * getCubeSample((xy + vec2(2.0, 0.0)) / vec2(size, size));
        
        vec4 row2 =
            st0.s * getCubeSample((xy + vec2(-1.0, 1.0)) / vec2(size, size)) +
            st1.s * getCubeSample((xy + vec2(0.0, 1.0)) / vec2(size, size)) +
            st2.s * getCubeSample((xy + vec2(1.0, 1.0)) / vec2(size, size)) +
            st3.s * getCubeSample((xy + vec2(2.0, 1.0)) / vec2(size, size));
        
        vec4 row3 =
            st0.s * getCubeSample((xy + vec2(-1.0, 2.0)) / vec2(size, size)) +
            st1.s * getCubeSample((xy + vec2(0.0, 2.0)) / vec2(size, size)) +
            st2.s * getCubeSample((xy + vec2(1.0, 2.0)) / vec2(size, size)) +
            st3.s * getCubeSample((xy + vec2(2.0, 2.0)) / vec2(size, size));
        
        return 0.25 * ((st0.t * row0) + (st1.t * row1) + (st2.t * row2) + (st3.t * row3));
    }
    
    void main() {
        diffuse = filter4(UV * vec2(size, size));
        //diffuse = getCubeSample(UV);
    }\n";
)";

} // sgct_core::shaders_modern_fisheye

#endif // __SGCT__INTERNAL_FISHEYE_SHADERS_MODERN__H__
