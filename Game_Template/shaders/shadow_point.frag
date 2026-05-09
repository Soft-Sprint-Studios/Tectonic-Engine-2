layout(location = 0) out vec2 FragColor;

in vec4 FragPos;
uniform vec3 u_lightPos;
uniform float u_farPlane;

const float EVSM_EXP = 40.0;

void main()
{
    float depth = length(FragPos.xyz - u_lightPos) / u_farPlane;
    float exponent = 10.0;
    float warpedDepth = exp(exponent * depth);
    
    FragColor = vec2(warpedDepth, warpedDepth * warpedDepth);
}