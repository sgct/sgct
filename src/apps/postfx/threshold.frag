#version 330 core

in vec2 uv;
out vec4 color;

uniform sampler2D tex;

void main() {
  vec3 texel = texture(tex, uv).rgb;
  vec3 val = step(0.5, texel);
  float intensity = 0.3 * val.r + 0.59 * val.g + 0.11 * val.b;
  color.rgb = texel * intensity;
  color.a = 1.0;
}
