$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_texture, 0);

uniform vec4 u_textColor;
uniform vec4 u_uiParams;

void main()
{
    float alpha = 1.0;

    if (u_uiParams.x > 0.5)
    {
        float dist = length(v_texcoord0 - vec2_splat(0.5));
        if (dist > 0.5) 
        {
            discard;
        }
    }
    else
    {
        alpha = texture2D(s_texture, v_texcoord0).a;
    }

    gl_FragColor = vec4(u_textColor.rgb, u_textColor.a * alpha);
}