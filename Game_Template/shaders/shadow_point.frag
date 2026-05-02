layout(location = 0) out vec2 FragColor;

in vec4 FragPos;
uniform vec3 u_lightPos;
uniform float u_farPlane;

void main()
{
    float depth = length(FragPos.xyz - u_lightPos) / u_farPlane;
    FragColor = vec2(depth, depth * depth);
}