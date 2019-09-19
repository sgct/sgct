#version 330 core

uniform sampler2D tex;
uniform vec2 scaleUV;
uniform vec2 offsetUV;

in vec2 uv;
out vec4 color;

void main() {
  color = texture(tex, (uv.st * scaleUV) + offsetUV);
}
