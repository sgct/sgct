#version 430 core

uniform sampler2D Tex;

in vec2 UV;
out vec4 color;

void main()
{
	// Side-by-side to Top-bottom
	// Expected to be up-side-down as well
	vec2 pos = UV.t < 0.5
		? vec2((UV.s + 1.0) * 0.5, 1.0 - ((UV.t - 0.5) * 2.0)) 	// left frame = top frame
		: vec2(UV.s * 0.5, 1.0 - (UV.t * 2.0)); 						// right frame = bottom frame
        

	color = texture(Tex, pos);
}