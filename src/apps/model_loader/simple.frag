#version 330 core

uniform sampler2D tex;

in vec2 uv;
in vec3 tnormals;
out vec4 color;

void main() {
  color = texture(tex, uv);
}
