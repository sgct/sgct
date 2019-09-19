#version 120

uniform float size;

varying vec2 blurUV[14];

void main() {
  gl_TexCoord[0] = gl_MultiTexCoord0;
  gl_Position = gl_Vertex;
  
  float step = 1.0 / size;
  blurUV[ 0] = gl_TexCoord[0].st + vec2(0.0, -step * 7.0);
  blurUV[ 1] = gl_TexCoord[0].st + vec2(0.0, -step * 6.0);
  blurUV[ 2] = gl_TexCoord[0].st + vec2(0.0, -step * 5.0);
  blurUV[ 3] = gl_TexCoord[0].st + vec2(0.0, -step * 4.0);
  blurUV[ 4] = gl_TexCoord[0].st + vec2(0.0, -step * 3.0);
  blurUV[ 5] = gl_TexCoord[0].st + vec2(0.0, -step * 2.0);
  blurUV[ 6] = gl_TexCoord[0].st + vec2(0.0, -step);
  blurUV[ 7] = gl_TexCoord[0].st + vec2(0.0,  step);
  blurUV[ 8] = gl_TexCoord[0].st + vec2(0.0,  step * 2.0);
  blurUV[ 9] = gl_TexCoord[0].st + vec2(0.0,  step * 3.0);
  blurUV[10] = gl_TexCoord[0].st + vec2(0.0,  step * 4.0);
  blurUV[11] = gl_TexCoord[0].st + vec2(0.0,  step * 5.0);
  blurUV[12] = gl_TexCoord[0].st + vec2(0.0,  step * 6.0);
  blurUV[13] = gl_TexCoord[0].st + vec2(0.0,  step * 7.0);
}
