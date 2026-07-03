$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);
uniform vec4 u_color;

void main()
{
    vec4 texColor = texture2D(s_texture, v_texcoord0);
    vec4 finalColor = texColor * u_color;
    if (finalColor.a < 0.01)
    {
        discard;
    }
    gl_FragColor = finalColor;
}