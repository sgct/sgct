uniform sampler2D tex; // Used for texture sampling

varying vec3 v;

// Computes the diffues shading by using the normal for
// the fragment and direction from fragment to the light
vec4 calcShading( vec3 fragNormal, vec3 lightDir )
{
	//Diffuse contribution
	vec4 Idiff = gl_FrontLightProduct[0].diffuse * max(dot(fragNormal,lightDir), 0.0);
	//Ambient contribution
	Idiff += vec4(0.15,0.15,0.15,1.0);
	return clamp(Idiff, 0.0, 1.0);
}

void main()
{
	ivec2 textRes = textureSize( tex, 0 );
	float invTexWidth  = 1.0 / textRes.x;
	float invTexHeight = 1.0 / textRes.y;
	
	// val_row_column, where 0_0 = current fragment
	float val_0_0 = length( texture2D( tex, gl_TexCoord[0].st ).xyz );

	float val_1_0 = length( texture2D( tex, gl_TexCoord[0].st + vec2(		 0.0, invTexHeight ) ).xyz );
	float val_0_1 = length( texture2D( tex, gl_TexCoord[0].st + vec2( invTexWidth,		   0.0 ) ).xyz );

	vec3 normal = normalize( cross( vec3( 0.0, val_1_0-val_0_0, invTexHeight ), vec3( invTexWidth, val_0_1-val_0_0, 0.0 ) ) );
	

	/*vec3 x = vec3(invTexWidth,
		texture2D( tex, gl_TexCoord[0].st - vec2(invTexWidth, 0.0 ) ).r/textRes.x,
		0.0);
	vec3 y = vec3(0.0,
		texture2D( tex, gl_TexCoord[0].st - vec2(0.0, invTexHeight ) ).r/textRes.y,
		invTexHeight);
	vec3 normal = normalize( cross(y,x) );*/
	
	vec3 L = normalize( (gl_LightSource[0].position*gl_ModelViewMatrix).xyz - v );
	gl_FragColor = calcShading( normal, L );
}
