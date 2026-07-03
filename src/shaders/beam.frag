$input v_texcoord0

#include <bgfx_shader.sh>

uniform vec4 u_beamColor;
uniform vec4 u_beamTime;

void main()
{
    vec2 centered_uv = v_texcoord0 - 0.5;

    float core_width = 0.05;
    float core = 1.0 - smoothstep(core_width, core_width + 0.1, abs(centered_uv.x));

    float glow = pow(1.0 - abs(centered_uv.x * 2.0), 4.0);

    float flicker = sin(v_texcoord0.y * 30.0 + u_beamTime.x * 20.0) * 0.5 + 0.5;
    float slow_pulse = sin(v_texcoord0.y * 5.0 - u_beamTime.x * 5.0) * 0.5 + 0.5;

    float dynamic_effect = (flicker * 0.7 + slow_pulse * 0.3) * glow;
    float intensity = core * 1.5 + glow * 0.3 + dynamic_effect * 0.5;

    vec3 final_color = u_beamColor.xyz * intensity;
    float alpha = clamp(intensity * 0.3, 0.0, 1.0);
    
    gl_FragColor = vec4(final_color, alpha);
}