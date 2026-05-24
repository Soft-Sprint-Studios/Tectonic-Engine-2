layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec2 aLmCoord1;
layout (location = 3) in vec2 aLmCoord2;
layout (location = 4) in vec2 aLmCoord3;
layout (location = 5) in vec2 aLmCoord4;
layout (location = 6) in float aAlpha;
layout (location = 7) in vec3 aNormal;
layout (location = 8) in vec3 aTangent;
layout (location = 9) in vec3 aBitangent;

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

layout(std430, binding = 7) readonly buffer InstanceLM 
{
    vec4 lmTransforms[];
};

out centroid vec2 TexCoord;
out centroid vec2 LmCoord1;
out centroid vec2 LmCoord2;
out centroid vec2 LmCoord3;
out centroid vec2 LmCoord4;
out centroid float v_alpha;
out vec3 FragPos;
out centroid mat3 TBN;

void main()
{
    mat4 modelMat = u_isInstanced == 1 ? transforms[gl_InstanceID] : u_model;

    FragPos = vec3(modelMat * vec4(aPos, 1.0));
    mat3 normalMatrix = mat3(transpose(inverse(modelMat)));
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 B = normalize(normalMatrix * aBitangent);
    T = normalize(T - dot(T, N) * N);
    
    TBN = mat3(T, B, N);
    
    TexCoord = aTexCoord;
    LmCoord1 = aLmCoord1;
    LmCoord2 = aLmCoord2;
    LmCoord3 = aLmCoord3;
    LmCoord4 = aLmCoord4;

    if (u_isInstanced == 1) 
    {
        int transBase = gl_InstanceID * 4;
        LmCoord1 = aTexCoord * lmTransforms[transBase + 0].zw + lmTransforms[transBase + 0].xy;
        LmCoord2 = aTexCoord * lmTransforms[transBase + 1].zw + lmTransforms[transBase + 1].xy;
        LmCoord3 = aTexCoord * lmTransforms[transBase + 2].zw + lmTransforms[transBase + 2].xy;
        LmCoord4 = aTexCoord * lmTransforms[transBase + 3].zw + lmTransforms[transBase + 3].xy;
    }

    v_alpha = aAlpha;
    
    gl_Position = u_projection * u_view * modelMat * vec4(aPos, 1.0);
}