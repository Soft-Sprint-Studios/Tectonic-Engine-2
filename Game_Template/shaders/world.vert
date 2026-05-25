layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec2 aLmCoord;
layout (location = 3) in vec2 aLmSize;
layout (location = 4) in float aAlpha;
layout (location = 5) in vec3 aNormal;
layout (location = 6) in vec4 aTangent;

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
out centroid vec2 v_LmCoord;
out centroid vec2 v_LmSize;
out centroid float v_alpha;
out vec3 FragPos;
out centroid mat3 TBN;

void main()
{
    mat4 modelMat = u_isInstanced == 1 ? transforms[gl_InstanceID] : u_model;

    FragPos = vec3(modelMat * vec4(aPos, 1.0));
    mat3 normalMatrix = mat3(transpose(inverse(modelMat)));
    vec3 T = normalize(normalMatrix * aTangent.xyz);
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * aTangent.w;
    
    TBN = mat3(T, B, N);
    
    TexCoord = aTexCoord;
    v_LmCoord = aLmCoord;
    v_LmSize = aLmSize;

    if (u_isInstanced == 1) 
    {
        TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
        int transBase = gl_InstanceID * 4;
        v_LmCoord = aTexCoord * lmTransforms[transBase + 0].zw + lmTransforms[transBase + 0].xy;
        v_LmSize = lmTransforms[transBase + 0].zw;
    }

    v_alpha = aAlpha;
    
    gl_Position = u_projection * u_view * modelMat * vec4(aPos, 1.0);
}