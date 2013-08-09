#version 330 core

in vec2 UV;
out vec4 Color;

uniform sampler2D Tex;

void main()
{
	vec3 texel = texture(Tex, UV).rgb;
	vec3 val = step( 0.5, texel );
	float intensity = 0.3*val.r + 0.59*val.g + 0.11*val.b;
	Color.rgb = texel * intensity;
	Color.a = 1.0;
}