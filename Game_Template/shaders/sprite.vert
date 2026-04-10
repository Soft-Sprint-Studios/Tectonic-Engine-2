layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 u_view;
uniform mat4 u_projection;
uniform vec3 u_worldPos;
uniform vec2 u_scale;
uniform vec3 u_viewRight;
uniform vec3 u_viewUp;
uniform bool u_cylindrical;

out vec2 TexCoord;

void main()
{
    TexCoord = aTexCoord;
    vec3 vUp = u_cylindrical ? vec3(0.0, 1.0, 0.0) : u_viewUp;
    vec3 pos = u_worldPos + (u_viewRight * aPos.x * u_scale.x) + (vUp * aPos.y * u_scale.y);
    gl_Position = u_projection * u_view * vec4(pos, 1.0);
}