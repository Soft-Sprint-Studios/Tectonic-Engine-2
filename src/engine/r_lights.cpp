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
    m_shadowSpotShader.Load("shaders/shadow_spot.vert", "shaders/shadow_spot.frag");
    m_shadowCascadeShader.Load("shaders/shadow_cascade.vert", "shaders/shadow_cascade.frag", "shaders/shadow_cascade.geom");
    m_shadowPointShader.Load("shaders/shadow_point.vert", "shaders/shadow_point.frag", "shaders/shadow_point.geom");

    m_cascade = std::make_unique<R_Cascade>();
    m_cascade->Init(r_csm_res.GetInt());

    float dummyData[] = { 1e10f, 1e20f };

    glGenTextures(1, &m_SpotShadow);
    glBindTexture(GL_TEXTURE_2D, m_SpotShadow);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, 1, 1, 0, GL_RG, GL_FLOAT, dummyData);

    glGenTextures(1, &m_PointShadow);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_PointShadow);
    for (int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RG32F, 1, 1, 0, GL_RG, GL_FLOAT, dummyData);

    glGenBuffers(1, &m_lightSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_lightSSBO);
    glNamedBufferData(m_lightSSBO, 64 * sizeof(GPULight), NULL, GL_DYNAMIC_DRAW);

    return true;
}

void R_Lights::SetupShadowMap(std::shared_ptr<DynamicLight> light)
{
    auto& def = const_cast<DynamicLightDef&>(light->GetDef());

    if (def.shadowFBO != 0)
    {
        return;
    }

    glGenFramebuffers(1, &def.shadowFBO);
    glGenTextures(1, &def.shadowTex);

    glGenTextures(1, &def.shadowDepthTex);

    if (def.type == LightType::Spot)
    {
        glBindTexture(GL_TEXTURE_2D, def.shadowTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, def.shadowRes, def.shadowRes, 0, GL_RG, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindTexture(GL_TEXTURE_2D, def.shadowDepthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, def.shadowRes, def.shadowRes, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, def.shadowFBO);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, def.shadowTex, 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, def.shadowDepthTex, 0);

        if (def.shadowHandle == 0)
        {
            def.shadowHandle = glGetTextureHandleARB(def.shadowTex);
            glMakeTextureHandleResidentARB(def.shadowHandle);
        }
    }
    else
    {
        glBindTexture(GL_TEXTURE_CUBE_MAP, def.shadowTex);
        for (int i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RG32F, def.shadowRes, def.shadowRes, 0, GL_RG, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_CUBE_MAP, def.shadowDepthTex);
        for (int i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24, def.shadowRes, def.shadowRes, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, def.shadowFBO);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, def.shadowTex, 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, def.shadowDepthTex, 0);

        if (def.shadowHandle == 0)
        {
            def.shadowHandle = glGetTextureHandleARB(def.shadowTex);
            glMakeTextureHandleResidentARB(def.shadowHandle);
        }
    }

    static const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

        glViewport(0, 0, def.shadowRes, def.shadowRes);
        glBindFramebuffer(GL_FRAMEBUFFER, def.shadowFBO);
        glClearColor(1e10f, 1e20f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (def.type == LightType::Spot)
        {
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
        gpu.shadowHandle = d.shadowHandle;

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
    const auto& lights = DynamicLights::GetActiveLights();
    for (auto& l : lights)
    {
        auto& d = const_cast<DynamicLightDef&>(l->GetDef());
        if (d.shadowHandle != 0)
        {
            glMakeTextureHandleNonResidentARB(d.shadowHandle);
            d.shadowHandle = 0;
        }
        if (d.shadowFBO != 0)
        {
            glDeleteFramebuffers(1, &d.shadowFBO);
        }
        if (d.shadowTex != 0)
        {
            glDeleteTextures(1, &d.shadowTex);
        }
        if (d.shadowDepthTex != 0)
        {
            glDeleteTextures(1, &d.shadowDepthTex);
        }
    }

    if (m_SpotShadow != 0)
    {
        glDeleteTextures(1, &m_SpotShadow);
    }

    if (m_PointShadow != 0)
    {
        glDeleteTextures(1, &m_PointShadow);
    }
}