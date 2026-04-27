in vec2 fTex;
in float fDepth;
in vec4 fCol;
out vec4 Color;

uniform sampler2D u_tex;
uniform sampler2D u_depthTexture;
uniform mat4 u_invProj;
uniform vec2 u_screenSize;

float GetLinearDepth(float ndcDepth)
{
    float z = ndcDepth * 2.0 - 1.0;
    vec4 clipPos = vec4(0.0, 0.0, z, 1.0);
    vec4 viewPos = u_invProj * clipPos;
    return -(viewPos.z / viewPos.w);
}

void main() 
{
    vec2 screenUV = gl_FragCoord.xy / u_screenSize;
    float sceneZ = GetLinearDepth(texture(u_depthTexture, screenUV).r);

    float softFactor = 1.0; 
    float fade = clamp((sceneZ - fDepth) * softFactor, 0.0, 1.0);

    Color = texture(u_tex, fTex) * fCol;
    Color.a *= fade;
}