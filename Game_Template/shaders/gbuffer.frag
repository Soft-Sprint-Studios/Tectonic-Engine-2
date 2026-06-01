#include "lightmap.glsl"
#include "common.glsl"

layout (location = 0) out vec2 gNormal;
layout (location = 1) out vec4 gAlbedoSpec;
layout (location = 2) out vec4 gLightmap;

in vec2 TexCoord;
in vec2 v_LmCoord;
in vec2 v_LmSize;
in float v_alpha;
in vec3 FragPos;
in mat3 TBN;

uniform sampler2D u_diffuse;
uniform sampler2D u_normal;
uniform sampler2D u_lightmap;
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
uniform float u_pomMinSteps;
uniform float u_pomMaxSteps;
uniform int u_pomRefineSteps;

const vec3 basis0 = vec3(0.81649658, 0.0, 0.57735027);
const vec3 basis1 = vec3(-0.40824829, 0.70710678, 0.57735027);
const vec3 basis2 = vec3(-0.40824829, -0.70710678, 0.57735027);

vec2 ParallaxMapping(sampler2D heightMapSampler, vec2 texCoords, float hScale, vec3 viewDir)
{
    float numLayers = mix(u_pomMaxSteps, u_pomMinSteps, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    float layerDepth = 1.0 / numLayers;

    vec2 p = viewDir.xy * hScale;
    vec2 deltaTexCoords = p / numLayers;

    vec2 currentTexCoords = texCoords;
    float currentLayerDepth = 0.0;
    float currentHeightMapValue = 1.0 - texture(heightMapSampler, currentTexCoords).r;

    for (int i = 0; i < int(numLayers); i++)
    {
        if (currentLayerDepth >= currentHeightMapValue)
        {
            break;
        }

        currentTexCoords -= deltaTexCoords;
        currentLayerDepth += layerDepth;
        currentHeightMapValue = 1.0 - texture(heightMapSampler, currentTexCoords).r;
    }

    vec2 texCoordsStart = currentTexCoords + deltaTexCoords;
    vec2 texCoordsEnd = currentTexCoords;
    float depthStart = currentLayerDepth - layerDepth;
    float depthEnd = currentLayerDepth;

    for (int i = 0; i < u_pomRefineSteps; i++)
    {
        float midDepth = (depthStart + depthEnd) * 0.5;
        vec2 midTexCoords = mix(texCoordsStart, texCoordsEnd, 0.5);
        float midHeightMapValue = 1.0 - texture(heightMapSampler, midTexCoords).r;

        if (midDepth > midHeightMapValue)
        {
            depthEnd = midDepth;
            texCoordsEnd = midTexCoords;
        }
        else
        {
            depthStart = midDepth;
            texCoordsStart = midTexCoords;
        }
    }

    return texCoordsEnd;
}

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

    vec3 N = normalize(TBN[2]);
    vec3 tangentNormal = vec3(0.0, 0.0, 1.0);

    if (u_useBump && u_mat_bumpmap == 1)
    {
        vec3 n1 = texture(u_normal, finalUV).rgb * 2.0 - 1.0;
        vec3 n2 = texture(u_normal2, finalUV).rgb * 2.0 - 1.0;
        tangentNormal = normalize(mix(n1, n2, blend));
        N = normalize(TBN * tangentNormal);
    }
    gNormal = EncodeNormal(N);

    float spec1 = u_useBump ? texture(u_normal, finalUV).a : 0.0;
    float spec2 = u_useBump ? texture(u_normal2, finalUV).a : 0.0;
    float specMask = mix(spec1, spec2, blend);

    gAlbedoSpec = vec4(albedo.rgb, specMask);

    vec4 lmData = vec4(0.0);
    vec2 LmCoord1 = v_LmCoord;
    vec2 LmCoord2 = v_LmCoord + vec2(v_LmSize.x, 0.0);
    vec2 LmCoord3 = v_LmCoord + vec2(0.0, v_LmSize.y);
    vec2 LmCoord4 = v_LmCoord + v_LmSize;

    if (u_useBump)
    {
        vec3 w = max(vec3(0.0), vec3(dot(tangentNormal, basis0), dot(tangentNormal, basis1), dot(tangentNormal, basis2)));
        w /= (w.x + w.y + w.z); 

        vec4 l1 = GetLightmapData(u_lightmap, LmCoord2);
        vec4 l2 = GetLightmapData(u_lightmap, LmCoord3);
        vec4 l3 = GetLightmapData(u_lightmap, LmCoord4);
        
        lmData = l1 * w.x + l2 * w.y + l3 * w.z;
    }
    else
    {
        lmData = GetLightmapData(u_lightmap, LmCoord1);
    }
    gLightmap = lmData;
}