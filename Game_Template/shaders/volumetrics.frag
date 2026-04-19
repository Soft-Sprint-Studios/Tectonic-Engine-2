out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D u_depthTexture;

uniform mat4 u_invProjection;
uniform mat4 u_invView;
uniform mat4 u_view; 
uniform vec3 u_viewPos;

struct PointLight 
{
    vec3 pos; 
    vec3 color; 
    float radius;
    float volumetricIntensity;
    int volumetricSteps;
};

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
uniform float u_sunVolIntensity;
uniform int u_sunVolSteps;

float dither[16] = float[](
     0.0/16.0,  8.0/16.0,  2.0/16.0, 10.0/16.0,
    12.0/16.0,  4.0/16.0, 14.0/16.0,  6.0/16.0,
     3.0/16.0, 11.0/16.0,  1.0/16.0,  9.0/16.0,
    15.0/16.0,  7.0/16.0, 13.0/16.0,  5.0/16.0
);

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

float CalculateSunShadow(vec3 fragPosWorld)
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

    vec4 fragPosLightSpace = u_csmMatrices[layer] * vec4(fragPosWorld, 1.0);
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

void main() 
{
    float depth = texture(u_depthTexture, TexCoords).r;

    if (depth >= 1.0)
    {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
	
    vec4 clipSpace = vec4(TexCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpace = u_invProjection * clipSpace;
    viewSpace /= viewSpace.w;
    vec3 fragPos = (u_invView * viewSpace).xyz;

    vec3 startPosition = u_viewPos;
    vec3 rayVector = fragPos - startPosition;
    float rayLength = length(rayVector);
    vec3 rayDirection = rayVector / rayLength;
    
    vec3 accumFog = vec3(0.0);
    int ditherIndex = (int(gl_FragCoord.x) % 4) + (int(gl_FragCoord.y) % 4) * 4;
    float ditherVal = dither[ditherIndex];

    // Evaluate point lights
    for (int i = 0; i < u_numPointLights; ++i) 
    {
        if (u_pointLights[i].volumetricIntensity <= 0.0) 
        {
            continue;
        }

        vec3 L = u_pointLights[i].pos - startPosition;
        float tca = dot(L, rayDirection);
        float d2 = dot(L, L) - tca * tca;
        float radius2 = u_pointLights[i].radius * u_pointLights[i].radius;

        if (d2 > radius2) 
        {
            continue;
        }
        
        float thc = sqrt(radius2 - d2);
        float t0 = max(tca - thc, 0.0);
        float t1 = min(tca + thc, rayLength);
        
        float marchLen = t1 - t0;

        if (marchLen <= 0.0) 
        {
            continue;
        }
        
        int steps = u_pointLights[i].volumetricSteps;
        float stepSize = marchLen / float(steps);
        vec3 currentPos = startPosition + rayDirection * (t0 + stepSize * ditherVal);
        
        vec3 lightFog = vec3(0.0);

        for (int s = 0; s < steps; ++s) 
        {
            float dist = length(u_pointLights[i].pos - currentPos);
            float attenuation = 1.0 - (dist / u_pointLights[i].radius);

            if (attenuation > 0.0) 
            {
                float shadow = PointShadowCalc(currentPos, u_pointLights[i].pos, u_pointLights[i].radius, u_pointShadowMaps[i]);
                lightFog += u_pointLights[i].color * attenuation * (1.0 - shadow) * stepSize;
            }
            currentPos += rayDirection * stepSize;
        }
        accumFog += lightFog * u_pointLights[i].volumetricIntensity;
    }

    // Evaluate spot lights
    for (int i = 0; i < u_numSpotLights; ++i) 
    {
        if (u_spotLights[i].volumetricIntensity <= 0.0) 
        {
            continue;
        }

        vec3 L = u_spotLights[i].pos - startPosition;
        float tca = dot(L, rayDirection);
        float d2 = dot(L, L) - tca * tca;
        float radius2 = u_spotLights[i].radius * u_spotLights[i].radius;

        if (d2 > radius2) 
        {
            continue;
        }
        
        float thc = sqrt(radius2 - d2);
        float t0 = max(tca - thc, 0.0);
        float t1 = min(tca + thc, rayLength);
        
        float marchLen = t1 - t0;

        if (marchLen <= 0.0) 
        {
            continue;
        }
        
        int steps = u_spotLights[i].volumetricSteps;
        float stepSize = marchLen / float(steps);
        vec3 currentPos = startPosition + rayDirection * (t0 + stepSize * ditherVal);
        
        vec3 lightFog = vec3(0.0);

        for (int s = 0; s < steps; ++s) 
        {
            float dist = length(u_spotLights[i].pos - currentPos);
            vec3 dirToLight = normalize(u_spotLights[i].pos - currentPos);
            float theta = dot(dirToLight, normalize(-u_spotLights[i].dir));
            float epsilon = u_spotLights[i].innerAngle - u_spotLights[i].outerAngle;
            float intensity = clamp((theta - u_spotLights[i].outerAngle) / epsilon, 0.0, 1.0);
            
            float attenuation = 1.0 - (dist / u_spotLights[i].radius);

            if (attenuation > 0.0 && intensity > 0.0) 
            {
                float shadow = SpotShadowCalc(u_spotLights[i].lightSpaceMatrix * vec4(currentPos, 1.0), u_spotShadowMaps[i]);
                lightFog += u_spotLights[i].color * intensity * attenuation * (1.0 - shadow) * stepSize;
            }
            currentPos += rayDirection * stepSize;
        }
        accumFog += lightFog * u_spotLights[i].volumetricIntensity;
    }

    // Evaluate Sun Volumetrics
    if (u_csmEnabled == 1 && u_sunVolIntensity > 0.0)
    {
        int steps = u_sunVolSteps;
        float stepSize = rayLength / float(steps);
        vec3 currentPos = startPosition + rayDirection * (stepSize * ditherVal);
        
        vec3 sunFogAccum = vec3(0.0);

        for (int s = 0; s < steps; ++s)
        {
            float shadow = CalculateSunShadow(currentPos);
            sunFogAccum += u_sunColor * (1.0 - shadow) * stepSize;
            currentPos += rayDirection * stepSize;
        }

        accumFog += sunFogAccum * u_sunVolIntensity;
    }

    FragColor = vec4(accumFog, 1.0);
}