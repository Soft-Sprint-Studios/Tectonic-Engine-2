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
#include "r_postprocess.h"
#include "console.h"
#include "cvar.h"
#include "postprocess.h"
#include "timing.h"
#include "renderer.h"
#include <algorithm>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

CVar r_postprocess("r_postprocess", "1", "Enable entire post-processing stack.", CVAR_SAVE);
CVar r_gamma("r_gamma", "1.7", "Display gamma correction value.", CVAR_SAVE);
CVar r_tonemap("r_tonemap", "1", "Enable filmic ACES tonemapping.", CVAR_SAVE);
CVar r_fxaa("r_fxaa", "1", "Enable Fast Approximate Anti-Aliasing.", CVAR_SAVE);
CVar r_fxaa_strength("r_fxaa_strength", "1.0", "Strength of FXAA smoothing.", CVAR_SAVE);

R_PostProcess::R_PostProcess() {}
R_PostProcess::~R_PostProcess() { Shutdown(); }

bool R_PostProcess::Init(int width, int height, bgfx::TextureHandle depthTexture)
{
    m_width = width;
    m_height = height;

    m_shader.Load("shaders/postprocess.vert", "shaders/postprocess.frag");

    m_sSceneTexture = bgfx::createUniform("u_screenTexture", bgfx::UniformType::Sampler);
    m_sDepthTexture = bgfx::createUniform("u_depthTexture", bgfx::UniformType::Sampler);
    m_sBloomTexture = bgfx::createUniform("u_bloomTexture", bgfx::UniformType::Sampler);
    m_sSsaoTexture = bgfx::createUniform("u_ssaoTexture", bgfx::UniformType::Sampler);
    m_sSsrTexture = bgfx::createUniform("u_ssrTexture", bgfx::UniformType::Sampler);
    m_sMotionBlurTexture = bgfx::createUniform("u_motionBlurTexture", bgfx::UniformType::Sampler);
    m_sVolumetricTexture = bgfx::createUniform("u_volumetricTexture", bgfx::UniformType::Sampler);
    m_uParams = bgfx::createUniform("u_params", bgfx::UniformType::Vec4);
    m_uBloomParams = bgfx::createUniform("u_bloomParams", bgfx::UniformType::Vec4);
    m_uSsaoParams = bgfx::createUniform("u_ssaoParams", bgfx::UniformType::Vec4);
    m_uSsrParams = bgfx::createUniform("u_ssrParams", bgfx::UniformType::Vec4);
    m_uMotionBlurParams = bgfx::createUniform("u_motionBlurParams", bgfx::UniformType::Vec4);
    m_uVolumetricParams = bgfx::createUniform("u_volumetricParams", bgfx::UniformType::Vec4);
    m_uColorParams = bgfx::createUniform("u_colorParams", bgfx::UniformType::Vec4, 2);
    m_uFogColor = bgfx::createUniform("u_fogColor", bgfx::UniformType::Vec4);
    m_uFogParams = bgfx::createUniform("u_fogParams", bgfx::UniformType::Vec4);

    m_quadLayout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    m_autoExposure = std::make_unique<R_AutoExposure>();
    m_autoExposure->Init();

    m_bloom = std::make_unique<R_Bloom>();
    m_bloom->Init(m_width, m_height);

    m_ssao = std::make_unique<R_SSAO>();
    m_ssao->Init(m_width, m_height);

    m_ssr = std::make_unique<R_SSR>();
    m_ssr->Init(m_width, m_height);

    m_motionBlur = std::make_unique<R_MotionBlur>();
    m_motionBlur->Init(m_width, m_height);

    m_volumetrics = std::make_unique<R_Volumetrics>();
    m_volumetrics->Init(m_width, m_height);

    SetupBuffers(depthTexture);
    return true;
}

void R_PostProcess::SetupBuffers(bgfx::TextureHandle depthTexture)
{
    if (bgfx::isValid(m_fbo))
    {
        bgfx::destroy(m_fbo);
        m_fbo = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_texture))
    {
        bgfx::destroy(m_texture);
        m_texture = BGFX_INVALID_HANDLE;
    }

    uint64_t rtFlags = BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
    m_texture = bgfx::createTexture2D((uint16_t)m_width, (uint16_t)m_height, false, 1, bgfx::TextureFormat::RGBA16F, rtFlags);

    bgfx::TextureHandle attachments[] =
    {
        m_texture,
        depthTexture
    };

    m_fbo = bgfx::createFrameBuffer(2, attachments, true);
}

