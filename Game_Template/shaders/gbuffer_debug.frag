#include "common.glsl"

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D u_debugTex;
uniform int u_mode;

void main()
{
    if (u_mode == 0)
    {
        float depth = texture(u_debugTex, TexCoords).r;
        float near = 0.1;
        float far = 100.0;
        float linearDepth = (2.0 * near) / (far + near - depth * (far - near));
        FragColor = vec4(vec3(linearDepth * 5.0), 1.0);
    }
    else if (u_mode == 1)
    {
        vec2 encoded = texture(u_debugTex, TexCoords).rg;
        vec3 N = DecodeNormal(encoded);
        FragColor = vec4(N * 0.5 + 0.5, 1.0);
    }
    else if (u_mode == 2)
    {
        FragColor = vec4(texture(u_debugTex, TexCoords).rgb, 1.0);
    }
    else if (u_mode == 3)
    {
        FragColor = vec4(texture(u_debugTex, TexCoords).rgb * 2.0, 1.0);
    }
    else if (u_mode == 4)
    {
        float spec = texture(u_debugTex, TexCoords).a;
        FragColor = vec4(vec3(spec), 1.0);
    }
}