#version 120

uniform sampler2D hTex;			// Sampler for texture height map
uniform sampler2D nTex;			// Sampler for texture height map
uniform float curr_time;		// Time
varying float vScale;			// Height scaling
varying vec3 light_dir;			// Light direction
varying vec3 v;					// Translate vector

void main()
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_MultiTexCoord1;

	vScale = 0.2+0.10*sin(curr_time);
	float hVal = texture2D( hTex, gl_TexCoord[0].st ).x;
	vec4 vertPos = gl_Vertex + vec4(0.0, hVal * vScale, 0.0, 0.0);

	//Transform a vertex to eye space
	v = vec3(gl_ModelViewMatrix * vertPos);
	light_dir = normalize(gl_LightSource[0].position.xyz - v);

	gl_Position = gl_ModelViewProjectionMatrix * vertPos;
}
