/*
 * MIT License
 *
 * Copyright (c) 2025-2026 Soft Sprint Studios
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "r_ssao.h"
#include "cvar.h"
#include <random>
#include <glm/gtc/type_ptr.hpp>

CVar r_ssao("r_ssao", "1", "Enable Screen Space Ambient Occlusion.", CVAR_SAVE);
CVar r_ssao_radius("r_ssao_radius", "0.5", "Sampling radius for AO.", CVAR_SAVE);
CVar r_ssao_bias("r_ssao_bias", "0.025", "Occlusion bias to prevent self-shadowing.", CVAR_SAVE);
CVar r_ssao_samples("r_ssao_samples", "32", "Number of AO samples per pixel.", CVAR_SAVE);
CVar r_ssao_power("r_ssao_power", "2.0", "Contrast strength of the AO effect.", CVAR_SAVE);
CVar r_ssao_blur_passes("r_ssao_blur_passes", "4", "Number of blur passes for SSAO.", CVAR_SAVE);
CVar r_ssao_downsample("r_ssao_downsample", "2", "Downscaling factor for SSAO buffer (higher = faster).", CVAR_SAVE);

R_SSAO::R_SSAO() 
{
}

R_SSAO::~R_SSAO() 
{ 
    Shutdown(); 
}

bool R_SSAO::Init(int width, int height) 
{
    m_ssaoShader.Load("shaders/ssao.vert", "shaders/ssao.frag");
    m_blurShader.Load("shaders/ssao_blur.vert", "shaders/ssao_blur.frag");
    
    GenerateSampleKernel();
    
    CreateBuffers(width, height);
    return true;
}

void R_SSAO::GenerateSampleKernel()
{
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    
    m_ssaoKernel.clear();
    int samples = r_ssao_samples.GetInt();
    for (int i = 0; i < samples; ++i)
    {
        glm::vec3 sample(
            randomFloats(generator) * 2.0 - 1.0, 
            randomFloats(generator) * 2.0 - 1.0, 
            randomFloats(generator)
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        
        float scale = float(i) / float(samples);
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;
        
        m_ssaoKernel.push_back(sample);
    }
}

void R_SSAO::GenerateNoiseTexture()
{
    if (m_noiseTexture != 0) 
        glDeleteTextures(1, &m_noiseTexture);

    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise(
            randomFloats(generator) * 2.0 - 1.0, 
            randomFloats(generator) * 2.0 - 1.0, 
            0.0f); 
        ssaoNoise.push_back(noise);
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &m_noiseTexture);
    glTextureStorage2D(m_noiseTexture, 1, GL_RGB16F, 4, 4);
    glTextureSubImage2D(m_noiseTexture, 0, 0, 0, 4, 4, GL_RGB, GL_FLOAT, ssaoNoise.data());
    glTextureParameteri(m_noiseTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_noiseTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(m_noiseTexture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(m_noiseTexture, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void R_SSAO::CreateBuffers(int width, int height)
{
    m_width = width;
    m_height = height;

    int ds = std::max(1, r_ssao_downsample.GetInt());

    int vW = width / ds;
    int vH = height / ds;

    glCreateFramebuffers(1, &m_fbo);
    glCreateTextures(GL_TEXTURE_2D, 1, &m_texture);
    glTextureStorage2D(m_texture, 1, GL_R8, vW, vH);
    glTextureParameteri(m_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(m_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glNamedFramebufferTexture(m_fbo, GL_COLOR_ATTACHMENT0, m_texture, 0);

    glCreateFramebuffers(2, m_blurFbo);
    glCreateTextures(GL_TEXTURE_2D, 2, m_blurTexture);
    for (int i = 0; i < 2; i++)
    {
        glTextureStorage2D(m_blurTexture[i], 1, GL_R8, vW, vH);
        glTextureParameteri(m_blurTexture[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(m_blurTexture[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glNamedFramebufferTexture(m_blurFbo[i], GL_COLOR_ATTACHMENT0, m_blurTexture[i], 0);
    }

    GenerateNoiseTexture();
}

void R_SSAO::DeleteBuffers() 
{
    if (m_fbo)
        glDeleteFramebuffers(1, &m_fbo);
    if (m_texture)
        glDeleteTextures(1, &m_texture);
    if (m_blurFbo[0])
        glDeleteFramebuffers(2, m_blurFbo);
    if (m_blurTexture[0])
        glDeleteTextures(2, m_blurTexture);
    if (m_noiseTexture)
        glDeleteTextures(1, &m_noiseTexture);
    m_fbo = m_texture = m_blurFbo[0] = m_blurFbo[1] = m_blurTexture[0] = m_blurTexture[1] = m_noiseTexture = 0;
}

void R_SSAO::Rescale(int width, int height) 
{
    DeleteBuffers();
    CreateBuffers(width, height);
}

void R_SSAO::Shutdown() 
{
    DeleteBuffers();
}

void R_SSAO::Render(GLuint depthTexture, const Camera& camera, GLuint quadVAO, int screenW, int screenH)
{
    if (r_ssao.GetInt() == 0)
        return;

    // SSAO pass
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    int ds = std::max(1, r_ssao_downsample.GetInt());

    int vW = screenW / ds;
    int vH = screenH / ds;
    glViewport(0, 0, vW, vH);

    glClear(GL_COLOR_BUFFER_BIT);
    m_ssaoShader.Bind();
    m_ssaoShader.SetMat4("u_projection", camera.GetProjectionMatrix());
    m_ssaoShader.SetMat4("u_invProjection", glm::inverse(camera.GetProjectionMatrix()));
    for (unsigned int i = 0; i < m_ssaoKernel.size(); ++i)
        m_ssaoShader.SetVec3("u_samples[" + std::to_string(i) + "]", m_ssaoKernel[i]);
    
    m_ssaoShader.SetInt("u_kernelSize", r_ssao_samples.GetInt());
    m_ssaoShader.SetFloat("u_radius", r_ssao_radius.GetFloat());
    m_ssaoShader.SetFloat("u_bias", r_ssao_bias.GetFloat());
    m_ssaoShader.SetFloat("u_power", r_ssao_power.GetFloat());

    int res = std::max(1, r_ssao_downsample.GetInt());
    m_ssaoShader.SetVec2("u_noiseScale", glm::vec2((float)(screenW / res) / 4.0f, (float)(screenH / res) / 4.0f));
    
    glBindTextureUnit(0, depthTexture);
    glBindTextureUnit(1, m_noiseTexture);
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Blur pass
    m_blurShader.Bind();
    bool horizontal = true, first_iteration = true;
    for (int i = 0; i < r_ssao_blur_passes.GetInt(); i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_blurFbo[horizontal]);
        m_blurShader.SetInt("u_horizontal", horizontal ? 1 : 0);

        glBindTextureUnit(0, first_iteration ? m_texture : m_blurTexture[!horizontal]);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        horizontal = !horizontal;
        if (first_iteration)
            first_iteration = false;
    }

    glViewport(0, 0, screenW, screenH);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_SSAO::Bind(const R_Shader& shader)
{
    if (r_ssao.GetInt() > 0)
    {
        shader.SetInt("u_ssao_enabled", 1);
        glBindTextureUnit(4, m_blurTexture[0]);
    }
    else 
    {
        shader.SetInt("u_ssao_enabled", 0);
    }
}