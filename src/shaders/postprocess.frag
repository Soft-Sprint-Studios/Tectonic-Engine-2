$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(u_screenTexture, 0);
SAMPLER2D(u_depthTexture, 1);

SAMPLER2D(u_bloomTexture, 2);
SAMPLER2D(u_volumetricTexture, 3);
SAMPLER2D(u_ssaoTexture, 4);
SAMPLER2D(u_ssrTexture, 5);
SAMPLER2D(u_motionBlurTexture, 7);

uniform vec4 u_params;
#define u_time             u_params.x
#define u_vignetteStrength u_params.y
#define u_chromaStrength   u_params.z
#define u_grainStrength    u_params.w

uniform vec4 u_colorParams[2];
#define u_bwStrength          u_colorParams[0].x
#define u_negativeStrength    u_colorParams[0].y
#define u_sepiaStrength       u_colorParams[0].z
#define u_Gamma               u_colorParams[0].w
#define u_postprocess_enabled int(u_colorParams[1].x)
#define u_tonemap_enabled     int(u_colorParams[1].y)
#define u_fxaa                int(u_colorParams[1].z)
#define u_fxaaStrength        u_colorParams[1].w

uniform vec4 u_fogColor;
uniform vec4 u_fogParams;
#define u_fogEnabled    (u_fogParams.x > 0.5)
#define u_fogStart      u_fogParams.y
#define u_fogEnd        u_fogParams.z
#define u_fogAffectsSky (u_fogParams.w > 0.5)

uniform vec4 u_bloomParams;
#define u_bloom_enabled int(u_bloomParams.x)
#define u_bloom_intensity u_bloomParams.y

uniform vec4 u_ssaoParams;
#define u_ssao_enabled int(u_ssaoParams.x)

uniform vec4 u_ssrParams;
#define u_ssr_enabled int(u_ssrParams.x)

uniform vec4 u_motionBlurParams;
#define u_motionblur_enabled int(u_motionBlurParams.x)

uniform vec4 u_volumetricParams;
#define u_volumetrics_enabled int(u_volumetricParams.x)

StructuredBuffer<vec4> u_exposureData : register(t6);

