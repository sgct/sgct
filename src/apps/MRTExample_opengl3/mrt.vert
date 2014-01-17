#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec2 texCoords;
layout(location = 1) in vec3 normals;
layout(location = 2) in vec3 vertPositions;

uniform mat4 MVPMatrix;
uniform mat4 WorldMatrixTranspose;
uniform mat3 NormalMatrix;

out vec2 UV;
out vec3 N;
out vec4 P;

void main()
{
    // Move the normals back from the camera space to the world space
    mat3 worldRotationInverse = mat3(WorldMatrixTranspose);
	
	// Output position of the vertex, in clip space : MVP * position
	gl_Position =  MVPMatrix * vec4(vertPositions, 1.0);
	UV = texCoords;
	N  = normalize(worldRotationInverse * NormalMatrix * normals);
    P  = gl_Position;
}
