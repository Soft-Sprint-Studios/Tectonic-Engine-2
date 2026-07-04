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
#include "r_cas.h"
#include "cvar.h"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define A_CPU 1
#include "ffx_a.h"
#include "ffx_cas.h"

CVar r_cas("r_cas", "1", "Enable FidelityFX Contrast Adaptive Sharpening.", CVAR_SAVE);
CVar r_cas_sharpness("r_cas_sharpness", "0.5", "Sharpness intensity for CAS (0.0 to 1.0).", CVAR_SAVE);

R_CAS::R_CAS() 
{
}

R_CAS::~R_CAS() 
{ 
    Shutdown(); 
}

bool R_CAS::Init(int width, int height)
{
    m_shader.LoadCompute("shaders/cas.comp");

    m_sInput = bgfx::createUniform("u_input", bgfx::UniformType::Sampler);
    m_uConst0 = bgfx::createUniform("u_const0", bgfx::UniformType::Vec4);
    m_uConst1 = bgfx::createUniform("u_const1", bgfx::UniformType::Vec4);

    CreateBuffers(width, height);
    return true;
}

void R_CAS::CreateBuffers(int width, int height)
{
    m_width = width;
    m_height = height;
    uint64_t flags = BGFX_TEXTURE_COMPUTE_WRITE | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
    m_texture = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::RGBA16F, flags);
}

void R_CAS::DeleteBuffers()
{
    if (bgfx::isValid(m_texture)) 
        bgfx::destroy(m_texture);
    m_texture = BGFX_INVALID_HANDLE;
}

void R_CAS::Rescale(int width, int height)
{
    DeleteBuffers();
    CreateBuffers(width, height);
}

void R_CAS::Shutdown()
{
    DeleteBuffers();
    if (bgfx::isValid(m_sInput))
    {
        bgfx::destroy(m_sInput);
        bgfx::destroy(m_uConst0);
        bgfx::destroy(m_uConst1);
        m_sInput = m_uConst0 = m_uConst1 = BGFX_INVALID_HANDLE;
    }
}

void R_CAS::Render(bgfx::ViewId viewId, bgfx::TextureHandle inputTex)
{
    if (r_cas.GetInt() == 0) 
        return;

    float sharpness = r_cas_sharpness.GetFloat();

    varAU4(const0);
    varAU4(const1);

    CasSetup(const0, const1, sharpness, (float)m_width, (float)m_height, (float)m_width, (float)m_height);

    glm::uvec4 const0_u(const0[0], const0[1], const0[2], const0[3]);
    glm::uvec4 const1_u(const1[0], const1[1], const1[2], const1[3]);

    glm::vec4 c0 = glm::uintBitsToFloat(const0_u);
    glm::vec4 c1 = glm::uintBitsToFloat(const1_u);

    bgfx::setUniform(m_uConst0, glm::value_ptr(c0));
    bgfx::setUniform(m_uConst1, glm::value_ptr(c1));

    bgfx::setTexture(0, m_sInput, inputTex);
    bgfx::setImage(1, m_texture, 0, bgfx::Access::Write, bgfx::TextureFormat::RGBA16F);

    bgfx::dispatch(viewId, m_shader.GetProgram(), (m_width + 15) / 16, (m_height + 15) / 16, 1);
}