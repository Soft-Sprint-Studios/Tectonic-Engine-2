#version 460 core

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
uniform bool u_useBump;
uniform bool u_isModel;
uniform vec3 u_viewPos;
uniform int u_debugMode;

out vec4 FragColor;

const vec3 basis0 = vec3(0.81649658, 0.0, 0.57735027);
const vec3 basis1 = vec3(-0.40824829, 0.70710678, 0.57735027);
const vec3 basis2 = vec3(-0.40824829, -0.70710678, 0.57735027);

void main()
{
    vec4 albedo = texture(u_diffuse, TexCoord);
    if(albedo.a < 0.1) discard;

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
    FragColor = vec4(finalDiffuse + specularLight, albedo.a);

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