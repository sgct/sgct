#version 330 core

layout (location = 0) in vec2 TexCoords;
layout (location = 1) in vec3 Position;

uniform float Size;
out vec2 UV;
out vec2 BlurUV[14]; 

void main()
{
	gl_Position = vec4(Position, 1.0);
	UV = TexCoords;
	
	float step = 1.0/Size;
	BlurUV[ 0] = UV + vec2(0.0, -step*7.0);
	BlurUV[ 1] = UV + vec2(0.0, -step*6.0);
	BlurUV[ 2] = UV + vec2(0.0, -step*5.0);
	BlurUV[ 3] = UV + vec2(0.0, -step*4.0);
	BlurUV[ 4] = UV + vec2(0.0, -step*3.0);
	BlurUV[ 5] = UV + vec2(0.0, -step*2.0);
	BlurUV[ 6] = UV + vec2(0.0, -step);
	BlurUV[ 7] = UV + vec2(0.0,  step);
	BlurUV[ 8] = UV + vec2(0.0,  step*2.0);
	BlurUV[ 9] = UV + vec2(0.0,  step*3.0);
	BlurUV[10] = UV + vec2(0.0,  step*4.0);
	BlurUV[11] = UV + vec2(0.0,  step*5.0);
	BlurUV[12] = UV + vec2(0.0,  step*6.0);
	BlurUV[13] = UV + vec2(0.0,  step*7.0);
}