layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aCol;
layout (location = 2) in float aSize;

out vec4 vCol;
out float vSize;

void main() 
{
    vCol = aCol;
    vSize = aSize;
    gl_Position = vec4(aPos, 1.0);
}