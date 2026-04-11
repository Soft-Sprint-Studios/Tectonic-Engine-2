out vec4 FragColor;

in centroid vec3 v_FragPos;
in vec2 v_TexCoord;
in centroid mat3 v_TBN;

uniform sampler2D u_reflectionTexture;
uniform sampler2D u_dudvMap;
uniform sampler2D u_normalMap;

uniform mat4 u_reflectView;
uniform mat4 u_reflectProj;

uniform vec3 u_viewPos;
uniform float u_time;

const float waveStrength = 0.02;
const float normalTiling = 1.0;
const float normalSpeed = 0.015;
const float dudvMoveSpeed = 0.01;

void main() 
{
    vec2 worldUV = v_FragPos.xz * 0.25;

    float move = u_time * dudvMoveSpeed;
    vec2 distortion = (texture(u_dudvMap, worldUV + vec2(move)).rg * 2.0 - 1.0) * waveStrength;

    vec4 reflectClip = u_reflectProj * u_reflectView * vec4(v_FragPos, 1.0);
    vec2 ndc = (reflectClip.xy / reflectClip.w) * 0.5 + 0.5;
    vec3 reflection = texture(u_reflectionTexture, clamp(ndc + distortion, 0.001, 0.999)).rgb;

    vec2 scroll = worldUV * normalTiling + vec2(u_time * normalSpeed);
    vec3 normalSample = texture(u_normalMap, scroll + distortion).rgb * 2.0 - 1.0;
    vec3 N = normalize(v_TBN * normalSample);

    vec3 V = normalize(u_viewPos - v_FragPos);
    float fresnel = clamp(1.0 - dot(V, vec3(0.0, 1.0, 0.0)), 0.0, 1.0);
    fresnel = pow(fresnel, 2.0);

    vec3 finalColor = mix(vec3(0.0), reflection, fresnel);

    FragColor = vec4(finalColor, 0.95);
}