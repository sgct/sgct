#version 330 core

layout(location = 0) in vec3 vertPosition;
layout(location = 1) in vec3 vertColor;

uniform mat4 MVP;
out vec3 fragColor;

void main()
{
    // Output position of the vertex, in clip space : MVP * position
	gl_Position =  MVP * vec4(vertPosition, 1.0);
	fragColor = vertColor;
}

