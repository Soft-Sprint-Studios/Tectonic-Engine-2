#include "common.glsl"

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D u_gDepth;
uniform sampler2D u_gNormal;
uniform sampler2D u_gAlbedoSpec;
uniform sampler2D u_sceneTex;

uniform mat4 u_projection;
uniform mat4 u_invProjection;
uniform mat4 u_view;

uniform float u_maxDist;
uniform float u_resolution;
uniform float u_thickness;
uniform int u_maxSteps;
uniform int u_binarySteps;

vec3 GetViewPos(vec2 uv)
{
    float depth = texture(u_gDepth, uv).r;
    vec4 clipSpace = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpace = u_invProjection * clipSpace;
    return viewSpace.xyz / viewSpace.w;
}

void main()
{
    vec4 albSpec = texture(u_gAlbedoSpec, TexCoords);
    float specMask = albSpec.a;
    
    if (specMask <= 0.01) 
    {
        discard;
    }

    vec3 viewPos = GetViewPos(TexCoords);
    vec3 worldNormal = DecodeNormal(texture(u_gNormal, TexCoords).rg);
    vec3 viewNormal = normalize(mat3(u_view) * worldNormal);

    vec3 rayDir = normalize(reflect(normalize(viewPos), viewNormal));
    
    vec3 currentPos = viewPos;
    vec3 step = rayDir * u_resolution;
    
    vec2 hitUV = vec2(0.0);
    float visibility = 0.0;
    bool hit = false;

    for(int i = 0; i < u_maxSteps; i++)
    {
        currentPos += step;
        
        vec4 projectedPos = u_projection * vec4(currentPos, 1.0);
        vec2 sampleUV = (projectedPos.xy / projectedPos.w) * 0.5 + 0.5;

        if (sampleUV.x < 0 || sampleUV.x > 1 || sampleUV.y < 0 || sampleUV.y > 1) 
            break;

        float sampledZ = GetViewPos(sampleUV).z;
        float depthDiff = currentPos.z - sampledZ;

        if (depthDiff < 0.0 && abs(depthDiff) < u_thickness)
        {
            vec3 minStep = currentPos - step;
            vec3 maxStep = currentPos;
            vec3 midStep;

            for(int j = 0; j < u_binarySteps; j++)
            {
                midStep = mix(minStep, maxStep, 0.5);
                vec4 pMid = u_projection * vec4(midStep, 1.0);
                vec2 uvMid = (pMid.xy / pMid.w) * 0.5 + 0.5;
                float zMid = GetViewPos(uvMid).z;

                if (midStep.z - zMid < 0.0)
                {
                    maxStep = midStep;
                    hitUV = uvMid;
                }
                else
                {
                    minStep = midStep;
                }
            }
            
            visibility = 1.0;
            hit = true;
            break;
        }
    }

    if (!hit) 
        discard;

    vec2 edgeFactor = smoothstep(0.0, 0.1, hitUV) * (1.0 - smoothstep(0.9, 1.0, hitUV));
    visibility *= edgeFactor.x * edgeFactor.y;

    visibility *= (1.0 - clamp(length(GetViewPos(hitUV) - viewPos) / u_maxDist, 0.0, 1.0));

    float fresnel = pow(1.0 - max(dot(viewNormal, -normalize(viewPos)), 0.0), 3.0);
    
    vec3 color = texture(u_sceneTex, hitUV).rgb;
    FragColor = vec4(color * specMask, visibility * fresnel);
}