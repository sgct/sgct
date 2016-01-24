#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertPositions;

uniform mat4 MVP;

void main()
{
    // Output position of the vertex, in clip space : MVP * position
	gl_Position =  MVP * vec4(vertPositions, 1.0);
}

