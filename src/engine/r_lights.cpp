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
#include "r_state.h"
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

    float dummyData[] = { 1.0f, 1.0f };

    glGenTextures(1, &m_SpotShadow);
    glBindTexture(GL_TEXTURE_2D, m_SpotShadow);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, 1, 1, 0, GL_RG, GL_FLOAT, dummyData);

    glGenTextures(1, &m_PointShadow);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_PointShadow);
    for (int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RG32F, 1, 1, 0, GL_RG, GL_FLOAT, dummyData);

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
    }

    static const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_Lights::RenderShadowMaps(Camera& camera, R_BSP* bsp, R_Models* models)
{
    // CSM
    const auto& sky = DynamicSky::GetSettings();
    if (r_shadows.GetInt() > 0 && r_csm.GetInt() > 0 && sky.hasCSM)
    {
        m_cascade->Render(camera, m_shadowCascadeShader, bsp, models);
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

        R_State::SetViewport(0, 0, def.shadowRes, def.shadowRes);
        glBindFramebuffer(GL_FRAMEBUFFER, def.shadowFBO);
        R_State::SetClearColor({ 1.0f, 1.0f, 0.0f, 0.0f });
        R_State::Clear(true, true, false);

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

            Renderer::DrawSceneDepth(m_shadowSpotShader, lightFrustum, bsp, models);
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

            Renderer::DrawSceneDepth(m_shadowPointShader, pointFrustum, bsp, models);
        }

        if (def.isStaticShadow)
        {
            def.shadowRendered = true;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_Lights::Bind(const R_Shader& shader)
{
    // CSM
    const auto& sky = DynamicSky::GetSettings();
    bool csmEnabled = (r_shadows.GetInt() > 0 && r_csm.GetInt() > 0 && sky.hasCSM);
    m_cascade->Bind(const_cast<R_Shader&>(shader), sky.sunColor, sky.sunDir, csmEnabled, sky.sunVolIntensity, sky.sunVolSteps);

    // Then the scene lights
    for (int i = 0; i < 4; ++i)
    {
        glActiveTexture(GL_TEXTURE5 + i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_PointShadow);
        shader.SetInt("u_pointShadowMaps[" + std::to_string(i) + "]", 5 + i);

        glActiveTexture(GL_TEXTURE9 + i);
        glBindTexture(GL_TEXTURE_2D, m_SpotShadow);
        shader.SetInt("u_spotShadowMaps[" + std::to_string(i) + "]", 9 + i);
    }

    const auto& lights = DynamicLights::GetActiveLights();
    int pIdx = 0;
    int sIdx = 0;

    for (const auto& light : lights)
    {
        if (!light->IsActive())
        {
            continue;
        }

        const auto& def = light->GetDef();

        if (def.type == LightType::Point && pIdx < 4)
        {
            std::string b = "u_pointLights[" + std::to_string(pIdx) + "].";
            shader.SetVec3(b + "pos", light->GetPosition());
            shader.SetVec3(b + "color", def.color);
            shader.SetFloat(b + "radius", def.radius);
            shader.SetFloat(b + "volumetricIntensity", def.volumetricIntensity);
            shader.SetInt(b + "volumetricSteps", def.volumetricSteps);

            if (def.castsShadows && def.shadowTex != 0)
            {
                glActiveTexture(GL_TEXTURE5 + pIdx);
                glBindTexture(GL_TEXTURE_CUBE_MAP, def.shadowTex);
            }

            pIdx++;
        }
        else if (def.type == LightType::Spot && sIdx < 4)
        {
            std::string b = "u_spotLights[" + std::to_string(sIdx) + "].";
            shader.SetVec3(b + "pos", light->GetPosition());
            shader.SetVec3(b + "dir", light->GetDirection());
            shader.SetVec3(b + "color", def.color);
            shader.SetFloat(b + "radius", def.radius);
            shader.SetFloat(b + "volumetricIntensity", def.volumetricIntensity);
            shader.SetInt(b + "volumetricSteps", def.volumetricSteps);
            shader.SetFloat(b + "innerAngle", glm::cos(glm::radians(def.innerAngle)));
            shader.SetFloat(b + "outerAngle", glm::cos(glm::radians(def.outerAngle)));
            shader.SetMat4(b + "lightSpaceMatrix", def.lightSpaceMatrix);

            if (def.castsShadows && def.shadowTex != 0)
            {
                glActiveTexture(GL_TEXTURE9 + sIdx);
                glBindTexture(GL_TEXTURE_2D, def.shadowTex);
            }

            sIdx++;
        }
    }

    shader.SetInt("u_numPointLights", pIdx);
    shader.SetInt("u_numSpotLights", sIdx);
}

void R_Lights::Shutdown()
{
    const auto& lights = DynamicLights::GetActiveLights();
    for (auto& l : lights)
    {
        auto& d = const_cast<DynamicLightDef&>(l->GetDef());
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