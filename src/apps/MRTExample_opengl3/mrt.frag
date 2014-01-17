#version 330 core

uniform sampler2D tDiffuse;

in vec2 UV;
in vec3 N;
in vec4 P;

out vec4 diffuse;
out vec3 normal;
out vec3 position;

void main()
{
	diffuse = texture(tDiffuse, UV.st);
	normal = N;
	position = P.xyz;
}