#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertPositions;
layout(location = 1) in vec2 texCoords;
layout(location = 2) in vec3 normals;

uniform mat4 MVP;
uniform mat3 NM; //Normal Matrix
/*
	in your c++ code:
	
	#include <glm/gtc/matrix_inverse.hpp>
	glm::mat3 NM = glm::inverseTranspose(glm::mat3( ModelViewMatrix ));
*/

out vec2 UV;
out vec3 tnormals; //transformed normals

void main()
{
    // Output position of the vertex, in clip space : MVP * position
	gl_Position =  MVP * vec4(vertPositions, 1.0);
	UV = texCoords;
	tnormals = normalize( NM * normals );
}

