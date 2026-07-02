$input v_texcoord0

#include <bgfx_shader.sh>
#include "common.sh"
#include "pbr.sh"
#include "lightmap.sh"

SAMPLER2D(s_gDepth, 0);
SAMPLER2D(s_gNormal, 1);
SAMPLER2D(s_gAlbedo, 2);
SAMPLER2D(s_gMRAO, 3);
SAMPLERCUBE(s_cubemap, 4);
SAMPLER2D(s_lightmap, 5);
SAMPLER2D(s_gLightmapUV, 6);

// #include "lights.sh"

uniform vec4 u_viewPos;
uniform vec4 u_cubemapParams; // x = u_useCubemap
uniform vec4 u_cubemapOrigin;
uniform vec4 u_cubemapMins;
uniform vec4 u_cubemapMaxs;

const vec3 basis0 = vec3(0.81649658, 0.0, 0.57735027);
const vec3 basis1 = vec3(-0.40824829, 0.70710678, 0.57735027);
const vec3 basis2 = vec3(-0.40824829, -0.70710678, 0.57735027);

vec3 ParallaxCorrect(vec3 R, vec3 fragPos, vec3 boxMin, vec3 boxMax, vec3 probePos)
{
    vec3 invR = vec3_splat(1.0) / R;
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
    vec2 gBufferUV = v_texcoord0;
    float depth = texture2D(s_gDepth, gBufferUV).r;

    if (depth >= 1.0)
    {
        discard;
    }

    vec2 ndcXY = vec2(gBufferUV.x * 2.0 - 1.0, (1.0 - gBufferUV.y) * 2.0 - 1.0);
    vec4 clipSpacePos = vec4(ndcXY, depth, 1.0);
    vec4 viewSpacePos = mul(u_invProj, clipSpacePos);
    viewSpacePos /= viewSpacePos.w;
    vec3 fragPos = mul(u_invView, viewSpacePos).xyz;

    vec4 normalData = texture2D(s_gNormal, gBufferUV);
    vec3 N = DecodeNormal(normalData.rg);
    
    vec4 albedoData = texture2D(s_gAlbedo, gBufferUV);
    vec3 albedo = albedoData.rgb;

    vec4 mrao = texture2D(s_gMRAO, gBufferUV);
    
    float metallic = mrao.r;
    float roughness = max(mrao.g, 0.05); 
    float ao = mrao.b;
    float packed_tx = albedoData.a;
    float packed_ty = mrao.a;

    vec2 lmCoord;
    vec2 lmSize;

    vec2 packedFloat = texture2D(s_gLightmapUV, gBufferUV).rg;
    UnpackLightmapUV(packedFloat, lmCoord, lmSize);

    vec2 LmCoord2 = lmCoord + vec2(lmSize.x, 0.0);
    vec2 LmCoord3 = lmCoord + vec2(0.0, lmSize.y);
    vec2 LmCoord4 = lmCoord + lmSize;

    vec4 baseLM = SampleLightmap(s_lightmap, lmCoord);
    vec3 tsN = vec3(packed_tx * 2.0 - 1.0, packed_ty * 2.0 - 1.0, 0.0);
    tsN.z = sqrt(max(0.0, 1.0 - dot(tsN.xy, tsN.xy)));

    vec3 w = max(vec3_splat(0.0), vec3(dot(tsN, basis0), dot(tsN, basis1), dot(tsN, basis2)));
    w /= max(w.x + w.y + w.z, 0.0001);

    vec3 bumped = SampleLightmap(s_lightmap, LmCoord2).rgb * w.x + SampleLightmap(s_lightmap, LmCoord3).rgb * w.y + SampleLightmap(s_lightmap, LmCoord4).rgb * w.z;

    vec4 lightmapData = vec4(mix(baseLM.rgb, bumped, step(0.02, packed_ty)), baseLM.a);

    vec3 viewDir = normalize(u_viewPos.xyz - fragPos);
    vec3 F0 = vec3_splat(0.04); 
    F0 = mix(F0, albedo, metallic);

    vec3 irradiance = lightmapData.rgb * 2.0;
    vec3 F_ambient = FresnelSchlickRoughness(max(dot(N, viewDir), 0.0), F0, roughness);
    vec3 kS_ambient = F_ambient;
    vec3 kD_ambient = vec3_splat(1.0) - kS_ambient;
    kD_ambient *= 1.0 - metallic;	
    
    vec3 ambientDiffuse = irradiance * albedo * kD_ambient;
    vec3 ambientSpecular = vec3_splat(0.0);

    // Unpack custom directional data (if map has it)
    float dirX = texture2D(s_lightmap, lmCoord).a * 2.0 - 1.0;
    float dirY = texture2D(s_lightmap, LmCoord2).a * 2.0 - 1.0;
    float dirZ = texture2D(s_lightmap, LmCoord3).a * 2.0 - 1.0;

    vec3 worldLightDir = normalize(vec3(dirX, dirZ, -dirY));
    float lightmapLuminance = dot(irradiance, vec3_splat(0.333));

    // Specular baked reflection calculations
    vec3 H_spec = normalize(worldLightDir + viewDir);
    float NDF = DistributionGGX(N, H_spec, roughness);
    float G   = GeometrySmith(N, viewDir, worldLightDir, roughness);
    vec3 F    = FresnelSchlick(max(dot(H_spec, viewDir), 0.0), F0);
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, viewDir), 0.0) * max(dot(N, worldLightDir), 0.0) + 0.0001;
    vec3 specularBaked = (numerator / denominator) * irradiance * max(dot(N, worldLightDir), 0.0);
    specularBaked *= lightmapLuminance;

    if (u_cubemapParams.x > 0.5) 
    {
        vec3 R = reflect(-viewDir, N);
        vec3 lookup = ParallaxCorrect(R, fragPos, u_cubemapMaxs.xyz, u_cubemapMaxs.xyz, u_cubemapOrigin.xyz);

        vec3 prefilteredColor = textureCubeLod(s_cubemap, lookup, roughness * 5.0).rgb; 
        vec2 envBRDF = vec2(1.0 - roughness, roughness); 
        
        ambientSpecular = prefilteredColor * (F_ambient * envBRDF.x + envBRDF.y) * lightmapLuminance;
    }
    
    vec3 ambient = (ambientDiffuse + ambientSpecular + specularBaked) * ao;

    vec3 dynDiffuse = vec3_splat(0.0);

    /*
    // Dynamic Spots
    for (int i = 0; i < u_numSpotLights; ++i)
    {
        vec3 lPos = u_spotLights[i].posRadius.xyz;
        float lRad = u_spotLights[i].posRadius.w;
        vec3 L = normalize(lPos - fragPos);
        float dist = length(lPos - fragPos);

        if (dist > lRad) 
            continue;

        float theta = dot(L, normalize(-u_spotLights[i].dirInner.xyz));
        float epsilon = u_spotLights[i].dirInner.w - u_spotLights[i].shadowData.x;
        float intensity = clamp((theta - u_spotLights[i].shadowData.x) / epsilon, 0.0, 1.0);
        float attenuation = 1.0 - (dist / lRad);

        float shadow = SpotShadowCalc(fragPos, lPos, lRad, u_spotLights[i].lightSpace, u_spotLights[i].shadowLayer);
        vec3 lightEnergy = u_spotLights[i].colorVol.rgb * intensity * attenuation * (1.0 - shadow);

        vec3 H = normalize(L + viewDir);
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, viewDir, L, roughness);
        vec3 F    = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, viewDir), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;
        vec3 kS = F;
        vec3 kD = vec3_splat(1.0) - kS;
        kD *= 1.0 - metallic;
        dynDiffuse += (kD * albedo / PI + specular) * lightEnergy * max(dot(N, L), 0.0);
    }

    // Dynamic Points
    for (int i = 0; i < u_numPointLights; ++i)
    {
        vec3 lPos = u_pointLights[i].posRadius.xyz;
        float lRad = u_pointLights[i].posRadius.w;
        vec3 L = normalize(lPos - fragPos);
        float dist = length(lPos - fragPos);

        if (dist > lRad) 
            continue;

        float attenuation = 1.0 - (dist / lRad);
        float shadow = PointShadowCalc(fragPos, lPos, lRad, u_pointLights[i].shadowLayer);
        vec3 lightEnergy = u_pointLights[i].colorVol.rgb * attenuation * (1.0 - shadow);

        vec3 H = normalize(L + viewDir);
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, viewDir, L, roughness);
        vec3 F    = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, viewDir), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;
        vec3 kS = F;
        vec3 kD = vec3_splat(1.0) - kS;
        kD *= 1.0 - metallic;
        dynDiffuse += (kD * albedo / PI + specular) * lightEnergy * max(dot(N, L), 0.0);
    }
    
    // CSM Cascaded Shadows
    if (u_sunColorEn.w > 0.5)
    {
        vec3 sunL = normalize(u_sunDirVol.xyz);
        float sunMask = lightmapData.a;
        float sunShadow = CalculateSunShadow(fragPos, u_view, u_csmArray);
        vec3 sunEnergy = u_sunColorEn.rgb * (1.0 - sunShadow) * sunMask;

        vec3 H = normalize(sunL + viewDir);
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, viewDir, sunL, roughness);
        vec3 F    = FresnelSchlick(max(dot(H, viewDir), 0.0), F0);
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, viewDir), 0.0) * max(dot(N, sunL), 0.0) + 0.0001;
        vec3 specular     = numerator / denominator;
        vec3 kS = F;
        vec3 kD = vec3_splat(1.0) - kS;
        kD *= 1.0 - metallic;
        dynDiffuse += (kD * albedo / PI + specular) * sunEnergy * max(dot(N, sunL), 0.0);
    }
    */

    vec3 finalColor = ambient + dynDiffuse;
    gl_FragColor = vec4(finalColor, 1.0);
}