#include "common.glsl"
#include "parallax.glsl"

layout (location = 0) out vec4 gNormal;
layout (location = 1) out vec4 gAlbedo;
layout (location = 2) out vec4 gMRAO;

in vec2 TexCoord;
in vec3 v_FragPos;
in mat3 TBN;

layout(binding = 0) uniform sampler2D u_diffuse;
layout(binding = 1) uniform sampler2D u_normal;
layout(binding = 2) uniform sampler2D u_mraohMap;

uniform vec3 u_viewPos;
uniform int u_mat_parallax;
uniform float u_heightScale;

void main()
{
    vec2 finalUV = TexCoord;

    if (u_mat_parallax == 1 && u_heightScale > 0.0)
    {
        vec3 tsViewDir = normalize(transpose(TBN) * (u_viewPos - v_FragPos));
        finalUV = ParallaxMapping(u_mraohMap, TexCoord, u_heightScale, tsViewDir);
    }

    vec4 albedo = texture(u_diffuse, finalUV);

    if (albedo.a < 0.1)
    {
        discard;
    }

    vec3 tsSample = texture(u_normal, finalUV).rgb * 2.0 - 1.0;
    vec3 tangentNormal = normalize(tsSample);
		
    if (!gl_FrontFacing)
    {
        tangentNormal = -tangentNormal;
    }
		
    vec3 worldNormal = normalize(TBN * tangentNormal);
    vec4 mraoh = texture(u_mraohMap, finalUV);

    gNormal = vec4(EncodeNormal(worldNormal), tangentNormal.x, tangentNormal.y);
    gAlbedo = vec4(albedo.rgb, 1.0);
    gMRAO.rgb = mraoh.rgb;
}