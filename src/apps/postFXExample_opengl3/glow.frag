#version 330 core

in vec2 uv;
out vec4 color;

uniform sampler2D tex;
uniform sampler2D texOrig;

void main() {
  color = clamp(texture(tex, uv) + texture(texOrig, uv), 0.0, 1.0);
}
