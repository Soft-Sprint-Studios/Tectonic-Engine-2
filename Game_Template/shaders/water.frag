out vec4 FragColor;

in centroid vec3 v_FragPos;
in vec2 v_TexCoord;
in vec2 v_LmCoord;
in vec2 v_LmSize;
in centroid mat3 v_TBN;

uniform sampler2D u_reflectionTexture;
uniform sampler2D u_lightmap;
uniform sampler2D u_dudvMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_flowMap;
uniform float u_flowSpeed;
uniform bool u_hasFlow;

uniform mat4 u_reflectView;
uniform mat4 u_reflectProj;

uniform vec3 u_viewPos;
uniform float u_time;

uniform bool u_useBump;
uniform int u_mat_specular;
uniform int u_mat_bumpmap;
uniform int u_lightmap_bicubic;

const float waveStrength = 0.02;
const float normalTiling = 1.0;
const float normalSpeed = 0.015;
const float dudvMoveSpeed = 0.01;

const vec3 basis0 = vec3(0.81649658, 0.0, 0.57735027);
const vec3 basis1 = vec3(-0.40824829, 0.70710678, 0.57735027);
const vec3 basis2 = vec3(-0.40824829, -0.70710678, 0.57735027);

// Bicubic filtering functions adapted from Godot Engine
float w0(float a) 
{ 
    return (1.0 / 6.0) * (a * (a * (-a + 3.0) - 3.0) + 1.0); 
}

float w1(float a) 
{ 
    return (1.0 / 6.0) * (a * a * (3.0 * a - 6.0) + 4.0); 
}

float w2(float a) 
{ 
    return (1.0 / 6.0) * (a * (a * (-3.0 * a + 3.0) + 3.0) + 1.0); 
}

float w3(float a) 
{ 
    return (1.0 / 6.0) * (a * a * a); 
}

float g0(float a) 
{ 
    return w0(a) + w1(a); 
}

float g1(float a) 
{ 
    return w2(a) + w3(a); 
}

float h0(float a) 
{ 
    return -1.0 + w1(a) / (w0(a) + w1(a)); 
}

float h1(float a) 
{ 
    return 1.0 + w3(a) / (w2(a) + w3(a)); 
}

vec4 textureBicubic(sampler2D tex, vec2 uv)
{
    vec2 texture_size = vec2(textureSize(tex, 0));
    vec2 texel_size = 1.0 / texture_size;

    uv = uv * texture_size + vec2(0.5);
    vec2 iuv = floor(uv);
    vec2 fuv = fract(uv);

    float g0x = g0(fuv.x);
    float g1x = g1(fuv.x);
    float h0x = h0(fuv.x);
    float h1x = h1(fuv.x);
    float h0y = h0(fuv.y);
    float h1y = h1(fuv.y);

    vec2 p0 = (vec2(iuv.x + h0x, iuv.y + h0y) - vec2(0.5)) * texel_size;
    vec2 p1 = (vec2(iuv.x + h1x, iuv.y + h0y) - vec2(0.5)) * texel_size;
    vec2 p2 = (vec2(iuv.x + h0x, iuv.y + h1y) - vec2(0.5)) * texel_size;
    vec2 p3 = (vec2(iuv.x + h1x, iuv.y + h1y) - vec2(0.5)) * texel_size;

    return (g0(fuv.y) * (g0x * texture(tex, p0) + g1x * texture(tex, p1))) +
           (g1(fuv.y) * (g0x * texture(tex, p2) + g1x * texture(tex, p3)));
}

vec4 GetLightmapData(vec2 uv) 
{
    if (u_lightmap_bicubic == 1) 
    {
        return textureBicubic(u_lightmap, uv);
    }
    return texture(u_lightmap, uv);
}

void main() 
{
    vec2 worldUV = v_FragPos.xz * 0.25;

    vec2 flowDir = vec2(1.0, 0.0); 
    if (u_hasFlow)
    {
        flowDir = texture(u_flowMap, worldUV).rg * 2.0 - 1.0;
    }

    float time = u_time * u_flowSpeed;
    float phase0 = fract(time);
    float phase1 = fract(time + 0.5);

    vec2 dist0 = (texture(u_dudvMap, worldUV + flowDir * phase0).rg * 2.0 - 1.0) * waveStrength;
    vec2 dist1 = (texture(u_dudvMap, worldUV + flowDir * phase1).rg * 2.0 - 1.0) * waveStrength;

    float lerpFlow = abs(0.5 - phase0) / 0.5;
    vec2 distortion = mix(dist0, dist1, lerpFlow);

    vec4 reflectClip = u_reflectProj * u_reflectView * vec4(v_FragPos, 1.0);
    vec2 ndc = (reflectClip.xy / reflectClip.w) * 0.5 + 0.5;
    vec3 reflection = texture(u_reflectionTexture, clamp(ndc + distortion, 0.001, 0.999)).rgb;

    vec2 scroll = worldUV * normalTiling + vec2(u_time * normalSpeed);
    vec3 normalSample = texture(u_normalMap, scroll + distortion).rgb * 2.0 - 1.0;
    vec3 N = normalize(v_TBN[2]);
    vec3 tsNormal = vec3(0.0, 0.0, 1.0);
    if (u_useBump && u_mat_bumpmap == 1)
    {
        tsNormal = normalSample;
        N = normalize(v_TBN * tsNormal);
    }

    vec3 V = normalize(u_viewPos - v_FragPos);
    vec3 diffuseLight = vec3(0.0);
    vec3 dominantL = N;
	
    vec2 v_LmCoord2 = v_LmCoord + vec2(v_LmSize.x, 0.0);
    vec2 v_LmCoord3 = v_LmCoord + vec2(0.0, v_LmSize.y);
    vec2 v_LmCoord4 = v_LmCoord + v_LmSize;

    if (u_useBump)
    {
        float w1 = clamp(dot(tsNormal, basis0), 0.0, 1.0);
        float w2 = clamp(dot(tsNormal, basis1), 0.0, 1.0);
        float w3 = clamp(dot(tsNormal, basis2), 0.0, 1.0);
        float sumW = w1 + w2 + w3 + 1e-5;
        w1 /= sumW; 
        w2 /= sumW; 
        w3 /= sumW;

        vec3 l1 = GetLightmapData(v_LmCoord2).rgb;
        vec3 l2 = GetLightmapData(v_LmCoord3).rgb;
        vec3 l3 = GetLightmapData(v_LmCoord4).rgb;
        diffuseLight = (l1 * w1 + l2 * w2 + l3 * w3) * 2.0;

        vec3 L1 = normalize(v_TBN * basis0);
        vec3 L2 = normalize(v_TBN * basis1);
        vec3 L3 = normalize(v_TBN * basis2);
        dominantL = normalize(L1 * w1 + L2 * w2 + L3 * w3);
    }
    else
    {
        diffuseLight = GetLightmapData(v_LmCoord).rgb * 2.0;
    }

    vec3 specularLight = vec3(0.0);
    if (u_mat_specular == 1)
    {
        vec3 H = normalize(dominantL + V);
        float spec = pow(max(dot(N, H), 0.0), 2048.0);
        specularLight = diffuseLight * spec;
    }

    float fresnel = clamp(1.0 - dot(V, vec3(0.0, 1.0, 0.0)), 0.0, 1.0);
    fresnel = pow(fresnel, 3.0);

    vec3 finalColor = (reflection * fresnel) + specularLight;
    finalColor *= diffuseLight;

    FragColor = vec4(finalColor, 0.95);
}