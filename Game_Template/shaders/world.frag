in centroid vec2 TexCoord;
in centroid vec2 LmCoord1;
in centroid vec2 LmCoord2;
in centroid vec2 LmCoord3;
in centroid vec2 LmCoord4;
in centroid vec3 Color;
in centroid vec3 Color2;
in centroid vec3 Color3;
in vec3 FragPos;
in centroid vec3 Normal;

uniform sampler2D u_diffuse;
uniform sampler2D u_normal;
uniform sampler2D u_lightmap;
uniform sampler2D u_specular;
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

// Dynamic point lights
struct PointLight
{
    vec3 pos; 
    vec3 color; 
    float radius;
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
};

uniform int u_numPointLights;
uniform PointLight u_pointLights[4];
uniform samplerCube u_pointShadowMaps[4];

uniform int u_numSpotLights;
uniform SpotLight u_spotLights[4];
uniform sampler2D u_spotShadowMaps[4];

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

float SpotShadowCalc(vec4 fragPosLightSpace, sampler2D shadowMap)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0)
    {
        return 0.0;
    }
    
    float currentDepth = projCoords.z;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - 0.005 > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    
    return shadow / 9.0;
}

float PointShadowCalc(vec3 fragPos, vec3 lightPos, float far_plane, samplerCube shadowMap)
{
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    float shadow = 0.0;
    float bias = 0.05; 
    float samples = 4.0;
    float offset = 0.1;
    
    for (float x = -offset; x < offset; x += offset / (samples * 0.5))
    {
        for (float y = -offset; y < offset; y += offset / (samples * 0.5))
        {
            for (float z = -offset; z < offset; z += offset / (samples * 0.5))
            {
                float closestDepth = texture(shadowMap, fragToLight + vec3(x, y, z)).r; 
                closestDepth *= far_plane;
                if (currentDepth - bias > closestDepth)
                {
                    shadow += 1.0;
                }
            }
        }
    }
    
    return shadow / (samples * samples * samples);
}

