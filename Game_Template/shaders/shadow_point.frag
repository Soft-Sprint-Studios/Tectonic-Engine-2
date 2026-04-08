in vec4 FragPos;
uniform vec3 u_lightPos;
uniform float u_farPlane;

void main()
{
    float distance = length(FragPos.xyz - u_lightPos);
    distance = distance / u_farPlane;
    gl_FragDepth = distance;
}