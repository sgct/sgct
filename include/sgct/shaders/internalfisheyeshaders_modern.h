/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__INTERNAL_FISHEYE_SHADERS__H__
#define __SGCT__INTERNAL_FISHEYE_SHADERS__H__

namespace sgct::core::shaders_fisheye {

constexpr const char* SampleFun = R"(
  vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg) {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s*s + t*t;
    if (r2 <= 1.0) {
      float phi = sqrt(r2) * halfFov;
      float theta = atan(s, t);
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

constexpr const char* SampleLatlonFun = R"(
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

constexpr const char* SampleOffsetFun = R"(
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
      **rotVec**;
      return texture(map, rotVec);
    }
    else {
      return bg;
    }
  }
)";
   
constexpr const char* BaseVert = R"(
  #version 330 core
    
  layout (location = 0) in vec2 TexCoords;
  layout (location = 1) in vec3 Position;
    
  out vec2 UV;
    
  void main() {
    gl_Position = vec4(Position, 1.0);
    UV = TexCoords;
  }
)";

constexpr const char* FisheyeVert = R"(
  #version 330 core
    
  layout (location = 0) in vec2 TexCoords;
  layout (location = 1) in vec3 Position;
    
  out vec2 UV;
  
  void main() {
    gl_Position = vec4(Position, 1.0);
    UV = TexCoords;
  }
)";

constexpr const char* FisheyeFrag = R"(
  #version 330 core
    
  in vec2 UV;
  out vec4 diffuse;
    
  uniform samplerCube cubemap;
  uniform float halfFov;
  const float angle45Factor = 0.7071067812;
    
  **sample_fun**
    
  void main() {
    diffuse = getCubeSample(UV, cubemap, **bgColor**);
  }
)";

constexpr const char* FisheyeFragNormal = R"(
  #version 330 core
    
  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 normal;
    
  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform float halfFov;
  const float angle45Factor = 0.7071067812;
    
  void main() {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s*s + t*t;
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
      normal = vec3(0.0);
    }
  }
)";

constexpr const char* FisheyeFragPosition = R"(
  #version 330 core
    
  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 position;
    
  uniform samplerCube cubemap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  const float angle45Factor = 0.7071067812;
    
  void main() {
    float s = 2.0 * (UV.s - 0.5);
    float t = 2.0 * (UV.t - 0.5);
    float r2 = s*s + t*t;
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
      position = vec3(0.0);
    }
  }
)";

constexpr const char* FisheyeFragNormalPosition = R"(
  #version 330 core
    
  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 normal;
  layout(location = 2) out vec3 position;
    
  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  const float angle45Factor = 0.7071067812;
    
  void main() {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s*s + t*t;
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
      normal = vec3(0.0);
      position = vec3(0.0);
    }
  }
)";

constexpr const char* FisheyeFragDepth = R"(
  #version 330 core
    
  in vec2 UV;
  out vec4 diffuse;
    
  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform float halfFov;
  const float angle45Factor = 0.7071067812;
    
   void main() {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s*s + t*t;
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

constexpr const char* FisheyeFragDepthNormal = R"(
  #version 330 core
    
  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 normal;
    
  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform samplerCube depthmap;
  uniform float halfFov;
  const float angle45Factor = 0.7071067812;
    
  void main() {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s*s + t*t;
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
      normal = vec3(0.0);
      gl_FragDepth = 1.0;
    }
  }
)";

constexpr const char* FisheyeFragDepthPosition = R"(
  #version 330 core
    
  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 position;
    
  uniform samplerCube cubemap;
  uniform samplerCube positionmap;
  uniform samplerCube depthmap;
  uniform float halfFov;
  const float angle45Factor = 0.7071067812;
    
  void main() {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s*s + t*t;
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
      position = vec3(0.0);
      gl_FragDepth = 1.0;
    }
  }
)";

constexpr const char* FisheyeFragDepthNormalPosition = R"(
  #version 330 core
    
  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 normal;
  layout(location = 2) out vec3 position;
    
  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform samplerCube positionmap;
  uniform samplerCube depthmap;
  uniform float halfFov;
  const float angle45Factor = 0.7071067812;
    
  void main() {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s*s + t*t;
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
      normal = vec3(0.0);
      position = vec3(0.0);
      gl_FragDepth = 1.0;
    }
  }
)";

constexpr const char* FisheyeFragOffAxis = R"(
  #version 330 core
    
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

constexpr const char* FisheyeFragOffAxisNormal = R"(
  #version 330 core
   
  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 normal;
    
  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform float halfFov;
  uniform vec3 offset;
  const float angle45Factor = 0.7071067812;
    
  void main() {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s*s + t*t;
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

constexpr const char* FisheyeFragOffAxisPosition = R"(
  #version 330 core
    
  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 position;
    
  uniform samplerCube cubemap;
  uniform samplerCube positionmap;
  uniform float halfFov;
  uniform vec3 offset;
  const float angle45Factor = 0.7071067812;
    
  void main() {
    float s = 2.0 * (UV.s - 0.5);
    float t = 2.0 * (UV.t - 0.5);
    float r2 = s*s + t*t;
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

constexpr const char* FisheyeFragOffAxisNormalPosition = R"(
  #version 330 core
    
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
    
  void main() {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s*s + t*t;
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
      normal = vec3(0.0);
      position = vec3(0.0);
    }
  }
)";

constexpr const char* FisheyeFragOffAxisDepth = R"(
  #version 330 core
    
  in vec2 UV;
  out vec4 diffuse;
    
  uniform samplerCube cubemap;
  uniform samplerCube depthmap;
  uniform float halfFov;
  uniform vec3 offset;
  const float angle45Factor = 0.7071067812;
    
  void main() {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s*s + t*t;
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

constexpr const char* FisheyeFragOffAxisDepthNormal = R"(
  #version 330 core
    
  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 normal;
    
  uniform samplerCube cubemap;
  uniform samplerCube normalmap;
  uniform samplerCube depthmap;
  uniform float halfFov;
  uniform vec3 offset;
  const float angle45Factor = 0.7071067812;
    
  void main() {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s*s + t*t;
    if (r2 <= 1.0) {
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
      normal = vec3(0.0);
      gl_FragDepth = 1.0;
    }
  }
)";

constexpr const char* FisheyeFragOffAxisDepthPosition = R"(
  #version 330 core
    
  in vec2 UV;
  layout(location = 0) out vec4 diffuse;
  layout(location = 1) out vec3 position;
    
  uniform samplerCube cubemap;
  uniform samplerCube positionmap;
  uniform samplerCube depthmap;
  uniform float halfFov;
  uniform vec3 offset;
  const float angle45Factor = 0.7071067812;
    
  void main() {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s*s + t*t;
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
      position = vec3(0.0);
      gl_FragDepth = 1.0;
    }
  }
)";

constexpr const char* FisheyeFragOffAxisDepthNormalPosition = R"(
  #version 330 core
    
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
  const float angle45Factor = 0.7071067812;
    
  void main() {
    float s = 2.0 * (texel.s - 0.5);
    float t = 2.0 * (texel.t - 0.5);
    float r2 = s*s + t*t;
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
      normal = vec3(0.0);
      position = vec3(0.0);
      gl_FragDepth = 1.0;
    }
  }
)";

constexpr const char* FisheyeDepthCorrectionFrag = R"(
  #version 330 core
    
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

} // sgct::core::shaders_fisheye

#endif // __SGCT__INTERNAL_FISHEYE_SHADERS__H__
