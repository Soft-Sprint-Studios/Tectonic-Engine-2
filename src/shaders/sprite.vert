$input a_position, a_texcoord0
$output v_texcoord0

#include <bgfx_shader.sh>

uniform vec4 u_spriteParams;
#define u_scale       u_spriteParams.xy
#define u_cylindrical (u_spriteParams.z > 0.5)

uniform vec4 u_viewRightLocal;
uniform vec4 u_viewUpLocal;
uniform vec4 u_worldPosLocal;

void main()
{
    v_texcoord0 = a_texcoord0;
    vec3 vUp = u_cylindrical ? vec3(0.0, 1.0, 0.0) : u_viewUpLocal.xyz;
    vec3 pos = u_worldPosLocal.xyz 
             + (u_viewRightLocal.xyz * a_position.x * u_scale.x) 
             + (vUp * a_position.y * u_scale.y);

    gl_Position = mul(u_viewProj, vec4(pos, 1.0));
}