out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D u_texture;
uniform sampler2D u_lightmap;
uniform vec2 u_screenSize;

void main()
{
    vec4 texColor = texture(u_texture, TexCoord);
    vec2 screenUV = gl_FragCoord.xy / u_screenSize;
    vec3 lightColor = texture(u_lightmap, screenUV).rgb;
    FragColor = vec4(texColor.rgb * lightColor, texColor.a);
}