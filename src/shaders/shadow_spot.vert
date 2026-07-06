$input a_position, a_indices, a_weight, i_data0, i_data1, i_data2, i_data3
$output v_fragPos

#include <bgfx_shader.sh>

uniform vec4 u_modelParams;
#define u_isInstanced (u_modelParams.x > 0.5)
#define u_isAnimated  (u_modelParams.y > 0.5)
uniform mat4 u_bones[128];

void main()
{
    mat4 modelMat = u_model[0];
    if (u_isInstanced) 
    {
        modelMat = mtxFromCols(i_data0, i_data1, i_data2, i_data3);
    }

    if (u_isAnimated) 
    {
        mat4 skinMat = a_weight.x * u_bones[int(a_indices.x)] 
                     + a_weight.y * u_bones[int(a_indices.y)] 
                     + a_weight.z * u_bones[int(a_indices.z)] 
                     + a_weight.w * u_bones[int(a_indices.w)];
        modelMat = mul(modelMat, skinMat);
    }

    vec4 worldPos = mul(modelMat, vec4(a_position, 1.0));
    v_fragPos = worldPos.xyz;
    gl_Position = mul(u_viewProj, worldPos);
}