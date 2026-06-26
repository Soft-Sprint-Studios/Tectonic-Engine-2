out vec4 FragColor;
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D u_ssaoTexture;
uniform bool u_horizontal;

float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    vec2 tex_offset = 1.0 / textureSize(u_ssaoTexture, 0); 
    float result = texture(u_ssaoTexture, TexCoords).r * weight[0]; 

    if(u_horizontal)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(u_ssaoTexture, TexCoords + vec2(tex_offset.x * i, 0.0)).r * weight[i];
            result += texture(u_ssaoTexture, TexCoords - vec2(tex_offset.x * i, 0.0)).r * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(u_ssaoTexture, TexCoords + vec2(0.0, tex_offset.y * i)).r * weight[i];
            result += texture(u_ssaoTexture, TexCoords - vec2(0.0, tex_offset.y * i)).r * weight[i];
        }
    }
    FragColor = vec4(vec3(result), 1.0);
}