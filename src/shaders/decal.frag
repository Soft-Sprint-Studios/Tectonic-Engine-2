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
    vec2 finalUV = v_texcoord0;

    if (u_parallaxParams.w > 0.5 && u_params.x > 0.0)
    {
        vec3 V = u_viewPos.xyz - v_fragPos;
        vec3 tsViewDir = normalize(vec3(dot(v_tbn0, V), dot(v_tbn1, V), dot(v_tbn2, V)));
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

    vec3 viewDir = normalize(u_viewPos.xyz - v_fragPos);
    if (dot(v_tbn2, viewDir) < 0.0)
    {
        tangentNormal = -tangentNormal;
    }
		
    vec3 worldNormal = normalize(v_tbn0 * tangentNormal.x + v_tbn1 * tangentNormal.y + v_tbn2 * tangentNormal.z);
    vec4 mraoh = texture2D(s_mraohMap, finalUV);

    float packed_tx = tangentNormal.x * 0.5 + 0.5;
    float packed_ty = tangentNormal.y * 0.5 + 0.5;

    gl_FragData[0] = vec4(EncodeNormal(worldNormal), 0.0, 0.0);
    gl_FragData[1] = vec4(albedo.rgb, packed_tx);
    gl_FragData[2] = vec4(mraoh.rgb, packed_ty);
}