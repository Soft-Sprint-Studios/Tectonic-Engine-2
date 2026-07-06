#ifndef COMMON_SH
#define COMMON_SH

vec2 OctWrap(vec2 v)
{
    return (1.0 - abs(v.yx)) * vec2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

vec2 EncodeNormal(vec3 n)
{
    vec2 p = n.xy / (abs(n.x) + abs(n.y) + abs(n.z));
    if (n.z < 0.0)
    {
        p = OctWrap(p);
    }
    return p * 0.5 + 0.5;
}

vec3 DecodeNormal(vec2 f)
{
    f = f * 2.0 - 1.0;
    vec3 n = vec3(f.xy, 1.0 - abs(f.x) - abs(f.y));
    if (n.z < 0.0)
    {
        n.xy = (1.0 - abs(n.yx)) * vec2(n.x >= 0.0 ? 1.0 : -1.0, n.y >= 0.0 ? 1.0 : -1.0);
    }
    return normalize(n);
}

#endif