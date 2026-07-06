$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);

uniform vec4 u_overlayParams;
#define u_alpha u_overlayParams.x

void main()
{
    vec4 tex = texture2D(s_texture, v_texcoord0);
    gl_FragColor = vec4(tex.rgb, tex.a * u_alpha);
}