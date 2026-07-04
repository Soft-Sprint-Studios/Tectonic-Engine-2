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
#include "r_ssr.h"
#include "cvar.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

CVar r_ssr("r_ssr", "0", "Enable Screen Space Reflections.", CVAR_SAVE);
CVar r_ssr_max_dist("r_ssr_max_dist", "20.0", "Maximum ray distance for SSR.", CVAR_SAVE);
CVar r_ssr_resolution("r_ssr_resolution", "0.3", "Step resolution for ray marching.", CVAR_SAVE);
CVar r_ssr_thickness("r_ssr_thickness", "1.0", "Z-thickness for ray collision.", CVAR_SAVE);
CVar r_ssr_steps("r_ssr_steps", "64", "Number of linear search steps for SSR.", CVAR_SAVE);
CVar r_ssr_binary_steps("r_ssr_binary_steps", "16", "Number of binary search refinement steps.", CVAR_SAVE);
CVar r_ssr_downsample("r_ssr_downsample", "2", "Downscaling factor for SSR buffer (higher = faster).", CVAR_SAVE);

R_SSR::R_SSR() 
{
}

R_SSR::~R_SSR() 
{ 
    Shutdown();
}

bool R_SSR::Init(int width, int height)
{
    m_ssrShader.LoadCompute("shaders/ssr.comp");
    m_blurShader.LoadCompute("shaders/ssr_blur.comp");

    m_sDepth  = bgfx::createUniform("s_depth",  bgfx::UniformType::Sampler);
    m_sNormal = bgfx::createUniform("s_normal", bgfx::UniformType::Sampler);
    m_sMRAO   = bgfx::createUniform("s_mrao",   bgfx::UniformType::Sampler);
    m_sScene  = bgfx::createUniform("s_scene",  bgfx::UniformType::Sampler);
    m_sSSR    = bgfx::createUniform("s_ssrTex", bgfx::UniformType::Sampler);

    m_uParams1 = bgfx::createUniform("u_ssrParams1", bgfx::UniformType::Vec4);
    m_uParams2 = bgfx::createUniform("u_ssrParams2", bgfx::UniformType::Vec4);

    CreateBuffers(width, height);
    return true;
}

void R_SSR::CreateBuffers(int width, int height)
{
    m_width = width;
    m_height = height;

    int ds = std::max(1, r_ssr_downsample.GetInt());
    int ssrW = width / ds;
    int ssrH = height / ds;

    uint64_t writeFlags = BGFX_TEXTURE_COMPUTE_WRITE | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
    m_texture = bgfx::createTexture2D((uint16_t)ssrW, (uint16_t)ssrH, false, 1, bgfx::TextureFormat::RGBA16F, writeFlags);
    m_blurTexture = bgfx::createTexture2D((uint16_t)ssrW, (uint16_t)ssrH, false, 1, bgfx::TextureFormat::RGBA16F, writeFlags);
}

void R_SSR::Render(bgfx::ViewId viewId, bgfx::TextureHandle depthTex, bgfx::TextureHandle normalTex, bgfx::TextureHandle mraoTex, bgfx::TextureHandle sceneTex, const Camera& camera)
{
    if (r_ssr.GetInt() == 0)
    {
        return;
    }

    int ds = std::max(1, r_ssr_downsample.GetInt());
    int ssrW = m_width / ds;
    int ssrH = m_height / ds;

    bgfx::setTexture(0, m_sDepth,  depthTex);
    bgfx::setTexture(1, m_sNormal, normalTex);
    bgfx::setTexture(2, m_sMRAO,   mraoTex);
    bgfx::setTexture(3, m_sScene,  sceneTex);
    bgfx::setImage(4, m_texture, 0, bgfx::Access::Write, bgfx::TextureFormat::RGBA16F);

    float params1[4] = { r_ssr_max_dist.GetFloat(), r_ssr_resolution.GetFloat(), r_ssr_thickness.GetFloat(), (float)r_ssr_steps.GetInt() };
    bgfx::setUniform(m_uParams1, params1);

    float params2[4] = { (float)r_ssr_binary_steps.GetInt(), 0.0f, 0.0f, 0.0f };
    bgfx::setUniform(m_uParams2, params2);

    bgfx::dispatch(viewId, m_ssrShader.GetProgram(), (ssrW + 15) / 16, (ssrH + 15) / 16, 1);

    bgfx::setTexture(0, m_sSSR, m_texture);
    bgfx::setImage(1, m_blurTexture, 0, bgfx::Access::Write, bgfx::TextureFormat::RGBA16F);

    bgfx::dispatch(viewId + 1, m_blurShader.GetProgram(), (ssrW + 15) / 16, (ssrH + 15) / 16, 1);
}

void R_SSR::Bind(bgfx::UniformHandle s_ssrTex)
{
    if (r_ssr.GetInt() > 0)
    {
        bgfx::setTexture(5, s_ssrTex, m_blurTexture);
    }
}

void R_SSR::DeleteBuffers()
{
    if (bgfx::isValid(m_texture))
        bgfx::destroy(m_texture);
    if (bgfx::isValid(m_blurTexture))
        bgfx::destroy(m_blurTexture);
    m_texture = m_blurTexture = BGFX_INVALID_HANDLE;
}

void R_SSR::Rescale(int width, int height)
{
    DeleteBuffers();
    CreateBuffers(width, height);
}

void R_SSR::Shutdown()
{
    DeleteBuffers();
    if (bgfx::isValid(m_sDepth))
    {
        bgfx::destroy(m_sDepth);
        bgfx::destroy(m_sNormal);
        bgfx::destroy(m_sMRAO);
        bgfx::destroy(m_sScene);
        bgfx::destroy(m_sSSR);
        bgfx::destroy(m_uParams1);
        bgfx::destroy(m_uParams2);
        m_sDepth = m_sNormal = m_sMRAO = m_sScene = m_sSSR = BGFX_INVALID_HANDLE;
    }
}