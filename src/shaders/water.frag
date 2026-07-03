$input v_fragPos, v_texcoord0, v_lmCoord, v_lmSize, v_tbn0, v_tbn1, v_tbn2

#include <bgfx_shader.sh>
#include "pbr.sh"
#include "common.sh"
#include "lightmap.sh"

SAMPLER2D(s_reflectionTexture, 0);
SAMPLER2D(s_dudvMap, 1);
SAMPLER2D(s_normalMap, 2);
SAMPLER2D(s_flowMap, 3);
SAMPLER2D(s_lightmap, 4);

uniform vec4 u_viewPos;
uniform vec4 u_time;
uniform vec4 u_flowParams;
#define u_flowSpeed u_flowParams.x
#define u_hasFlow (u_flowParams.y > 0.5)

uniform mat4 u_reflectViewProj[2];
#define u_reflectView u_reflectViewProj[0]
#define u_reflectProj u_reflectViewProj[1]

const float waveStrength = 0.02;
const float normalTiling = 1.0;
const float normalSpeed = 0.015;

void main() 
{
    vec2 worldUV = v_fragPos.xz * 0.25;

    vec2 flowDir = vec2(1.0, 0.0); 
    if (u_hasFlow)
    {
        flowDir = texture2D(s_flowMap, worldUV).rg * 2.0 - 1.0;
    }

    float timeVal = u_time.x * u_flowSpeed;
    float phase0 = fract(timeVal);
    float phase1 = fract(timeVal + 0.5);

    vec2 dist0 = (texture2D(s_dudvMap, worldUV + flowDir * phase0).rg * 2.0 - 1.0) * waveStrength;
    vec2 dist1 = (texture2D(s_dudvMap, worldUV + flowDir * phase1).rg * 2.0 - 1.0) * waveStrength;

    float lerpFlow = abs(0.5 - phase0) / 0.5;
    vec2 distortion = mix(dist0, dist1, lerpFlow);

    vec4 reflectClip = mul(u_reflectProj, mul(u_reflectView, vec4(v_fragPos, 1.0)));
    vec2 ndc = (reflectClip.xy / reflectClip.w) * 0.5 + 0.5;

    ndc.y = 1.0 - ndc.y;

    vec3 reflection = texture2D(s_reflectionTexture, clamp(ndc + distortion, 0.001, 0.999)).rgb;

    vec2 scroll = worldUV * normalTiling + vec2_splat(u_time.x * normalSpeed);
    vec3 normalSample = texture2D(s_normalMap, scroll + distortion).rgb * 2.0 - 1.0;
    
    mat3 TBN = mat3(v_tbn0, v_tbn1, v_tbn2);
    vec3 N = normalize(TBN[2]);

    N = normalize(mul(TBN, normalSample));

    vec3 V = normalize(u_viewPos.xyz - v_fragPos);
    vec3 F0 = vec3_splat(0.04);
    vec3 F_reflect = FresnelSchlick(max(dot(N, V), 0.0), F0);

    vec3 worldNormal = N;
    vec3 albedo = reflection * F_reflect;

    // dithering to eliminate 8-bit gbuffer color banding
	const float bayer[16] = float[]
    (
         0.0/16.0,  8.0/16.0,  2.0/16.0, 10.0/16.0,
        12.0/16.0,  4.0/16.0, 14.0/16.0,  6.0/16.0,
         3.0/16.0, 11.0/16.0,  1.0/16.0,  9.0/16.0,
        15.0/16.0,  7.0/16.0, 13.0/16.0,  5.0/16.0
    );
    int ditherX = int(gl_FragCoord.x) % 4;
    int ditherY = int(gl_FragCoord.y) % 4;
    float ditherValue = bayer[ditherY * 4 + ditherX];
    albedo.rgb += (ditherValue - 0.5) / 255.0;

    vec3 mraoh = vec3(0.0, 0.1, 1.0);

    float packed_tx = normalSample.x * 0.5 + 0.5;
    float packed_ty = u_useBump ? (normalSample.y * 0.5 + 0.5) : 0.0;

    gl_FragData[0] = vec4(EncodeNormal(worldNormal), 0.0, 0.0);
    gl_FragData[1] = vec4(albedo.rgb, packed_tx);
    gl_FragData[2] = vec4(mraoh.rgb, packed_ty);     
    gl_FragData[3] = vec4(PackLightmapUV(v_lmCoord, v_lmSize), 0.0, 0.0);
}