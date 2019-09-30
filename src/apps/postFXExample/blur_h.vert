#version 330 core

layout (location = 0) in vec2 texCoords;
layout (location = 1) in vec3 position;

uniform float size;
out vec2 uv;
out vec2 blurUV[14];

void main() {
  gl_Position = vec4(position, 1.0);
  uv = texCoords;
  
  float step = 1.0 / size;
  blurUV[ 0] = uv + vec2(-step * 7.0, 0.0);
  blurUV[ 1] = uv + vec2(-step * 6.0, 0.0);
  blurUV[ 2] = uv + vec2(-step * 5.0, 0.0);
  blurUV[ 3] = uv + vec2(-step * 4.0, 0.0);
  blurUV[ 4] = uv + vec2(-step * 3.0, 0.0);
  blurUV[ 5] = uv + vec2(-step * 2.0, 0.0);
  blurUV[ 6] = uv + vec2(-step,     0.0);
  blurUV[ 7] = uv + vec2( step,     0.0);
  blurUV[ 8] = uv + vec2( step * 2.0, 0.0);
  blurUV[ 9] = uv + vec2( step * 3.0, 0.0);
  blurUV[10] = uv + vec2( step * 4.0, 0.0);
  blurUV[11] = uv + vec2( step * 5.0, 0.0);
  blurUV[12] = uv + vec2( step * 6.0, 0.0);
  blurUV[13] = uv + vec2( step * 7.0, 0.0);
}
