out vec4 FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D u_texture;
uniform vec4 u_textColor;
uniform int u_type;

void main()
{
    float alpha = 1.0;

    if (u_type == 1)
    {
        float dist = length(TexCoords - vec2(0.5));
        if (dist > 0.5) 
            discard;
    }
    else
    {
        alpha = texture(u_texture, TexCoords).a;
    }

    FragColor = vec4(u_textColor.rgb, u_textColor.a * alpha);
}