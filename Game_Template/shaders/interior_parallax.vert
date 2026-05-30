layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 5) in vec3 aNormal;
layout (location = 6) in vec4 aTangent;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec3 v_FragPos;
out vec2 v_TexCoord;
out mat3 v_TBN;

void main()
{
    vec4 worldPos = u_model * vec4(aPos, 1.0);
    v_FragPos = worldPos.xyz;
    v_TexCoord = aTexCoord;

    mat3 normalMatrix = mat3(transpose(inverse(u_model)));
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent.xyz);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * aTangent.w;
    
    v_TBN = mat3(T, B, N);

    gl_Position = u_projection * u_view * worldPos;
}