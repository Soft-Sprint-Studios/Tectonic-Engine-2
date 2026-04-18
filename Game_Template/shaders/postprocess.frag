out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D u_screenTexture;
uniform sampler2D u_depthTexture;
uniform sampler2D u_bloomTexture;
uniform sampler2D u_volumetricTexture;
uniform sampler2D u_ssaoTexture;
uniform float u_time;

uniform float u_vignetteStrength;
uniform float u_chromaStrength;
uniform float u_grainStrength;
uniform float u_bwStrength;
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
    vec3 hdrColor = vec3(r, g, b);

    hdrColor *= u_exposure;
	
    // SSAO
    if (u_ssao_enabled == 1)
    {
        hdrColor *= texture(u_ssaoTexture, TexCoords).r;
    }

    // Black and White
    float luminance = dot(hdrColor, vec3(0.299, 0.587, 0.114));
    hdrColor = mix(hdrColor, vec3(luminance), u_bwStrength);

    // Vignette
    float vignette = smoothstep(0.8, 0.2, length(TexCoords - 0.5));
    hdrColor *= mix(1.0, vignette, u_vignetteStrength);

    // Film Grain
    float grain = (random(TexCoords + u_time) - 0.5) * u_grainStrength;
    hdrColor += grain;

    // Bloom
    if (u_bloom_enabled == 1)
    {
        hdrColor += texture(u_bloomTexture, TexCoords).rgb * u_bloom_intensity;
    }

    // Volumetrics
    if (u_volumetrics_enabled == 1)
    {
        hdrColor += texture(u_volumetricTexture, TexCoords).rgb;
    }

    float depth = texture(u_depthTexture, TexCoords).r;

    if (u_fogEnabled == 1 && (u_fogAffectsSky == 1 || depth < 0.9999))
    {
        vec3 viewPos = GetViewPos(depth);
        float radialDist = length(viewPos);
        float fogFactor = smoothstep(u_fogStart, u_fogEnd, radialDist);
        hdrColor = mix(hdrColor, u_fogColor, fogFactor);
    }

    // Tone mapping
    if (u_tonemap_enabled == 1)
    {
        hdrColor = ACESFilm(hdrColor);
    }

    // Gamma correct
    hdrColor = pow(hdrColor, vec3(1.0 / u_Gamma));

    FragColor = vec4(hdrColor, 1.0);
}