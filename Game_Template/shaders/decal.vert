layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
out mat3 TBN;

uniform mat4 u_view;
uniform mat4 u_projection;

layout(std430, binding = 8) readonly buffer DecalInstances
{
    mat4 transforms[];
};

void main()
{
    mat4 model = transforms[gl_InstanceID];
    TexCoord = aTexCoord;

    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vec3 N = normalize(normalMatrix * vec3(0.0, 0.0, 1.0));
    vec3 T = normalize(normalMatrix * vec3(1.0, 0.0, 0.0));
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);

    gl_Position = u_projection * u_view * model * vec4(aPos, 1.0);
}