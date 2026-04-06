#version 460 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;

    FragColor = vec4(hdrColor, 1.0);
}