void R_PostProcess::Begin()
{
    bgfx::ViewId resolveView = RenderView::Resolve;
    bgfx::setViewClear(resolveView, BGFX_CLEAR_COLOR, 0x000000FF, 1.0f, 0);
    bgfx::setViewRect(resolveView, 0, 0, (uint16_t)m_width, (uint16_t)m_height);
    bgfx::setViewFrameBuffer(resolveView, m_fbo);
}

void R_PostProcess::Draw(const Camera& camera, R_Lights* lights, R_GBuffer* gbuffer)
{
    m_autoExposure->Render(RenderView::AutoExposure, m_texture, m_width, m_height);
    m_bloom->Render(RenderView::Bloom, m_texture, m_width, m_height);
    m_ssao->Render(RenderView::SSAO, gbuffer->GetDepthTex(), camera, m_width, m_height);
    m_ssr->Render(RenderView::SSR, gbuffer->GetDepthTex(), gbuffer->GetNormalTex(), gbuffer->GetMRAOTex(), m_texture, camera);
    m_motionBlur->Render(RenderView::MotionBlur, m_texture, gbuffer->GetDepthTex(), camera);
    m_volumetrics->Render(RenderView::Volumetrics, gbuffer->GetDepthTex(), camera, lights, m_width, m_height);

    bgfx::ViewId viewId = RenderView::PostProcess;

    bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000FF, 1.0f, 0);
    bgfx::setViewRect(viewId, 0, 0, (uint16_t)m_width, (uint16_t)m_height);
    bgfx::setViewFrameBuffer(viewId, BGFX_INVALID_HANDLE);

    const auto& ppSettings = PostProcess::GetCurrentSettings();

    float params[4] = { (float)Time::TotalTime(), ppSettings.vignetteStrength, ppSettings.chromaStrength, ppSettings.grainStrength };
    bgfx::setUniform(m_uParams, params);

    float colorParams[8] = 
    {
        ppSettings.bwStrength, ppSettings.negativeStrength, ppSettings.sepiaStrength, r_gamma.GetFloat(),
        r_postprocess.GetInt() > 0 ? 1.0f : 0.0f, r_tonemap.GetInt() > 0 ? 1.0f : 0.0f, r_fxaa.GetInt() > 0 ? 1.0f : 0.0f, r_fxaa_strength.GetFloat()
    };
    bgfx::setUniform(m_uColorParams, colorParams, 2);

    float fogColor[4] = { ppSettings.fogColor.x, ppSettings.fogColor.y, ppSettings.fogColor.z, 0.0f };
    bgfx::setUniform(m_uFogColor, fogColor);

    float fogParams[4] = { ppSettings.fogEnabled ? 1.0f : 0.0f, ppSettings.fogStart, ppSettings.fogEnd, ppSettings.fogAffectsSky ? 1.0f : 0.0f };
    bgfx::setUniform(m_uFogParams, fogParams);

    m_autoExposure->Bind();
    m_bloom->Bind(m_sBloomTexture);
    m_ssao->Bind(m_sSsaoTexture);
    m_ssr->Bind(m_sSsrTexture);
    m_motionBlur->Bind(m_sMotionBlurTexture);
    m_volumetrics->Bind(m_sVolumetricTexture);

    float bloomParams[4] = { CVar::GetInt("r_bloom", 1) > 0 ? 1.0f : 0.0f, CVar::GetFloat("r_bloom_intensity", 2.0f), 0.0f, 0.0f };
    bgfx::setUniform(m_uBloomParams, bloomParams);

    float ssaoParams[4] = { CVar::GetInt("r_ssao", 1) > 0 ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f };
    bgfx::setUniform(m_uSsaoParams, ssaoParams);

    float ssrParams[4] = { CVar::GetInt("r_ssr", 1) > 0 ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f };
    bgfx::setUniform(m_uSsrParams, ssaoParams);

    float volParams[4] = { CVar::GetInt("r_volumetrics", 1) > 0 ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f };
    bgfx::setUniform(m_uVolumetricParams, volParams);

    float mbParams[4] = { CVar::GetInt("r_motionblur", 1) > 0 ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f };
    bgfx::setUniform(m_uMotionBlurParams, mbParams);

    bgfx::setTexture(0, m_sSceneTexture, m_texture);
    bgfx::setTexture(1, m_sDepthTexture, gbuffer->GetDepthTex());

    bgfx::TransientVertexBuffer tvb;
    bgfx::allocTransientVertexBuffer(&tvb, 6, m_quadLayout);

    struct QuadVertex
    {
        float x, y, z;
        float u, v;
    };

    QuadVertex* v = (QuadVertex*)tvb.data;
    v[0] = { -1.0f,  1.0f, 0.0f, 0.0f, 0.0f };
    v[1] = { -1.0f, -1.0f, 0.0f, 0.0f, 1.0f };
    v[2] = {  1.0f, -1.0f, 0.0f, 1.0f, 1.0f };
    v[3] = { -1.0f,  1.0f, 0.0f, 0.0f, 0.0f };
    v[4] = {  1.0f, -1.0f, 0.0f, 1.0f, 1.0f };
    v[5] = {  1.0f,  1.0f, 0.0f, 1.0f, 0.0f };

    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
    bgfx::submit(viewId, m_shader.GetProgram());
}

