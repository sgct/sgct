#version 330 core

uniform float alpha;

out vec4 color;

void main() {
  color = vec4(1.0, 0.0, 0.5, alpha);
}