#version 330 core

layout(location = 0) in vec2 texCoords;
layout(location = 1) in vec3 normals;
layout(location = 2) in vec3 vertPositions;

uniform mat4 mvp;
uniform int flip;

out vec2 uv;

void main() {
  // Output position of the vertex, in clip space : MVP * position
  gl_Position =  mvp * vec4(vertPositions, 1.0);
  uv = texCoords;
}
