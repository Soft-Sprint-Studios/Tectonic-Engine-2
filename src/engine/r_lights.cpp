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
#include "r_lights.h"
#include "dynamic_sky.h"
#include "renderer.h"
#include "cvar.h"
#include <algorithm>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

R_Lights::R_Lights()
{
    m_lightSSBO = BGFX_INVALID_HANDLE;
    m_SpotShadow = BGFX_INVALID_HANDLE;
    m_PointShadow = BGFX_INVALID_HANDLE;
    m_shadowDepthTex = BGFX_INVALID_HANDLE;
    m_PointDepth = BGFX_INVALID_HANDLE;

    m_sSpotShadowMaps = BGFX_INVALID_HANDLE;
    m_sPointShadowMaps = BGFX_INVALID_HANDLE;
    m_uLightParams = BGFX_INVALID_HANDLE;
    m_uShadowParams = BGFX_INVALID_HANDLE;

    for (int i = 0; i < 32; i++)
    {
        m_spotFB[i] = BGFX_INVALID_HANDLE;
        for (int j = 0; j < 6; j++)
        {
            m_pointFB[i][j] = BGFX_INVALID_HANDLE;
        }
    }
}

R_Lights::~R_Lights()
{
    Shutdown();
}

bool R_Lights::Init()
{
    Shutdown();
    m_nextSpotLayer = 0;
    m_nextPointLayer = 0;

    m_shadowSpotShader.Load("shaders/shadow_spot.vert", "shaders/shadow_spot.frag");
    m_shadowCascadeShader.Load("shaders/shadow_cascade.vert", "shaders/shadow_cascade.frag");
    m_shadowPointShader.Load("shaders/shadow_point.vert", "shaders/shadow_point.frag");

    m_cascade = std::make_unique<R_Cascade>();
    m_cascade->Init();

    uint64_t depthRtFlags = BGFX_TEXTURE_RT | BGFX_SAMPLER_U_BORDER | BGFX_SAMPLER_V_BORDER | BGFX_SAMPLER_COMPARE_LESS;
    uint64_t colorRtFlags = BGFX_TEXTURE_RT | BGFX_SAMPLER_U_BORDER | BGFX_SAMPLER_V_BORDER;

    m_shadowDepthTex = bgfx::createTexture2D(256, 256, false, 1, bgfx::TextureFormat::D32F, depthRtFlags);
    m_PointDepth = bgfx::createTextureCube(256, false, 32, bgfx::TextureFormat::D32F, depthRtFlags);

    m_SpotShadow = bgfx::createTexture2D(256, 256, false, 32, bgfx::TextureFormat::RG16F, colorRtFlags);
    m_PointShadow = bgfx::createTextureCube(256, false, 32, bgfx::TextureFormat::RG16F, colorRtFlags);

    for (int i = 0; i < 32; i++)
    {
        bgfx::Attachment at[2];
        at[0].init(m_SpotShadow, bgfx::Access::Write, (uint16_t)i);
        at[1].init(m_shadowDepthTex, bgfx::Access::Write, 0);
        m_spotFB[i] = bgfx::createFrameBuffer(2, at);
    }

    for (int i = 0; i < 32; i++)
    {
        for (int face = 0; face < 6; face++)
        {
            bgfx::Attachment at[2];
            at[0].init(m_PointShadow, bgfx::Access::Write, (uint16_t)(i * 6 + face));
            at[1].init(m_PointDepth, bgfx::Access::Write, (uint16_t)(i * 6 + face));
            m_pointFB[i][face] = bgfx::createFrameBuffer(2, at);
        }
    }

    m_sSpotShadowMaps = bgfx::createUniform("u_spotShadowMaps", bgfx::UniformType::Sampler);
    m_sPointShadowMaps = bgfx::createUniform("u_pointShadowMaps", bgfx::UniformType::Sampler);
    m_uLightParams = bgfx::createUniform("u_lightParams", bgfx::UniformType::Vec4);
    m_uShadowParams = bgfx::createUniform("u_shadowParams", bgfx::UniformType::Vec4);

    m_lightLayout.begin()
        .add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float)
        .end();

    m_lightSSBO = bgfx::createDynamicVertexBuffer(576, m_lightLayout, BGFX_BUFFER_COMPUTE_READ);

    return true;
}

