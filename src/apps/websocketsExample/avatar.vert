#version 330 core

layout(location = 0) in vec3 vertPosition;
layout(location = 1) in vec2 texCoord;

uniform mat4 MVP;

out vec2 uv;

void main()
{
    // Output position of the vertex, in clip space : MVP * position
	gl_Position =  MVP * vec4(vertPosition, 1.0);
	uv = texCoord;
}

