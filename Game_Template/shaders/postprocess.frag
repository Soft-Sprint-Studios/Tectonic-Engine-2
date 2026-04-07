#version 460 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float u_time;

uniform float u_vignetteStrength;
uniform float u_chromaStrength;
uniform float u_grainStrength;
uniform float u_bwStrength;

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

void main()
{
    // Chromatic Aberration
    vec2 redOffset = u_chromaStrength * (TexCoords - 0.5);
    vec2 greenOffset = -u_chromaStrength * (TexCoords - 0.5) * 0.5;
    float r = texture(screenTexture, TexCoords - redOffset).r;
    float g = texture(screenTexture, TexCoords - greenOffset).g;
    float b = texture(screenTexture, TexCoords).b;
    vec3 hdrColor = vec3(r, g, b);

    // Black and White
    float luminance = dot(hdrColor, vec3(0.299, 0.587, 0.114));
    hdrColor = mix(hdrColor, vec3(luminance), u_bwStrength);

    // Vignette
    float vignette = smoothstep(0.8, 0.2, length(TexCoords - 0.5));
    hdrColor *= mix(1.0, vignette, u_vignetteStrength);

    // Film Grain
    float grain = (random(TexCoords + u_time) - 0.5) * u_grainStrength;
    hdrColor += grain;

    FragColor = vec4(hdrColor, 1.0);
}