#include "common.glsl"

layout (location = 0) out vec2 gNormal;
layout (location = 1) out vec4 gAlbedoSpec;

in vec2 TexCoord;
in mat3 TBN;

uniform sampler2D u_diffuse;
uniform sampler2D u_normal;

void main()
{
    vec4 col = texture(u_diffuse, TexCoord);

    if (col.a < 0.1) 
        discard;

    vec4 nSample = texture(u_normal, TexCoord);
    gAlbedoSpec = vec4(col.rgb, nSample.a);

    vec3 tangentNormal = nSample.rgb * 2.0 - 1.0;
	
    if (!gl_FrontFacing)
        tangentNormal = -tangentNormal;
	
    gNormal = EncodeNormal(normalize(TBN * tangentNormal));
}