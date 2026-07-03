$input a_position, a_normal, a_tangent, a_texcoord0, a_texcoord1, a_texcoord2, a_texcoord3, a_indices, a_weight, i_data0, i_data1, i_data2, i_data3
$output v_texcoord0, v_lmCoord, v_lmSize, v_alpha, v_fragPos, v_tbn0, v_tbn1, v_tbn2

#include <bgfx_shader.sh>

uniform vec4 u_modelParams;
#define u_isInstanced (u_modelParams.x > 0.5)
#define u_isAnimated  (u_modelParams.y > 0.5)
#define u_hasLM        (u_modelParams.z > 0.5)

uniform mat4 u_bones[128];

StructuredBuffer<vec4> u_lmTransforms : register(t7);

void main()
{
    mat4 modelMat = u_model[0];
    if (u_isInstanced)
    {
        modelMat = mtxFromCols(i_data0, i_data1, i_data2, i_data3);
    }

    if (u_isAnimated)
    {
        mat4 skinMat = a_weight.x * u_bones[a_indices.x] 
                     + a_weight.y * u_bones[a_indices.y] 
                     + a_weight.z * u_bones[a_indices.z] 
                     + a_weight.w * u_bones[a_indices.w];
        modelMat = mul(modelMat, skinMat);
    }

    vec4 worldPos = mul(modelMat, vec4(a_position, 1.0));
    v_fragPos = worldPos.xyz;
    gl_Position = mul(u_viewProj, worldPos);

    mat3 normalMatrix = mat3(modelMat[0].xyz, modelMat[1].xyz, modelMat[2].xyz);
    vec3 N = normalize(mul(normalMatrix, a_normal));
    vec3 T = vec3_splat(0.0);

    if (length(a_tangent.xyz) > 0.0001)
    {
        T = normalize(mul(normalMatrix, a_tangent.xyz));
        T = normalize(T - dot(T, N) * N);
    }
    else
    {
        vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        T = normalize(cross(up, N));
    }

    vec3 B = cross(N, T);
    if (length(a_tangent.xyz) > 0.0001)
    {
        B *= a_tangent.w;
    }
    
    v_tbn0 = T;
    v_tbn1 = B;
    v_tbn2 = N;

    v_texcoord0 = a_texcoord0;
    v_lmCoord = a_texcoord1;
    v_lmSize = a_texcoord2;

    if (u_isInstanced) 
    {
        v_texcoord0 = vec2(a_texcoord0.x, 1.0 - a_texcoord0.y);
        if (u_hasLM)
        {
            int transBase = gl_InstanceID * 4;
            v_lmCoord = a_texcoord0 * u_lmTransforms[transBase + 0].zw + u_lmTransforms[transBase + 0].xy;
            v_lmSize = u_lmTransforms[transBase + 0].zw;
        }
    }

    v_alpha = a_texcoord3;
}