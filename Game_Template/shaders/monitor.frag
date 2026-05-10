out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D u_texture;
uniform int u_grayscale;

void main()
{
    vec4 texColor = texture(u_texture, TexCoord);
    
    if (u_grayscale == 1)
    {
        vec3 weights = vec3(0.32, 0.59, 0.09);
        float gray = dot(texColor.rgb, weights);
        FragColor = vec4(vec3(gray), texColor.a);
    }
    else
    {
        FragColor = texColor;
    }
}