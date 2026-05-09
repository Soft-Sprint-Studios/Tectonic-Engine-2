layout(location = 0) out vec2 FragColor;

void main()
{
    float depth = gl_FragCoord.z; 
    float exponent = 10.0;
    float warpedDepth = exp(exponent * depth);

    FragColor = vec2(warpedDepth, warpedDepth * warpedDepth);
}