struct LightData 
{
    vec4 posRadius;
    vec4 colorVol;
    vec4 dirInner;
    vec4 shadowData;
    mat4 lightSpace;
    float shadowLayer; 
    float pad1;
    float pad2;
    float pad3;
};

layout(binding = 14) uniform sampler2DArray u_spotShadowMaps;
layout(binding = 15) uniform samplerCubeArray u_pointShadowMaps;

layout(std430, binding = 10) readonly buffer LightBlock 
{
    LightData u_pointLights[32];
    LightData u_spotLights[32];
};

uniform int u_numPointLights;
uniform int u_numSpotLights;

layout(binding = 13) uniform sampler2DArray u_csmArray;

layout(std430, binding = 6) readonly buffer SunBlock 
{
    mat4 u_csmMatrices[4];
    vec4 u_csmSplitsLow;
    vec4 u_sunDirVol;
    vec4 u_sunColorEn;
    float u_csmSplit4;
    float u_sunVolSteps;
};

const float EVSM_EXP = 10.0;

float linstep(float min, float max, float v)
{
    return clamp((v - min) / (max - min), 0.0, 1.0);
}

float GetLinearShadowDepth(float warpedDepth)
{
    return log(max(warpedDepth, 0.0001)) / EVSM_EXP;
}

float ChebyshevUpperBound(vec2 moments, float warpedDepth)
{
    if (warpedDepth <= moments.x) 
    {
        return 1.0;
    }

    float variance = moments.y - (moments.x * moments.x);
    variance = max(variance, 0.00001);

    float d = warpedDepth - moments.x;
    float pMax = variance / (variance + d * d);

    return linstep(0.2, 1.0, pMax); 
}

float SpotShadowCalc(vec3 worldPos, vec3 lightPos, float radius, mat4 lightSpaceMatrix, float layer) 
{
    if (layer < 0.0) 
        return 0.0;
	
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(worldPos, 1.0);
    vec3 projCoords = (fragPosLightSpace.xyz / fragPosLightSpace.w) * 0.5 + 0.5;
    if (projCoords.z > 1.0) 
        return 0.0;

    float linearDepth = (distance(worldPos, lightPos) / radius) - 0.001;
    float warpedDepth = exp(EVSM_EXP * linearDepth);
    vec2 moments = texture(u_spotShadowMaps, vec3(projCoords.xy, layer)).rg;
    return 1.0 - ChebyshevUpperBound(moments, warpedDepth);
}

float PointShadowCalc(vec3 worldPos, vec3 lightPos, float far_plane, float layer) 
{
    if (layer < 0.0) 
        return 0.0;

    vec3 fragToLight = worldPos - lightPos;
    float depth = (length(fragToLight) / far_plane) - 0.001;
    
    vec2 moments = texture(u_pointShadowMaps, vec4(fragToLight, layer)).rg;
    float warpedDepth = exp(EVSM_EXP * depth);
    return 1.0 - ChebyshevUpperBound(moments, warpedDepth);
}

float CalculateSunShadow(vec3 worldPos, mat4 viewMat, sampler2DArray shadowArray)
{
    float viewDepth = abs((viewMat * vec4(worldPos, 1.0)).z);
    int layer = -1;

    if (viewDepth < u_csmSplitsLow.y)
    {
        layer = 0;
    }
    else if (viewDepth < u_csmSplitsLow.z)
    {
        layer = 1;
    }
    else if (viewDepth < u_csmSplitsLow.w)
    {
        layer = 2;
    }
    else if (viewDepth < u_csmSplit4)
    {
        layer = 3;
    }

    if (layer == -1) 
    {
        return 0.0;
    }

    vec4 fragPosLightSpace = u_csmMatrices[layer] * vec4(worldPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) 
    {
        return 0.0;
    }

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowArray, 0));

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowArray, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
            if (projCoords.z > pcfDepth) 
            {
                shadow += 1.0;
            }
        }
    }

    return shadow / 9.0;
}