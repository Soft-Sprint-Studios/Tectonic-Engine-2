#ifdef __cplusplus
#pragma once
#endif

#define LIGHTMAP_ATLAS_SIZE 4096

#ifndef __cplusplus
uniform int r_lightmap_bicubic;

vec4 cubic(float v)
{
    float v2 = v * v;
    float v3 = v2 * v;
    return vec4(-0.5 * v3 + v2 - 0.5 * v, 1.5 * v3 - 2.5 * v2 + 1.0, -1.5 * v3 + 2.0 * v2 + 0.5 * v, 0.5 * v3 - 0.5 * v2);
}

vec4 texture_bicubic(sampler2D tex, vec2 uv)
{
    vec2 texSize = vec2(textureSize(tex, 0));
    vec2 texelSize = 1.0 / texSize;
    
    vec2 f = fract(uv * texSize - 0.5);
    vec2 centroidUV = floor(uv * texSize - 0.5) + 0.5;

    vec4 colX = cubic(f.x);
    vec4 colY = cubic(f.y);

    vec4 r0 = colX.x * texture(tex, (centroidUV + vec2(-1.0, -1.0)) * texelSize) + colX.y * texture(tex, (centroidUV + vec2(0.0, -1.0)) * texelSize) + colX.z * texture(tex, (centroidUV + vec2(1.0, -1.0)) * texelSize) + colX.w * texture(tex, (centroidUV + vec2(2.0, -1.0)) * texelSize);
    vec4 r1 = colX.x * texture(tex, (centroidUV + vec2(-1.0,  0.0)) * texelSize) + colX.y * texture(tex, (centroidUV + vec2(0.0,  0.0)) * texelSize) + colX.z * texture(tex, (centroidUV + vec2(1.0,  0.0)) * texelSize) + colX.w * texture(tex, (centroidUV + vec2(2.0,  0.0)) * texelSize);
    vec4 r2 = colX.x * texture(tex, (centroidUV + vec2(-1.0,  1.0)) * texelSize) + colX.y * texture(tex, (centroidUV + vec2(0.0,  1.0)) * texelSize) + colX.z * texture(tex, (centroidUV + vec2(1.0,  1.0)) * texelSize) + colX.w * texture(tex, (centroidUV + vec2(2.0,  1.0)) * texelSize);
    vec4 r3 = colX.x * texture(tex, (centroidUV + vec2(-1.0,  2.0)) * texelSize) + colX.y * texture(tex, (centroidUV + vec2(0.0,  2.0)) * texelSize) + colX.z * texture(tex, (centroidUV + vec2(1.0,  2.0)) * texelSize) + colX.w * texture(tex, (centroidUV + vec2(2.0,  2.0)) * texelSize);

    return colY.x * r0 + colY.y * r1 + colY.z * r2 + colY.w * r3;
}

vec4 SampleLightmap(sampler2D tex, vec2 uv)
{
    if (r_lightmap_bicubic == 1)
    {
        return texture_bicubic(tex, uv);
    }
    else
    {
        return texture(tex, uv);
    }
}
#endif