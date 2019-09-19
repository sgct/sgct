varying vec4 gl_FrontColor;

uniform float currTime;

void main() {
  float cVal = abs(sin(currTime * 3.1415926 * 0.2));
  gl_FrontColor = vec4(cVal, 1.0 - cVal, 0.0, 1.0);
  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
