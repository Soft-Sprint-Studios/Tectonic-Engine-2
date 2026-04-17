#define PI 3.14159265359

layout(location = 0) out vec4 FragColor;

in vec3 TexCoords;
in centroid vec3 v_worldPos;

uniform bool u_use_dynamic;
uniform samplerCube skybox;

uniform vec3 u_sunDir;
uniform vec3 u_cameraPos;
uniform float u_time;

uniform int u_sky_steps_primary;
uniform int u_sky_steps_light;

vec2 rsi(vec3 r0, vec3 rd, float sr) 
{
    float a = dot(rd, rd);
    float b = 2.0 * dot(rd, r0);
    float c = dot(r0, r0) - (sr * sr);
    float d = (b * b) - 4.0 * a * c;
    
    if (d < 0.0) 
    {
        return vec2(1e5, -1e5);
    }
    
    return vec2(
        (-b - sqrt(d)) / (2.0 * a),
        (-b + sqrt(d)) / (2.0 * a)
    );
}

vec3 atmosphere(vec3 r, vec3 r0, vec3 pSun, float iSun, float rPlanet, float rAtmos, vec3 kRlh, float kMie, float shRlh, float shMie, float g) 
{
    pSun = normalize(pSun);
    r = normalize(r);

    vec2 p = rsi(r0, r, rAtmos);
    
    if (p.x > p.y) 
    {
        return vec3(0, 0, 0);
    }
    
    p.y = min(p.y, rsi(r0, r, rPlanet).x);
    float iStepSize = (p.y - p.x) / float(u_sky_steps_primary);

    float iTime = 0.0;
    vec3 totalRlh = vec3(0, 0, 0);
    vec3 totalMie = vec3(0, 0, 0);

    float iOdRlh = 0.0;
    float iOdMie = 0.0;
    
    float mu = dot(r, pSun);
    float mumu = mu * mu;
    float gg = g * g;
    float pRlh = 3.0 / (16.0 * PI) * (1.0 + mumu);
    float pMie = 3.0 / (8.0 * PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg));

    for (int i = 0; i < u_sky_steps_primary; i++) 
    {
        vec3 iPos = r0 + r * (iTime + iStepSize * 0.5);
        float iHeight = length(iPos) - rPlanet;
        
        if (iHeight < 0.0) 
        {
            break;
        }

        float odStepRlh = exp(max(-20.0, -iHeight / shRlh)) * iStepSize;
        float odStepMie = exp(max(-20.0, -iHeight / shMie)) * iStepSize;
        
        iOdRlh += odStepRlh;
        iOdMie += odStepMie;

        float jStepSize = rsi(iPos, pSun, rAtmos).y / float(u_sky_steps_light);
        float jTime = 0.0;
        float jOdRlh = 0.0;
        float jOdMie = 0.0;

        for (int j = 0; j < u_sky_steps_light; j++) 
        {
            vec3 jPos = iPos + pSun * (jTime + jStepSize * 0.5);
            float jHeight = length(jPos) - rPlanet;
            
            if (jHeight > 0.0) 
            {
                 jOdRlh += exp(-jHeight / shRlh) * jStepSize;
                 jOdMie += exp(-jHeight / shMie) * jStepSize;
            }
            jTime += jStepSize;
        }

        vec3 attn = exp(-(kMie * (iOdMie + jOdMie) + kRlh * (iOdRlh + jOdRlh)));
        totalRlh += odStepRlh * attn;
        totalMie += odStepMie * attn;
        iTime += iStepSize;
    }

    return iSun * (pRlh * kRlh * totalRlh + pMie * kMie * totalMie);
}

float rand(vec3 p) 
{
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float noise(vec3 p) 
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * (3.0 - 2.0 * f);
    
    return mix(
        mix(
            mix(rand(i + vec3(0,0,0)), rand(i + vec3(1,0,0)), u.x),
            mix(rand(i + vec3(0,1,0)), rand(i + vec3(1,1,0)), u.x),
            u.y
        ),
        mix(
            mix(rand(i + vec3(0,0,1)), rand(i + vec3(1,0,1)), u.x),
            mix(rand(i + vec3(0,1,1)), rand(i + vec3(1,1,1)), u.x),
            u.y
        ),
        u.z
    );
}

float fbm(vec3 p, int octaves) 
{
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < octaves; i++) 
    {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

vec3 getCloudColor(vec3 r0, vec3 rd, vec3 sunDir) 
{
    const float cloudSpeed = 0.04;
    const float cloudScale = 1.5;
    const int   cloudOctaves = 8;
    
    vec3 p_sample = rd * cloudScale + vec3(u_time * cloudSpeed, 0.0, 0.0);
    float density = fbm(p_sample, cloudOctaves);
    density = smoothstep(0.45, 0.65, density);
    
    float viewFade = smoothstep(0.0, 0.25, rd.y);
    float cloudRadius = 6471e3 + 2000.0;
    
    vec2 t = rsi(r0, rd, cloudRadius);
    if (t.x > t.y) 
    {
        return vec3(0.0);
    }

    vec3 pCloud = r0 + rd * t.x;
    
    vec3 normal = normalize(pCloud);
    float directLight = max(dot(normal, sunDir), 0.0);
    float light = 0.35 + (0.65) * pow(directLight, 4.0);
    
    return vec3(1.0) * (density * 0.6 * viewFade * light);
}

void main()
{
    if (u_use_dynamic == false) 
    {
        FragColor = texture(skybox, TexCoords);
        return;
    }

    vec3 rayDir = normalize(v_worldPos);
    vec3 rayOrigin = vec3(0.0, u_cameraPos.y + 6371e3, 0.0);

    vec3 color = atmosphere(
        rayDir,
        rayOrigin,
        u_sunDir,
        22.0,
        6371e3,
        6471e3,
        vec3(5.5e-6, 13.0e-6, 22.4e-6),
        21e-6,
        8e3,
        1.2e3,
        0.758
    );

    color += getCloudColor(rayOrigin, rayDir, u_sunDir);
    
    FragColor = vec4(color * 0.2, 1.0);
}