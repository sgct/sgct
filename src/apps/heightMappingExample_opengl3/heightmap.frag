#version 330 core

uniform sampler2D hTex;
uniform sampler2D nTex;
uniform vec4 light_ambient;
uniform vec4 light_diffuse;
uniform vec4 light_specular;
uniform mat3 normalMatrix;

in vec2 UV;
in float vScale;
in vec3 light_dir;
in vec3 v;

out vec4 color;

// Computes the diffues shading by using the normal for
// the fragment and direction from fragment to the light
vec4 calcShading( vec3 N, vec3 L )
{
	//Ambient contribution
	vec4 Iamb = light_ambient;

	//Diffuse contribution
	vec4 Idiff = light_diffuse * max(dot(N,L), 0.0);
	Idiff = clamp(Idiff, 0.0, 1.0);

	//Specular contribution
	vec3 E = normalize(-v);
	vec3 R = normalize(reflect(-L,N));
	const float specExp = 32.0;
	vec4 Ispec = light_specular
		* pow(max(dot(R,E),0.0), specExp);
    Ispec = clamp(Ispec, 0.0, 1.0);

	return Iamb + Idiff + Ispec;
}

void main()
{
	vec3 pixelVals = texture( nTex, UV).rgb;
	vec3 normal;
	normal.x = (pixelVals.r * 2.0 - 1.0);
	normal.y = (pixelVals.b * 2.0 - 1.0)/vScale;
	normal.z = (pixelVals.g * 2.0 - 1.0);
	if(vScale < 0)
		normal = -normal;

	// Set fragment color
	// This will result in a non-linear color temperature scale based on height value
	float hVal = texture( hTex, UV).x;
	float Pi = 3.14159265358979323846264;
	color.rgb = vec3(1.0-cos(Pi*hVal),sin(Pi*hVal),cos(Pi*hVal));

	// multiply color with shading
	color.rgb *= calcShading( normalize(normalMatrix * normal), light_dir ).rgb;
	color.a = 1.0;
}