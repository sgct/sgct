uniform float curr_time;	// Time
varying vec4 gl_FrontColor;

void main()
{
	float cVal = abs( sin( curr_time * 3.14 * 0.2 ) );
	gl_FrontColor = vec4( cVal, 1.0-cVal, 0.0, 1.0 );
	gl_Position =  gl_ModelViewProjectionMatrix * gl_Vertex;
}
