#version 330 core

uniform sampler2D Tex;
uniform vec2 scaleUV;
uniform vec2 offsetUV;

in vec2 UV;
out vec4 color;

void main()
{
	// Side-by-side to Top-bottom
	vec2 pos = UV.t < 0.5
		? vec2(UV.s * 0.5, UV.t * 2.0) 							// left frame = bottom frame
        : vec2((UV.s + 1.0) * 0.5, (UV.t - 0.5) * 2.0); 	// right frame = top frame
		
	//pos = UV;

	color = texture(Tex, (pos * scaleUV) + offsetUV);
}