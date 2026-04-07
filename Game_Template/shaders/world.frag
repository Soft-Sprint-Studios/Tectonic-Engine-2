in vec2 TexCoord;
in vec2 LmCoord1;
in vec2 LmCoord2;
in vec2 LmCoord3;
in vec2 LmCoord4;
in vec3 Color;
in vec3 Color2;
in vec3 Color3;
in vec3 FragPos;
in vec3 Normal;

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

void main()
{
    vec4 albedo = texture(u_diffuse, TexCoord);
    if(albedo.a < 0.1) 
        discard;

    vec3 specMask = texture(u_specular, TexCoord).rgb;
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(u_viewPos - FragPos);
    vec3 reflectDir = reflect(-viewDir, norm);
    
    vec3 diffuseLight = vec3(0.0);
    vec3 specularLight = vec3(0.0);
    float shine = 32.0;

    if (u_isModel)
    {
        if (u_useBump)
        {
            vec3 tNorm = texture(u_normal, TexCoord).rgb * 2.0 - 1.0;
            vec3 blendedNorm = normalize(norm + tNorm * 0.4);
            
            float w1 = max(0.0, dot(blendedNorm, basis0));
            float w2 = max(0.0, dot(blendedNorm, basis1));
            float w3 = max(0.0, dot(blendedNorm, basis2));
            float totalWeight = (w1*w1 + w2*w2 + w3*w3);
            diffuseLight = (Color * w1*w1 + Color2 * w2*w2 + Color3 * w3*w3) / max(totalWeight, 0.001);

            float s1 = pow(max(dot(reflectDir, basis0), 0.0), shine);
            float s2 = pow(max(dot(reflectDir, basis1), 0.0), shine);
            float s3 = pow(max(dot(reflectDir, basis2), 0.0), shine);
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
            vec3 tNorm = texture(u_normal, TexCoord).rgb * 2.0 - 1.0;
            vec3 blendedNorm = normalize(norm + tNorm * 0.4);

            vec3 l1 = texture(u_lightmap, LmCoord2).rgb;
            vec3 l2 = texture(u_lightmap, LmCoord3).rgb;
            vec3 l3 = texture(u_lightmap, LmCoord4).rgb;
            
            float w1 = max(0.0, dot(blendedNorm, basis0));
            float w2 = max(0.0, dot(blendedNorm, basis1));
            float w3 = max(0.0, dot(blendedNorm, basis2));
            
            float totalWeight = (w1*w1 + w2*w2 + w3*w3);
            diffuseLight = (l1 * w1*w1 + l2 * w2*w2 + l3 * w3*w3) / max(totalWeight, 0.001);

            float s1 = pow(max(dot(reflectDir, basis0), 0.0), shine);
            float s2 = pow(max(dot(reflectDir, basis1), 0.0), shine);
            float s3 = pow(max(dot(reflectDir, basis2), 0.0), shine);
            specularLight = (l1 * s1 + l2 * s2 + l3 * s3) * 0.4 * specMask;
        }
        else
        {
            diffuseLight = texture(u_lightmap, LmCoord1).rgb;
        }
    }

    vec3 finalDiffuse = albedo.rgb * diffuseLight * 2.0;
    if (u_useCubemap)
    {
        vec3 N = norm;

        if (u_useBump)
        {
            vec3 tNorm = texture(u_normal, TexCoord).rgb * 2.0 - 1.0;
            N = normalize(norm + tNorm * 0.4);
        }

        vec3 R = reflect(-viewDir, N);

        vec3 lookup = ParallaxCorrect(R, FragPos, u_cubemapMins, u_cubemapMaxs, u_cubemapOrigin);

        vec3 envMap = texture(u_cubemap, lookup).rgb;

        float brightness = dot(diffuseLight, vec3(0.2126, 0.7152, 0.0722));
        specularLight += envMap * specMask * brightness;
    }

    if (u_fullbright == 1)
    {
        FragColor = vec4(albedo.rgb, albedo.a);
    }
    else
    {
        FragColor = vec4(finalDiffuse + specularLight, albedo.a);
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