out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTexture;
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

layout(std430, binding = 2) buffer LumData 
{
    float u_exposure;
};

float random(vec2 st) 
{
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

float LinearizeDepth(float depth)
{
    vec4 clipSpace = vec4(TexCoords * 2.0 - 1.0, depth, 1.0);
    vec4 viewSpace = u_invProjection * clipSpace;
    return viewSpace.z / viewSpace.w;
}

void main()
{
    if (u_postprocess_enabled == 0)
    {
	    vec3 rawColor = texture(screenTexture, TexCoords).rgb;
        FragColor = vec4(pow(rawColor, vec3(1.0/u_Gamma)), 1.0);
        return;
    }

    // Chromatic Aberration
    vec2 redOffset = u_chromaStrength * (TexCoords - 0.5);
    vec2 greenOffset = -u_chromaStrength * (TexCoords - 0.5) * 0.5;
    float r = texture(screenTexture, TexCoords - redOffset).r;
    float g = texture(screenTexture, TexCoords - greenOffset).g;
    float b = texture(screenTexture, TexCoords).b;
    vec3 hdrColor = vec3(r, g, b);

    hdrColor *= u_exposure;

    // Black and White
    float luminance = dot(hdrColor, vec3(0.299, 0.587, 0.114));
    hdrColor = mix(hdrColor, vec3(luminance), u_bwStrength);

    // Vignette
    float vignette = smoothstep(0.8, 0.2, length(TexCoords - 0.5));
    hdrColor *= mix(1.0, vignette, u_vignetteStrength);

    // Film Grain
    float grain = (random(TexCoords + u_time) - 0.5) * u_grainStrength;
    hdrColor += grain;

    float depth = texture(depthTexture, TexCoords).r;

    if (u_fogAffectsSky == 1 || depth < 0.9999)
    {
        float linearDepth = -LinearizeDepth(depth);
        float fogFactor = smoothstep(u_fogStart, u_fogEnd, linearDepth);
        hdrColor = mix(hdrColor, u_fogColor, fogFactor);
    }

    // Gamma correct
    hdrColor = pow(hdrColor, vec3(1.0 / u_Gamma));
    FragColor = vec4(hdrColor, 1.0);
}