out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D text;
uniform vec4 textColor;
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
        alpha = texture(text, TexCoords).a;
    }

    FragColor = vec4(textColor.rgb, textColor.a * alpha);
}