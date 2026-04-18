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
#include "r_cascade.h"
#include "camera.h"
#include "r_bsp.h"
#include "r_models.h"
#include "renderer.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

R_Cascade::R_Cascade() 
    : m_resolution(4096)
{
}

R_Cascade::~R_Cascade()
{
    Shutdown();
}

void R_Cascade::Init(int res)
{
    m_resolution = res;

    // Cascade ranges
    m_splits = { 0.1f, 20.0f, 60.0f, 150.0f, 500.0f };

    glGenFramebuffers(1, &m_fbo);
    glGenTextures(1, &m_texArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texArray);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, m_resolution, m_resolution, 4, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_texArray, 0);

    glGenBuffers(1, &m_matrixSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_matrixSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::mat4) * 4, nullptr, GL_DYNAMIC_DRAW);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenTextures(1, &m_dummyTex);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_dummyTex);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, 1, 1, 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

std::vector<glm::vec4> R_Cascade::GetFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
{
    const auto inv = glm::inverse(proj * view);
    std::vector<glm::vec4> corners;
    for (unsigned int x = 0; x < 2; ++x)
    {
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int z = 0; z < 2; ++z)
            {
                const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                corners.push_back(pt / pt.w);
            }
        }
    }
    return corners;
}

void R_Cascade::UpdateMatrices(const Camera& cam, const glm::vec3& sunDir)
{
    m_matrices.clear();
    for (size_t i = 0; i < 4; ++i)
    {
        glm::mat4 proj = glm::perspective(glm::radians(cam.GetFOV()), cam.GetAspectRatio(), m_splits[i], m_splits[i + 1]);
        auto corners = GetFrustumCornersWorldSpace(proj, cam.GetViewMatrix());

        glm::vec3 center = glm::vec3(0);
        for (const auto& v : corners)
        {
            center += glm::vec3(v);
        }
        center /= corners.size();

        glm::mat4 lightView = glm::lookAt(center + sunDir, center, glm::vec3(0, 1, 0));

        float minX = 1e10, minY = 1e10, minZ = 1e10;
        float maxX = -1e10, maxY = -1e10, maxZ = -1e10;

        for (const auto& v : corners)
        {
            const auto trf = lightView * v;
            minX = std::min(minX, trf.x);
            maxX = std::max(maxX, trf.x);
            minY = std::min(minY, trf.y);
            maxY = std::max(maxY, trf.y);
            minZ = std::min(minZ, trf.z);
            maxZ = std::max(maxZ, trf.z);
        }

        // Pull back near plane to catch high geometry behind the view
        if (minZ < 0) 
            minZ *= 10.0f;
        else 
            minZ /= 10.0f;

        if (maxZ < 0) 
            maxZ /= 10.0f;
        else 
            maxZ *= 10.0f;

        float worldUnitsPerTexel = (maxX - minX) / (float)m_resolution;
        minX = floor(minX / worldUnitsPerTexel) * worldUnitsPerTexel;
        maxX = floor(maxX / worldUnitsPerTexel) * worldUnitsPerTexel;
        minY = floor(minY / worldUnitsPerTexel) * worldUnitsPerTexel;
        maxY = floor(maxY / worldUnitsPerTexel) * worldUnitsPerTexel;

        m_matrices.push_back(glm::ortho(minX, maxX, minY, maxY, minZ, maxZ) * lightView);
    }
}

void R_Cascade::Render(const Camera& camera, const glm::vec3& sunDir, Shader& shadowShader, R_BSP* bsp, R_Models* models)
{
    UpdateMatrices(camera, sunDir);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_matrixSSBO);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::mat4) * 4, m_matrices.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, m_matrixSSBO);
    
    glViewport(0, 0, m_resolution, m_resolution);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);

    shadowShader.Bind();
    shadowShader.SetMat4("u_model", glm::mat4(1.0f));

    Frustum sunFrustum;
    sunFrustum.valid = false;

    Renderer::DrawSceneDepth(shadowShader, sunFrustum, bsp, models);

    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_Cascade::Bind(Shader& shader, const glm::vec3& sunColor, const glm::vec3& sunDir, bool enabled)
{
    shader.SetInt("u_csmArray", 13);
    glActiveTexture(GL_TEXTURE13);

    if (enabled)
    {
        shader.SetInt("u_csmEnabled", 1);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_texArray);
        shader.SetVec3("u_sunColor", sunColor);
        shader.SetVec3("u_sunDir", sunDir);

        for (int i = 0; i < 4; i++)
        {
            shader.SetMat4("u_csmMatrices[" + std::to_string(i) + "]", m_matrices[i]);
        }

        for (int i = 0; i < 5; i++)
        {
            shader.SetFloat("u_csmSplits[" + std::to_string(i) + "]", m_splits[i]);
        }
    }
    else
    {
        shader.SetInt("u_csmEnabled", 0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_dummyTex);
    }
}

void R_Cascade::Shutdown()
{
    if (m_fbo)
    {
        glDeleteFramebuffers(1, &m_fbo);
    }

    if (m_texArray)
    {
        glDeleteTextures(1, &m_texArray);
    }

    if (m_matrixSSBO)
    {
        glDeleteBuffers(1, &m_matrixSSBO);
    }

    if (m_dummyTex) 
    {
        glDeleteTextures(1, &m_dummyTex);
    }
}