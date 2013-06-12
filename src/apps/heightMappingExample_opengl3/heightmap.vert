#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertPositions;
layout(location = 1) in vec2 texCoords;

uniform sampler2D hTex;
uniform float curr_time;
uniform mat4 MVP;
uniform mat4 MV;
uniform mat4 MV_light;
uniform vec4 lightPos;

out vec2 UV; //texture coords
out float vScale; // Height scaling
out vec3 light_dir;
out vec3 v;

void main()
{
    UV = texCoords;
	
	vScale = 0.2+0.10*sin(curr_time);
	float hVal = texture( hTex, UV ).r;
	vec4 transformedVertex = vec4(vertPositions + vec3(0.0, hVal * vScale, 0.0), 1.0);
	
	//Transform a vertex to model space
	v = vec3(MV * transformedVertex);
	vec3 l = vec3(MV_light * lightPos);
	light_dir = normalize(l - v);
	
	// Output position of the vertex, in clip space : MVP * position
	gl_Position =  MVP * transformedVertex;
}

