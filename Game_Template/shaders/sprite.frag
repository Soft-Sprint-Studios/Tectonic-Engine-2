out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D u_texture;
uniform vec4 u_color;

void main()
{
    FragColor = texture(u_texture, TexCoord) * u_color;
    if (FragColor.a < 0.01) 
        discard;
}