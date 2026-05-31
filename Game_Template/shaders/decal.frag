out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D u_texture;
uniform vec4 u_color;
uniform int u_blendMode;

void main()
{
    vec4 texColor = texture(u_texture, TexCoord) * u_color;
    
    if (u_blendMode == 0)
    {
        vec3 blendedColor = mix(vec3(1.0), texColor.rgb, texColor.a);
        FragColor = vec4(blendedColor, 1.0);
    }
    else
    {
        FragColor = vec4(texColor.rgb * texColor.a, texColor.a);
    }
}