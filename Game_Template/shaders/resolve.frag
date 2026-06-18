#include "lights.h"
#include "common.h"
#include "pbr.h"
#include "lightmap.h"

out vec4 FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D u_gDepth;
layout(binding = 1) uniform sampler2D u_gNormal;
layout(binding = 2) uniform sampler2D u_gAlbedo;
layout(binding = 3) uniform sampler2D u_gMRAO;
layout(binding = 4) uniform samplerCube u_cubemap;
layout(binding = 5) uniform sampler2D u_lightmap;
layout(binding = 6) uniform sampler2D u_gLightmapUV;

uniform bool u_useCubemap;
uniform vec3 u_cubemapOrigin;
uniform vec3 u_cubemapMins;
uniform vec3 u_cubemapMaxs;

uniform vec3 u_viewPos;
uniform mat4 u_view;
uniform mat4 u_invProjection;
uniform mat4 u_invView;

const vec3 basis0 = vec3(0.81649658, 0.0, 0.57735027);
const vec3 basis1 = vec3(-0.40824829, 0.70710678, 0.57735027);
const vec3 basis2 = vec3(-0.40824829, -0.70710678, 0.57735027);

vec3 ParallaxCorrect(vec3 R, vec3 fragPos, vec3 boxMin, vec3 boxMax, vec3 probePos)
{
    vec3 invR = 1.0 / R;
    vec3 t1 = (boxMin - fragPos) * invR;
    vec3 t2 = (boxMax - fragPos) * invR;
    vec3 tmin = min(t1, t2);
    vec3 tmax = max(t1, t2);
    float t_near = max(max(tmin.x, tmin.y), tmin.z);
    float t_far = min(min(tmax.x, tmax.y), tmax.z);
    
    if (t_near > t_far || t_far < 0.0)
    {
        return R;
    }
    
    float intersection_t = t_far;
    vec3 intersectPos = fragPos + R * intersection_t;
    return normalize(intersectPos - probePos);
}

vec3 CalculateDynamicLightPBR(vec3 L, vec3 V, vec3 N, vec3 F0, vec3 albedo, float metallic, float roughness, vec3 lightEnergy)
{
    vec3 H = normalize(L + V);
    
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    
    return (kD * albedo / PI + specular) * lightEnergy * NdotL;
}

vec3 CalculateDynamicLightSpecularPBR(vec3 L, vec3 V, vec3 N, vec3 F0, float roughness, vec3 lightEnergy)
{
    vec3 H = normalize(L + V);
    
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);
    
    return specular * lightEnergy * NdotL;
}


