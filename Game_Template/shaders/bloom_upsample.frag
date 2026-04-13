out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D u_sourceTexture;
uniform float u_scatter;

void main()
{
    vec2 texelSize = 1.0 / textureSize(u_sourceTexture, 0);
    vec4 sum = vec4(0.0);

    float d = u_scatter;
    sum += texture(u_sourceTexture, TexCoords + texelSize * vec2(-d, -d));
    sum += texture(u_sourceTexture, TexCoords + texelSize * vec2( 0, -d));
    sum += texture(u_sourceTexture, TexCoords + texelSize * vec2( d, -d));
    sum += texture(u_sourceTexture, TexCoords + texelSize * vec2(-d,  0));
    sum += texture(u_sourceTexture, TexCoords + texelSize * vec2( 0,  0));
    sum += texture(u_sourceTexture, TexCoords + texelSize * vec2( d,  0));
    sum += texture(u_sourceTexture, TexCoords + texelSize * vec2(-d,  d));
    sum += texture(u_sourceTexture, TexCoords + texelSize * vec2( 0,  d));
    sum += texture(u_sourceTexture, TexCoords + texelSize * vec2( d,  d));

    FragColor = sum / 9.0;
}