layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec2 aLmCoord;
layout (location = 3) in vec2 aLmCoord2;
layout (location = 4) in vec2 aLmCoord3;
layout (location = 5) in vec2 aLmCoord4;
layout (location = 9) in vec3 aNormal;
layout (location = 10) in vec3 aTangent;
layout (location = 11) in vec3 aBitangent;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;

out centroid vec3 v_FragPos;
out vec2 v_TexCoord;
out vec2 v_LmCoord;
out vec2 v_LmCoord2;
out vec2 v_LmCoord3;
out vec2 v_LmCoord4;
out centroid mat3 v_TBN;

void main()
{
    vec4 worldPos = u_model * vec4(aPos, 1.0);
    v_FragPos = worldPos.xyz;
    v_TexCoord = aTexCoord;
    v_LmCoord = aLmCoord;
    v_LmCoord2 = aLmCoord2;
    v_LmCoord3 = aLmCoord3;
    v_LmCoord4 = aLmCoord4;
    
    mat3 normalMatrix = mat3(transpose(inverse(u_model)));
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    v_TBN = mat3(T, B, N);

    gl_Position = u_projection * u_view * worldPos;
}