void main()
{
    vec2 gBufferUV = gl_FragCoord.xy / textureSize(u_gDepth, 0).xy;
    float depth = texture(u_gDepth, gBufferUV).r;

    if (depth >= 1.0)
    {
        discard;
    }

    vec4 clipSpacePos = vec4(gBufferUV * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpacePos = u_invProjection * clipSpacePos;
    viewSpacePos /= viewSpacePos.w;
    vec3 fragPos = (u_invView * viewSpacePos).xyz;

    vec4 normalData = texture(u_gNormal, gBufferUV);
    vec3 N = DecodeNormal(normalData.rg);
    
    vec3 albedo = texture(u_gAlbedo, gBufferUV).rgb;
    vec3 mrao = texture(u_gMRAO, gBufferUV).rgb;
    
    float metallic = mrao.r;
    float roughness = max(mrao.g, 0.05); 
    float ao = mrao.b;

    vec2 lmCoord = texture(u_gLightmapUV, gBufferUV).rg;
    float w_pixels = texture(u_gAlbedo, gBufferUV).a * 255.0;
    float h_pixels = texture(u_gMRAO, gBufferUV).a * 255.0;
    vec2 lmSize = vec2(w_pixels, h_pixels) / float(LIGHTMAP_ATLAS_SIZE);

    vec4 lightmapData = vec4(0.0);
	vec2 LmCoord2 = lmCoord + vec2(lmSize.x, 0.0);
    vec2 LmCoord3 = lmCoord + vec2(0.0, lmSize.y);
    vec2 LmCoord4 = lmCoord + lmSize;

    if (normalData.a > 0.02)
    {
        float tx = normalData.z * 2.0 - 1.0;
        float ty = normalData.w * 2.0 - 1.0;
        vec3 tsNormal = vec3(tx, ty, sqrt(max(0.0, 1.0 - (tx * tx + ty * ty))));

        vec3 w = max(vec3(0.0), vec3(dot(tsNormal, basis0), dot(tsNormal, basis1), dot(tsNormal, basis2)));

        float sumW = w.x + w.y + w.z;
        w /= max(sumW, 0.0001); 

        lightmapData = SampleLightmap(u_lightmap, LmCoord2) * w.x + SampleLightmap(u_lightmap, LmCoord3) * w.y + SampleLightmap(u_lightmap, LmCoord4) * w.z;
    }
    else
    {
        lightmapData = SampleLightmap(u_lightmap, lmCoord);
    }

    vec3 viewDir = normalize(u_viewPos - fragPos);
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    vec3 irradiance = lightmapData.rgb * 2.0;
    vec3 F_ambient = FresnelSchlickRoughness(max(dot(N, viewDir), 0.0), F0, roughness);
    vec3 kS_ambient = F_ambient;
    vec3 kD_ambient = 1.0 - kS_ambient;
    kD_ambient *= 1.0 - metallic;	
    
    vec3 ambientDiffuse = irradiance * albedo * kD_ambient;
    vec3 ambientSpecular = vec3(0.0);

    // Unpack custom directional data (if map has it)
    float dirX = texture(u_lightmap, lmCoord).a * 2.0 - 1.0;
    float dirY = texture(u_lightmap, LmCoord2).a * 2.0 - 1.0;
    float dirZ = texture(u_lightmap, LmCoord3).a * 2.0 - 1.0;

    vec3 worldLightDir = normalize(vec3(dirX, dirZ, -dirY));
    float lightmapLuminance = dot(irradiance, vec3(0.333));

    vec3 specularBaked = CalculateDynamicLightSpecularPBR(worldLightDir, viewDir, N, F0, roughness, irradiance);
	specularBaked *= lightmapLuminance;

    if (u_useCubemap) 
    {
        vec3 R = reflect(-viewDir, N);
        vec3 lookup = ParallaxCorrect(R, fragPos, u_cubemapMins, u_cubemapMaxs, u_cubemapOrigin);

        vec3 prefilteredColor = textureLod(u_cubemap, lookup, roughness * 5.0).rgb; 
        vec2 envBRDF = vec2(1.0 - roughness, roughness); 
        
        ambientSpecular = prefilteredColor * (F_ambient * envBRDF.x + envBRDF.y) * lightmapLuminance;
    }
    
    vec3 ambient = (ambientDiffuse + ambientSpecular + specularBaked) * ao;

    vec3 dynDiffuse = vec3(0.0);

    // Dynamic Spots
    for (int i = 0; i < u_numSpotLights; ++i)
    {
        vec3 lPos = u_spotLights[i].posRadius.xyz;
        float lRad = u_spotLights[i].posRadius.w;
        vec3 L = normalize(lPos - fragPos);
        float dist = length(lPos - fragPos);

        if (dist > lRad) 
        {
            continue;
        }

        float theta = dot(L, normalize(-u_spotLights[i].dirInner.xyz));
        float epsilon = u_spotLights[i].dirInner.w - u_spotLights[i].shadowData.x;
        float intensity = clamp((theta - u_spotLights[i].shadowData.x) / epsilon, 0.0, 1.0);
        float attenuation = 1.0 - (dist / lRad);

        float shadow = SpotShadowCalc(fragPos, lPos, lRad, u_spotLights[i].lightSpace, u_spotLights[i].shadowHandle);
        vec3 lightEnergy = u_spotLights[i].colorVol.rgb * intensity * attenuation * (1.0 - shadow);

        dynDiffuse += CalculateDynamicLightPBR(L, viewDir, N, F0, albedo, metallic, roughness, lightEnergy);
    }

    // Dynamic Points
    for (int i = 0; i < u_numPointLights; ++i)
    {
        vec3 lPos = u_pointLights[i].posRadius.xyz;
        float lRad = u_pointLights[i].posRadius.w;
        vec3 L = normalize(lPos - fragPos);
        float dist = length(lPos - fragPos);

        if (dist > lRad) 
        {
            continue;
        }

        float attenuation = 1.0 - (dist / lRad);
        float shadow = PointShadowCalc(fragPos, lPos, lRad, u_pointLights[i].shadowHandle);
        vec3 lightEnergy = u_pointLights[i].colorVol.rgb * attenuation * (1.0 - shadow);

        dynDiffuse += CalculateDynamicLightPBR(L, viewDir, N, F0, albedo, metallic, roughness, lightEnergy);
    }
    
    // CSM Cascaded Shadows
    if (u_csmEnabled == 1)
    {
        vec3 sunL = normalize(u_sunDir);
        float sunMask = lmSize.x > 0.0 ? lightmapData.a : 1.0;
        float sunShadow = CalculateSunShadow(fragPos, u_view, u_csmSplits, u_csmMatrices, u_csmArray);
        vec3 sunEnergy = u_sunColor * (1.0 - sunShadow) * sunMask;

        dynDiffuse += CalculateDynamicLightPBR(sunL, viewDir, N, F0, albedo, metallic, roughness, sunEnergy);
    }

    vec3 finalColor = ambient + dynDiffuse;
    FragColor = vec4(finalColor, 1.0);
}