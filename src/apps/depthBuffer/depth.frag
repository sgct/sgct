#version 120

uniform sampler2D cTex;
uniform sampler2D dTex;
uniform float near;
uniform float far;

float linearizeDepth() {
  float z = texture2D(dTex, gl_TexCoord[0].st).x;
  return (2.0 * near) / (far + near - z * (far - near));
}

void main() {
  vec4 texel = texture2D(cTex, gl_TexCoord[0].st);
  if (gl_TexCoord[0].x < 0.5) {
    gl_FragColor = texel;
  }
  else {
    float d = linearizeDepth();
    gl_FragColor = vec4(d, d, d, 1.0);
  }
}
