$input v_texcoord0, v_fragPos, v_tbn0, v_tbn1, v_tbn2

#include <bgfx_shader.sh>
#include "common.sh"
#include "parallax.sh"

SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_normal, 1);
SAMPLER2D(s_mraohMap, 2);

uniform vec4 u_viewPos;
uniform vec4 u_params;

void main()
{
    mat3 TBN = mat3(v_tbn0, v_tbn1, v_tbn2);
    vec2 finalUV = v_texcoord0;

    if (u_parallaxParams.w > 0.5 && u_params.x > 0.0)
    {
        vec3 tsViewDir = normalize(mul(transpose(TBN), (u_viewPos.xyz - v_fragPos)));
        finalUV = ParallaxMapping(s_mraohMap, v_texcoord0, u_params.x, tsViewDir);
    }

    vec4 albedo = texture2D(s_diffuse, finalUV);

    if (albedo.a < 0.1)
    {
        discard;
    }

    vec4 normalSample = texture2D(s_normal, finalUV);
    vec3 tsSample = normalSample.xyz * 2.0 - 1.0;
    vec3 tangentNormal = normalize(tsSample);
		
    vec3 worldNormal = normalize(mul(TBN, tangentNormal));
    vec4 mraoh = texture2D(s_mraohMap, finalUV);

    float packed_tx = tangentNormal.x * 0.5 + 0.5;
    float packed_ty = tangentNormal.y * 0.5 + 0.5;

    gl_FragData[0] = vec4(EncodeNormal(worldNormal), 0.0, 0.0);
    gl_FragData[1] = vec4(albedo.rgb, packed_tx);
    gl_FragData[2] = vec4(mraoh.rgb, packed_ty);
}