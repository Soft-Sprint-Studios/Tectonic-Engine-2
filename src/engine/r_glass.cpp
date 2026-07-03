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
#include <resources.h>
#include "r_glass.h"
#include "camera.h"
#include "renderer.h"
#include "entities.h"
#include "r_bsp.h"
#include <glm/gtc/type_ptr.hpp>

void R_Glass::Init(int width, int height)
{
    m_shader.Load("shaders/glass.vert", "shaders/glass.frag");

    m_sRefractTex = bgfx::createUniform("s_refractTex", bgfx::UniformType::Sampler);
    m_sNormalMap = bgfx::createUniform("s_normalMap", bgfx::UniformType::Sampler);
    m_uGlassParams = bgfx::createUniform("u_glassParams", bgfx::UniformType::Vec4);

    Rescale(width, height);
}

void R_Glass::Rescale(int width, int height)
{
    m_width = width;
    m_height = height;

    if (bgfx::isValid(m_refractTex))
    {
        bgfx::destroy(m_refractTex);
    }

    uint64_t flags = BGFX_TEXTURE_BLIT_DST | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
    m_refractTex = bgfx::createTexture2D((uint16_t)width, (uint16_t)height, false, 1, bgfx::TextureFormat::RGBA16F, flags);
}

void R_Glass::CaptureScreen(bgfx::ViewId viewId, bgfx::TextureHandle sceneTex)
{
    bgfx::blit(viewId, m_refractTex, 0, 0, sceneTex);
}

void R_Glass::Draw(bgfx::ViewId viewId, const Camera& camera, R_BSP* bsp)
{
    uint64_t state = BGFX_STATE_WRITE_RGB 
                   | BGFX_STATE_WRITE_A 
                   | BGFX_STATE_DEPTH_TEST_LESS 
                   | BGFX_STATE_BLEND_ALPHA 
                   | BGFX_STATE_CULL_CW;

    for (auto& ent : EntityManager::GetEntities())
    {
        if (ent->GetClassName() == "func_glass" && ent->IsEnabled())
        {
            std::string normName = ent->GetValue("normal", "water_normal.dds");
            auto normTex = Resources::LoadTexture("textures/" + normName, false);
            
            if (normTex)
            {
                bgfx::setTexture(0, m_sNormalMap, normTex->GetHandle());
            }

            bgfx::setTexture(10, m_sRefractTex, m_refractTex);

            float params[4] = { ent->GetFloat("refractamount", 0.1f), 0.0f, 0.0f, 0.0f };
            bgfx::setUniform(m_uGlassParams, params);

            glm::mat4 model = glm::translate(glm::mat4(1.0f), ent->GetOrigin());
            
            bsp->DrawBModel(ent->GetBModelIndex(), m_shader, viewId, model, camera.position, true);
        }
    }
}

void R_Glass::Shutdown()
{
    if (bgfx::isValid(m_refractTex))
    {
        bgfx::destroy(m_refractTex);
        m_refractTex = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_sRefractTex))
    {
        bgfx::destroy(m_sRefractTex);
        bgfx::destroy(m_sNormalMap);
        bgfx::destroy(m_uGlassParams);
        m_sRefractTex = m_sNormalMap = m_uGlassParams = BGFX_INVALID_HANDLE;
    }
}