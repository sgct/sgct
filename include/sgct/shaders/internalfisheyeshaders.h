/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__INTERNAL_FISHEYE_SHADERS__H__
#define __SGCT__INTERNAL_FISHEYE_SHADERS__H__

namespace sgct::core::shaders_fisheye {
    /*
        All shaders are in GLSL 1.2 for compability with Mac OS X
    */
constexpr const char* SampleFun = R"(
    vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg) {
        float s = 2.0 * (texel.s - 0.5);
        float t = 2.0 * (texel.t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s,t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            return textureCube(map, rotVec);
        }
        else {
            return bg;
        }
    }
)";

constexpr const char* SampleLatlonFun = R"(
    vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg) {
        float phi = 3.14159265359 * (1.0 - texel.t);
        float theta = 6.28318530718 * (texel.s - 0.5);
        float x = sin(phi) * sin(theta);
        float y = sin(phi) * cos(theta);
        float z = cos(phi);
        **rotVec**;
        return textureCube(map, rotVec);
    }
)";

constexpr const char* SampleOffsetFun = R"(
    vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg) {
        float s = 2.0 * (texel.s - 0.5);
        float t = 2.0 * (texel.t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s,t);
            float x = sin(phi) * sin(theta) - offset.x;
            float y = -sin(phi) * cos(theta) - offset.y;
            float z = cos(phi) - offset.z;
            **rotVec**;
            return textureCube(map, rotVec);
        }
        else {
            return bg;
        }
)";
        
constexpr const char* BaseVert = R"(
    **glsl_version**

    void main() {
        gl_TexCoord[0] = gl_MultiTexCoord0;
        gl_Position = gl_Vertex;
    }
)";
        
constexpr const char* FisheyeVert = R"(
    **glsl_version**

    void main() {
        gl_TexCoord[0] = gl_MultiTexCoord0;
        gl_Position = gl_Vertex;
    }
)";

constexpr const char* FisheyeFrag = R"(
    **glsl_version**
    //#pragma optionNV(fastmath off) // For NVIDIA cards.
    //#pragma optionNV(fastprecision off) // For NVIDIA cards.

    uniform samplerCube cubemap;
    uniform float halfFov;
    float angle45Factor = 0.7071067812;

    **sample_fun**

    void main() {
        gl_FragColor = getCubeSample(gl_TexCoord[0].st, cubemap, **bgColor**);
    }
)";

constexpr const char* FisheyeFragNormal = R"(
    **glsl_version**

    uniform samplerCube cubemap;
    uniform samplerCube normalmap;
    uniform float halfFov;
    float angle45Factor = 0.7071067812;

    void main() {
        float s = 2.0 * (gl_TexCoord[0].s - 0.5);
        float t = 2.0 * (gl_TexCoord[0].t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s,t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            gl_FragData[0] = textureCube(cubemap, rotVec);
            gl_FragData[1] = textureCube(normalmap, rotVec);
        }
        else {
            gl_FragData[0] = **bgColor**;
            gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);
        }
    }
)";

constexpr const char* FisheyeFragPosition = R"(
    **glsl_version**

    uniform samplerCube cubemap;
    uniform samplerCube positionmap;
    uniform float halfFov;
    float angle45Factor = 0.7071067812;

    void main() {
        float s = 2.0 * (gl_TexCoord[0].s - 0.5);
        float t = 2.0 * (gl_TexCoord[0].t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s,t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            gl_FragData[0] = textureCube(cubemap, rotVec);
            gl_FragData[1] = textureCube(positionmap, rotVec);
        }
        else {
            gl_FragData[0] = **bgColor**;
            gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);
        }
    }
)";

