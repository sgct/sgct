#version 330 core

uniform sampler2D Tex;

in vec2 UV;
in vec3 tnormals;
out vec4 color;

void main()
{
	/*vec4 test;
	test.r = (tnormals.x + 1.0)/2.0;
	test.g = (tnormals.y + 1.0)/2.0;
	test.b = (tnormals.z + 1.0)/2.0;
	test.a = 1.0;
	color = test;*/
	
	color = texture(Tex, UV.st);
	
}