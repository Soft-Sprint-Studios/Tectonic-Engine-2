out vec4 FragColor;
in vec2 TexCoords;
layout(binding = 0) uniform sampler2D u_ssrTex;

void main()
{
    vec2 texelSize = 1.0 / textureSize(u_ssrTex, 0);
    vec4 result = vec4(0.0);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            result += texture(u_ssrTex, TexCoords + vec2(x, y) * texelSize);
        }
    }
    FragColor = result / 9.0;
}