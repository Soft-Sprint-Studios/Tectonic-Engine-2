layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec2 aLmCoord1;
layout (location = 3) in vec2 aLmCoord2;
layout (location = 4) in vec2 aLmCoord3;
layout (location = 5) in vec2 aLmCoord4;
layout (location = 6) in vec3 aColor;
layout (location = 7) in vec3 aColor2;
layout (location = 8) in vec3 aColor3;
layout (location = 9) in vec3 aNormal;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec2 TexCoord;
out vec2 LmCoord1;
out vec2 LmCoord2;
out vec2 LmCoord3;
out vec2 LmCoord4;
out vec3 Color;
out vec3 Color2;
out vec3 Color3;
out vec3 FragPos;
out vec3 Normal;

void main()
{
    FragPos = vec3(u_model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(u_model))) * aNormal;
    
    TexCoord = aTexCoord;
    LmCoord1 = aLmCoord1;
    LmCoord2 = aLmCoord2;
    LmCoord3 = aLmCoord3;
    LmCoord4 = aLmCoord4;
    
    Color = aColor;
    Color2 = aColor2;
    Color3 = aColor3;
    
    gl_Position = u_projection * u_view * u_model * vec4(aPos, 1.0);
}