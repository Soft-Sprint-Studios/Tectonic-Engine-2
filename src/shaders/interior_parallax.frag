$input v_fragPos, v_texcoord0, v_tbn0, v_tbn1, v_tbn2

#include <bgfx_shader.sh>

SAMPLERCUBE(s_interiorCube, 0);

uniform vec4 u_viewPos;
uniform vec4 u_roomParams;
#define u_roomSize u_roomParams.xyz

void main()
{
    mat3 TBN = mat3(v_tbn0, v_tbn1, v_tbn2);
    vec3 viewDirWorld = normalize(v_fragPos - u_viewPos.xyz);
    vec3 viewDirTangent = normalize(mul(transpose(TBN), viewDirWorld));

    vec2 localUV = fract(v_texcoord0);
    vec2 posXY = localUV * 2.0 - 1.0;

    vec3 ro = vec3(posXY, 1.0);
    vec3 rd = viewDirTangent;
    rd.z = -abs(rd.z);

    vec3 scaledRd = rd / max(u_roomSize, vec3_splat(0.001));
    vec3 invRd = 1.0 / scaledRd;
    vec3 t0 = (vec3_splat(-1.0) - ro) * invRd;
    vec3 t1 = (vec3_splat(1.0) - ro) * invRd;

    vec3 tMax = max(t0, t1);
    float t = min(min(tMax.x, tMax.y), tMax.z);

    vec3 hitPos = ro + scaledRd * t;
    vec3 sampleVec = hitPos;
    sampleVec.y *= -1.0;

    vec3 roomColor = textureCube(s_interiorCube, sampleVec).rgb;
    gl_FragColor = vec4(roomColor, 1.0);
}