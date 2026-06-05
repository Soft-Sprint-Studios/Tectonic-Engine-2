#include "common.glsl"
#include "parallax.glsl"

layout (location = 0) out vec4 gNormal;
layout (location = 1) out vec4 gAlbedoSpec;

in vec2 TexCoord;
in vec3 v_FragPos;
in mat3 TBN;

layout(binding = 0) uniform sampler2D u_diffuse;
layout(binding = 1) uniform sampler2D u_normal;
layout(binding = 2) uniform sampler2D u_heightMap;

uniform vec3 u_viewPos;
uniform int u_mat_bumpmap;
uniform int u_mat_parallax;
uniform float u_heightScale;

void main()
{
    vec2 finalUV = TexCoord;

    if (u_mat_parallax == 1 && u_heightScale > 0.0)
    {
        vec3 tsViewDir = normalize(transpose(TBN) * (u_viewPos - v_FragPos));
        finalUV = ParallaxMapping(u_heightMap, TexCoord, u_heightScale, tsViewDir);
    }

    vec4 albedo = texture(u_diffuse, finalUV);

    if (albedo.a < 0.1)
        discard;

    vec3 worldNormal = normalize(TBN[2]);
    vec3 tangentNormal = vec3(0.0, 0.0, 1.0);

    if (u_mat_bumpmap == 1)
    {
        vec3 tsSample = texture(u_normal, finalUV).rgb * 2.0 - 1.0;
        tangentNormal = normalize(tsSample);
		
        if (!gl_FrontFacing)
            tangentNormal = -tangentNormal;
		
        worldNormal = normalize(TBN * tangentNormal);
    }

    float specMask = (u_mat_bumpmap == 1) ? texture(u_normal, finalUV).a : 0.0;

    gNormal = vec4(EncodeNormal(worldNormal), tangentNormal.x, (u_mat_bumpmap == 1) ? tangentNormal.y : -2.0);
    gAlbedoSpec = vec4(albedo.rgb, specMask);
}