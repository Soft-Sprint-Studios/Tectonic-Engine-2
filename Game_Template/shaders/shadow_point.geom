layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 u_shadowMatrices[6];
uniform int u_shadowLayer;
out vec4 FragPos; 

void main()
{
    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = (u_shadowLayer * 6) + face;
        for(int i = 0; i < 3; ++i) 
        {
            FragPos = gl_in[i].gl_Position;
            gl_Position = u_shadowMatrices[face] * FragPos;
            EmitVertex();
        }    
        EndPrimitive();
    }
}