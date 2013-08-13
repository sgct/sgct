#version 330 core

in vec3 tnormals;

out vec4 Color;
out vec3 Normal;

void main()
{
	Normal = tnormals;
	float intensity = dot( tnormals, vec3(0.0, 0.0, 1.0) );
	Color = vec4(intensity, 1.0, 1.0, 1.0);
}