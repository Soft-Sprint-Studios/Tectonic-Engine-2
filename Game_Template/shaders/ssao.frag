out float FragColor;
in vec2 TexCoords;

uniform sampler2D u_depthTexture;
uniform sampler2D u_noiseTexture;

uniform mat4 u_projection;
uniform mat4 u_invProjection;

uniform vec3 u_samples[64];
uniform int u_kernelSize;
uniform float u_radius;
uniform float u_bias;
uniform float u_power;

uniform vec2 u_noiseScale;

vec3 GetViewPos(vec2 texCoords)
{
    float depth = texture(u_depthTexture, texCoords).r;
    vec4 clipSpace = vec4(texCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpace = u_invProjection * clipSpace;
    return viewSpace.xyz / viewSpace.w;
}

void main()
{
    vec3 fragPos = GetViewPos(TexCoords);
    vec3 normal = normalize(cross(dFdx(fragPos), dFdy(fragPos)));
    vec3 randomVec = normalize(texture(u_noiseTexture, TexCoords * u_noiseScale).xyz);
    
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for(int i = 0; i < u_kernelSize; ++i)
    {
        vec3 samplePos = TBN * u_samples[i];
        samplePos = fragPos + samplePos * u_radius; 
        
        vec4 offset = vec4(samplePos, 1.0);
        offset = u_projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;
        
        float sampleDepth = GetViewPos(offset.xy).z;
        
        float rangeCheck = smoothstep(0.0, 1.0, u_radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + u_bias ? 1.0 : 0.0) * rangeCheck;           
    }
    
    occlusion = 1.0 - (occlusion / u_kernelSize);
    FragColor = pow(occlusion, u_power);
}