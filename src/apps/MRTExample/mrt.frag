#version 120

varying vec4 position;
varying vec3 normals;
uniform sampler2D tDiffuse;

void main() {
  gl_FragData[0] = texture2D(tDiffuse, gl_TexCoord[0].st);
  gl_FragData[1] = vec4(normals, 0);
  gl_FragData[2] = vec4(position.xyz, 0);
}
