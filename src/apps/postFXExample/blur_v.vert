#version 120

uniform float Size;

varying vec2 BlurUV[14]; 

void main()
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	
	float step = 1.0/Size;
	BlurUV[ 0] = gl_TexCoord[0].st + vec2(0.0, -step*7.0);
	BlurUV[ 1] = gl_TexCoord[0].st + vec2(0.0, -step*6.0);
	BlurUV[ 2] = gl_TexCoord[0].st + vec2(0.0, -step*5.0);
	BlurUV[ 3] = gl_TexCoord[0].st + vec2(0.0, -step*4.0);
	BlurUV[ 4] = gl_TexCoord[0].st + vec2(0.0, -step*3.0);
	BlurUV[ 5] = gl_TexCoord[0].st + vec2(0.0, -step*2.0);
	BlurUV[ 6] = gl_TexCoord[0].st + vec2(0.0, -step);
	BlurUV[ 7] = gl_TexCoord[0].st + vec2(0.0,  step);
	BlurUV[ 8] = gl_TexCoord[0].st + vec2(0.0,  step*2.0);
	BlurUV[ 9] = gl_TexCoord[0].st + vec2(0.0,  step*3.0);
	BlurUV[10] = gl_TexCoord[0].st + vec2(0.0,  step*4.0);
	BlurUV[11] = gl_TexCoord[0].st + vec2(0.0,  step*5.0);
	BlurUV[12] = gl_TexCoord[0].st + vec2(0.0,  step*6.0);
	BlurUV[13] = gl_TexCoord[0].st + vec2(0.0,  step*7.0);
}