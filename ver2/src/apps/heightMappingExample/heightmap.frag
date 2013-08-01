#version 120

uniform sampler2D hTex;
uniform sampler2D nTex;
uniform float curr_time;
varying float vScale;
varying vec3 light_dir;
varying vec3 v;

// Computes the diffues shading by using the normal for
// the fragment and direction from fragment to the light
vec4 calcShading( vec3 N, vec3 L )
{
	//Ambient contribution
	vec4 Iamb = gl_LightSource[0].ambient;

	//Diffuse contribution
	vec4 Idiff = gl_LightSource[0].diffuse * max(dot(N,L), 0.0);
	Idiff = clamp(Idiff, 0.0, 1.0);

	//Specular contribution
	vec3 E = normalize(-v); // we are in Eye Coordinates, so EyePos is (0,0,0)
	vec3 R = normalize(reflect(-L,N));
	const float specExp = 32.0;
	vec4 Ispec = gl_LightSource[0].specular
		* pow(max(dot(R,E),0.0), specExp);
    Ispec = clamp(Ispec, 0.0, 1.0);

	return Iamb + Idiff + Ispec;
}

void main()
{
	vec3 pixelVals = texture2D( nTex, gl_TexCoord[1].st).rgb;
	vec3 normal;
	normal.x = (pixelVals.r * 2.0 - 1.0);
	normal.y = (pixelVals.b * 2.0 - 1.0)/vScale;
	normal.z = (pixelVals.g * 2.0 - 1.0);
	if(vScale < 0)
		normal = -normal;

	// Set fragment color
	// This will result in a non-linear color temperature scale based on height value
	float hVal = texture2D( hTex, gl_TexCoord[0].st).x;
	float Pi = 3.14159265358979323846264;
	gl_FragColor.rgb = vec3(1.0-cos(Pi*hVal),sin(Pi*hVal),cos(Pi*hVal));

	// multiply color with shading
	gl_FragColor.rgb *= calcShading( normalize(gl_NormalMatrix * normal), light_dir ).xyz;
	gl_FragColor.a = 1.0;
}
