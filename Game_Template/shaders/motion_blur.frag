out vec4 FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D u_sceneTex;
layout(binding = 1) uniform sampler2D u_depthTex;

uniform mat4 u_invViewProj;
uniform mat4 u_prevViewProj;

uniform int u_samples;
uniform float u_blurScale;

void main()
{
    float depth = texture(u_depthTex, TexCoords).r;

    vec4 ndc = vec4(TexCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    vec4 worldPos = u_invViewProj * ndc;
    worldPos /= worldPos.w;

    vec4 prevNdc = u_prevViewProj * worldPos;
    prevNdc /= prevNdc.w;

    vec2 prevUV = prevNdc.xy * 0.5 + 0.5;
    vec2 velocity = (TexCoords - prevUV) * u_blurScale;
    
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