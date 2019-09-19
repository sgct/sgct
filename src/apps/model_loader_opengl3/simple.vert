#version 330 core

layout(location = 0) in vec3 vertPositions;
layout(location = 1) in vec2 texCoords;
layout(location = 2) in vec3 normals;

uniform mat4 mvp;
uniform mat3 nm; //Normal Matrix

out vec2 uv;
out vec3 tnormals; //transformed normals

void main() {
  // Output position of the vertex, in clip space : MVP * position
  gl_Position = mvp * vec4(vertPositions, 1.0);
  uv = texCoords;
  tnormals = normalize(nm * normals);
}
