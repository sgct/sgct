#version 330 core

in vec2 UV;
in vec2 BlurUV[14];
out vec4 Color;

uniform sampler2D Tex;

void main()
{
	Color = vec4(0.0);
    Color += texture(Tex, BlurUV[ 0])*0.0044299121055113265;
    Color += texture(Tex, BlurUV[ 1])*0.00895781211794;
    Color += texture(Tex, BlurUV[ 2])*0.0215963866053;
    Color += texture(Tex, BlurUV[ 3])*0.0443683338718;
    Color += texture(Tex, BlurUV[ 4])*0.0776744219933;
    Color += texture(Tex, BlurUV[ 5])*0.115876621105;
    Color += texture(Tex, BlurUV[ 6])*0.147308056121;
    Color += texture(Tex, UV        )*0.159576912161;
    Color += texture(Tex, BlurUV[ 7])*0.147308056121;
    Color += texture(Tex, BlurUV[ 8])*0.115876621105;
    Color += texture(Tex, BlurUV[ 9])*0.0776744219933;
    Color += texture(Tex, BlurUV[10])*0.0443683338718;
    Color += texture(Tex, BlurUV[11])*0.0215963866053;
    Color += texture(Tex, BlurUV[12])*0.00895781211794;
    Color += texture(Tex, BlurUV[13])*0.0044299121055113265;
}