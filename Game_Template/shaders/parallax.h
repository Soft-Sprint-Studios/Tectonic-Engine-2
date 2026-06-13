uniform float u_pomMinSteps;
uniform float u_pomMaxSteps;
uniform int u_pomRefineSteps;

vec2 ParallaxMapping(sampler2D heightMapSampler, vec2 texCoords, float hScale, vec3 viewDir)
{
    float numLayers = mix(u_pomMaxSteps, u_pomMinSteps, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    float layerDepth = 1.0 / numLayers;

    vec2 p = viewDir.xy * hScale;
    vec2 deltaTexCoords = p / numLayers;

    vec2 currentTexCoords = texCoords;
    float currentLayerDepth = 0.0;
    float currentHeightMapValue = 1.0 - texture(heightMapSampler, currentTexCoords).a;

    for (int i = 0; i < int(numLayers); i++)
    {
        if (currentLayerDepth >= currentHeightMapValue)
        {
            break;
        }

        currentTexCoords -= deltaTexCoords;
        currentLayerDepth += layerDepth;
        currentHeightMapValue = 1.0 - texture(heightMapSampler, currentTexCoords).a;
    }

    vec2 texCoordsStart = currentTexCoords + deltaTexCoords;
    vec2 texCoordsEnd = currentTexCoords;
    float depthStart = currentLayerDepth - layerDepth;
    float depthEnd = currentLayerDepth;

    for (int i = 0; i < u_pomRefineSteps; i++)
    {
        float midDepth = (depthStart + depthEnd) * 0.5;
        vec2 midTexCoords = mix(texCoordsStart, texCoordsEnd, 0.5);
        float midHeightMapValue = 1.0 - texture(heightMapSampler, midTexCoords).a;

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