layout (location = 0) in vec3 aPos;

uniform mat4 u_model;
uniform int u_isInstanced;

layout(std430, binding = 4) readonly buffer InstanceTransforms 
{
    mat4 transforms[];
};

void main()
{
    mat4 modelMat = u_isInstanced == 1 ? transforms[gl_InstanceID] : u_model;
    gl_Position = modelMat * vec4(aPos, 1.0);
}