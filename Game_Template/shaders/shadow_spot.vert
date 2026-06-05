layout (location = 0) in vec3 aPos;
layout (location = 7) in uvec4 aJoints;
layout (location = 8) in vec4 aWeights;

uniform mat4 u_lightSpaceMatrix;
uniform mat4 u_model;
uniform int u_isInstanced;

uniform mat4 u_bones[128];
uniform bool u_isAnimated;

layout(std430, binding = 4) readonly buffer InstanceTransforms 
{
    mat4 transforms[];
};

out vec3 v_FragPos;

void main()
{
    mat4 modelMat = u_isInstanced == 1 ? transforms[gl_InstanceID] : u_model;
	
    if (u_isAnimated)
    {
        mat4 skinMat = aWeights.x * u_bones[aJoints.x] + aWeights.y * u_bones[aJoints.y] + aWeights.z * u_bones[aJoints.z] + aWeights.w * u_bones[aJoints.w];
        modelMat = modelMat * skinMat;
    }
	
    vec4 worldPos = modelMat * vec4(aPos, 1.0);
    v_FragPos = worldPos.xyz;
    gl_Position = u_lightSpaceMatrix * worldPos;
}