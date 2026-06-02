#include "lightmap.glsl"
#include "lights.glsl"
#include "common.glsl"

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D u_gDepth;
uniform sampler2D u_gNormal;
uniform sampler2D u_gAlbedoSpec;
uniform sampler2D u_gLightmapUV;

uniform sampler2D u_lightmap;

uniform samplerCube u_cubemap;
uniform bool u_useCubemap;
uniform vec3 u_cubemapOrigin;
uniform vec3 u_cubemapMins;
uniform vec3 u_cubemapMaxs;

uniform vec3 u_viewPos;
uniform mat4 u_view;
uniform mat4 u_invProjection;
uniform mat4 u_invView;
uniform vec2 u_gBufferScale;

uniform int u_mat_specular;

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

void main()
{
    vec2 gBufferUV = TexCoords * u_gBufferScale;
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
    vec4 albedoSpec = texture(u_gAlbedoSpec, gBufferUV);
    vec4 lmUV = texture(u_gLightmapUV, gBufferUV);

    vec4 lightmapData = vec4(0.0);

    if (normalData.w > -1.0)
    {
        vec3 tsNormal = vec3(normalData.zw, sqrt(max(0.0, 1.0 - dot(normalData.zw, normalData.zw))));

        vec2 LmCoord2 = lmUV.xy + vec2(lmUV.z, 0.0);
        vec2 LmCoord3 = lmUV.xy + vec2(0.0, lmUV.w);
        vec2 LmCoord4 = lmUV.xy + lmUV.zw;

        vec3 w = max(vec3(0.0), vec3(dot(tsNormal, basis0), dot(tsNormal, basis1), dot(tsNormal, basis2)));

        float sumW = w.x + w.y + w.z;
        w /= max(sumW, 0.0001); 

        lightmapData = GetLightmapData(u_lightmap, LmCoord2) * w.x + GetLightmapData(u_lightmap, LmCoord3) * w.y + GetLightmapData(u_lightmap, LmCoord4) * w.z;
    }
    else
    {
        lightmapData = GetLightmapData(u_lightmap, lmUV.xy);
    }

    vec3 albedo = albedoSpec.rgb;
    vec3 specMask = vec3(albedoSpec.a);
    if (u_mat_specular == 0) 
        specMask = vec3(0.0);

    vec3 viewDir = normalize(u_viewPos - fragPos);
    float shine = 32.0;

    vec3 diffuseLight = lightmapData.rgb;
    vec3 specularLight = vec3(0.0);

    // Baked Specular
    vec3 dominantL = N;
    vec3 H_baked = normalize(dominantL + viewDir);
    float spec_baked = pow(max(dot(N, H_baked), 0.0), shine);
    specularLight = diffuseLight * spec_baked * specMask;

    // Environmental Reflection
    if (u_useCubemap)
    {
        vec3 reflectViewDir = reflect(-viewDir, N);
        vec3 lookup = ParallaxCorrect(reflectViewDir, fragPos, u_cubemapMins, u_cubemapMaxs, u_cubemapOrigin);
        vec3 envMap = texture(u_cubemap, lookup).rgb;
        float brightness = dot(diffuseLight, vec3(0.2126, 0.7152, 0.0722));
        specularLight += (envMap * specMask * brightness);
    }

    // Dynamic Lighting
    vec3 dynDiffuse = vec3(0.0);
    vec3 dynSpecular = vec3(0.0);

    // Dynamic Spots
    for (int i = 0; i < u_numSpotLights; ++i)
    {
        vec3 L = normalize(u_spotLights[i].pos - fragPos);
        float dist = length(u_spotLights[i].pos - fragPos);
        if (dist > u_spotLights[i].radius) 
            continue;

        float theta = dot(L, normalize(-u_spotLights[i].dir));
        float epsilon = u_spotLights[i].innerAngle - u_spotLights[i].outerAngle;
        float intensity = clamp((theta - u_spotLights[i].outerAngle) / epsilon, 0.0, 1.0);
        float attenuation = 1.0 - (dist / u_spotLights[i].radius);

        float shadow = SpotShadowCalc(fragPos, u_spotLights[i].pos, u_spotLights[i].radius, u_spotLights[i].lightSpaceMatrix, u_spotShadowMaps[i]);
        vec3 lightEnergy = u_spotLights[i].color * intensity * attenuation * (1.0 - shadow);

        dynDiffuse += lightEnergy * max(dot(N, L), 0.0);
        vec3 H = normalize(L + viewDir);
        dynSpecular += lightEnergy * pow(max(dot(N, H), 0.0), shine) * specMask * 0.5;
    }

    // Dynamic Points
    for (int i = 0; i < u_numPointLights; ++i)
    {
        vec3 L = normalize(u_pointLights[i].pos - fragPos);
        float dist = length(u_pointLights[i].pos - fragPos);
        if (dist > u_pointLights[i].radius) 
            continue;

        float attenuation = 1.0 - (dist / u_pointLights[i].radius);
        float shadow = PointShadowCalc(fragPos, u_pointLights[i].pos, u_pointLights[i].radius, u_pointShadowMaps[i]);
        vec3 lightEnergy = u_pointLights[i].color * attenuation * (1.0 - shadow);

        dynDiffuse += lightEnergy * max(dot(N, L), 0.0);
        vec3 H = normalize(L + viewDir);
        dynSpecular += lightEnergy * pow(max(dot(N, H), 0.0), shine) * specMask * 0.5;
    }
    
    // CSM Cascaded Shadows
    vec3 sunFinal = vec3(0.0);
    if (u_csmEnabled == 1)
    {
        vec3 sunL = normalize(u_sunDir);
        float sunMask = lmUV.z > 0.0 ? lightmapData.a : 1.0;
        float sunShadow = CalculateSunShadow(fragPos, u_view, u_csmSplits, u_csmMatrices, u_csmArray);
        vec3 sunEnergy = u_sunColor * (1.0 - sunShadow) * sunMask;

        sunFinal = sunEnergy * max(dot(N, sunL), 0.0);
        vec3 H = normalize(sunL + viewDir);
        dynSpecular += sunEnergy * pow(max(dot(N, H), 0.0), shine) * specMask * 0.5;
    }

    vec3 finalDiffuse = albedo * (diffuseLight * 2.0 + dynDiffuse + sunFinal);
    vec3 finalSpecular = specularLight + dynSpecular;

    FragColor = vec4(finalDiffuse + finalSpecular, 1.0);
}