#version 120

varying vec2 blurUV[14];

uniform sampler2D tex;

void main() {
  gl_FragColor = vec4(0.0);
  gl_FragColor += texture2D(tex, blurUV[ 0]) * 0.0044299121055113265;
  gl_FragColor += texture2D(tex, blurUV[ 1]) * 0.00895781211794;
  gl_FragColor += texture2D(tex, blurUV[ 2]) * 0.0215963866053;
  gl_FragColor += texture2D(tex, blurUV[ 3]) * 0.0443683338718;
  gl_FragColor += texture2D(tex, blurUV[ 4]) * 0.0776744219933;
  gl_FragColor += texture2D(tex, blurUV[ 5]) * 0.115876621105;
  gl_FragColor += texture2D(tex, blurUV[ 6]) * 0.147308056121;
  gl_FragColor += texture2D(tex, gl_TexCoord[0].st) * 0.159576912161;
  gl_FragColor += texture2D(tex, blurUV[ 7]) * 0.147308056121;
  gl_FragColor += texture2D(tex, blurUV[ 8]) * 0.115876621105;
  gl_FragColor += texture2D(tex, blurUV[ 9]) * 0.0776744219933;
  gl_FragColor += texture2D(tex, blurUV[10]) * 0.0443683338718;
  gl_FragColor += texture2D(tex, blurUV[11]) * 0.0215963866053;
  gl_FragColor += texture2D(tex, blurUV[12]) * 0.00895781211794;
  gl_FragColor += texture2D(tex, blurUV[13]) * 0.0044299121055113265;
}
