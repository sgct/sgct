#version 330 core

layout(location = 0) in vec3 vertPosition;

uniform mat4 mvp;
uniform float currTime;
out vec3 fragColor;

void main() {
  // Output position of the vertex, in clip space : MVP * position
  gl_Position =  mvp * vec4(vertPosition, 1.0);

  float cVal = abs(sin(currTime * 3.1415926 * 0.2));
  fragColor = vec3(cVal, 1.0 - cVal, 0.0);
}
