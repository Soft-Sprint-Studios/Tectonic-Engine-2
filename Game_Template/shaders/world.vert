layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec2 aLmCoord1;
layout (location = 3) in vec2 aLmCoord2;
layout (location = 4) in vec2 aLmCoord3;
layout (location = 5) in vec2 aLmCoord4;
layout (location = 6) in vec4 aColor;
layout (location = 7) in vec4 aColor2;
layout (location = 8) in vec4 aColor3;
layout (location = 9) in vec3 aNormal;
layout (location = 10) in vec3 aTangent;
layout (location = 11) in vec3 aBitangent;
layout (location = 12) in vec2 aLmCoord5;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform int u_isInstanced;
uniform int u_totalVertices;
uniform int u_vertexOffset;

layout(std430, binding = 4) readonly buffer InstanceTransforms 
{
    mat4 transforms[];
};

layout(std430, binding = 5) readonly buffer InstanceColors 
{
    vec4 vertColors[];
};

out centroid vec2 TexCoord;
out centroid vec2 LmCoord1;
out centroid vec2 LmCoord2;
out centroid vec2 LmCoord3;
out centroid vec2 LmCoord4;
out centroid vec2 LmCoord5;
out centroid vec4 Color;
out centroid vec4 Color2;
out centroid vec4 Color3;
out vec3 FragPos;
out centroid mat3 TBN;

void main()
{
    mat4 modelMat = u_isInstanced == 1 ? transforms[gl_InstanceID] : u_model;

    FragPos = vec3(modelMat * vec4(aPos, 1.0));
    mat3 normalMatrix = mat3(transpose(inverse(modelMat)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N);
    B = normalize(cross(N, T)); 
    
    TBN = mat3(T, B, N);
    
    TexCoord = aTexCoord;
    LmCoord1 = aLmCoord1;
    LmCoord2 = aLmCoord2;
    LmCoord3 = aLmCoord3;
    LmCoord4 = aLmCoord4;
    LmCoord5 = aLmCoord5;
    
    vec4 c1 = aColor;
    vec4 c2 = aColor2;
    vec4 c3 = aColor3;

    if (u_isInstanced == 1) 
    {
        int baseIdx = (gl_InstanceID * u_totalVertices + u_vertexOffset + gl_VertexID) * 3;
        c1 = vertColors[baseIdx];
        c2 = vertColors[baseIdx + 1];
        c3 = vertColors[baseIdx + 2];
    }

    Color = c1;
    Color2 = c2;
    Color3 = c3;
    
    gl_Position = u_projection * u_view * modelMat * vec4(aPos, 1.0);
}