void R_PostProcess::Rescale(int width, int height, bgfx::TextureHandle depthTexture)
{
    m_width = width;
    m_height = height;
    m_bloom->Rescale(width, height);
    m_ssao->Rescale(width, height);
    m_ssr->Rescale(width, height);
    m_motionBlur->Rescale(width, height);
    m_volumetrics->Rescale(width, height);
    SetupBuffers(depthTexture);
}

void R_PostProcess::Shutdown()
{
    if (bgfx::isValid(m_fbo))
    {
        bgfx::destroy(m_fbo);
        m_fbo = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_texture))
    {
        bgfx::destroy(m_texture);
        m_texture = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_sSceneTexture)) 
        bgfx::destroy(m_sSceneTexture);
    if (bgfx::isValid(m_sDepthTexture))
        bgfx::destroy(m_sDepthTexture);
    if (bgfx::isValid(m_sBloomTexture))
        bgfx::destroy(m_sBloomTexture);
    if (bgfx::isValid(m_sSsaoTexture))
        bgfx::destroy(m_sSsaoTexture);
    if (bgfx::isValid(m_sSsaoTexture))
        bgfx::destroy(m_sSsaoTexture);
    if (bgfx::isValid(m_sMotionBlurTexture))
        bgfx::destroy(m_sMotionBlurTexture);
    if (bgfx::isValid(m_sVolumetricTexture))
        bgfx::destroy(m_sVolumetricTexture);
    if (bgfx::isValid(m_uParams))
        bgfx::destroy(m_uParams);
    if (bgfx::isValid(m_uBloomParams))
        bgfx::destroy(m_uBloomParams);
    if (bgfx::isValid(m_uSsaoParams))
        bgfx::destroy(m_uSsaoParams);
    if (bgfx::isValid(m_uSsaoParams))
        bgfx::destroy(m_uSsaoParams);
    if (bgfx::isValid(m_uMotionBlurParams))
        bgfx::destroy(m_uMotionBlurParams);
    if (bgfx::isValid(m_uVolumetricParams))
        bgfx::destroy(m_uVolumetricParams);
    if (bgfx::isValid(m_uColorParams)) 
        bgfx::destroy(m_uColorParams);
    if (bgfx::isValid(m_uFogColor))
        bgfx::destroy(m_uFogColor);
    if (bgfx::isValid(m_uFogParams)) 
        bgfx::destroy(m_uFogParams);

    if (m_autoExposure)
    {
        m_autoExposure->Shutdown();
        m_autoExposure.reset();
    }

    if (m_bloom)
    {
        m_bloom->Shutdown();
        m_bloom.reset();
    }

    if (m_ssao)
    {
        m_ssao->Shutdown();
        m_ssao.reset();
    }

    if (m_ssr)
    {
        m_ssr->Shutdown();
        m_ssr.reset();
    }

    if (m_motionBlur)
    {
        m_motionBlur->Shutdown();
        m_motionBlur.reset();
    }

    if (m_volumetrics)
    {
        m_volumetrics->Shutdown();
        m_volumetrics.reset();
    }

    m_sSceneTexture = m_sDepthTexture = m_sBloomTexture = m_sSsaoTexture = m_sSsrTexture = m_sMotionBlurTexture = m_sVolumetricTexture = m_uParams = m_uBloomParams = m_uSsaoParams = m_uSsrParams = m_uMotionBlurParams = m_uVolumetricParams = m_uColorParams = m_uFogColor = m_uFogParams = BGFX_INVALID_HANDLE;
}