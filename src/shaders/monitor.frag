$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 10);
uniform vec4 u_monitorParams;
#define u_grayscale u_monitorParams.x
#define u_monitorWidth u_monitorParams.y
#define u_monitorHeight u_monitorParams.z
#define u_resolution u_monitorParams.w

void main()
{
    vec2 uvScale = vec2(u_resolution) / vec2(u_monitorWidth, u_monitorHeight);
    uvScale *= 0.0625;

    vec2 finalUV = fract(v_texcoord0 * uvScale);
    vec4 texColor = texture2D(s_texture, vec2(finalUV.x, finalUV.y));

    if (u_grayscale > 0.5)
    {
        vec3 weights = vec3(0.32, 0.59, 0.09);
        float gray = dot(texColor.rgb, weights);
        gl_FragColor = vec4(vec3_splat(gray), 1.0);
    }
    else
    {
        gl_FragColor = vec4(texColor.rgb * 10.0, 1.0);
    }
}