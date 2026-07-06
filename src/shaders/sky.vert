$input a_position
$output v_dir, v_fragPos

#include <bgfx_shader.sh>

void main()
{
    v_dir = a_position;
    v_fragPos = a_position;
    
    vec4 pos = mul(u_viewProj, vec4(a_position, 1.0));
    gl_Position = pos.xyww;
}