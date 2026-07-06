$input v_texcoord0

#include <bgfx_shader.sh>
#include "common.sh"
#include "lightmap.sh"

SAMPLER2D(s_tex0, 0);
SAMPLER2D(s_tex1, 1);

uniform vec4 u_mode;
#define u_modeVal int(u_mode.x)

void main()
{
    vec3 finalColor = vec3_splat(0.0);

    if (u_modeVal == 0)
    {
        float depth = texture2D(s_tex0, v_texcoord0).r;
        float linearDepth = (2.0 * 0.1) / (1000.0 + 0.1 - depth * (1000.0 - 0.1));
        finalColor = vec3_splat(linearDepth * 10.0); 
    }
    else if (u_modeVal == 1)
    {
        vec2 normalData = texture2D(s_tex0, v_texcoord0).rg;
        vec3 worldNormal = DecodeNormal(normalData);
        finalColor = worldNormal * 0.5 + 0.5;
    }
    else if (u_modeVal == 2)
    {
        finalColor = texture2D(s_tex0, v_texcoord0).rgb;
    }
    else if (u_modeVal == 3)
    {
        vec2 packedFloat = texture2D(s_tex0, v_texcoord0).rg;
        vec2 lmCoord;
        vec2 lmSize;
        UnpackLightmapUV(packedFloat, lmCoord, lmSize);
        finalColor = vec3(lmCoord, 0.0);
    }
    else if (u_modeVal == 4)
    {
        float metallic = texture2D(s_tex0, v_texcoord0).r;
        finalColor = vec3_splat(metallic);
    }
    else if (u_modeVal == 5)
    {
        float roughness = texture2D(s_tex0, v_texcoord0).g;
        finalColor = vec3_splat(roughness);
    }
    else if (u_modeVal == 6)
    {
        float ao = texture2D(s_tex0, v_texcoord0).b;
        finalColor = vec3_splat(ao);
    }
    else if (u_modeVal == 7)
    {
        float tx = texture2D(s_tex0, v_texcoord0).a;
        float ty = texture2D(s_tex1, v_texcoord0).a;
        
        vec3 tsN = vec3(tx * 2.0 - 1.0, ty * 2.0 - 1.0, 0.0);
        tsN.z = sqrt(max(0.0, 1.0 - dot(tsN.xy, tsN.xy)));
        finalColor = tsN * 0.5 + 0.5;
    }

    gl_FragColor = vec4(finalColor, 1.0);
}