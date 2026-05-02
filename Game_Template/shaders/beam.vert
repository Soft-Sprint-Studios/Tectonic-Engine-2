layout (location = 0) in vec2 aPos;

uniform mat4 u_view;
uniform mat4 u_projection;
uniform vec3 u_startPos;
uniform vec3 u_endPos;
uniform float u_width;
uniform vec3 u_viewPos;

out vec2 TexCoords;

void main()
{
    vec3 dir = u_endPos - u_startPos;
    vec3 pos = u_startPos + dir * aPos.x;

    vec3 viewDir = normalize(u_viewPos - pos);
    vec3 side = normalize(cross(dir, viewDir));
    
    pos += side * aPos.y * (u_width * 0.5);
    
    TexCoords = vec2(aPos.y * 0.5 + 0.5, aPos.x);
    gl_Position = u_projection * u_view * vec4(pos, 1.0);
}