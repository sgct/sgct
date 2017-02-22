#version 330 core

layout(location = 0) in vec3 vertPosition;

uniform mat4 MVP;

void main()
{
    // Output position of the vertex, in clip space : MVP * position
	gl_Position =  MVP * vec4(vertPosition, 1.0);
}

