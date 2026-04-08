in vec2 fTex;
in vec4 fCol;
out vec4 Color;

uniform sampler2D u_tex;

void main() 
{
    Color = texture(u_tex, fTex) * fCol;
    if(Color.a < 0.1) 
        discard;
}