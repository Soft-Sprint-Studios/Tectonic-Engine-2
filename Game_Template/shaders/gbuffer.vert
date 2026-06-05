layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec2 aLmCoord;
layout (location = 3) in vec2 aLmSize;
layout (location = 4) in float aAlpha;
layout (location = 5) in vec3 aNormal;
layout (location = 6) in vec4 aTangent;
layout (location = 7) in uvec4 aJoints;
layout (location = 8) in vec4 aWeights;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform int u_isInstanced;
uniform int u_totalVertices;
uniform int u_vertexOffset;

uniform mat4 u_bones[128];
uniform bool u_isAnimated;

layout(std430, binding = 4) readonly buffer InstanceTransforms 
{
    mat4 transforms[];
};

layout(std430, binding = 7) readonly buffer InstanceLM 
{
    vec4 lmTransforms[];
};

out vec2 TexCoord;
out vec2 v_LmCoord;
out vec2 v_LmSize;
out float v_alpha;
out vec3 FragPos;
out mat3 TBN;

void main()
{
    mat4 modelMat = u_isInstanced == 1 ? transforms[gl_InstanceID] : u_model;
	
    if (u_isAnimated)
    {
        mat4 skinMat = aWeights.x * u_bones[aJoints.x] + aWeights.y * u_bones[aJoints.y] + aWeights.z * u_bones[aJoints.z] +aWeights.w * u_bones[aJoints.w];
        modelMat = modelMat * skinMat;
    }

    FragPos = vec3(modelMat * vec4(aPos, 1.0));
    mat3 normalMatrix = mat3(transpose(inverse(modelMat)));
    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = vec3(0.0);

    if (length(aTangent.xyz) > 0.0001)
    {
        T = normalize(normalMatrix * aTangent.xyz);
        T = normalize(T - dot(T, N) * N);
    }
    else
    {
        vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        T = normalize(cross(up, N));
    }

    vec3 B = cross(N, T);
    if (length(aTangent.xyz) > 0.0001)
    {
        B *= aTangent.w;
    }
    
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