void R_Lights::SetupShadowMap(std::shared_ptr<DynamicLight> light)
{
    auto& def = const_cast<DynamicLightDef&>(light->GetDef());
    if (def.shadowLayer != -1) 
        return;

    if (def.type == LightType::Spot && m_nextSpotLayer < 32)
        def.shadowLayer = m_nextSpotLayer++;
    else if (def.type == LightType::Point && m_nextPointLayer < 32)
        def.shadowLayer = m_nextPointLayer++;
}

void R_Lights::RenderShadowMaps(Camera& camera, Renderer* renderer)
{
    const auto& sky = DynamicSky::GetSettings();
    if (CVar::GetInt("r_shadows") > 0 && CVar::GetInt("r_csm") > 0 && sky.hasCSM)
    {
        m_cascade->Render(camera, m_shadowCascadeShader, renderer);
    }

    const auto& lights = DynamicLights::GetActiveLights();
    bgfx::ViewId shadowViewBase = 10; // 10-210

    for (auto& light : lights)
    {
        auto& def = const_cast<DynamicLightDef&>(light->GetDef());

        if (CVar::GetInt("r_shadows") == 0) 
        {
            def.shadowRendered = false;
            continue;
        }

        if (!light->IsActive() || !def.castsShadows)
        {
            continue;
        }

        if (def.isStaticShadow && def.shadowRendered)
        {
            continue;
        }

        SetupShadowMap(light);
        if (def.shadowLayer == -1) 
            continue;

        if (def.type == LightType::Spot)
        {
            bgfx::ViewId viewId = shadowViewBase++;
            bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xFFFFFFFF, 1.0f, 0);
            bgfx::setViewRect(viewId, 0, 0, 256, 256);
            bgfx::setViewFrameBuffer(viewId, m_spotFB[def.shadowLayer]);

            Camera lightCam(def.outerAngle * 2.0f, 1.0f, 0.1f, def.radius);
            lightCam.position = light->GetPosition();

            glm::vec3 dir = light->GetDirection();
            lightCam.pitch = glm::degrees(asin(dir.y));
            lightCam.yaw = glm::degrees(atan2(dir.z, dir.x));

            def.lightSpaceMatrix = lightCam.GetProjectionMatrix() * lightCam.GetViewMatrix();
            Frustum lightFrustum = lightCam.GetFrustum();

            bgfx::setViewTransform(viewId, nullptr, glm::value_ptr(def.lightSpaceMatrix));

            float shadowParams[4] = { light->GetPosition().x, light->GetPosition().y, light->GetPosition().z, def.radius };
            bgfx::setUniform(m_uShadowParams, shadowParams);

            renderer->DrawSceneDepth(viewId, m_shadowSpotShader, lightFrustum);
        }
        else
        {
            float farP = def.radius;
            glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, farP);
            glm::vec3 pos = light->GetPosition();

            glm::mat4 shadowTransforms[] =
            {
                shadowProj * glm::lookAt(pos, pos + glm::vec3(1,0,0),  glm::vec3(0,-1,0)),
                shadowProj * glm::lookAt(pos, pos + glm::vec3(-1,0,0), glm::vec3(0,-1,0)),
                shadowProj * glm::lookAt(pos, pos + glm::vec3(0,1,0),  glm::vec3(0,0,1)),
                shadowProj * glm::lookAt(pos, pos + glm::vec3(0,-1,0), glm::vec3(0,0,-1)),
                shadowProj * glm::lookAt(pos, pos + glm::vec3(0,0,1),  glm::vec3(0,-1,0)),
                shadowProj * glm::lookAt(pos, pos + glm::vec3(0,0,-1), glm::vec3(0,-1,0))
            };

            for (int face = 0; face < 6; ++face)
            {
                bgfx::ViewId viewId = shadowViewBase++;
                bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xFFFFFFFF, 1.0f, 0);
                bgfx::setViewRect(viewId, 0, 0, 256, 256);
                bgfx::setViewFrameBuffer(viewId, m_pointFB[def.shadowLayer][face]);
                bgfx::setViewTransform(viewId, nullptr, glm::value_ptr(shadowTransforms[face]));

                float shadowParams[4] = { pos.x, pos.y, pos.z, farP };
                bgfx::setUniform(m_uShadowParams, shadowParams);

                Frustum pointFrustum;
                pointFrustum.valid = false;

                renderer->DrawSceneDepth(viewId, m_shadowPointShader, pointFrustum);
            }
        }

        if (def.isStaticShadow)
        {
            def.shadowRendered = true;
        }
    }
}

