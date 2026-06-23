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
    
    vec2 p = uv * texSize - 0.5;
    vec2 f = fract(p);
    vec2 centroidUV = floor(p) + 0.5;

    vec4 wX = cubic(f.x);
    vec4 wY = cubic(f.y);

    vec2 wInner = vec2(wX.y + wX.z, wY.y + wY.z);
    vec2 offset = vec2(wX.z, wY.z) / wInner;

    vec3 cX = (centroidUV.x + vec3(-1.0, offset.x, 2.0)) * texelSize.x;
    vec3 cY = (centroidUV.y + vec3(-1.0, offset.y, 2.0)) * texelSize.y;

    vec3 wtX = vec3(wX.x, wInner.x, wX.w);
    vec3 wtY = vec3(wY.x, wInner.y, wY.w);

    vec4 r0 = wtX.x * texture(tex, vec2(cX.x, cY.x)) + wtX.y * texture(tex, vec2(cX.y, cY.x)) + wtX.z * texture(tex, vec2(cX.z, cY.x));
    vec4 r1 = wtX.x * texture(tex, vec2(cX.x, cY.y)) + wtX.y * texture(tex, vec2(cX.y, cY.y)) + wtX.z * texture(tex, vec2(cX.z, cY.y));
    vec4 r2 = wtX.x * texture(tex, vec2(cX.x, cY.z)) + wtX.y * texture(tex, vec2(cX.y, cY.z)) + wtX.z * texture(tex, vec2(cX.z, cY.z));

    return wtY.x * r0 + wtY.y * r1 + wtY.z * r2;
}

vec2 PackLightmapUV(vec2 lmCoord, vec2 lmSize)
{
    vec2 size_in_pixels = lmSize * vec2(float(LIGHTMAP_ATLAS_SIZE));
    uint w_pixels = uint(round(clamp(size_in_pixels.x, 0.0, 255.0)));
    uint h_pixels = uint(round(clamp(size_in_pixels.y, 0.0, 255.0)));

    uint u_val = uint(clamp(lmCoord.x, 0.0, 1.0) * 16777215.0);
    uint v_val = uint(clamp(lmCoord.y, 0.0, 1.0) * 16777215.0);

    uint packed_u = (u_val & 0x00FFFFFFu) | (w_pixels << 24);
    uint packed_v = (v_val & 0x00FFFFFFu) | (h_pixels << 24);

    return vec2(uintBitsToFloat(packed_u), uintBitsToFloat(packed_v));
}

void UnpackLightmapUV(vec2 packedFloat, out vec2 lmCoord, out vec2 lmSize)
{
    uint packed_u = floatBitsToUint(packedFloat.x);
    uint packed_v = floatBitsToUint(packedFloat.y);

    lmCoord = vec2(float(packed_u & 0x00FFFFFFu), float(packed_v & 0x00FFFFFFu)) / 16777215.0;
    lmSize = vec2(float(packed_u >> 24), float(packed_v >> 24)) / float(LIGHTMAP_ATLAS_SIZE);
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