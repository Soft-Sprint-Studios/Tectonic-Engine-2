in centroid vec2 TexCoord;
in centroid vec2 LmCoord1;
in centroid vec2 LmCoord2;
in centroid vec2 LmCoord3;
in centroid vec2 LmCoord4;
in centroid vec2 LmCoord5;
in centroid vec4 Color;
in centroid vec4 Color2;
in centroid vec4 Color3;
in vec3 FragPos;
in centroid mat3 TBN;

uniform sampler2D u_diffuse;
uniform sampler2D u_normal;
uniform sampler2D u_lightmap;
uniform sampler2D u_specular;
uniform sampler2D u_heightMap;
uniform sampler2D u_diffuse2;
uniform sampler2D u_normal2;
uniform sampler2D u_specular2;
uniform sampler2D u_heightMap2;
uniform samplerCube u_cubemap;
uniform bool u_useCubemap;
uniform vec3 u_cubemapOrigin;
uniform vec3 u_cubemapMins;
uniform vec3 u_cubemapMaxs;
uniform bool u_useBump;
uniform bool u_isModel;
uniform vec3 u_viewPos;
uniform int u_debugMode;
uniform int u_fullbright;
uniform mat4 u_view;

uniform int u_mat_specular;
uniform int u_mat_bumpmap;

uniform int u_mat_parallax;
uniform float u_heightScale1;
uniform float u_heightScale2;
uniform float u_pomMinSteps;
uniform float u_pomMaxSteps;
uniform int u_pomRefineSteps;

// Dynamic point lights
struct PointLight 
{
    vec3 pos; 
    vec3 color; 
    float radius;
    float volumetricIntensity;
    int volumetricSteps;
};

// Dynamic spot lights
struct SpotLight 
{
    vec3 pos; 
    vec3 dir; 
    vec3 color; 
    float radius;
    float innerAngle; 
    float outerAngle; 
    mat4 lightSpaceMatrix;
    float volumetricIntensity;
    int volumetricSteps;
};

uniform int u_numPointLights;
uniform PointLight u_pointLights[4];
uniform samplerCube u_pointShadowMaps[4];

uniform int u_numSpotLights;
uniform SpotLight u_spotLights[4];
uniform sampler2D u_spotShadowMaps[4];

uniform int u_csmEnabled;
uniform sampler2DArray u_csmArray;
uniform mat4 u_csmMatrices[4];
uniform float u_csmSplits[5];
uniform vec3 u_sunColor;
uniform vec3 u_sunDir;

out vec4 FragColor;

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

float ChebyshevUpperBound(vec2 moments, float t)
{
    if (t <= moments.x)
    {
        return 1.0;
    }

    float mean = moments.x;
    float meanSquared = mean * mean;

    float variance = moments.y - meanSquared;

    variance = max(variance, 0.000002);

    float d = t - mean;

    float pMax = variance / (variance + d * d);

    return pMax;
}

float SpotShadowCalc(vec4 fragPosLightSpace, sampler2D shadowMap)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
    {
        return 0.0;
    }

    vec2 moments = texture(shadowMap, projCoords.xy).rg;

    float shadow = ChebyshevUpperBound(moments, projCoords.z);

    return 1.0 - shadow;
}

float PointShadowCalc(vec3 fragPos, vec3 lightPos, float far_plane, samplerCube shadowMap)
{
    vec3 fragToLight = fragPos - lightPos;

    float distanceToLight = length(fragToLight);
    float currentDepth = distanceToLight / far_plane;

    vec2 moments = texture(shadowMap, fragToLight).rg;

    float shadow = ChebyshevUpperBound(moments, currentDepth);

    return 1.0 - shadow;
}

