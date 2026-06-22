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
#include "dynamic_light.h"
#include "dynamic_sky.h"
#include "r_bsp.h"
#include "r_models.h"
#include "r_sky.h"
#include "renderer.h"
#include "cvar.h"
#include <string>
#include <glm/gtc/matrix_transform.hpp>

CVar r_shadows("r_shadows", "1", "Global toggle for dynamic shadows.", CVAR_SAVE);
CVar r_csm("r_csm", "1", "Enable Cascaded Shadow Maps for the sun.", CVAR_SAVE);
CVar r_csm_res("r_csm_res", "4096", "Resolution of the CSM shadow map array.", CVAR_SAVE);

R_Lights::R_Lights() : m_SpotShadow(0), m_PointShadow(0)
{
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
    m_shadowCascadeShader.Load("shaders/shadow_cascade.vert", "shaders/shadow_cascade.frag", "shaders/shadow_cascade.geom");
    m_shadowPointShader.Load("shaders/shadow_point.vert", "shaders/shadow_point.frag", "shaders/shadow_point.geom");

    m_cascade = std::make_unique<R_Cascade>();
    m_cascade->Init(r_csm_res.GetInt());

    glCreateFramebuffers(1, &m_shadowFBO);

    glCreateTextures(GL_TEXTURE_2D, 1, &m_shadowDepthTex);
    glTextureStorage2D(m_shadowDepthTex, 1, GL_DEPTH_COMPONENT24, 256, 256);

    glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &m_PointDepth);
    glTextureStorage3D(m_PointDepth, 1, GL_DEPTH_COMPONENT24, 256, 256, 32 * 6);

    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_SpotShadow);
    glTextureStorage3D(m_SpotShadow, 1, GL_RG16F, 256, 256, 32);

    const GLfloat spotClear[2] = { 1e10f, 1e20f };
    glClearTexImage(m_SpotShadow, 0, GL_RG, GL_FLOAT, spotClear);
    glTextureParameteri(m_SpotShadow, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_SpotShadow, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glCreateTextures(GL_TEXTURE_CUBE_MAP_ARRAY, 1, &m_PointShadow);
    glTextureStorage3D(m_PointShadow, 1, GL_RG16F, 256, 256, 32 * 6);

    const GLfloat pointClear[2] = { 1e10f, 1e20f };
    glClearTexImage(m_PointShadow, 0, GL_RG, GL_FLOAT, pointClear);
    glTextureParameteri(m_PointShadow, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_PointShadow, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glCreateBuffers(1, &m_lightSSBO);
    glNamedBufferData(m_lightSSBO, 64 * sizeof(GPULight), NULL, GL_DYNAMIC_DRAW);

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
    // CSM
    const auto& sky = DynamicSky::GetSettings();
    if (r_shadows.GetInt() > 0 && r_csm.GetInt() > 0 && sky.hasCSM)
    {
        m_cascade->Render(camera, m_shadowCascadeShader, renderer);
    }

    const auto& lights = DynamicLights::GetActiveLights();

    for (auto& light : lights)
    {
        auto& def = const_cast<DynamicLightDef&>(light->GetDef());

        if (r_shadows.GetInt() == 0) 
        {
            def.shadowRendered = false;
            continue;
        }

        if (!light->IsActive() || !def.castsShadows)
        {
            continue;
        }

        // If we have static shadowmaps dont draw anymore
        if (def.isStaticShadow && def.shadowRendered)
        {
            continue;
        }

        SetupShadowMap(light);
        if (def.shadowLayer == -1) 
            continue;

        glViewport(0, 0, 256, 256);
        glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);

        if (def.type == LightType::Spot)
        {
            glNamedFramebufferTextureLayer(m_shadowFBO, GL_COLOR_ATTACHMENT0, m_SpotShadow, 0, def.shadowLayer);
            glNamedFramebufferTexture(m_shadowFBO, GL_DEPTH_ATTACHMENT, m_shadowDepthTex, 0);
            glClearColor(1e10f, 1e20f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            m_shadowSpotShader.Bind();

            // Create temporary camera for the light
            Camera lightCam(def.outerAngle * 2.0f, 1.0f, 0.1f, def.radius);
            lightCam.position = light->GetPosition();

            glm::vec3 dir = light->GetDirection();
            lightCam.pitch = glm::degrees(asin(dir.y));
            lightCam.yaw = glm::degrees(atan2(dir.z, dir.x));

            const_cast<DynamicLightDef&>(def).lightSpaceMatrix = lightCam.GetProjectionMatrix() * lightCam.GetViewMatrix();
            Frustum lightFrustum = lightCam.GetFrustum();

            m_shadowSpotShader.SetMat4("u_lightSpaceMatrix", def.lightSpaceMatrix);
            m_shadowSpotShader.SetMat4("u_model", glm::mat4(1.0f));
            m_shadowSpotShader.SetVec3("u_lightPos", light->GetPosition());
            m_shadowSpotShader.SetFloat("u_farPlane", def.radius);

            renderer->DrawSceneDepth(m_shadowSpotShader, lightFrustum);
        }
        else
        {
            for (int face = 0; face < 6; ++face) 
            {
                glNamedFramebufferTextureLayer(m_shadowFBO, GL_COLOR_ATTACHMENT0, m_PointShadow, 0, def.shadowLayer * 6 + face);
                glNamedFramebufferTextureLayer(m_shadowFBO, GL_DEPTH_ATTACHMENT, m_PointDepth, 0, def.shadowLayer * 6 + face);
                glClearColor(1e10f, 1e20f, 0.0f, 0.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }

            glNamedFramebufferTexture(m_shadowFBO, GL_COLOR_ATTACHMENT0, m_PointShadow, 0);
            glNamedFramebufferTexture(m_shadowFBO, GL_DEPTH_ATTACHMENT, m_PointDepth, 0);

            m_shadowPointShader.Bind();

            float farP = def.radius;
            glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, farP);
            glm::vec3 pos = light->GetPosition();

            // Must render in cubemap 6 faces
            glm::mat4 shadowTransforms[] =
            {
                shadowProj * glm::lookAt(pos, pos + glm::vec3(1,0,0),  glm::vec3(0,-1,0)),
                shadowProj * glm::lookAt(pos, pos + glm::vec3(-1,0,0), glm::vec3(0,-1,0)),
                shadowProj * glm::lookAt(pos, pos + glm::vec3(0,1,0),  glm::vec3(0,0,1)),
                shadowProj * glm::lookAt(pos, pos + glm::vec3(0,-1,0), glm::vec3(0,0,-1)),
                shadowProj * glm::lookAt(pos, pos + glm::vec3(0,0,1),  glm::vec3(0,-1,0)),
                shadowProj * glm::lookAt(pos, pos + glm::vec3(0,0,-1), glm::vec3(0,-1,0))
            };

            for (int i = 0; i < 6; ++i)
            {
                m_shadowPointShader.SetMat4("u_shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
            }

            m_shadowPointShader.SetInt("u_shadowLayer", def.shadowLayer);
            m_shadowPointShader.SetFloat("u_farPlane", farP);
            m_shadowPointShader.SetVec3("u_lightPos", pos);

            // Point lights render 6 face so no need for frustum
            Frustum pointFrustum;
            pointFrustum.valid = false;

            m_shadowPointShader.SetMat4("u_model", glm::mat4(1.0f));

            renderer->DrawSceneDepth(m_shadowPointShader, pointFrustum);
        }

        if (def.isStaticShadow)
        {
            def.shadowRendered = true;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void R_Lights::Bind(const R_Shader& shader)
{
    // CSM
    const auto& sky = DynamicSky::GetSettings();
    bool csmEnabled = (r_shadows.GetInt() > 0 && r_csm.GetInt() > 0 && sky.hasCSM);
    m_cascade->Bind(const_cast<R_Shader&>(shader), sky.sunColor, sky.sunDir, csmEnabled, sky.sunVolIntensity, sky.sunVolSteps);

    // Then the scene dynamic lights
    std::vector<GPULight> points, spots;

    glBindTextureUnit(14, m_SpotShadow);
    glBindTextureUnit(15, m_PointShadow);

    for (const auto& light : DynamicLights::GetActiveLights())
    {
        if (!light->IsActive()) 
            continue;

        auto& d = light->GetDef();
        GPULight gpu = {};
        gpu.posRadius = glm::vec4(light->GetPosition(), d.radius);
        gpu.colorVol = glm::vec4(d.color, d.volumetricIntensity);
        gpu.dirInner = glm::vec4(light->GetDirection(), glm::cos(glm::radians(d.innerAngle)));
        gpu.shadowData = glm::vec4(glm::cos(glm::radians(d.outerAngle)), (float)d.volumetricSteps, 0, 0);
        gpu.lightSpace = d.lightSpaceMatrix;
        gpu.shadowLayer = (float)d.shadowLayer;

        if (d.type == LightType::Point)
            points.push_back(gpu);
        else
            spots.push_back(gpu);
    }

    if (!points.empty())
    {
        size_t count = std::min(points.size(), (size_t)32);
        glNamedBufferSubData(m_lightSSBO, 0, count * sizeof(GPULight), points.data());
    }
    if (!spots.empty())
    {
        size_t count = std::min(spots.size(), (size_t)32);
        glNamedBufferSubData(m_lightSSBO, 32 * sizeof(GPULight), count * sizeof(GPULight), spots.data());
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, m_lightSSBO);

    shader.SetInt("u_numPointLights", (int)points.size());
    shader.SetInt("u_numSpotLights", (int)spots.size());
}

void R_Lights::Shutdown()
{
    if (m_shadowFBO != 0)
        glDeleteFramebuffers(1, &m_shadowFBO);
    if (m_SpotShadow != 0) 
        glDeleteTextures(1, &m_SpotShadow);
    if (m_PointShadow != 0) 
        glDeleteTextures(1, &m_PointShadow);
    if (m_shadowDepthTex != 0) 
        glDeleteTextures(1, &m_shadowDepthTex);
    if (m_PointDepth != 0) 
        glDeleteTextures(1, &m_PointDepth);
    m_shadowFBO = m_SpotShadow = m_PointShadow = m_shadowDepthTex = m_PointDepth = 0;
    m_nextSpotLayer = m_nextPointLayer = 0;
}