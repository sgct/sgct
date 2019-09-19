#version 330 core

in vec2 uv;
in vec2 blurUV[14];
out vec4 color;

uniform sampler2D tex;

void main() {
  color = vec4(0.0);
  color += texture(tex, blurUV[ 0]) * 0.0044299121055113265;
  color += texture(tex, blurUV[ 1]) * 0.00895781211794;
  color += texture(tex, blurUV[ 2]) * 0.0215963866053;
  color += texture(tex, blurUV[ 3]) * 0.0443683338718;
  color += texture(tex, blurUV[ 4]) * 0.0776744219933;
  color += texture(tex, blurUV[ 5]) * 0.115876621105;
  color += texture(tex, blurUV[ 6]) * 0.147308056121;
  color += texture(tex, uv        ) * 0.159576912161;
  color += texture(tex, blurUV[ 7]) * 0.147308056121;
  color += texture(tex, blurUV[ 8]) * 0.115876621105;
  color += texture(tex, blurUV[ 9]) * 0.0776744219933;
  color += texture(tex, blurUV[10]) * 0.0443683338718;
  color += texture(tex, blurUV[11]) * 0.0215963866053;
  color += texture(tex, blurUV[12]) * 0.00895781211794;
  color += texture(tex, blurUV[13]) * 0.0044299121055113265;
}
