#version 330 core

layout(location = 0) out vec4 diffuse;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 position;

uniform sampler2D tDiffuse;

in vec2 UV;
in vec3 N;
in vec4 P;

void main()
{
	diffuse = texture(tDiffuse, UV.st);
	normal = N;
	position = P.xyz;
}