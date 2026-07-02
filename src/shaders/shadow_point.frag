$input v_fragPos

#include <bgfx_shader.sh>

uniform vec4 u_shadowParams;

void main()
{
    float depth = length(v_fragPos - u_shadowParams.xyz) / u_shadowParams.w;
    float warpedDepth = exp(10.0 * depth);
    gl_FragColor = vec4(warpedDepth, warpedDepth * warpedDepth, 0.0, 0.0);
}