float CalculateSunShadow(vec3 fragPosWorld, vec3 N, vec3 L)
{
    float depth = abs((u_view * vec4(fragPosWorld, 1.0)).z);
    int layer = -1;

    for (int i = 0; i < 4; i++)
    {
        if (depth < u_csmSplits[i + 1])
        {
            layer = i;
            break;
        }
    }

    if (layer == -1)
    {
        return 0.0;
    }

    float offsetScale = 0.05 * (1.0 - dot(N, L));
    vec3 offsetPos = fragPosWorld + N * offsetScale;
    vec4 fragPosLightSpace = u_csmMatrices[layer] * vec4(offsetPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
    {
        return 0.0;
    }

    float shadow = 0.0;

    vec2 texelSize = 1.0 / vec2(textureSize(u_csmArray, 0));

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(u_csmArray, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;

            if (projCoords.z > pcfDepth)
            {
                shadow += 1.0;
            }
        }
    }

    return shadow / 9.0;
}

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
            break;

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
    float blend = u_isModel ? 0.0 : Color.r;

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

    vec3 spec1 = texture(u_specular, finalUV).rgb;
    vec3 spec2 = texture(u_specular2, finalUV).rgb;
    vec3 specMask = mix(spec1, spec2, blend);

    if (u_mat_specular == 0) 
        specMask = vec3(0.0);
    vec3 baseNorm = normalize(TBN[2]);
    vec3 viewDir = normalize(u_viewPos - FragPos);
    float shine = 32.0;

    vec3 N = baseNorm;

	vec3 tangentNormal = vec3(0.0, 0.0, 1.0);
    if (u_useBump && u_mat_bumpmap == 1)
    {
        vec3 n1 = texture(u_normal, finalUV).rgb * 2.0 - 1.0;
        vec3 n2 = texture(u_normal2, finalUV).rgb * 2.0 - 1.0;
        tangentNormal = normalize(mix(n1, n2, blend));
        N = normalize(TBN * tangentNormal);
    }

	vec3 reflectViewDir = reflect(-viewDir, baseNorm);

    vec3 diffuseLight = vec3(0.0);
    vec3 specularLight = vec3(0.0);

    // Baked Lighting Phase
    if (u_useBump)
    {
        float w1 = clamp(dot(tangentNormal, basis0), 0.0, 1.0);
        float w2 = clamp(dot(tangentNormal, basis1), 0.0, 1.0);
        float w3 = clamp(dot(tangentNormal, basis2), 0.0, 1.0);

        if (u_isModel)
        {
            diffuseLight = Color.rgb * w1 + Color2.rgb * w2 + Color3.rgb * w3;
        }
        else
        {
            vec3 l1 = texture(u_lightmap, LmCoord2).rgb;
            vec3 l2 = texture(u_lightmap, LmCoord3).rgb;
            vec3 l3 = texture(u_lightmap, LmCoord4).rgb;
            diffuseLight = l1 * w1 + l2 * w2 + l3 * w3;
        }

        vec3 light1 = u_isModel ? Color.rgb : texture(u_lightmap, LmCoord2).rgb;
        vec3 light2 = u_isModel ? Color2.rgb : texture(u_lightmap, LmCoord3).rgb;
        vec3 light3 = u_isModel ? Color3.rgb : texture(u_lightmap, LmCoord4).rgb;

        vec3 H1 = normalize(TBN * basis0 + viewDir);
        vec3 H2 = normalize(TBN * basis1 + viewDir);
        vec3 H3 = normalize(TBN * basis2 + viewDir);

        float s1 = pow(max(dot(N, H1), 0.0), shine);
        float s2 = pow(max(dot(N, H2), 0.0), shine);
        float s3 = pow(max(dot(N, H3), 0.0), shine);

        specularLight = (light1 * s1 + light2 * s2 + light3 * s3) * specMask;
    }
    else
    {
        diffuseLight = u_isModel ? Color.rgb : texture(u_lightmap, LmCoord1).rgb;
    }

    // Environmental Lighting Phase
    if (u_useCubemap)
    {
        vec3 lookup = ParallaxCorrect(reflectViewDir, FragPos, u_cubemapMins, u_cubemapMaxs, u_cubemapOrigin);

        if (u_useBump)
        {
            lookup = normalize(lookup + TBN * (tangentNormal * 0.2));
        }

        vec3 envMap = texture(u_cubemap, lookup).rgb;

        float brightness = dot(diffuseLight, vec3(0.2126, 0.7152, 0.0722));
        specularLight += (envMap * specMask * brightness);
    }

    // Dynamic Lighting Phase
    vec3 dynDiffuse = vec3(0.0);
    vec3 dynSpecular = vec3(0.0);
    
    // Add Dynamic Spots
    for (int i = 0; i < u_numSpotLights; ++i)
    {
        vec3 L = normalize(u_spotLights[i].pos - FragPos);
        float dist = length(u_spotLights[i].pos - FragPos);
        
        if (dist > u_spotLights[i].radius)
        {
            continue;
        }

        float theta = dot(L, normalize(-u_spotLights[i].dir)); 
        float epsilon = u_spotLights[i].innerAngle - u_spotLights[i].outerAngle;
        float intensity = clamp((theta - u_spotLights[i].outerAngle) / epsilon, 0.0, 1.0);
        float attenuation = 1.0 - (dist / u_spotLights[i].radius);

        float shadow = SpotShadowCalc(u_spotLights[i].lightSpaceMatrix * vec4(FragPos, 1.0), u_spotShadowMaps[i]);

        vec3 lightEnergy = u_spotLights[i].color * intensity * attenuation * (1.0 - shadow);

        float diff = max(dot(baseNorm, L), 0.0);
        if (u_useBump) 
            diff *= clamp(dot(N, L) * 1.4, 0.5, 1.5);
        
        vec3 H = normalize(L + viewDir);
        vec3 specN = u_useBump ? normalize(baseNorm + TBN * (tangentNormal * 0.4)) : N;
        float spec = pow(max(dot(specN, H), 0.0), shine);

        dynDiffuse += lightEnergy * diff;
        dynSpecular += lightEnergy * spec * specMask * 0.5;
    }

    // Add Dynamic Points
    for (int i = 0; i < u_numPointLights; ++i)
    {
        vec3 L = normalize(u_pointLights[i].pos - FragPos);
        float dist = length(u_pointLights[i].pos - FragPos);
        
        if (dist > u_pointLights[i].radius)
        {
            continue;
        }

        float attenuation = 1.0 - (dist / u_pointLights[i].radius);

        float shadow = PointShadowCalc(FragPos, u_pointLights[i].pos, u_pointLights[i].radius, u_pointShadowMaps[i]);

        vec3 lightEnergy = u_pointLights[i].color * attenuation * (1.0 - shadow);

        float diff = max(dot(baseNorm, L), 0.0);
        if (u_useBump) 
            diff *= clamp(dot(N, L) * 1.4, 0.5, 1.5);
        
        vec3 H = normalize(L + viewDir);
        vec3 specN = u_useBump ? normalize(baseNorm + TBN * (tangentNormal * 0.4)) : N;
        float spec = pow(max(dot(specN, H), 0.0), shine);

        dynDiffuse += lightEnergy * diff;
        dynSpecular += lightEnergy * spec * specMask * 0.5;
    }
	
    // CSM pass
    vec3 sunFinal = vec3(0.0);

    if (u_csmEnabled == 1)
    {
        vec3 sunL = normalize(u_sunDir);
        
        float sunMask = 1.0;
        if (u_isModel)
            sunMask = Color.a;
        else
            sunMask = u_useBump ? texture(u_lightmap, LmCoord5).r : texture(u_lightmap, LmCoord1).a;

        float sunShadow = CalculateSunShadow(FragPos, N, sunL);

        vec3 sunEnergy = u_sunColor * (1.0 - sunShadow) * sunMask;

        sunFinal = sunEnergy * max(dot(N, sunL), 0.0);

        vec3 H = normalize(sunL + viewDir);
        vec3 specN = u_useBump ? normalize(baseNorm + TBN * (tangentNormal * 0.4)) : N;
        float spec = pow(max(dot(specN, H), 0.0), shine);

        dynSpecular += sunEnergy * spec * specMask * 0.5;
    }

    // Final Composition
    vec3 finalDiffuse = albedo.rgb * (diffuseLight * 2.0 + dynDiffuse + sunFinal);
    vec3 finalSpecular = specularLight + dynSpecular;

    if (u_fullbright == 1)
    {
        FragColor = vec4(albedo.rgb, albedo.a);
    }
    else
    {
        FragColor = vec4(finalDiffuse + finalSpecular, albedo.a);
    }

    // Debug Tools
    if (u_debugMode == 1) // r_debug_lightmaps
    {
        if (!u_isModel) 
        {
            FragColor = vec4(texture(u_lightmap, LmCoord1).rgb, 1.0);
        }
        else 
        {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
    }
    else if (u_debugMode == 2) // r_debug_lightmaps_directional
    {
        if (!u_isModel) 
        {
            float w1 = max(dot(tangentNormal, basis0), 0.0);
            float w2 = max(dot(tangentNormal, basis1), 0.0);
            float w3 = max(dot(tangentNormal, basis2), 0.0);

            vec3 l1 = texture(u_lightmap, LmCoord2).rgb;
            vec3 l2 = texture(u_lightmap, LmCoord3).rgb;
            vec3 l3 = texture(u_lightmap, LmCoord4).rgb;

            FragColor = vec4(l1 * w1 + l2 * w2 + l3 * w3, 1.0);
        }
        else 
        {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
    }
    else if (u_debugMode == 3) // r_debug_vertexlight
    {
        if (u_isModel) 
        {
            FragColor = vec4(Color.rgb, 1.0);
        }
        else 
        {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
    }
    else if (u_debugMode == 4) // r_debug_vertexlight_directional
    {
        if (u_isModel) 
        {
            float w1 = max(dot(tangentNormal, basis0), 0.0);
            float w2 = max(dot(tangentNormal, basis1), 0.0);
            float w3 = max(dot(tangentNormal, basis2), 0.0);

            FragColor = vec4(Color.rgb * w1 + Color2.rgb * w2 + Color3.rgb * w3, 1.0);
        }
        else 
        {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
    }
}