layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
out vec4 ScreenPos;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    TexCoord = aTexCoord;
    ScreenPos = u_projection * u_view * u_model * vec4(aPos, 1.0);
    gl_Position = ScreenPos;
}