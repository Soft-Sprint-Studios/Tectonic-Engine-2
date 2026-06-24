#include "common.h"
#include "lightmap.h"

out vec4 FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D u_debugTex;
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
        vec2 packedFloat = texture(u_debugTex, TexCoords).rg;
        vec2 lmCoord, lmSize;
        UnpackLightmapUV(packedFloat, lmCoord, lmSize);
        FragColor = vec4(lmCoord, 0.0, 1.0);
    }
    else if (u_mode == 4)
    {
        float metal = texture(u_debugTex, TexCoords).r;
        FragColor = vec4(vec3(metal), 1.0);
    }
    else if (u_mode == 5)
    {
        float rough = texture(u_debugTex, TexCoords).g;
        FragColor = vec4(vec3(rough), 1.0);
    }
    else if (u_mode == 6)
    {
        float ao = texture(u_debugTex, TexCoords).b;
        FragColor = vec4(vec3(ao), 1.0);
    }
    else if (u_mode == 7)
    {
        vec2 ts = texture(u_debugTex, TexCoords).ba;
        FragColor = vec4(ts, 1.0, 1.0);
    }
    else if (u_mode == 8)
    {
        float emissive = 1.0 - texture(u_debugTex, TexCoords).a;
        FragColor = vec4(vec3(emissive), 1.0);
    }
}