#include "common.h"
#include "parallax.h"

layout (location = 0) out vec2 gNormal;
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

    vec4 normalSample = texture(u_normal, finalUV);
    vec3 tsSample = normalSample.rgb * 2.0 - 1.0;
    vec3 tangentNormal = normalize(tsSample);
		
    if (!gl_FrontFacing)
    {
        tangentNormal = -tangentNormal;
    }
		
    vec3 worldNormal = normalize(TBN * tangentNormal);
    vec4 mraoh = texture(u_mraohMap, finalUV);

    float packed_tx = tangentNormal.x * 0.5 + 0.5;
    float packed_ty = tangentNormal.y * 0.5 + 0.5;

    gNormal = EncodeNormal(worldNormal);
    gAlbedo = vec4(albedo.rgb, packed_tx);
    gMRAO = vec4(mraoh.rgb, packed_ty);
}