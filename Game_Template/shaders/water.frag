#include "pbr.h"
#include "common.h"

in vec3 v_FragPos;
in vec2 v_TexCoord;
in vec2 v_LmCoord;
in vec2 v_LmSize;
in mat3 v_TBN;

layout(binding = 0) uniform sampler2D u_reflectionTexture;
layout(binding = 1) uniform sampler2D u_dudvMap;
layout(binding = 2) uniform sampler2D u_normalMap;
layout(binding = 3) uniform sampler2D u_flowMap;

uniform float u_flowSpeed;
uniform bool u_hasFlow;
uniform mat4 u_reflectView;
uniform mat4 u_reflectProj;
uniform vec3 u_viewPos;
uniform float u_time;
uniform bool u_useBump;

layout (location = 0) out vec4 gNormal;
layout (location = 1) out vec4 gAlbedo;
layout (location = 2) out vec4 gMRAO;
layout (location = 3) out vec2 gLightmapUV;

const float waveStrength = 0.02;
const float normalTiling = 1.0;
const float normalSpeed = 0.015;

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

    if (u_useBump)
    {
        N = normalize(v_TBN * normalSample);
    }

    vec3 V = normalize(u_viewPos - v_FragPos);
    vec3 F0 = vec3(0.04);
    vec3 F_reflect = FresnelSchlick(max(dot(N, V), 0.0), F0);

    vec3 worldNormal = N;
    vec3 albedo = reflection * F_reflect;
    vec3 mraoh = vec3(0.0, 0.1, 1.0);

    float packed_tx = normalSample.x * 0.5 + 0.5;
    float packed_ty = u_useBump ? (normalSample.y * 0.5 + 0.5) : 0.0;
    vec2 size_in_pixels = v_LmSize * vec2(4096.0);
    float packed_w = size_in_pixels.x / 255.0;
    float packed_h = size_in_pixels.y / 255.0;

    gNormal = vec4(EncodeNormal(worldNormal), packed_tx, packed_ty);
    gAlbedo = vec4(albedo.rgb, packed_w);
    gMRAO = vec4(mraoh.rgb, packed_h);     
    gLightmapUV = v_LmCoord;
}