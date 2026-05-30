uniform int u_lightmap_bicubic;

// Bicubic filtering functions adapted from Godot Engine
float w0(float a) 
{ 
    return (1.0 / 6.0) * (a * (a * (-a + 3.0) - 3.0) + 1.0); 
}

float w1(float a) 
{ 
    return (1.0 / 6.0) * (a * a * (3.0 * a - 6.0) + 4.0); 
}

float w2(float a) 
{ 
    return (1.0 / 6.0) * (a * (a * (-3.0 * a + 3.0) + 3.0) + 1.0); 
}

float w3(float a) 
{ 
    return (1.0 / 6.0) * (a * a * a); 
}

float g0(float a) 
{ 
    return w0(a) + w1(a); 
}

float g1(float a) 
{ 
    return w2(a) + w3(a); 
}

float h0(float a) 
{ 
    return -1.0 + w1(a) / (w0(a) + w1(a)); 
}

float h1(float a) 
{ 
    return 1.0 + w3(a) / (w2(a) + w3(a)); 
}

vec4 textureBicubic(sampler2D tex, vec2 uv)
{
    vec2 texture_size = vec2(textureSize(tex, 0));
    vec2 texel_size = 1.0 / texture_size;

    uv = uv * texture_size + vec2(0.5);
    vec2 iuv = floor(uv);
    vec2 fuv = fract(uv);

    float g0x = g0(fuv.x);
    float g1x = g1(fuv.x);
    float h0x = h0(fuv.x);
    float h1x = h1(fuv.x);
    float h0y = h0(fuv.y);
    float h1y = h1(fuv.y);

    vec2 p0 = (vec2(iuv.x + h0x, iuv.y + h0y) - vec2(0.5)) * texel_size;
    vec2 p1 = (vec2(iuv.x + h1x, iuv.y + h0y) - vec2(0.5)) * texel_size;
    vec2 p2 = (vec2(iuv.x + h0x, iuv.y + h1y) - vec2(0.5)) * texel_size;
    vec2 p3 = (vec2(iuv.x + h1x, iuv.y + h1y) - vec2(0.5)) * texel_size;

    return (g0(fuv.y) * (g0x * texture(tex, p0) + g1x * texture(tex, p1))) +
           (g1(fuv.y) * (g0x * texture(tex, p2) + g1x * texture(tex, p3)));
}

vec4 GetLightmapData(sampler2D lmSampler, vec2 uv) 
{
    if (u_lightmap_bicubic == 1) 
    {
        return textureBicubic(lmSampler, uv);
    }
    return texture(lmSampler, uv);
}