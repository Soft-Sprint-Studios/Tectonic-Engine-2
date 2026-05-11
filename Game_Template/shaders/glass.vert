layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 9) in vec3 aNormal;
layout (location = 10) in vec3 aTangent;
layout (location = 11) in vec3 aBitangent;

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
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    TBN = mat3(T, B, N);

    ScreenPos = u_projection * u_view * worldPos;
    gl_Position = ScreenPos;
}