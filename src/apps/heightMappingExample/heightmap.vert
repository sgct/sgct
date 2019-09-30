#version 330 core

layout(location = 0) in vec3 vertPositions;
layout(location = 1) in vec2 texCoords;

uniform sampler2D hTex;
uniform float currTime;
uniform mat4 mvp;
uniform mat4 mv;
uniform mat4 mvLight;
uniform vec4 lightPos;

out vec2 uv; //texture coords
out float vScale; // Height scaling
out vec3 lightDir;
out vec3 v;

void main() {
  uv = texCoords;

  vScale = 0.2 + 0.10 * sin(currTime);
  float hVal = texture(hTex, uv).r;
  vec4 transformedVertex = vec4(vertPositions + vec3(0.0, hVal * vScale, 0.0), 1.0);

  // Transform a vertex to model space
  v = vec3(mv * transformedVertex);
  vec3 l = vec3(mvLight * lightPos);
  lightDir = normalize(l - v);
  
  // Output position of the vertex, in clip space : MVP * position
  gl_Position =  mvp * transformedVertex;
}
