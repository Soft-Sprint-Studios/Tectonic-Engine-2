out vec4 FragColor;

in centroid vec3 v_FragPos;
in vec2 v_TexCoord;
in vec2 v_LmCoord;
in centroid mat3 v_TBN;

uniform sampler2D u_reflectionTexture;
uniform sampler2D u_lightmap;
uniform sampler2D u_dudvMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_flowMap;
uniform float u_flowSpeed;
uniform bool u_hasFlow;

uniform mat4 u_reflectView;
uniform mat4 u_reflectProj;

uniform vec3 u_viewPos;
uniform float u_time;
uniform int u_debugMode;

const float waveStrength = 0.02;
const float normalTiling = 1.0;
const float normalSpeed = 0.015;
const float dudvMoveSpeed = 0.01;

void main() 
{
    vec2 worldUV = v_FragPos.xz * 0.25;

    vec2 flowDir = vec2(1.0, 0.0); 
    if (u_hasFlow)
    {
        flowDir = texture(u_flowMap, worldUV).rg * 2.0 - 1.0;
    }

    float time = u_time * u_flowSpeed;
    float phase0 = fract(time);
    float phase1 = fract(time + 0.5);

    vec2 dist0 = (texture(u_dudvMap, worldUV + flowDir * phase0).rg * 2.0 - 1.0) * waveStrength;
    vec2 dist1 = (texture(u_dudvMap, worldUV + flowDir * phase1).rg * 2.0 - 1.0) * waveStrength;

    float lerpFlow = abs(0.5 - phase0) / 0.5;
    vec2 distortion = mix(dist0, dist1, lerpFlow);

    vec4 reflectClip = u_reflectProj * u_reflectView * vec4(v_FragPos, 1.0);
    vec2 ndc = (reflectClip.xy / reflectClip.w) * 0.5 + 0.5;
    vec3 reflection = texture(u_reflectionTexture, clamp(ndc + distortion, 0.001, 0.999)).rgb;

    vec2 scroll = worldUV * normalTiling + vec2(u_time * normalSpeed);
    vec3 normalSample = texture(u_normalMap, scroll + distortion).rgb * 2.0 - 1.0;
    vec3 N = normalize(v_TBN * normalSample);

    vec3 V = normalize(u_viewPos - v_FragPos);
    float fresnel = clamp(1.0 - dot(V, vec3(0.0, 1.0, 0.0)), 0.0, 1.0);
    fresnel = pow(fresnel, 2.0);

    vec3 finalColor = reflection * fresnel;
    finalColor *= texture(u_lightmap, v_LmCoord).rgb * 2.0;
	
	if (u_debugMode == 1) 
    {
        FragColor = vec4(texture(u_lightmap, v_LmCoord).rgb * 2.0, 1.0);
        return;
    }

    FragColor = vec4(finalColor, 0.95);
}