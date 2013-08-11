#version 120

varying vec2 BlurUV[14];

uniform sampler2D Tex;

void main()
{
	gl_FragColor = vec4(0.0);
    gl_FragColor += texture2D(Tex, BlurUV[ 0])*0.0044299121055113265;
    gl_FragColor += texture2D(Tex, BlurUV[ 1])*0.00895781211794;
    gl_FragColor += texture2D(Tex, BlurUV[ 2])*0.0215963866053;
    gl_FragColor += texture2D(Tex, BlurUV[ 3])*0.0443683338718;
    gl_FragColor += texture2D(Tex, BlurUV[ 4])*0.0776744219933;
    gl_FragColor += texture2D(Tex, BlurUV[ 5])*0.115876621105;
    gl_FragColor += texture2D(Tex, BlurUV[ 6])*0.147308056121;
    gl_FragColor += texture2D(Tex, gl_TexCoord[0].st )*0.159576912161;
    gl_FragColor += texture2D(Tex, BlurUV[ 7])*0.147308056121;
    gl_FragColor += texture2D(Tex, BlurUV[ 8])*0.115876621105;
    gl_FragColor += texture2D(Tex, BlurUV[ 9])*0.0776744219933;
    gl_FragColor += texture2D(Tex, BlurUV[10])*0.0443683338718;
    gl_FragColor += texture2D(Tex, BlurUV[11])*0.0215963866053;
    gl_FragColor += texture2D(Tex, BlurUV[12])*0.00895781211794;
    gl_FragColor += texture2D(Tex, BlurUV[13])*0.0044299121055113265;
}