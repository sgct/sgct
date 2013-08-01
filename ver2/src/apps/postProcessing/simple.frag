uniform sampler2D tex;

void main()
{
	vec4 texel = texture2D(tex,gl_TexCoord[0].st);
	gl_FragColor.rgb = vec3(1.0) - texel.rgb;
	gl_FragColor.a = 1.0;
}