float random(vec2 st) 
{
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

vec3 AcesFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 ApplyFXAA(sampler2D tex, vec2 uv)
{
    float FXAA_SPAN_MAX = 8.0;
    float FXAA_REDUCE_MUL = 1.0 / 8.0;
    float FXAA_REDUCE_MIN = 1.0 / 128.0;

    vec2 texel = u_viewTexel.xy;

    vec3 rgbNW = texture2D(tex, uv + vec2(-1.0, -1.0) * texel).xyz;
    vec3 rgbNE = texture2D(tex, uv + vec2( 1.0, -1.0) * texel).xyz;
    vec3 rgbSW = texture2D(tex, uv + vec2(-1.0,  1.0) * texel).xyz;
    vec3 rgbSE = texture2D(tex, uv + vec2( 1.0,  1.0) * texel).xyz;
    vec3 rgbM  = texture2D(tex, uv).xyz;

    vec3 luma = vec3(0.299, 0.587, 0.114);

    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = clamp(dir * rcpDirMin, vec2(-FXAA_SPAN_MAX), vec2(FXAA_SPAN_MAX)) * texel;

    vec3 rgbA = 0.5 * (texture2D(tex, uv + dir * (1.0 / 3.0 - 0.5)).xyz + texture2D(tex, uv + dir * (2.0 / 3.0 - 0.5)).xyz);

    vec3 rgbB = rgbA * 0.5 + 0.25 * (texture2D(tex, uv + dir * (0.0 / 3.0 - 0.5)).xyz + texture2D(tex, uv + dir * (3.0 / 3.0 - 0.5)).xyz);

    float lumaB = dot(rgbB, luma);

    if (lumaB < lumaMin || lumaB > lumaMax)
        return rgbA;

    return rgbB;
}

void main()
{
    if (u_postprocess_enabled == 0)
    {
        vec3 rawColor = texture2D(u_screenTexture, v_texcoord0).rgb;
        gl_FragColor = vec4(pow(rawColor, vec3_splat(1.0/u_Gamma)), 1.0);
        return;
    }

    // Chromatic Aberration
    vec2 caDir = (v_texcoord0 - 0.5) * u_chromaStrength;
    
    vec3 sampledColor;

    sampledColor.r = (u_fxaa == 1) ? ApplyFXAA(u_screenTexture, v_texcoord0 - caDir).r : texture2D(u_screenTexture, v_texcoord0 - caDir).r;
    sampledColor.g = (u_fxaa == 1) ? ApplyFXAA(u_screenTexture, v_texcoord0 - caDir * 0.5).g : texture2D(u_screenTexture, v_texcoord0 - caDir * 0.5).g;
    sampledColor.b = (u_fxaa == 1) ? ApplyFXAA(u_screenTexture, v_texcoord0).b : texture2D(u_screenTexture, v_texcoord0).b;

    vec3 hdrColor = sampledColor;

    float exposure = u_exposureData[0].x;
    hdrColor *= exposure;

    // SSAO
    if (u_ssao_enabled == 1)
    {
        hdrColor *= texture2D(u_ssaoTexture, v_texcoord0).r;
    }
	
    // SSR
    if (u_ssr_enabled == 1)
    {
        vec4 ssr = texture2D(u_ssrTexture, v_texcoord0);
        hdrColor += ssr.rgb * ssr.a;
    }

    // Bloom
    if (u_bloom_enabled == 1)
    {
        vec3 bloom = texture2D(u_bloomTexture, v_texcoord0).rgb * u_bloom_intensity;
        hdrColor += bloom;
    }

    // Volumetrics
    if (u_volumetrics_enabled == 1)
    {
        hdrColor += texture2D(u_volumetricTexture, v_texcoord0).rgb;
    }

    // Fog
    float depth = texture2D(u_depthTexture, v_texcoord0).r;
    if (u_fogEnabled && (u_fogAffectsSky || depth < 0.9999))
    {
        float z = depth;
        vec2 ndcXY = vec2(v_texcoord0.x * 2.0 - 1.0, (1.0 - v_texcoord0.y) * 2.0 - 1.0);
        vec4 clipSpace = vec4(ndcXY, z, 1.0);
        vec4 viewSpace = mul(u_invProj, clipSpace);
        vec3 viewPos = viewSpace.xyz / viewSpace.w;

        float radialDist = length(viewPos);
        float fogFactor = smoothstep(u_fogStart, u_fogEnd, radialDist);
        hdrColor = mix(hdrColor, u_fogColor.xyz, fogFactor);
    }

    // Black and White
    float luminance = dot(hdrColor, vec3(0.299, 0.587, 0.114));
    hdrColor = mix(hdrColor, vec3_splat(luminance), u_bwStrength);
    
    // Negative
    vec3 negativeColor = 1.0 - hdrColor;
    hdrColor = mix(hdrColor, negativeColor, u_negativeStrength);

    // Sepia
    vec3 sepiaColor;
    sepiaColor.r = (hdrColor.r * 0.393) + (hdrColor.g * 0.769) + (hdrColor.b * 0.189);
    sepiaColor.g = (hdrColor.r * 0.349) + (hdrColor.g * 0.686) + (hdrColor.b * 0.168);
    sepiaColor.b = (hdrColor.r * 0.272) + (hdrColor.g * 0.534) + (hdrColor.b * 0.131);
    hdrColor = mix(hdrColor, sepiaColor, u_sepiaStrength);

    // Vignette
    float vignette = smoothstep(0.8, 0.2, length(v_texcoord0 - 0.5));
    hdrColor *= mix(1.0, vignette, u_vignetteStrength);

    // Film Grain
    float grain = (random(v_texcoord0 + u_time) - 0.5) * u_grainStrength;
    hdrColor += grain;

    // Tone mapping
    if (u_tonemap_enabled == 1)
    {
        hdrColor = AcesFilm(hdrColor);
    }

    // Gamma correct
    hdrColor = pow(hdrColor, vec3_splat(1.0 / u_Gamma));

    gl_FragColor = vec4(hdrColor, 1.0);
}