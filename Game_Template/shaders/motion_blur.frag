out vec4 FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D u_sceneTex;
layout(binding = 1) uniform sampler2D u_velocityTex;

uniform int u_samples;
uniform float u_blurScale;

void main()
{
    vec2 velocity = texture(u_velocityTex, TexCoords).rg * u_blurScale;
    
    if (length(velocity) < 0.0001)
    {
        FragColor = texture(u_sceneTex, TexCoords);
        return;
    }

    vec4 color = texture(u_sceneTex, TexCoords);
    for(int i = 1; i < u_samples; ++i)
    {
        vec2 offset = velocity * (float(i) / float(u_samples - 1) - 0.5);
        color += texture(u_sceneTex, TexCoords + offset);
    }
    
    FragColor = color / float(u_samples);
}