uniform sampler2D hTex;			// Sampler for texture height map
uniform sampler2D nTex;			// Sampler for texture height map
uniform float time;				// Time
varying float vScale;			// Height scaling
varying vec3 light_dir;			// Light direction
varying vec3 v;					// Translate vector

void main()
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_MultiTexCoord1;
	
	vScale = 0.15+0.05*sin(time);
	float hVal = texture2D( hTex, gl_TexCoord[0].st ).x;
	gl_Vertex.y += hVal * vScale;
	
	//Transform a vertex to eye space
	v = vec3(gl_ModelViewMatrix * gl_Vertex);
	light_dir = normalize(gl_LightSource[0].position.xyz - v);
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}