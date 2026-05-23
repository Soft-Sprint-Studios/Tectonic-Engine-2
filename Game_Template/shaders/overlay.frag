out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D u_texture;
uniform float u_alpha;

void main()
{
    vec4 tex = texture(u_texture, TexCoords);
    FragColor = vec4(tex.rgb, tex.a * u_alpha);
}