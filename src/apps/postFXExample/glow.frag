#version 120

uniform sampler2D tex;
uniform sampler2D texOrig;

void main() {
  gl_FragColor = clamp(texture2D(tex, gl_TexCoord[0].st) + texture2D(texOrig, gl_TexCoord[0].st), 0.0, 1.0 );
}
