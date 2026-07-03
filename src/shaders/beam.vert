$input a_position
$output v_texcoord0

#include <bgfx_shader.sh>

uniform vec4 u_beamParams;
#define u_width u_beamParams.x

uniform vec4 u_startPosLocal;
uniform vec4 u_endPosLocal;
uniform vec4 u_viewPosLocal;

void main()
{
    vec3 dir = u_endPosLocal.xyz - u_startPosLocal.xyz;
    vec3 pos = u_startPosLocal.xyz + dir * a_position.x;

    vec3 viewDir = normalize(u_viewPosLocal.xyz - pos);
    vec3 side = normalize(cross(dir, viewDir));
    
    pos += side * a_position.y * (u_width * 0.5);
    
    v_texcoord0 = vec2(a_position.y * 0.5 + 0.5, a_position.x);
    gl_Position = mul(u_viewProj, vec4(pos, 1.0));
}