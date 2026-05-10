layout(location = 0) out vec2 FragColor;

in vec3 v_FragPos;

uniform vec3 u_lightPos;
uniform float u_farPlane;

void main()
{
    float depth = distance(v_FragPos, u_lightPos) / u_farPlane;
    float warpedDepth = exp(10.0 * depth);

    FragColor = vec2(warpedDepth, warpedDepth * warpedDepth);
}