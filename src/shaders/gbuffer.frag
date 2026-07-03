$input v_texcoord0, v_lmCoord, v_lmSize, v_alpha, v_fragPos, v_tbn0, v_tbn1, v_tbn2

#include <bgfx_shader.sh>
#include "common.sh"
#include "parallax.sh"
#include "lightmap.sh"

SAMPLER2D(s_diffuse, 0);
SAMPLER2D(s_normal, 1);
SAMPLER2D(s_mraohMap, 2);
SAMPLER2D(s_diffuse2, 3);
SAMPLER2D(s_normal2, 4);
SAMPLER2D(s_mraohMap2, 5);

uniform vec4 u_viewPos; 
uniform vec4 u_bumpAndHeights;

void main()
{
    float blend = v_alpha;
    vec2 finalUV = v_texcoord0;

    mat3 TBN = mat3(v_tbn0, v_tbn1, v_tbn2);

    if (u_parallaxParams.w > 0.5) 
    {
        vec3 tsViewDir = normalize(mul(transpose(TBN), (u_viewPos.xyz - v_fragPos)));
        
        if (blend > 0.01 && u_bumpAndHeights.z > 0.0)
        {
            vec2 uv1 = ParallaxMapping(s_mraohMap, v_texcoord0, u_bumpAndHeights.y, tsViewDir);
            vec2 uv2 = ParallaxMapping(s_mraohMap2, v_texcoord0, u_bumpAndHeights.z, tsViewDir);
            finalUV = mix(uv1, uv2, blend);
        }
        else if (u_bumpAndHeights.y > 0.0)
        {
            finalUV = ParallaxMapping(s_mraohMap, v_texcoord0, u_bumpAndHeights.y, tsViewDir);
        }
    }

    vec4 alb1 = texture2D(s_diffuse, finalUV);
    vec4 alb2 = texture2D(s_diffuse2, finalUV);
    vec4 albedo = mix(alb1, alb2, blend);

    if (albedo.a < 0.1)
    {
        discard;
    }

    vec3 worldNormal = normalize(v_tbn2);
    vec3 tangentNormal = vec3(0.0, 0.0, 1.0);

    if (u_bumpAndHeights.x > 0.5)
    {
        vec4 norm1 = texture2D(s_normal, finalUV);
        vec4 norm2 = texture2D(s_normal2, finalUV);

        vec3 n1 = norm1.xyz * 2.0 - 1.0;
        vec3 n2 = norm2.xyz * 2.0 - 1.0;

        tangentNormal = normalize(mix(n1, n2, blend));
        worldNormal = normalize(mul(TBN, tangentNormal));
    }

    vec4 mraoh1 = texture2D(s_mraohMap, finalUV);
    vec4 mraoh2 = texture2D(s_mraohMap2, finalUV);
    vec4 mraoh = mix(mraoh1, mraoh2, blend);

    float packed_tx = tangentNormal.x * 0.5 + 0.5;
    float packed_ty = (u_bumpAndHeights.x > 0.5) ? (tangentNormal.y * 0.5 + 0.5) : 0.0;

    gl_FragData[0] = vec4(EncodeNormal(worldNormal), 0.0, 0.0);
    gl_FragData[1] = vec4(albedo.rgb, packed_tx);
    gl_FragData[2] = vec4(mraoh.rgb, packed_ty);
    gl_FragData[3] = vec4(PackLightmapUV(v_lmCoord, v_lmSize), 0.0, 0.0);
}