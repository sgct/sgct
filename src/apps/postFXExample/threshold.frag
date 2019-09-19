#version 120

uniform sampler2D tex;

void main() {
  vec3 texel = texture2D(tex, gl_TexCoord[0].st).rgb;
  vec3 val = step(0.5, texel);
  float intensity = 0.3 * val.r + 0.59 * val.g + 0.11 * val.b;
  gl_FragColor.rgb = texel * intensity;
  gl_FragColor.a = 1.0;
}
