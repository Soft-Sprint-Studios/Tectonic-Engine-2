#include "parallax.h"
#include "common.h"
#include "lightmap.h"

layout (location = 0) out vec4 gNormal;
layout (location = 1) out vec4 gAlbedo;
layout (location = 2) out vec4 gMRAO;
layout (location = 3) out vec2 gLightmapUV;

in vec2 TexCoord;
in vec2 v_LmCoord;
in vec2 v_LmSize;
in float v_alpha;
in vec3 FragPos;
in mat3 TBN;

layout(binding = 0) uniform sampler2D u_diffuse;
layout(binding = 1) uniform sampler2D u_normal;
layout(binding = 2) uniform sampler2D u_mraohMap;
layout(binding = 3) uniform sampler2D u_diffuse2;
layout(binding = 4) uniform sampler2D u_normal2;
layout(binding = 5) uniform sampler2D u_mraohMap2;

uniform bool u_useBump;
uniform vec3 u_viewPos;

uniform int u_mat_parallax;
uniform float u_heightScale1;
uniform float u_heightScale2;

void main()
{
    float blend = v_alpha;
    vec2 finalUV = TexCoord;

    if (u_mat_parallax == 1)
    {
        vec3 tsViewDir = normalize(transpose(TBN) * (u_viewPos - FragPos));
        
        if (blend > 0.01 && u_heightScale2 > 0.0)
        {
            vec2 uv1 = ParallaxMapping(u_mraohMap, TexCoord, u_heightScale1, tsViewDir);
            vec2 uv2 = ParallaxMapping(u_mraohMap2, TexCoord, u_heightScale2, tsViewDir);
            finalUV = mix(uv1, uv2, blend);
        }
        else
        {
            finalUV = ParallaxMapping(u_mraohMap, TexCoord, u_heightScale1, tsViewDir);
        }
    }

    vec4 alb1 = texture(u_diffuse, finalUV);
    vec4 alb2 = texture(u_diffuse2, finalUV);
    vec4 albedo = mix(alb1, alb2, blend);

    if (albedo.a < 0.1)
    {
        discard;
    }

    float emissive = 1.0;
    vec3 worldNormal = normalize(TBN[2]);
    vec3 tangentNormal = vec3(0.0, 0.0, 1.0);

    if (u_useBump)
    {
        vec4 norm1 = texture(u_normal, finalUV);
        vec4 norm2 = texture(u_normal2, finalUV);

        vec3 n1 = norm1.rgb * 2.0 - 1.0;
        vec3 n2 = norm2.rgb * 2.0 - 1.0;

        emissive = mix(norm1.a, norm2.a, blend);
        tangentNormal = normalize(mix(n1, n2, blend));
        worldNormal = normalize(TBN * tangentNormal);
    }

    vec4 mraoh1 = texture(u_mraohMap, finalUV);
    vec4 mraoh2 = texture(u_mraohMap2, finalUV);
    vec4 mraoh = mix(mraoh1, mraoh2, blend);

    float packed_tx = tangentNormal.x * 0.5 + 0.5;
    float packed_ty = u_useBump ? (tangentNormal.y * 0.5 + 0.5) : 0.0;

    gNormal = vec4(EncodeNormal(worldNormal), packed_tx, packed_ty);
    gAlbedo = vec4(albedo.rgb, emissive);
    gMRAO = vec4(mraoh.rgb, 1.0);     
    gLightmapUV = PackLightmapUV(v_LmCoord, v_LmSize);
}