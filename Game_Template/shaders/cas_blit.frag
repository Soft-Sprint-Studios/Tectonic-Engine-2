out vec4 FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D u_texture;

void main()
{
    FragColor = vec4(texture(u_texture, TexCoords).rgb, 1.0);
}