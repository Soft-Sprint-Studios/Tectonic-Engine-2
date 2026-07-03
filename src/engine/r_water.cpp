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
#include "r_water.h"
#include "renderer.h"
#include "timing.h"
#include "waters.h"
#include "resources.h"
#include "cvar.h"
#include <algorithm>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>

CVar r_water_downsample("r_water_downsample", "2", "Downscaling factor for water reflection buffer.", CVAR_SAVE);

void R_Water::Init(int width, int height)
{
    m_width = width;
    m_height = height;

    m_shader.Load("shaders/water.vert", "shaders/water.frag");

    m_sReflectionTexture = bgfx::createUniform("s_reflectionTexture", bgfx::UniformType::Sampler);
    m_sDudvMap = bgfx::createUniform("s_dudvMap", bgfx::UniformType::Sampler);
    m_sNormalMap = bgfx::createUniform("s_normalMap", bgfx::UniformType::Sampler);
    m_sFlowMap = bgfx::createUniform("s_flowMap", bgfx::UniformType::Sampler);
    m_sLightmap = bgfx::createUniform("s_lightmap", bgfx::UniformType::Sampler);
    m_uViewPos = bgfx::createUniform("u_viewPos", bgfx::UniformType::Vec4);
    m_uTime = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);
    m_uFlowParams = bgfx::createUniform("u_flowParams", bgfx::UniformType::Vec4);
    m_uReflectViewProj = bgfx::createUniform("u_reflectViewProj", bgfx::UniformType::Mat4, 2);

    uint8_t whitePixel[] = { 255, 255, 255, 255 };
    m_whiteTexture = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE, bgfx::copy(whitePixel, 4));

    uint8_t normalPixel[] = { 128, 128, 255, 255 };
    m_defaultNormal = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE, bgfx::copy(normalPixel, 4));
}

void R_Water::AddSurface(const WaterSurface& surface)
{
    m_surfaces.push_back(surface);
    m_starts.push_back((int32_t)surface.start);
    m_counts.push_back((uint32_t)surface.count);
}

void R_Water::ClearSurfaces()
{
    m_surfaces.clear();
    m_starts.clear();
    m_counts.clear();
}

void R_Water::RenderReflection(Renderer* renderer, const Camera& mainCam)
{
    if (m_surfaces.empty())
    {
        return;
    }

    if (!bgfx::isValid(m_reflectFBO))
    {
        int ds = std::max(1, CVar::GetInt("r_water_downsample", 2));
        int fboW = m_width / ds;
        int fboH = m_height / ds;

        uint64_t rtFlags = BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
        m_reflectTex = bgfx::createTexture2D((uint16_t)fboW, (uint16_t)fboH, false, 1, bgfx::TextureFormat::RGBA16F, rtFlags);
        m_reflectDepthTex = bgfx::createTexture2D((uint16_t)fboW, (uint16_t)fboH, false, 1, bgfx::TextureFormat::D24S8, rtFlags);

        bgfx::TextureHandle attachments[] = { m_reflectTex, m_reflectDepthTex };
        m_reflectFBO = bgfx::createFrameBuffer(2, attachments, false);
    }

    // Planar reflection algorithm
    float h = m_surfaces[0].height;
    Camera reflectCam = mainCam;
    float dist = 2.0f * (mainCam.position.y - h);
    reflectCam.position.y -= dist;
    reflectCam.pitch = -mainCam.pitch;

    m_reflectView = reflectCam.GetViewMatrix();
    m_reflectProj = reflectCam.GetProjectionMatrix();

    renderer->RenderWorld(reflectCam, 0, false, RenderView::ReflectionGBuffer, RenderView::WaterReflection, m_reflectFBO);
}

