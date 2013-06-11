#version 330 core

layout(location = 0) in vec3 vertPosition;

uniform mat4 MVP;
uniform float curr_time;
out vec3 fragColor;

void main()
{
    // Output position of the vertex, in clip space : MVP * position
	gl_Position =  MVP * vec4(vertPosition, 1.0);
	
	float cVal = abs( sin( curr_time * 3.14 * 0.2 ) );
	fragColor = vec3( cVal, 1.0-cVal, 0.0 );
}


