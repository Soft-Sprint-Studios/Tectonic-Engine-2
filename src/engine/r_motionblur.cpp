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
 * furnished to do_ subject to the following conditions:
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
#include "r_motionblur.h"
#include "cvar.h"
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

CVar r_motionblur("r_motionblur", "1", "Enable Motion Blur.", CVAR_SAVE);
CVar r_motionblur_samples("r_motionblur_samples", "16", "Number of samples for motion blur.", CVAR_SAVE);
CVar r_motionblur_scale("r_motionblur_scale", "1.0", "Intensity of the motion blur effect.", CVAR_SAVE);

R_MotionBlur::R_MotionBlur() {}
R_MotionBlur::~R_MotionBlur() { Shutdown(); }

bool R_MotionBlur::Init(int width, int height)
{
    m_shader.LoadCompute("shaders/motion_blur.comp");

    m_sSceneTex     = bgfx::createUniform("s_scene",        bgfx::UniformType::Sampler);
    m_sDepthTex     = bgfx::createUniform("s_depth",        bgfx::UniformType::Sampler);
    m_uInvViewProj  = bgfx::createUniform("u_currentInvViewProj",  bgfx::UniformType::Mat4);
    m_uPrevViewProj = bgfx::createUniform("u_prevViewProj", bgfx::UniformType::Mat4);
    m_uBlurParams   = bgfx::createUniform("u_blurParams",   bgfx::UniformType::Vec4);

    CreateBuffers(width, height);
    return true;
}

void R_MotionBlur::CreateBuffers(int width, int height)
{
    m_width = width;
    m_height = height;

    uint64_t writeFlags = BGFX_TEXTURE_COMPUTE_WRITE | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
    m_texture = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::RGBA16F, writeFlags);
}

void R_MotionBlur::Render(bgfx::ViewId viewId, bgfx::TextureHandle sceneTex, bgfx::TextureHandle depthTex, const Camera& camera)
{
    if (r_motionblur.GetInt() == 0)
    {
        return;
    }

    glm::mat4 viewProj = camera.GetProjectionMatrix() * camera.GetViewMatrix();
    glm::mat4 invViewProj = glm::inverse(viewProj);
    glm::mat4 prevViewProj = camera.GetPrevViewProj();

    bgfx::setUniform(m_uInvViewProj,  glm::value_ptr(invViewProj));
    bgfx::setUniform(m_uPrevViewProj, glm::value_ptr(prevViewProj));

    float params[4] = { (float)r_motionblur_samples.GetInt(), r_motionblur_scale.GetFloat(), 0.0f, 0.0f };
    bgfx::setUniform(m_uBlurParams, params);

    bgfx::setTexture(0, m_sSceneTex, sceneTex);
    bgfx::setTexture(1, m_sDepthTex, depthTex);
    bgfx::setImage(2, m_texture, 0, bgfx::Access::Write, bgfx::TextureFormat::RGBA16F);

    bgfx::dispatch(viewId, m_shader.GetProgram(), (m_width + 15) / 16, (m_height + 15) / 16, 1);
}

void R_MotionBlur::Bind(bgfx::UniformHandle s_motionBlurTex)
{
    if (r_motionblur.GetInt() > 0)
    {
        bgfx::setTexture(7, s_motionBlurTex, m_texture);
    }
}

void R_MotionBlur::DeleteBuffers()
{
    if (bgfx::isValid(m_texture))
        bgfx::destroy(m_texture);
    m_texture = BGFX_INVALID_HANDLE;
}

void R_MotionBlur::Rescale(int width, int height)
{
    DeleteBuffers();
    CreateBuffers(width, height);
}

void R_MotionBlur::Shutdown()
{
    DeleteBuffers();
    if (bgfx::isValid(m_sSceneTex))
    {
        bgfx::destroy(m_sSceneTex);
        bgfx::destroy(m_sDepthTex);
        bgfx::destroy(m_uInvViewProj);
        bgfx::destroy(m_uPrevViewProj);
        bgfx::destroy(m_uBlurParams);
        m_sSceneTex = m_sDepthTex = m_uInvViewProj = m_uPrevViewProj = m_uBlurParams = BGFX_INVALID_HANDLE;
    }
}