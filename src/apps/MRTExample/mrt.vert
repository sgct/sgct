#version 330 core

layout(location = 0) in vec2 texCoords;
layout(location = 1) in vec3 normals;
layout(location = 2) in vec3 vertPositions;

uniform mat4 mvpMatrix;
uniform mat4 worldMatrixTranspose;
uniform mat3 normalMatrix;

out vec2 uv;
out vec3 n;
out vec4 p;

void main() {
  // Move the normals back from the camera space to the world space
  mat3 worldRotationInverse = mat3(worldMatrixTranspose);

  // Output position of the vertex, in clip space : MVP * position
  gl_Position =  mvpMatrix * vec4(vertPositions, 1.0);
  uv = texCoords;
  n  = normalize(worldRotationInverse * normalMatrix * normals);
  p  = gl_Position;
}
