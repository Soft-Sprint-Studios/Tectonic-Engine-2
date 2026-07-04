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
#include "r_volumetrics.h"
#include "cvar.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

CVar r_volumetrics("r_volumetrics", "1", "Enable volumetric lighting (God rays).", CVAR_SAVE);
CVar r_volumetrics_downsample("r_volumetrics_downsample", "8", "Downscaling factor for volumetric buffer (higher = faster).", CVAR_SAVE);
CVar r_volumetrics_blur_passes("r_volumetrics_blur_passes", "6", "Number of blur passes for volumetrics.", CVAR_SAVE);

R_Volumetrics::R_Volumetrics() {}
R_Volumetrics::~R_Volumetrics() { Shutdown(); }

bool R_Volumetrics::Init(int width, int height) 
{
    m_volShader.LoadCompute("shaders/volumetrics.comp");
    m_blurShader.LoadCompute("shaders/volumetrics_blur.comp");

    m_sDepth          = bgfx::createUniform("s_depth",          bgfx::UniformType::Sampler);
    m_sImage          = bgfx::createUniform("image",            bgfx::UniformType::Sampler);
    m_uCurrentInvProj = bgfx::createUniform("u_currentInvProj", bgfx::UniformType::Mat4);
    m_uCurrentInvView = bgfx::createUniform("u_currentInvView", bgfx::UniformType::Mat4);
    m_uCurrentView    = bgfx::createUniform("u_currentView",    bgfx::UniformType::Mat4);
    m_uCurrentViewPos = bgfx::createUniform("u_currentViewPos", bgfx::UniformType::Vec4);
    m_uBlurParams     = bgfx::createUniform("u_blurParams",     bgfx::UniformType::Vec4);

    CreateBuffers(width, height);
    return true;
}

void R_Volumetrics::CreateBuffers(int width, int height) 
{
    m_width = width;
    m_height = height;

    int ds = std::max(1, r_volumetrics_downsample.GetInt());
    int vW = width / ds;
    int vH = height / ds;

    uint64_t writeFlags = BGFX_TEXTURE_COMPUTE_WRITE | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
    m_texture = bgfx::createTexture2D((uint16_t)vW, (uint16_t)vH, false, 1, bgfx::TextureFormat::RGBA16F, writeFlags);
    m_blurTexture[0] = bgfx::createTexture2D((uint16_t)vW, (uint16_t)vH, false, 1, bgfx::TextureFormat::RGBA16F, writeFlags);
    m_blurTexture[1] = bgfx::createTexture2D((uint16_t)vW, (uint16_t)vH, false, 1, bgfx::TextureFormat::RGBA16F, writeFlags);
}

void R_Volumetrics::DeleteBuffers() 
{
    if (bgfx::isValid(m_texture))
        bgfx::destroy(m_texture);
    if (bgfx::isValid(m_blurTexture[0]))
        bgfx::destroy(m_blurTexture[0]);
    if (bgfx::isValid(m_blurTexture[1]))
        bgfx::destroy(m_blurTexture[1]);
    m_texture = m_blurTexture[0] = m_blurTexture[1] = BGFX_INVALID_HANDLE;
}

void R_Volumetrics::Rescale(int width, int height) 
{
    DeleteBuffers();
    CreateBuffers(width, height);
}

void R_Volumetrics::Shutdown() 
{
    DeleteBuffers();
    if (bgfx::isValid(m_sDepth))
    {
        bgfx::destroy(m_sDepth);
        bgfx::destroy(m_sImage);
        bgfx::destroy(m_uCurrentInvProj);
        bgfx::destroy(m_uCurrentInvView);
        bgfx::destroy(m_uCurrentView);
        bgfx::destroy(m_uCurrentViewPos);
        bgfx::destroy(m_uBlurParams);
        m_sDepth = m_sImage = m_uCurrentInvProj = m_uCurrentInvView = m_uCurrentView = m_uCurrentViewPos = m_uBlurParams = BGFX_INVALID_HANDLE;
    }
}

void R_Volumetrics::Render(bgfx::ViewId viewId, bgfx::TextureHandle depthTexture, const Camera& camera, R_Lights* lights, int screenW, int screenH) 
{
    if (r_volumetrics.GetInt() == 0) 
        return;

    int ds = std::max(1, r_volumetrics_downsample.GetInt());
    int vW = screenW / ds;
    int vH = screenH / ds;

    bgfx::setTexture(0, m_sDepth, depthTexture);
    bgfx::setImage(1, m_texture, 0, bgfx::Access::Write, bgfx::TextureFormat::RGBA16F);

    bgfx::setUniform(m_uCurrentInvProj, glm::value_ptr(glm::inverse(camera.GetProjectionMatrix())));
    bgfx::setUniform(m_uCurrentInvView, glm::value_ptr(glm::inverse(camera.GetViewMatrix())));
    bgfx::setUniform(m_uCurrentView,    glm::value_ptr(camera.GetViewMatrix()));
    
    float vp[4] = { camera.position.x, camera.position.y, camera.position.z, 0.0f };
    bgfx::setUniform(m_uCurrentViewPos, vp);

    if (lights) 
    {
        lights->Bind(m_volShader);
    }

    bgfx::dispatch(viewId, m_volShader.GetProgram(), (vW + 15) / 16, (vH + 15) / 16, 1);

    bool horizontal = true;
    bgfx::TextureHandle currentInput = m_texture;
    int blurPasses = r_volumetrics_blur_passes.GetInt();

    for (int i = 0; i < blurPasses; i++) 
    {
        bgfx::ViewId blurView = viewId + 1 + i;
        bgfx::setTexture(0, m_sImage, currentInput);
        bgfx::setImage(1, m_blurTexture[horizontal ? 1 : 0], 0, bgfx::Access::Write, bgfx::TextureFormat::RGBA16F);

        float blurParams[4] = { horizontal ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f };
        bgfx::setUniform(m_uBlurParams, blurParams);

        bgfx::dispatch(blurView, m_blurShader.GetProgram(), (vW + 15) / 16, (vH + 15) / 16, 1);

        currentInput = m_blurTexture[horizontal ? 1 : 0];
        horizontal = !horizontal;
    }
}

void R_Volumetrics::Bind(bgfx::UniformHandle s_volumetricTex)
{
    if (r_volumetrics.GetInt() > 0) 
    {
        int blurPasses = r_volumetrics_blur_passes.GetInt();
        bgfx::TextureHandle finalTex = (blurPasses % 2 == 0) ? m_blurTexture[0] : m_blurTexture[1];
        bgfx::setTexture(3, s_volumetricTex, finalTex);
    }
}