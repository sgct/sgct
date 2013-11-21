#version 330 core

layout (location = 0) in vec2 TexCoords;
layout (location = 1) in vec3 Position;

uniform mat4 MVP;
out vec2 UV;

void main()
{
   gl_Position = MVP * vec4(Position, 1.0);
   UV = TexCoords;
}