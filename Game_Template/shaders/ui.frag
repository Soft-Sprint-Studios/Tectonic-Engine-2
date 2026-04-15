out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D text;
uniform vec4 textColor;

void main()
{
    float alpha = texture(text, TexCoords).a;
    FragColor = vec4(textColor.rgb, textColor.a * alpha);
}