constexpr const char* FisheyeFragNormalPosition = R"(
    **glsl_version**

    uniform samplerCube cubemap;
    uniform samplerCube normalmap;
    uniform samplerCube positionmap;
    uniform float halfFov;
    float angle45Factor = 0.7071067812;

    void main() {
        float s = 2.0 * (gl_TexCoord[0].s - 0.5);
        float t = 2.0 * (gl_TexCoord[0].t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s,t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            gl_FragData[0] = textureCube(cubemap, rotVec);
            gl_FragData[1] = textureCube(normalmap, rotVec);
            gl_FragData[2] = textureCube(positionmap, rotVec);
        }
        else {
            gl_FragData[0] = **bgColor**;
            gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);
            gl_FragData[2] = vec4(0.0, 0.0, 0.0, 0.0);
        }
    }
)";

constexpr const char* FisheyeFragDepth = R"(
    **glsl_version**

    uniform samplerCube cubemap;
    uniform samplerCube depthmap;
    uniform float halfFov;
    float angle45Factor = 0.7071067812;

    void main() {
        float s = 2.0 * (gl_TexCoord[0].s - 0.5);
        float t = 2.0 * (gl_TexCoord[0].t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s,t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            gl_FragColor = textureCube(cubemap, rotVec);
            gl_FragDepth = textureCube(depthmap, rotVec).x;
        }
        else {
            gl_FragColor = **bgColor**;
            gl_FragDepth = 1.0;
        }
    }
)";

constexpr const char* FisheyeFragDepthNormal = R"(
    **glsl_version**

    uniform samplerCube cubemap;
    uniform samplerCube depthmap;
    uniform samplerCube normalmap;
    uniform float halfFov;
    float angle45Factor = 0.7071067812;

    void main() {
        float s = 2.0 * (gl_TexCoord[0].s - 0.5);
        float t = 2.0 * (gl_TexCoord[0].t - 0.5);
        float r2 = s*s + t*t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s,t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            gl_FragData[0] = textureCube(cubemap, rotVec);
            gl_FragData[1] = textureCube(normalmap, rotVec);
            gl_FragDepth = textureCube(depthmap, rotVec).x;
        }
        else {
            gl_FragData[0] = **bgColor**;
            gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);
            gl_FragDepth = 1.0;
        }
    }
)";

constexpr const char* FisheyeFragDepthPosition = R"(
    **glsl_version**

    uniform samplerCube cubemap;
    uniform samplerCube depthmap;
    uniform samplerCube positionmap;
    uniform float halfFov;
    float angle45Factor = 0.7071067812;

    void main() {
        float s = 2.0 * (gl_TexCoord[0].s - 0.5);
        float t = 2.0 * (gl_TexCoord[0].t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s,t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            gl_FragData[0] = textureCube(cubemap, rotVec);
            gl_FragData[1] = textureCube(positionmap, rotVec);
            gl_FragDepth = textureCube(depthmap, rotVec).x;
        }
        else {
            gl_FragData[0] = **bgColor**;
            gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);
            gl_FragDepth = 1.0;
        }
    }
)";

constexpr const char* FisheyeFragDepthNormalPosition = R"(
    **glsl_version**

    uniform samplerCube cubemap;
    uniform samplerCube depthmap;
    uniform samplerCube normalmap;
    uniform samplerCube positionmap;
    uniform float halfFov;
    float angle45Factor = 0.7071067812;

    void main() {
        float s = 2.0 * (gl_TexCoord[0].s - 0.5);
        float t = 2.0 * (gl_TexCoord[0].t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s,t);
            float x = sin(phi) * sin(theta);
            float y = -sin(phi) * cos(theta);
            float z = cos(phi);
            **rotVec**;
            gl_FragData[0] = textureCube(cubemap, rotVec);
            gl_FragData[1] = textureCube(normalmap, rotVec);
            gl_FragData[2] = textureCube(positionmap, rotVec);
            gl_FragDepth = textureCube(depthmap, rotVec).x;
        }
        else {
            gl_FragData[0] = **bgColor**;
            gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);
            gl_FragData[2] = vec4(0.0, 0.0, 0.0, 0.0);
            gl_FragDepth = 1.0;
        }
    }
)";

constexpr const char* FisheyeFragOffAxis = R"(
    **glsl_version**

    uniform samplerCube cubemap;
    uniform float halfFov;
    uniform vec3 offset;
    float angle45Factor = 0.7071067812;

    **sample_fun**

    void main() {
        gl_FragColor = getCubeSample(gl_TexCoord[0].st, cubemap, **bgColor**);
    }
)";

