#ifndef LIGHTS_SH
#define LIGHTS_SH

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

SAMPLER2DARRAY(u_spotShadowMaps, 14);
SAMPLER2DARRAY(u_pointShadowMaps, 15);
SAMPLER2DARRAY(u_csmArray, 13);

uniform vec4 u_lightParams;
#define u_numPointLights int(u_lightParams.x)
#define u_numSpotLights  int(u_lightParams.y)

uniform mat4 u_csmMatrices[4];
uniform vec4 u_csmSplitsLow;
uniform vec4 u_sunDirVol;
uniform vec4 u_sunColorEn;
uniform vec4 u_csmSplit4_volSteps;
#define u_csmSplit4   u_csmSplit4_volSteps.x
#define u_sunVolSteps u_csmSplit4_volSteps.y

StructuredBuffer<LightData> u_pointLights : register(t10);
StructuredBuffer<LightData> u_spotLights : register(t11);

const float EVSM_EXP = 10.0;

float linstep(float min_val, float max_val, float v)
{
    return clamp((v - min_val) / (max_val - min_val), 0.0, 1.0);
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
	
    vec4 fragPosLightSpace = mul(lightSpaceMatrix, vec4(worldPos, 1.0));
    vec3 projCoords = (fragPosLightSpace.xyz / fragPosLightSpace.w) * 0.5 + 0.5;
    projCoords.y = 1.0 - projCoords.y;
    if (projCoords.z > 1.0) 
        return 0.0;

    float linearDepth = (distance(worldPos, lightPos) / radius) - 0.001;
    float warpedDepth = exp(EVSM_EXP * linearDepth);
    vec2 moments = texture2DArrayLod(u_spotShadowMaps, vec3(projCoords.xy, layer), 0.0).rg;
    return 1.0 - ChebyshevUpperBound(moments, warpedDepth);
}

float PointShadowCalc(vec3 worldPos, vec3 lightPos, float far_plane, float layer) 
{
    if (layer < 0.0) 
        return 0.0;

    vec3 fragToLight = worldPos - lightPos;
    float depth = (length(fragToLight) / far_plane) - 0.001;

    vec3 vAbs = abs(fragToLight);
    float faceIndex;
    vec2 uv;

    if (vAbs.z >= vAbs.x && vAbs.z >= vAbs.y) 
    {
        faceIndex = fragToLight.z < 0.0 ? 5.0 : 4.0;
        uv = vec2(fragToLight.z < 0.0 ? -fragToLight.x : fragToLight.x, fragToLight.y);
    } 
    else if (vAbs.y >= vAbs.x)
    {
        faceIndex = fragToLight.y < 0.0 ? 3.0 : 2.0;
        uv = vec2(fragToLight.x, fragToLight.y < 0.0 ? fragToLight.z : -fragToLight.z);
    } 
    else 
    {
        faceIndex = fragToLight.x < 0.0 ? 1.0 : 0.0;
        uv = vec2(fragToLight.x < 0.0 ? fragToLight.z : -fragToLight.z, fragToLight.y);
    }
    
    uv = uv * (0.5 / max(vAbs.x, max(vAbs.y, vAbs.z))) + 0.5;
    vec2 moments = texture2DArrayLod(u_pointShadowMaps, vec3(uv, layer * 6.0 + faceIndex), 0.0).rg;
    float warpedDepth = exp(EVSM_EXP * depth);
    return 1.0 - ChebyshevUpperBound(moments, warpedDepth);
}

float CalculateSunShadow(vec3 worldPos, mat4 viewMat, sampler2DArray shadowArray)
{
    float viewDepth = abs(mul(viewMat, vec4(worldPos, 1.0)).z);
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

    vec4 fragPosLightSpace = mul(u_csmMatrices[layer], vec4(worldPos, 1.0));
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    projCoords.y = 1.0 - projCoords.y; 
    projCoords.z -= 0.0005;

    if (projCoords.z > 1.0) 
    {
        return 0.0;
    }

    float shadow = 0.0;
    vec2 texelSize = vec2_splat(1.0 / 4096.0);

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture2DArrayLod(shadowArray, vec3(projCoords.xy + vec2(x, y) * texelSize, layer), 0.0).r;
            if (projCoords.z > pcfDepth) 
            {
                shadow += 1.0;
            }
        }
    }

    return shadow / 9.0;
}

#endif