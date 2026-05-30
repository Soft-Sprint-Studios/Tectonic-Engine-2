#include "lights.glsl"
in vec2 TexCoords;

uniform sampler2D u_depthTexture;

uniform mat4 u_invProjection;
uniform mat4 u_invView;
uniform mat4 u_view; 
uniform vec3 u_viewPos;

out vec4 FragColor;

float dither[16] = float[](
     0.0/16.0,  8.0/16.0,  2.0/16.0, 10.0/16.0,
    12.0/16.0,  4.0/16.0, 14.0/16.0,  6.0/16.0,
     3.0/16.0, 11.0/16.0,  1.0/16.0,  9.0/16.0,
    15.0/16.0,  7.0/16.0, 13.0/16.0,  5.0/16.0
);

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
            continue;

        vec3 L = u_pointLights[i].pos - startPosition;
        float tca = dot(L, rayDirection);
        float d2 = dot(L, L) - tca * tca;
        float radius2 = u_pointLights[i].radius * u_pointLights[i].radius;

        if (d2 > radius2) 
            continue;
        
        float thc = sqrt(radius2 - d2);
        float t0 = max(tca - thc, 0.0);
        float t1 = min(tca + thc, rayLength);
        float marchLen = t1 - t0;

        if (marchLen <= 0.0) 
            continue;
        
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
            continue;

        vec3 L = u_spotLights[i].pos - startPosition;
        float tca = dot(L, rayDirection);
        float d2 = dot(L, L) - tca * tca;
        float radius2 = u_spotLights[i].radius * u_spotLights[i].radius;

        if (d2 > radius2) 
            continue;
        
        float thc = sqrt(radius2 - d2);
        float t0 = max(tca - thc, 0.0);
        float t1 = min(tca + thc, rayLength);
        float marchLen = t1 - t0;

        if (marchLen <= 0.0) 
            continue;
        
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
                float shadow = SpotShadowCalc(currentPos, u_spotLights[i].pos, u_spotLights[i].radius, u_spotLights[i].lightSpaceMatrix,  u_spotShadowMaps[i]);
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
            float shadow = CalculateSunShadow(currentPos, u_view, u_csmSplits, u_csmMatrices, u_csmArray);
            sunFogAccum += u_sunColor * (1.0 - shadow) * stepSize;
            currentPos += rayDirection * stepSize;
        }
        accumFog += sunFogAccum * u_sunVolIntensity;
    }

    FragColor = vec4(accumFog, 1.0);
}