out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D u_sourceTexture;
uniform float u_threshold;

void main()
{
    vec3 color = texture(u_sourceTexture, TexCoords).rgb;

    color = max(vec3(0.0), color - u_threshold);
    FragColor = vec4(color / (color + 1.0), 1.0);
}