constexpr const char* FisheyeFragOffAxisNormal = R"(
    **glsl_version**

    uniform samplerCube cubemap;
    uniform samplerCube normalmap;
    uniform float halfFov;
    uniform vec3 offset;
    float angle45Factor = 0.7071067812;

    void main() {
        float s = 2.0 * (gl_TexCoord[0].s - 0.5);
        float t = 2.0 * (gl_TexCoord[0].t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta) - offset.x;
            float y = -sin(phi) * cos(theta) - offset.y;
            float z = cos(phi) - offset.z;
            **rotVec**;
            gl_FragData[0] = textureCube(cubemap, rotVec);
            gl_FragData[1] = textureCube(normalmap, rotVec);
        }
        else {
            gl_FragData[0] = **bgColor**;
            gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);
        }
    }
)";

constexpr const char* FisheyeFragOffAxisPosition = R"(
    **glsl_version**

    uniform samplerCube cubemap;
    uniform samplerCube positionmap;
    uniform float halfFov;
    uniform vec3 offset;
    float angle45Factor = 0.7071067812;

    void main() {
        float s = 2.0 * (gl_TexCoord[0].s - 0.5);
        float t = 2.0 * (gl_TexCoord[0].t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta) - offset.x;
            float y = -sin(phi) * cos(theta) - offset.y;
            float z = cos(phi) - offset.z;
            **rotVec**;
            gl_FragData[0] = textureCube(cubemap, rotVec);
            gl_FragData[1] = textureCube(positionmap, rotVec);
        }
        else {
            gl_FragData[0] = **bgColor**;
            gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);
        }
    }
)";

constexpr const char* FisheyeFragOffAxisNormalPosition = R"(
    **glsl_version**

    uniform samplerCube cubemap;
    uniform samplerCube normalmap;
    uniform samplerCube positionmap;
    uniform float halfFov;
    uniform vec3 offset;
    float angle45Factor = 0.7071067812;

    void main() {
        float s = 2.0 * (gl_TexCoord[0].s - 0.5);
        float t = 2.0 * (gl_TexCoord[0].t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta) - offset.x;
            float y = -sin(phi) * cos(theta) - offset.y;
            float z = cos(phi) - offset.z;
            **rotVec**;
            gl_FragData[0] = textureCube(cubemap, rotVec);
            gl_FragData[1] = textureCube(normalmap, rotVec);
            gl_FragData[2] = textureCube(positionmap, rotVec);
        }
        else {
            gl_FragData[0] = **bgColor**;
            gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);
            gl_FragData[2] = vec4(0.0, 0.0, 0.0, 0.0);
        }
    }
)";

constexpr const char* FisheyeFragOffAxisDepth = R"(
    **glsl_version**

    uniform samplerCube cubemap;
    uniform samplerCube depthmap;
    uniform float halfFov;
    uniform vec3 offset;
    float angle45Factor = 0.7071067812;

    void main() {
        float s = 2.0 * (gl_TexCoord[0].s - 0.5);
        float t = 2.0 * (gl_TexCoord[0].t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta) - offset.x;
            float y = -sin(phi) * cos(theta) - offset.y;
            float z = cos(phi) - offset.z;
            **rotVec**;
            gl_FragColor = textureCube(cubemap, rotVec);
            gl_FragDepth = textureCube(depthmap, rotVec).x;
        }
        else {
            gl_FragColor = **bgColor**;
            gl_FragDepth = 1.0;
        }
    }
)";

