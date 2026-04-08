layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in vec4 vCol[];
in float vSize[];

out vec2 fTex;
out vec4 fCol;

uniform mat4 u_proj;
uniform mat4 u_view;
uniform vec3 u_right;
uniform vec3 u_up;

void main()
{
    vec3 p = gl_in[0].gl_Position.xyz;
    float s = vSize[0];
    fCol = vCol[0];

    vec3 corners[4] = vec3[](p + (-u_right + u_up) * s, p + (-u_right - u_up) * s, p + (u_right + u_up) * s, p + (u_right - u_up) * s);

    vec2 uvs[4] = vec2[](vec2(0, 1), vec2(0, 0), vec2(1, 1), vec2(1, 0));

    for (int i = 0; i < 4; i++)
    {
        gl_Position = u_proj * u_view * vec4(corners[i], 1.0);
        fTex = uvs[i];
        EmitVertex();
    }

    EndPrimitive();
}