#version 330 core

in vec2 UV;
out vec4 Color;

uniform sampler2D Tex;
uniform sampler2D TexOrig;

void main()
{
	Color = clamp( texture(Tex, UV) + texture(TexOrig, UV), 0.0, 1.0 );
}