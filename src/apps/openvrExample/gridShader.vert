#version 330 core

layout(location = 0) in vec3 vertPosition;

uniform mat4 mvp;

void main()
{
  // Output position of the vertex, in clip space : MVP * position
  gl_Position =  mvp * vec4(vertPosition, 1.0);
}
