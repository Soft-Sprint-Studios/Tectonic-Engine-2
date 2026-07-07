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
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

CVar r_ssao("r_ssao", "1", "Enable Screen Space Ambient Occlusion.", CVAR_SAVE);
CVar r_ssao_radius("r_ssao_radius", "0.5", "Sampling radius for AO.", CVAR_SAVE);
CVar r_ssao_bias("r_ssao_bias", "0.025", "Occlusion bias to prevent self-shadowing.", CVAR_SAVE);
CVar r_ssao_samples("r_ssao_samples", "16", "Number of AO samples per pixel.", CVAR_SAVE);
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
    m_ssaoShader.LoadCompute("shaders/ssao.comp");
    m_blurShader.LoadCompute("shaders/ssao_blur.comp");

    m_sDepthTexture = bgfx::createUniform("s_depth", bgfx::UniformType::Sampler);
    m_sNoiseTexture = bgfx::createUniform("s_noise", bgfx::UniformType::Sampler);
    m_sNormalTexture = bgfx::createUniform("s_normal", bgfx::UniformType::Sampler);
    m_sAOTexture = bgfx::createUniform("s_aoTex", bgfx::UniformType::Sampler);
    m_uKernel = bgfx::createUniform("u_kernel", bgfx::UniformType::Vec4, 32);
    m_uParams = bgfx::createUniform("u_params", bgfx::UniformType::Vec4);
    m_uNoiseScale = bgfx::createUniform("u_noiseScale", bgfx::UniformType::Vec4);
    m_uBlurParams = bgfx::createUniform("u_blurParams", bgfx::UniformType::Vec4);
    m_uCurrentProj = bgfx::createUniform("u_currentProj", bgfx::UniformType::Mat4);
    m_uCurrentInvProj = bgfx::createUniform("u_currentInvProj", bgfx::UniformType::Mat4);
    m_uCurrentView = bgfx::createUniform("u_currentView", bgfx::UniformType::Mat4);

    GenerateSampleKernel();
    CreateBuffers(width, height);
    return true;
}

void R_SSAO::GenerateSampleKernel()
{
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;
    
    m_ssaoKernel.clear();
    int samples = r_ssao_samples.GetInt();
    for (int i = 0; i < samples; ++i)
    {
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f, 
            randomFloats(generator) * 2.0f - 1.0f, 
            randomFloats(generator)
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        
        float scale = float(i) / float(samples);
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;
        
        m_ssaoKernel.push_back(glm::vec4(sample, 0.0f));
    }
}

void R_SSAO::GenerateNoiseTexture()
{
    if (bgfx::isValid(m_noiseTexture)) 
        bgfx::destroy(m_noiseTexture);

    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;
    std::vector<glm::vec4> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec4 noise(
            randomFloats(generator) * 2.0f - 1.0f, 
            randomFloats(generator) * 2.0f - 1.0f, 
            0.0f,
            0.0f
        ); 
        ssaoNoise.push_back(glm::normalize(noise));
    }

    const bgfx::Memory* mem = bgfx::copy(ssaoNoise.data(), (uint32_t)(16 * sizeof(glm::vec4)));
    m_noiseTexture = bgfx::createTexture2D(4, 4, false, 1, bgfx::TextureFormat::RGBA32F, BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT, mem);
}

void R_SSAO::CreateBuffers(int width, int height)
{
    m_width = width;
    m_height = height;

    int ds = std::max(1, r_ssao_downsample.GetInt());
    int vW = width / ds;
    int vH = height / ds;

    uint64_t writeFlags = BGFX_TEXTURE_COMPUTE_WRITE | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

    m_texture = bgfx::createTexture2D((uint16_t)vW, (uint16_t)vH, false, 1, bgfx::TextureFormat::R8, writeFlags);
    m_blurTexture[0] = bgfx::createTexture2D((uint16_t)vW, (uint16_t)vH, false, 1, bgfx::TextureFormat::R8, writeFlags);
    m_blurTexture[1] = bgfx::createTexture2D((uint16_t)vW, (uint16_t)vH, false, 1, bgfx::TextureFormat::R8, writeFlags);

    GenerateNoiseTexture();
}

void R_SSAO::DeleteBuffers() 
{
    if (bgfx::isValid(m_texture))
        bgfx::destroy(m_texture);
    if (bgfx::isValid(m_blurTexture[0]))
        bgfx::destroy(m_blurTexture[0]);
    if (bgfx::isValid(m_blurTexture[1]))
        bgfx::destroy(m_blurTexture[1]);
    if (bgfx::isValid(m_noiseTexture))
        bgfx::destroy(m_noiseTexture);

    m_texture = m_blurTexture[0] = m_blurTexture[1] = m_noiseTexture = BGFX_INVALID_HANDLE;
}

