#include "parallax.glsl"
#include "common.glsl"

layout (location = 0) out vec4 gNormal;
layout (location = 1) out vec4 gAlbedoSpec;
layout (location = 2) out vec3 gLightmapUV;

in vec2 TexCoord;
in vec2 v_LmCoord;
in vec2 v_LmSize;
in float v_alpha;
in vec3 FragPos;
in mat3 TBN;

uniform sampler2D u_diffuse;
uniform sampler2D u_normal;
uniform sampler2D u_heightMap;
uniform sampler2D u_diffuse2;
uniform sampler2D u_normal2;
uniform sampler2D u_heightMap2;

uniform bool u_useBump;
uniform vec3 u_viewPos;

uniform int u_mat_bumpmap;
uniform int u_mat_parallax;
uniform float u_heightScale1;
uniform float u_heightScale2;

const vec3 basis0 = vec3(0.81649658, 0.0, 0.57735027);
const vec3 basis1 = vec3(-0.40824829, 0.70710678, 0.57735027);
const vec3 basis2 = vec3(-0.40824829, -0.70710678, 0.57735027);

void main()
{
    float blend = v_alpha;
    vec2 finalUV = TexCoord;

    if (u_mat_parallax == 1)
    {
        vec3 tsViewDir = normalize(transpose(TBN) * (u_viewPos - FragPos));
        
        if (blend > 0.01 && u_heightScale2 > 0.0)
        {
            vec2 uv1 = ParallaxMapping(u_heightMap, TexCoord, u_heightScale1, tsViewDir);
            vec2 uv2 = ParallaxMapping(u_heightMap2, TexCoord, u_heightScale2, tsViewDir);
            finalUV = mix(uv1, uv2, blend);
        }
        else
        {
            finalUV = ParallaxMapping(u_heightMap, TexCoord, u_heightScale1, tsViewDir);
        }
    }

    vec4 alb1 = texture(u_diffuse, finalUV);
    vec4 alb2 = texture(u_diffuse2, finalUV);
    vec4 albedo = mix(alb1, alb2, blend);

    if (albedo.a < 0.1)
    {
        discard;
    }

    vec3 worldNormal = normalize(TBN[2]);
    vec3 tangentNormal = vec3(0.0, 0.0, 1.0);

    if (u_useBump && u_mat_bumpmap == 1)
    {
        vec3 n1 = texture(u_normal, finalUV).rgb * 2.0 - 1.0;
        vec3 n2 = texture(u_normal2, finalUV).rgb * 2.0 - 1.0;
        tangentNormal = normalize(mix(n1, n2, blend));
        worldNormal = normalize(TBN * tangentNormal);
    }

    float spec1 = u_useBump ? texture(u_normal, finalUV).a : 0.0;
    float spec2 = u_useBump ? texture(u_normal2, finalUV).a : 0.0;
    float specMask = mix(spec1, spec2, blend);

    gNormal = vec4(EncodeNormal(worldNormal), tangentNormal.x, u_useBump ? tangentNormal.y : -2.0);
    gAlbedoSpec = vec4(albedo.rgb, specMask);
    gLightmapUV.xy = v_LmCoord;
    gLightmapUV.z = uintBitsToFloat(packHalf2x16(v_LmSize)); 
}