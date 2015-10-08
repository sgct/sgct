#version 330 core

in vec2 uv;
out vec4 color;

uniform vec3 FaceColor;
uniform sampler2D Tex;

void main()
{
	color = vec4(FaceColor, 1.0) * texture(Tex, uv.st);
}