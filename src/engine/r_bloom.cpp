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
#include "r_bloom.h"
#include "cvar.h"
#include <algorithm>

CVar r_bloom("r_bloom", "1", "Enable bloom post-processing.", CVAR_SAVE);
CVar r_bloom_intensity("r_bloom_intensity", "2", "Strength of the bloom effect.", CVAR_SAVE);
CVar r_bloom_threshold("r_bloom_threshold", "0.9", "Brightness threshold for pixels to bloom.", CVAR_SAVE);
CVar r_bloom_scatter("r_bloom_scatter", "0.7", "Scatter amount for bloom blur.", CVAR_SAVE);
CVar r_bloom_passes("r_bloom_passes", "6", "Number of downsample/upsample passes.", CVAR_SAVE);

R_Bloom::R_Bloom() 
{
}

R_Bloom::~R_Bloom() 
{ 
    Shutdown();
}

bool R_Bloom::Init(int width, int height)
{
    m_downsampleShader.LoadCompute("shaders/bloom_downsample.comp");
    m_upsampleShader.LoadCompute("shaders/bloom_upsample.comp");

    m_sSource = bgfx::createUniform("s_source", bgfx::UniformType::Sampler);
    m_sCurrentMip = bgfx::createUniform("s_currentMip", bgfx::UniformType::Sampler);
    m_uBloomParams = bgfx::createUniform("u_bloomParams", bgfx::UniformType::Vec4);
    m_uBloomParams2 = bgfx::createUniform("u_bloomParams2", bgfx::UniformType::Vec4);

    CreateChain(width, height);
    return true;
}

void R_Bloom::Rescale(int width, int height)
{
    DeleteChain();
    CreateChain(width, height);
}

void R_Bloom::CreateChain(int width, int height)
{
    glm::ivec2 mipSize(width, height);
    int passes = std::max(1, r_bloom_passes.GetInt());

    uint64_t writeFlags = BGFX_TEXTURE_COMPUTE_WRITE | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

    for (int i = 0; i < passes; i++)
    {
        mipSize /= 2;
        if (mipSize.x == 0 || mipSize.y == 0)
            break;

        Mip mip;
        mip.size = mipSize;

        mip.downsampleTex = bgfx::createTexture2D((uint16_t)mip.size.x, (uint16_t)mip.size.y, false, 1, bgfx::TextureFormat::RGBA16F, writeFlags);

        if (i < passes - 1)
        {
            mip.upsampleTex = bgfx::createTexture2D((uint16_t)mip.size.x, (uint16_t)mip.size.y, false, 1, bgfx::TextureFormat::RGBA16F, writeFlags);
        }

        m_mipChain.push_back(mip);
    }
}

void R_Bloom::Render(bgfx::ViewId viewId, bgfx::TextureHandle sourceTexture, int screenW, int screenH)
{
    if (r_bloom.GetInt() == 0 || m_mipChain.empty())
        return;

    // Downsample
    bgfx::TextureHandle currentSource = sourceTexture;
    for (size_t i = 0; i < m_mipChain.size(); ++i)
    {
        const auto& mip = m_mipChain[i];

        float params[4] = { r_bloom_threshold.GetFloat(), (i == 0) ? 1.0f : 0.0f, 0.0f, 0.0f };
        bgfx::setUniform(m_uBloomParams, params);

        bgfx::setTexture(0, m_sSource, currentSource);
        bgfx::setImage(1, mip.downsampleTex, 0, bgfx::Access::Write, bgfx::TextureFormat::RGBA16F);

        bgfx::dispatch(viewId, m_downsampleShader.GetProgram(), (mip.size.x + 15) / 16, (mip.size.y + 15) / 16, 1);

        currentSource = mip.downsampleTex;
    }

    // Upsample
    for (int i = (int)m_mipChain.size() - 1; i > 0; --i)
    {
        const auto& mip = m_mipChain[i];
        const auto& nextMip = m_mipChain[i - 1];

        float params2[4] = { r_bloom_scatter.GetFloat(), 0.0f, 0.0f, 0.0f };
        bgfx::setUniform(m_uBloomParams2, params2);

        bgfx::TextureHandle smallerTex = (i == (int)m_mipChain.size() - 1) ? mip.downsampleTex : mip.upsampleTex;
        bgfx::setTexture(0, m_sSource, smallerTex);
        bgfx::setTexture(1, m_sCurrentMip, nextMip.downsampleTex);

        bgfx::setImage(2, nextMip.upsampleTex, 0, bgfx::Access::Write, bgfx::TextureFormat::RGBA16F);

        bgfx::dispatch(viewId, m_upsampleShader.GetProgram(), (nextMip.size.x + 15) / 16, (nextMip.size.y + 15) / 16, 1);
    }
}

void R_Bloom::Bind(bgfx::UniformHandle s_bloomTex)
{
    if (r_bloom.GetInt() > 0 && !m_mipChain.empty())
    {
        bgfx::setTexture(2, s_bloomTex, m_mipChain[0].upsampleTex);
    }
}

bgfx::TextureHandle R_Bloom::GetBloomTexture() const
{
    if (!m_mipChain.empty())
    {
        return m_mipChain[0].upsampleTex;
    }
    return BGFX_INVALID_HANDLE;
}

void R_Bloom::DeleteChain()
{
    for (auto& mip : m_mipChain)
    {
        if (bgfx::isValid(mip.downsampleTex)) 
            bgfx::destroy(mip.downsampleTex);
        if (bgfx::isValid(mip.upsampleTex)) 
            bgfx::destroy(mip.upsampleTex);
    }
    m_mipChain.clear();
}

void R_Bloom::Shutdown()
{
    DeleteChain();
    if (bgfx::isValid(m_sSource))
    {
        bgfx::destroy(m_sSource);
        bgfx::destroy(m_sCurrentMip);
        bgfx::destroy(m_uBloomParams);
        bgfx::destroy(m_uBloomParams2);
        m_sSource = m_sCurrentMip = m_uBloomParams = m_uBloomParams2 = BGFX_INVALID_HANDLE;
    }
}