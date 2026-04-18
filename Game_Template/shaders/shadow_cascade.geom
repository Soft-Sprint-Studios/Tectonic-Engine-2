layout(triangles, invocations = 4) in;
layout(triangle_strip, max_vertices = 3) out;

layout(std430, binding = 6) readonly buffer LightSpaceMatrices 
{
    mat4 u_lightSpaceMatrices[4];
};

void main() {          
    for (int i = 0; i < 3; ++i) 
    {
        gl_Position = u_lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
        gl_Layer = gl_InvocationID;
        EmitVertex();
    }
    EndPrimitive();
}