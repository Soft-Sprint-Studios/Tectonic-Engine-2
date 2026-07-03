$input v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(texY, 10);
SAMPLER2D(texCb, 11);
SAMPLER2D(texCr, 12);

void main()
{
    float y = (texture2D(texY, v_texcoord0).r - (16.0 / 255.0)) * 1.16438;
    float cb = texture2D(texCb, v_texcoord0).r - 0.5;
    float cr = texture2D(texCr, v_texcoord0).r - 0.5;

    float r = y + (1.59603 * cr);
    float g = y - (0.39176 * cb) - (0.81297 * cr);
    float b = y + (2.01723 * cb);

    vec3 rgb = vec3(r, g, b);
    rgb = pow(max(rgb, vec3_splat(0.0)), vec3_splat(2.2));

    gl_FragColor = vec4(rgb, 1.0);
}