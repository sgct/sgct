#version 120

varying vec3 normals;
varying vec4 position;
uniform mat4 WorldMatrixTranspose;

void main()
{
	// Move the normals back from the camera space to the world space
    mat3 worldRotationInverse = mat3(WorldMatrixTranspose);
    
    gl_Position    = gl_ModelViewProjectionMatrix * gl_Vertex;
    gl_TexCoord[0] = gl_MultiTexCoord0;
    normals        = normalize(worldRotationInverse * gl_NormalMatrix * gl_Normal);
    position       = gl_ModelViewMatrix * gl_Vertex;
}
