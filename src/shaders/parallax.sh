#ifndef PARALLAX_SH
#define PARALLAX_SH

uniform vec4 u_parallaxParams;

vec2 ParallaxMapping(sampler2D heightMapSampler, vec2 texCoords, float hScale, vec3 viewDir)
{
    float numLayers = mix(u_parallaxParams.y, u_parallaxParams.x, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    float layerDepth = 1.0 / numLayers;

    vec2 p = viewDir.xy * hScale;
    vec2 deltaTexCoords = p / numLayers;

    vec2 currentTexCoords = texCoords;
    float currentLayerDepth = 0.0;
    float currentHeightMapValue = 1.0 - texture2D(heightMapSampler, currentTexCoords).a;

    for (int i = 0; i < int(numLayers); i++)
    {
        if (currentLayerDepth >= currentHeightMapValue)
        {
            break;
        }

        currentTexCoords -= deltaTexCoords;
        currentLayerDepth += layerDepth;
        currentHeightMapValue = 1.0 - texture2D(heightMapSampler, currentTexCoords).a;
    }

    vec2 texCoordsStart = currentTexCoords + deltaTexCoords;
    vec2 texCoordsEnd = currentTexCoords;
    float depthStart = currentLayerDepth - layerDepth;
    float depthEnd = currentLayerDepth;

    int refineSteps = int(u_parallaxParams.z);
    for (int i = 0; i < refineSteps; i++)
    {
        float midDepth = (depthStart + depthEnd) * 0.5;
        vec2 midTexCoords = mix(texCoordsStart, texCoordsEnd, 0.5);
        float midHeightMapValue = 1.0 - texture2D(heightMapSampler, midTexCoords).a;

        if (midDepth > midHeightMapValue)
        {
            depthEnd = midDepth;
            texCoordsEnd = midTexCoords;
        }
        else
        {
            depthStart = midDepth;
            texCoordsStart = midTexCoords;
        }
    }

    return texCoordsEnd;
}

#endif