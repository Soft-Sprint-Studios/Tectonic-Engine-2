out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D u_screenTexture;
uniform sampler2D u_depthTexture;
uniform sampler2D u_bloomTexture;
uniform sampler2D u_volumetricTexture;
uniform sampler2D u_ssaoTexture;
uniform sampler2D u_lensDirtTexture;
uniform float u_time;

uniform float u_vignetteStrength;
uniform float u_chromaStrength;
uniform float u_grainStrength;
uniform float u_bwStrength;
uniform float u_negativeStrength;
uniform float u_sepiaStrength;
uniform float u_sharpenStrength;
uniform float u_lensDirtStrength;
uniform float u_Gamma;
uniform int u_postprocess_enabled;
uniform mat4 u_invProjection;
uniform int u_fogEnabled;
uniform vec3 u_fogColor;
uniform float u_fogStart;
uniform float u_fogEnd;
uniform int u_fogAffectsSky;
uniform int u_bloom_enabled;
uniform float u_bloom_intensity;
uniform int u_volumetrics_enabled;
uniform int u_ssao_enabled;
uniform int u_tonemap_enabled;
uniform int u_fxaa;
uniform float u_fxaaStrength;

layout(std430, binding = 2) buffer LumData 
{
    float u_exposure;
};

float random(vec2 st) 
{
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

vec3 GetViewPos(float depth)
{
    float z = depth * 2.0 - 1.0;
    vec4 clipSpace = vec4(TexCoords * 2.0 - 1.0, z, 1.0);
    vec4 viewSpace = u_invProjection * clipSpace;
    return viewSpace.xyz / viewSpace.w;
}

vec3 ACESFilm(vec3 x)
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

    vec2 texel = 1.0 / textureSize(tex, 0);

    vec3 rgbNW = texture(tex, uv + vec2(-1.0, -1.0) * texel).xyz;
    vec3 rgbNE = texture(tex, uv + vec2( 1.0, -1.0) * texel).xyz;
    vec3 rgbSW = texture(tex, uv + vec2(-1.0,  1.0) * texel).xyz;
    vec3 rgbSE = texture(tex, uv + vec2( 1.0,  1.0) * texel).xyz;
    vec3 rgbM  = texture(tex, uv).xyz;

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

    vec3 rgbA = 0.5 * (texture(tex, uv + dir * (1.0 / 3.0 - 0.5)).xyz + texture(tex, uv + dir * (2.0 / 3.0 - 0.5)).xyz);

    vec3 rgbB = rgbA * 0.5 + 0.25 * (texture(tex, uv + dir * (0.0 / 3.0 - 0.5)).xyz + texture(tex, uv + dir * (3.0 / 3.0 - 0.5)).xyz);

    float lumaB = dot(rgbB, luma);

    if (lumaB < lumaMin || lumaB > lumaMax)
        return rgbA;

    return rgbB;
}

void main()
{
    if (u_postprocess_enabled == 0)
    {
	    vec3 rawColor = texture(u_screenTexture, TexCoords).rgb;
        FragColor = vec4(pow(rawColor, vec3(1.0/u_Gamma)), 1.0);
        return;
    }

    // Chromatic Aberration
    vec2 redOffset = u_chromaStrength * (TexCoords - 0.5);
    vec2 greenOffset = -u_chromaStrength * (TexCoords - 0.5) * 0.5;
    float r = texture(u_screenTexture, TexCoords - redOffset).r;
    float g = texture(u_screenTexture, TexCoords - greenOffset).g;
    float b = texture(u_screenTexture, TexCoords).b;

    vec3 original = vec3(r, g, b);
    vec3 fxaaColor = original;

    if (u_fxaa == 1)
    {
        fxaaColor = ApplyFXAA(u_screenTexture, TexCoords);
    }

    vec3 hdrColor = mix(original, fxaaColor, u_fxaaStrength);

    hdrColor *= u_exposure;
	
    // SSAO
    if (u_ssao_enabled == 1)
    {
        hdrColor *= texture(u_ssaoTexture, TexCoords).r;
    }
	
    // Bloom
    if (u_bloom_enabled == 1)
    {
        vec3 bloom = texture(u_bloomTexture, TexCoords).rgb * u_bloom_intensity;
        vec3 dirt = texture(u_lensDirtTexture, TexCoords).rgb * u_lensDirtStrength;
        hdrColor += bloom + (bloom * dirt);
    }

    // Volumetrics
    if (u_volumetrics_enabled == 1)
    {
        hdrColor += texture(u_volumetricTexture, TexCoords).rgb;
    }

    // Fog
    float depth = texture(u_depthTexture, TexCoords).r;
    if (u_fogEnabled == 1 && (u_fogAffectsSky == 1 || depth < 0.9999))
    {
        vec3 viewPos = GetViewPos(depth);
        float radialDist = length(viewPos);
        float fogFactor = smoothstep(u_fogStart, u_fogEnd, radialDist);
        hdrColor = mix(hdrColor, u_fogColor, fogFactor);
    }
	
    // Sharpening
    if (u_sharpenStrength > 0.0)
    {
        vec2 texel = 1.0 / textureSize(u_screenTexture, 0);
        vec3 center = hdrColor;
        vec3 left   = texture(u_screenTexture, TexCoords + vec2(-texel.x, 0.0)).rgb * u_exposure;
        vec3 right  = texture(u_screenTexture, TexCoords + vec2(texel.x, 0.0)).rgb * u_exposure;
        vec3 up     = texture(u_screenTexture, TexCoords + vec2(0.0, texel.y)).rgb * u_exposure;
        vec3 down   = texture(u_screenTexture, TexCoords + vec2(0.0, -texel.y)).rgb * u_exposure;

        hdrColor = center + (center - (left + right + up + down) * 0.25) * u_sharpenStrength;
    }

    // Black and White
    float luminance = dot(hdrColor, vec3(0.299, 0.587, 0.114));
    hdrColor = mix(hdrColor, vec3(luminance), u_bwStrength);
	
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
    float vignette = smoothstep(0.8, 0.2, length(TexCoords - 0.5));
    hdrColor *= mix(1.0, vignette, u_vignetteStrength);

    // Film Grain
    float grain = (random(TexCoords + u_time) - 0.5) * u_grainStrength;
    hdrColor += grain;

    // Tone mapping
    if (u_tonemap_enabled == 1)
    {
        hdrColor = ACESFilm(hdrColor);
    }

    // Gamma correct
    hdrColor = pow(hdrColor, vec3(1.0 / u_Gamma));

    FragColor = vec4(hdrColor, 1.0);
}