#version 330 core

uniform sampler2D hTex;
uniform sampler2D nTex;
uniform vec4 lightAmbient;
uniform vec4 lightDiffuse;
uniform vec4 lightSpecular;
uniform mat3 normalMatrix;

in vec2 uv;
in float vScale;
in vec3 lightDir;
in vec3 v;

out vec4 color;

// Computes the diffues shading by using the normal for
// the fragment and direction from fragment to the light
vec4 calcShading(vec3 N, vec3 L) {
  // Ambient contribution
  vec4 iamb = lightAmbient;

  // Diffuse contribution
  vec4 idiff = lightDiffuse * max(dot(N, L), 0.0);
  idiff = clamp(idiff, 0.0, 1.0);

  // Specular contribution
  vec3 E = normalize(-v);
  vec3 R = normalize(reflect(-L,N));
  const float specExp = 32.0;
  vec4 ispec = lightSpecular * pow(max(dot(R, E), 0.0), specExp);
  ispec = clamp(ispec, 0.0, 1.0);

  return iamb + idiff + ispec;
}

void main() {
  vec3 pixelVals = texture(nTex, uv).rgb;
  vec3 normal = vec3(
    (pixelVals.r * 2.0 - 1.0),
    (pixelVals.b * 2.0 - 1.0) / vScale,
    (pixelVals.g * 2.0 - 1.0)
  );
  if (vScale < 0) {
    normal = -normal;
  }

  // Set fragment color
  // This will result in a non-linear color temperature scale based on height value
  float hVal = texture(hTex, uv).x;
  float Pi = 3.14159265358979323846264;
  color.rgb = vec3(1.0 - cos(Pi * hVal), sin(Pi * hVal), cos(Pi * hVal));

  // multiply color with shading
  color.rgb *= calcShading(normalize(normalMatrix * normal), lightDir).rgb;
  color.a = 1.0;
}