void R_Lights::Bind(const R_Shader& shader)
{
    const auto& sky = DynamicSky::GetSettings();
    bool csmEnabled = (CVar::GetInt("r_shadows") > 0 && CVar::GetInt("r_csm") > 0 && sky.hasCSM);
    m_cascade->Bind(const_cast<R_Shader&>(shader), sky.sunColor, sky.sunDir, csmEnabled, sky.sunVolIntensity, sky.sunVolSteps);

    std::vector<GPULight> points, spots;

    bgfx::setTexture(14, m_sSpotShadowMaps, m_SpotShadow);
    bgfx::setTexture(15, m_sPointShadowMaps, m_PointShadow);

    for (const auto& light : DynamicLights::GetActiveLights())
    {
        if (!light->IsActive()) 
            continue;

        auto& d = light->GetDef();
        GPULight gpu = {};
        
        gpu.posRadius[0] = light->GetPosition().x;
        gpu.posRadius[1] = light->GetPosition().y;
        gpu.posRadius[2] = light->GetPosition().z;
        gpu.posRadius[3] = d.radius;

        gpu.colorVol[0] = d.color.x;
        gpu.colorVol[1] = d.color.y;
        gpu.colorVol[2] = d.color.z;
        gpu.colorVol[3] = d.volumetricIntensity;

        gpu.dirInner[0] = light->GetDirection().x;
        gpu.dirInner[1] = light->GetDirection().y;
        gpu.dirInner[2] = light->GetDirection().z;
        gpu.dirInner[3] = glm::cos(glm::radians(d.innerAngle));

        gpu.shadowData[0] = glm::cos(glm::radians(d.outerAngle));
        gpu.shadowData[1] = (float)d.volumetricSteps;
        gpu.shadowData[2] = 0.0f;
        gpu.shadowData[3] = 0.0f;

        std::memcpy(gpu.lightSpace, glm::value_ptr(d.lightSpaceMatrix), sizeof(float) * 16);
        gpu.shadowLayer = (float)d.shadowLayer;

        if (d.type == LightType::Point)
            points.push_back(gpu);
        else
            spots.push_back(gpu);
    }

    std::vector<GPULight> allLights(64);
    for (size_t i = 0; i < points.size() && i < 32; ++i)
        allLights[i] = points[i];
    for (size_t i = 0; i < spots.size() && i < 32; ++i)
        allLights[32 + i] = spots[i];

    bgfx::update(m_lightSSBO, 0, bgfx::copy(allLights.data(), (uint32_t)(allLights.size() * sizeof(GPULight))));
    bgfx::setBuffer(10, m_lightSSBO, bgfx::Access::Read);

    float lightParams[4] = { (float)points.size(), (float)spots.size(), 0.0f, 0.0f };
    bgfx::setUniform(m_uLightParams, lightParams);
}

void R_Lights::Shutdown()
{
    if (m_cascade)
    {
        m_cascade->Shutdown();
        m_cascade.reset();
    }

    for (int i = 0; i < 32; i++)
    {
        if (bgfx::isValid(m_spotFB[i]))
        {
            bgfx::destroy(m_spotFB[i]);
            m_spotFB[i] = BGFX_INVALID_HANDLE;
        }
        for (int face = 0; face < 6; face++)
        {
            if (bgfx::isValid(m_pointFB[i][face]))
            {
                bgfx::destroy(m_pointFB[i][face]);
                m_pointFB[i][face] = BGFX_INVALID_HANDLE;
            }
        }
    }

    if (bgfx::isValid(m_SpotShadow))
        bgfx::destroy(m_SpotShadow);
    if (bgfx::isValid(m_PointShadow)) 
        bgfx::destroy(m_PointShadow);
    if (bgfx::isValid(m_shadowDepthTex))
        bgfx::destroy(m_shadowDepthTex);
    if (bgfx::isValid(m_PointDepth))
        bgfx::destroy(m_PointDepth);
    if (bgfx::isValid(m_lightSSBO)) 
        bgfx::destroy(m_lightSSBO);

    m_SpotShadow = m_PointShadow = m_shadowDepthTex = m_PointDepth = BGFX_INVALID_HANDLE;
    m_lightSSBO = BGFX_INVALID_HANDLE;
    m_nextSpotLayer = m_nextPointLayer = 0;
}