void R_SSAO::Rescale(int width, int height) 
{
    DeleteBuffers();
    CreateBuffers(width, height);
}

void R_SSAO::Shutdown()
{
    DeleteBuffers();
    if (bgfx::isValid(m_sDepthTexture))
    {
        bgfx::destroy(m_sDepthTexture);
        bgfx::destroy(m_sNormalTexture);
        bgfx::destroy(m_sNoiseTexture);
        bgfx::destroy(m_sAOTexture);
        bgfx::destroy(m_uKernel);
        bgfx::destroy(m_uParams);
        bgfx::destroy(m_uNoiseScale);
        bgfx::destroy(m_uBlurParams);
        bgfx::destroy(m_uCurrentProj);
        bgfx::destroy(m_uCurrentInvProj);
        bgfx::destroy(m_uCurrentView);
        m_sDepthTexture = m_sNormalTexture = m_sNoiseTexture = m_sAOTexture = m_uKernel = m_uParams = m_uNoiseScale = m_uBlurParams = m_uCurrentProj = m_uCurrentInvProj = m_uCurrentView = BGFX_INVALID_HANDLE;
    }
}

void R_SSAO::Render(bgfx::ViewId viewId, bgfx::TextureHandle depthTexture, bgfx::TextureHandle normalTexture, const Camera& camera, int screenW, int screenH)
{
    if (r_ssao.GetInt() == 0)
        return;

    int ds = std::max(1, r_ssao_downsample.GetInt());
    int vW = screenW / ds;
    int vH = screenH / ds;

    bgfx::setTexture(0, m_sDepthTexture, depthTexture);
    bgfx::setTexture(1, m_sNoiseTexture, m_noiseTexture);
    bgfx::setTexture(2, m_sNormalTexture, normalTexture);
    bgfx::setImage(3, m_texture, 0, bgfx::Access::Write, bgfx::TextureFormat::R8);

    bgfx::setUniform(m_uKernel, glm::value_ptr(m_ssaoKernel[0]), (uint16_t)m_ssaoKernel.size());

    float params[4] = { (float)r_ssao_samples.GetInt(), r_ssao_radius.GetFloat(), r_ssao_bias.GetFloat(), r_ssao_power.GetFloat() };
    bgfx::setUniform(m_uParams, params);

    float noiseScale[4] = { (float)vW / 4.0f, (float)vH / 4.0f, 0.0f, 0.0f };
    bgfx::setUniform(m_uNoiseScale, noiseScale);

    glm::mat4 proj = camera.GetProjectionMatrix();
    glm::mat4 invProj = glm::inverse(proj);
    glm::mat4 view = camera.GetViewMatrix();
    bgfx::setUniform(m_uCurrentProj, glm::value_ptr(proj));
    bgfx::setUniform(m_uCurrentInvProj, glm::value_ptr(invProj));
    bgfx::setUniform(m_uCurrentView, glm::value_ptr(view));

    bgfx::dispatch(viewId, m_ssaoShader.GetProgram(), (vW + 15) / 16, (vH + 15) / 16, 1);

    bool horizontal = true;
    bgfx::TextureHandle currentInput = m_texture;
    int blurPasses = r_ssao_blur_passes.GetInt();
    
    for (int i = 0; i < blurPasses; i++)
    {
        bgfx::ViewId blurView = viewId + 1 + i;
        bgfx::setTexture(0, m_sAOTexture, currentInput);
        bgfx::setImage(1, m_blurTexture[horizontal ? 1 : 0], 0, bgfx::Access::Write, bgfx::TextureFormat::R8);

        float blurParams[4] = { horizontal ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f };
        bgfx::setUniform(m_uBlurParams, blurParams);

        bgfx::dispatch(blurView, m_blurShader.GetProgram(), (vW + 15) / 16, (vH + 15) / 16, 1);

        currentInput = m_blurTexture[horizontal ? 1 : 0];
        horizontal = !horizontal;
    }
}

void R_SSAO::Bind(bgfx::UniformHandle s_ssaoTex)
{
    if (r_ssao.GetInt() > 0)
    {
        int blurPasses = r_ssao_blur_passes.GetInt();
        bgfx::TextureHandle finalTex = (blurPasses % 2 == 0) ? m_blurTexture[0] : m_blurTexture[1];
        bgfx::setTexture(4, s_ssaoTex, finalTex);
    }
}