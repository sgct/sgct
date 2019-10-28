#version 330 core

layout (location = 0) in vec2 texCoords;
layout (location = 1) in vec3 position;

out vec2 uv;

void main() {
  gl_Position = vec4(position, 1.0);
  uv = texCoords;
}