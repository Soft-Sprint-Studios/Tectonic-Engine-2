$input a_position, a_texcoord0, a_texcoord1, a_texcoord2, a_normal, a_tangent
$output v_texcoord0, v_lmCoord, v_lmSize, v_fragPos, v_tbn0, v_tbn1, v_tbn2

#include <bgfx_shader.sh>

void main()
{
    vec4 worldPos = mul(u_model[0], vec4(a_position, 1.0));
    v_fragPos = worldPos.xyz;
    v_texcoord0 = a_texcoord0;

    mat3 normalMatrix = mat3(u_model[0][0].xyz, u_model[0][1].xyz, u_model[0][2].xyz);
    vec3 N = normalize(mul(normalMatrix, a_normal));
    vec3 T = normalize(mul(normalMatrix, a_tangent.xyz));
    vec3 B = cross(N, T) * a_tangent.w;
    v_tbn0 = T;
    v_tbn1 = B;
    v_tbn2 = N;

    v_lmCoord = a_texcoord1;
    v_lmSize = a_texcoord2;

    gl_Position = mul(u_viewProj, worldPos);
}