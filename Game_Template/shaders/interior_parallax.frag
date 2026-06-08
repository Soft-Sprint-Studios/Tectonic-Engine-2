out vec4 FragColor;

in vec3 v_FragPos;
in vec2 v_TexCoord;
in mat3 v_TBN;

uniform vec3 u_viewPos;

layout(binding = 0) uniform samplerCube u_interiorCube;

uniform vec3 u_roomSize;

void main()
{
    vec3 viewDirWorld = normalize(v_FragPos - u_viewPos);

    vec3 viewDirTangent = normalize(transpose(v_TBN) * viewDirWorld);

    vec2 localUV = fract(v_TexCoord);
    vec2 posXY = localUV * 2.0 - 1.0;

    vec3 ro = vec3(posXY, 1.0);

    vec3 rd = viewDirTangent;
    rd.z = -abs(rd.z);

    vec3 scaledRd = rd / max(u_roomSize, vec3(0.001));

    vec3 invRd = 1.0 / scaledRd;
    vec3 t0 = (vec3(-1.0) - ro) * invRd;
    vec3 t1 = (vec3(1.0) - ro) * invRd;

    vec3 tMax = max(t0, t1);

    float t = min(min(tMax.x, tMax.y), tMax.z);

    vec3 hitPos = ro + scaledRd * t;

    vec3 sampleVec = hitPos;
    sampleVec.y *= -1.0;

    vec3 roomColor = texture(u_interiorCube, sampleVec).rgb;

    FragColor = vec4(roomColor, 1.0);
}