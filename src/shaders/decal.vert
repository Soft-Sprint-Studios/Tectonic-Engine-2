$input a_position, a_texcoord0, i_data0, i_data1, i_data2, i_data3
$output v_texcoord0, v_fragPos, v_tbn0, v_tbn1, v_tbn2

#include <bgfx_shader.sh>

void main()
{
    mat4 modelMat = mtxFromCols(i_data0, i_data1, i_data2, i_data3);
    v_texcoord0 = a_texcoord0;

    vec3 N = normalize(mul(modelMat, vec4(0.0, 0.0, 1.0, 0.0)).xyz);
    vec3 T = normalize(mul(modelMat, vec4(1.0, 0.0, 0.0, 0.0)).xyz);
    vec3 B = cross(N, T);

    v_tbn0 = T;
    v_tbn1 = B;
    v_tbn2 = N;

    vec4 worldPos = mul(modelMat, vec4(a_position, 1.0));
    v_fragPos = worldPos.xyz;
    
    vec4 clipPos = mul(u_viewProj, worldPos);

    clipPos.z -= 0.0001 * clipPos.w;
    gl_Position = clipPos;
}