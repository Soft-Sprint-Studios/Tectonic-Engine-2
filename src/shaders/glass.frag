$input v_texcoord0, v_screenPos, v_tbn0, v_tbn1, v_tbn2

#include <bgfx_shader.sh>

SAMPLER2D(s_refractTex, 10);
SAMPLER2D(s_normalMap, 0);

uniform vec4 u_glassParams;
#define u_amount u_glassParams.x

void main()
{
    vec2 screenUV = (v_screenPos.xy / v_screenPos.w) * 0.5 + 0.5;

    screenUV.y = 1.0 - screenUV.y;

    vec3 tN = normalize(v_tbn0);
    vec3 tB = normalize(v_tbn1);
    vec3 tZ = normalize(v_tbn2);

    vec3 normal = texture2D(s_normalMap, v_texcoord0).rgb * 2.0 - 1.0;
    vec3 worldNormal = normalize(tN * normal.x + tB * normal.y + tZ * normal.z);

    vec2 offset = worldNormal.xy * u_amount;
    vec3 scene = texture2D(s_refractTex, screenUV + offset).rgb;

    gl_FragColor = vec4(scene, 1.0);
}