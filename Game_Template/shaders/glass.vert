layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 7) in vec3 aNormal;
layout (location = 8) in vec4 aTangent;

out vec2 TexCoord;
out vec4 ScreenPos;
out mat3 TBN;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    vec4 worldPos = u_model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
    
    mat3 normalMatrix = mat3(transpose(inverse(u_model)));
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent.xyz);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * aTangent.w;
    TBN = mat3(T, B, N);

    ScreenPos = u_projection * u_view * worldPos;
    gl_Position = ScreenPos;
}