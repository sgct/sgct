#version 120

uniform sampler2D Tex;
uniform sampler2D TexOrig;

void main()
{
	gl_FragColor = clamp( texture2D(Tex,  gl_TexCoord[0].st) + texture2D(TexOrig, gl_TexCoord[0].st), 0.0, 1.0 );
}