layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
out mat3 TBN;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    TexCoord = aTexCoord;

    mat3 normalMatrix = mat3(transpose(inverse(u_model)));
    vec3 N = normalize(normalMatrix * vec3(0.0, 0.0, 1.0));
    vec3 T = normalize(normalMatrix * vec3(1.0, 0.0, 0.0));
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);

    gl_Position = u_projection * u_view * u_model * vec4(aPos, 1.0);
}