constexpr const char* FisheyeFragOffAxisDepthNormal = R"(
    **glsl_version**

    uniform samplerCube cubemap;
    uniform samplerCube depthmap;
    uniform samplerCube normalmap;
    uniform float halfFov;
    uniform vec3 offset;
    float angle45Factor = 0.7071067812;

    void main() {
        float s = 2.0 * (gl_TexCoord[0].s - 0.5);
        float t = 2.0 * (gl_TexCoord[0].t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta) - offset.x;
            float y = -sin(phi) * cos(theta) - offset.y;
            float z = cos(phi) - offset.z;
            **rotVec**;
            gl_FragData[0] = textureCube(cubemap, rotVec);
            gl_FragData[1] = textureCube(normalmap, rotVec);
            gl_FragDepth = textureCube(depthmap, rotVec).x;
        }
        else {
            gl_FragData[0] = **bgColor**;
            gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);
            gl_FragDepth = 1.0;
        }
    }
)";

constexpr const char* FisheyeFragOffAxisDepthPosition = R"(
    **glsl_version**

    uniform samplerCube cubemap;
    uniform samplerCube depthmap;
    uniform samplerCube positionmap;
    uniform float halfFov;
    uniform vec3 offset;
    float angle45Factor = 0.7071067812;

    void main() {
        float s = 2.0 * (gl_TexCoord[0].s - 0.5);
        float t = 2.0 * (gl_TexCoord[0].t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta) - offset.x;
            float y = -sin(phi) * cos(theta) - offset.y;
            float z = cos(phi) - offset.z;
            **rotVec**;
            gl_FragData[0] = textureCube(cubemap, rotVec);
            gl_FragData[1] = textureCube(positionmap, rotVec);
            gl_FragDepth = textureCube(depthmap, rotVec).x;
        }
        else {
            gl_FragData[0] = **bgColor**;
            gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);
            gl_FragDepth = 1.0;
        }
    }
)";

constexpr const char* FisheyeFragOffAxisDepthNormalPosition = R"(
    **glsl_version**

    uniform samplerCube cubemap;
    uniform samplerCube depthmap;
    uniform samplerCube normalmap;
    uniform samplerCube positionmap;
    uniform float halfFov;
    uniform vec3 offset;
    float angle45Factor = 0.7071067812;

    void main() {
        float s = 2.0 * (gl_TexCoord[0].s - 0.5);
        float t = 2.0 * (gl_TexCoord[0].t - 0.5);
        float r2 = s * s + t * t;
        if (r2 <= 1.0) {
            float phi = sqrt(r2) * halfFov;
            float theta = atan(s, t);
            float x = sin(phi) * sin(theta) - offset.x;
            float y = -sin(phi) * cos(theta) - offset.y;
            float z = cos(phi) - offset.z;
            **rotVec**;
            gl_FragData[0] = textureCube(cubemap, rotVec);
            gl_FragData[1] = textureCube(normalmap, rotVec);
            gl_FragData[2] = textureCube(positionmap, rotVec);
            gl_FragDepth = textureCube(depthmap, rotVec).x;
        }
        else {
            gl_FragData[0] = **bgColor**;
            gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);
            gl_FragData[2] = vec4(0.0, 0.0, 0.0, 0.0);
            gl_FragDepth = 1.0;
        }
    }
)";

constexpr const char* FisheyeDepthCorrectionFrag = R"(
    **glsl_version**

    uniform sampler2D cTex;
    uniform sampler2D dTex;
    uniform float near;
    uniform float far;

    void main() {
        float a = far / (far - near);
        float b = far * near / (near - far);
        float z = b / (texture2D(dTex, gl_TexCoord[0].st).x - a);

        // get angle from -45 to 45 degrees (-pi/4 to +pi/4)
        float xAngle = 1.57079632679 * (gl_TexCoord[0].s - 0.5);
        float yAngle = 1.57079632679 * (gl_TexCoord[0].t - 0.5);

        float x_norm = tan(xAngle);
        float y_norm = tan(yAngle);
        float r = z * sqrt(x_norm * x_norm + y_norm * y_norm + 1.0);

        gl_FragColor = texture2D(cTex, gl_TexCoord[0].st);
        gl_FragDepth = a + b / r;
        // No correction
        //gl_FragDepth = texture2D(dTex, gl_TexCoord[0].st).x;
    }
)";

} // namespace sgct::core::shaders_fisheye

#endif // __SGCT__INTERNAL_FISHEYE_SHADERS__H__
