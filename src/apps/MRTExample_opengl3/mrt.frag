#version 330 core

layout(location = 0) out vec4 diffuse;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 position;

uniform sampler2D tDiffuse;

in vec2 uv;
in vec3 n;
in vec4 p;

void main() {
  diffuse = texture(tDiffuse, uv);
  normal = n;
  position = p.xyz;
}