void R_Water::Draw(bgfx::ViewId viewId, const Camera& camera, bgfx::VertexBufferHandle bspVbo, bgfx::TextureHandle lightmap)
{
    if (m_surfaces.empty() || !bgfx::isValid(bspVbo))
    {
        return;
    }

    glm::mat4 identity(1.0f);

    for (const auto& s : m_surfaces)
    {
        float vp[4] = { camera.position.x, camera.position.y, camera.position.z, 0.0f };
        bgfx::setUniform(m_uViewPos, vp);

        float timeParam[4] = { (float)Time::TotalTime(), 0.0f, 0.0f, 0.0f };
        bgfx::setUniform(m_uTime, timeParam);

        float mats[32];
        std::memcpy(&mats[0], glm::value_ptr(m_reflectView), 16 * sizeof(float));
        std::memcpy(&mats[16], glm::value_ptr(m_reflectProj), 16 * sizeof(float));
        bgfx::setUniform(m_uReflectViewProj, mats, 2);

        WaterDef* def = Waters::GetDefinition(s.textureName);

        float flowParams[4] = { def->flowSpeed, def->flowMap ? 1.0f : 0.0f, 0.0f, 0.0f };
        bgfx::setUniform(m_uFlowParams, flowParams);

        bgfx::setTexture(0, m_sReflectionTexture, bgfx::isValid(m_reflectTex) ? m_reflectTex : m_whiteTexture);
        bgfx::setTexture(1, m_sDudvMap, def->dudvMap ? def->dudvMap->GetHandle() : m_whiteTexture);
        bgfx::setTexture(2, m_sNormalMap, def->normalMap ? def->normalMap->GetHandle() : m_defaultNormal);
        bgfx::setTexture(3, m_sFlowMap, def->flowMap ? def->flowMap->GetHandle() : m_whiteTexture);
        bgfx::setTexture(4, m_sLightmap, bgfx::isValid(lightmap) ? lightmap : m_whiteTexture);

        bgfx::setTransform(glm::value_ptr(identity));
        bgfx::setVertexBuffer(0, bspVbo, s.start, s.count);

        uint64_t state = BGFX_STATE_WRITE_RGB 
                       | BGFX_STATE_WRITE_A 
                       | BGFX_STATE_WRITE_Z 
                       | BGFX_STATE_DEPTH_TEST_LESS 
                       | BGFX_STATE_CULL_CW;

        bgfx::setState(state);
        bgfx::submit(viewId, m_shader.GetProgram());
    }
}

void R_Water::Rescale(int width, int height)
{
    m_width = width;
    m_height = height;

    if (bgfx::isValid(m_reflectFBO))
    {
        bgfx::destroy(m_reflectFBO);
        m_reflectFBO = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_reflectTex))
    {
        bgfx::destroy(m_reflectTex);
        m_reflectTex = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_reflectDepthTex))
    {
        bgfx::destroy(m_reflectDepthTex);
        m_reflectDepthTex = BGFX_INVALID_HANDLE;
    }
}

void R_Water::Shutdown()
{
    if (bgfx::isValid(m_reflectFBO))
    {
        bgfx::destroy(m_reflectFBO);
        m_reflectFBO = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_reflectTex))
    {
        bgfx::destroy(m_reflectTex);
        m_reflectTex = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_reflectDepthTex))
    {
        bgfx::destroy(m_reflectDepthTex);
        m_reflectDepthTex = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_whiteTexture))
    {
        bgfx::destroy(m_whiteTexture);
        m_whiteTexture = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_defaultNormal))
    {
        bgfx::destroy(m_defaultNormal);
        m_defaultNormal = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_sReflectionTexture))
    {
        bgfx::destroy(m_sReflectionTexture);
        bgfx::destroy(m_sDudvMap);
        bgfx::destroy(m_sNormalMap);
        bgfx::destroy(m_sFlowMap);
        bgfx::destroy(m_sLightmap);
        bgfx::destroy(m_uViewPos);
        bgfx::destroy(m_uTime);
        bgfx::destroy(m_uFlowParams);
        bgfx::destroy(m_uReflectViewProj);

        m_sReflectionTexture = BGFX_INVALID_HANDLE;
    }

    m_surfaces.clear();
    m_starts.clear();
    m_counts.clear();
}