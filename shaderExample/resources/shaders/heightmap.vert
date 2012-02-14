uniform sampler2D tex;			// Sampler for texture height map
uniform float height = 0.25f;	// Height scaling
varying vec3 v;					// Vertex position in eye coordinates

void main() {
	gl_TexCoord[0] = gl_MultiTexCoord0;
	
	gl_Vertex.y += length( texture2D( tex, gl_TexCoord[0].st ).xyz ) * height;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	v = vec3(gl_ModelViewMatrix * gl_Vertex);
}