out vec4 FragColor;

in vec2 TexCoord;
in vec4 ScreenPos;
in mat3 TBN;

layout(binding = 10) uniform sampler2D u_refractTex;
layout(binding = 0) uniform sampler2D u_normalMap;
uniform float u_amount;

void main()
{
    vec2 screenUV = (ScreenPos.xy / ScreenPos.w) * 0.5 + 0.5;

    vec3 normal = texture(u_normalMap, TexCoord).rgb * 2.0 - 1.0;
    vec3 worldNormal = normalize(TBN * normal);

    vec2 offset = worldNormal.xy * u_amount;
    vec3 scene = texture(u_refractTex, screenUV + offset).rgb;

    FragColor = vec4(scene, 1.0);
}