void main()
{
    vec4 albedo = texture(u_diffuse, TexCoord);
    if(albedo.a < 0.1) 
    {
        discard;
    }

    vec3 specMask = texture(u_specular, TexCoord).rgb;
    vec3 baseNorm = normalize(Normal);
    vec3 viewDir = normalize(u_viewPos - FragPos);
    float shine = 32.0;

    vec3 N = baseNorm;
    if (u_useBump)
    {
        vec3 tNorm = texture(u_normal, TexCoord).rgb * 2.0 - 1.0;
        N = normalize(baseNorm + tNorm * 0.4);
    }
    
    vec3 reflectViewDir = reflect(-viewDir, N);

    vec3 diffuseLight = vec3(0.0);
    vec3 specularLight = vec3(0.0);

    // Baked Lighting Phase
    if (u_isModel)
    {
        if (u_useBump)
        {
            float w1 = max(0.0, dot(N, basis0));
            float w2 = max(0.0, dot(N, basis1));
            float w3 = max(0.0, dot(N, basis2));
            float totalWeight = (w1*w1 + w2*w2 + w3*w3);
            diffuseLight = (Color * w1*w1 + Color2 * w2*w2 + Color3 * w3*w3) / max(totalWeight, 0.001);

            float s1 = pow(max(dot(reflectViewDir, basis0), 0.0), shine);
            float s2 = pow(max(dot(reflectViewDir, basis1), 0.0), shine);
            float s3 = pow(max(dot(reflectViewDir, basis2), 0.0), shine);
            specularLight = (Color * s1 + Color2 * s2 + Color3 * s3) * 0.3 * specMask;
        }
        else
        {
            diffuseLight = Color;
        }
    }
    else
    {
        if (u_useBump)
        {
            vec3 l1 = texture(u_lightmap, LmCoord2).rgb;
            vec3 l2 = texture(u_lightmap, LmCoord3).rgb;
            vec3 l3 = texture(u_lightmap, LmCoord4).rgb;
            
            float w1 = max(0.0, dot(N, basis0));
            float w2 = max(0.0, dot(N, basis1));
            float w3 = max(0.0, dot(N, basis2));
            
            float totalWeight = (w1*w1 + w2*w2 + w3*w3);
            diffuseLight = (l1 * w1*w1 + l2 * w2*w2 + l3 * w3*w3) / max(totalWeight, 0.001);

            float s1 = pow(max(dot(reflectViewDir, basis0), 0.0), shine);
            float s2 = pow(max(dot(reflectViewDir, basis1), 0.0), shine);
            float s3 = pow(max(dot(reflectViewDir, basis2), 0.0), shine);
            specularLight = (l1 * s1 + l2 * s2 + l3 * s3) * 0.4 * specMask;
        }
        else
        {
            diffuseLight = texture(u_lightmap, LmCoord1).rgb;
        }
    }

    // Environmental Lighting Phase
    if (u_useCubemap)
    {
        vec3 lookup = ParallaxCorrect(reflectViewDir, FragPos, u_cubemapMins, u_cubemapMaxs, u_cubemapOrigin);
        vec3 envMap = texture(u_cubemap, lookup).rgb;

        float brightness = dot(diffuseLight, vec3(0.2126, 0.7152, 0.0722));
        specularLight += envMap * specMask * brightness;
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
        float shadow = 0.0;
        
        if (i == 0)
        {
            shadow = SpotShadowCalc(u_spotLights[i].lightSpaceMatrix * vec4(FragPos, 1.0), u_spotShadowMaps[0]);
        }
        else if (i == 1)
        {
            shadow = SpotShadowCalc(u_spotLights[i].lightSpaceMatrix * vec4(FragPos, 1.0), u_spotShadowMaps[1]);
        }
        else if (i == 2)
        {
            shadow = SpotShadowCalc(u_spotLights[i].lightSpaceMatrix * vec4(FragPos, 1.0), u_spotShadowMaps[2]);
        }
        else if (i == 3)
        {
            shadow = SpotShadowCalc(u_spotLights[i].lightSpaceMatrix * vec4(FragPos, 1.0), u_spotShadowMaps[3]);
        }

        vec3 lightEnergy = u_spotLights[i].color * intensity * attenuation * (1.0 - shadow);

        float diff = max(dot(N, L), 0.0);
        
        vec3 reflectL = reflect(-L, N);
        float spec = pow(max(dot(viewDir, reflectL), 0.0), shine);

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
        float shadow = 0.0;
        
        if (i == 0)
        {
            shadow = PointShadowCalc(FragPos, u_pointLights[i].pos, u_pointLights[i].radius, u_pointShadowMaps[0]);
        }
        else if (i == 1)
        {
            shadow = PointShadowCalc(FragPos, u_pointLights[i].pos, u_pointLights[i].radius, u_pointShadowMaps[1]);
        }
        else if (i == 2)
        {
            shadow = PointShadowCalc(FragPos, u_pointLights[i].pos, u_pointLights[i].radius, u_pointShadowMaps[2]);
        }
        else if (i == 3)
        {
            shadow = PointShadowCalc(FragPos, u_pointLights[i].pos, u_pointLights[i].radius, u_pointShadowMaps[3]);
        }

        vec3 lightEnergy = u_pointLights[i].color * attenuation * (1.0 - shadow);

        float diff = max(dot(N, L), 0.0);
        
        vec3 reflectL = reflect(-L, N);
        float spec = pow(max(dot(viewDir, reflectL), 0.0), shine);

        dynDiffuse += lightEnergy * diff;
        dynSpecular += lightEnergy * spec * specMask * 0.5;
    }

    // Final Composition
    vec3 finalDiffuse = albedo.rgb * (diffuseLight * 2.0 + dynDiffuse);
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
            FragColor = vec4(diffuseLight * 2.0, 1.0);
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
            FragColor = vec4(Color, 1.0);
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
            FragColor = vec4(diffuseLight, 1.0);
        }
        else 
        {
            FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        }
    }
}