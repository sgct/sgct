#version 330 core

uniform sampler2D Tex;

in vec2 UV;
out vec4 color;

void main()
{
	color = texture(Tex, UV.st);
}