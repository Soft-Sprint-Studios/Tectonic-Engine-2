out vec4 FragColor;

in vec2 TexCoord;
in vec4 ScreenPos;

uniform sampler2D u_texture;
uniform sampler2D u_gLightmap;

void main()
{
    vec4 decalCol = texture(u_texture, TexCoord);
    if (decalCol.a < 0.01)
    {
        discard;
    }

    vec2 screenUV = (ScreenPos.xy / ScreenPos.w) * 0.5 + 0.5;

    vec3 light = texture(u_gLightmap, screenUV).rgb;

    FragColor = vec4(decalCol.rgb * light * 2.0, decalCol.a);
}