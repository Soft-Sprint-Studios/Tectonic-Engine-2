out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D texY;
uniform sampler2D texCb;
uniform sampler2D texCr;

void main()
{
    float y = (texture(texY, TexCoord).r - (16.0 / 255.0)) * 1.16438;
    float cb = texture(texCb, TexCoord).r - 0.5;
    float cr = texture(texCr, TexCoord).r - 0.5;

    float r = y + (1.59603 * cr);
    float g = y - (0.39176 * cb) - (0.81297 * cr);
    float b = y + (2.01723 * cb);

    vec3 rgb = vec3(r, g, b);
    rgb = pow(max(rgb, vec3(0.0)), vec3(2.2));

    FragColor = vec4(rgb, 1.0);
}