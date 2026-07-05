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
#include "dynamic_sky.h"
#include "renderer.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

R_Cascade::R_Cascade()
    : m_resolution(4096)
{
    m_texArray = BGFX_INVALID_HANDLE;
    m_dummyTex = BGFX_INVALID_HANDLE;
    m_uSunMatrices = BGFX_INVALID_HANDLE;
    m_uSunSplitsLow = BGFX_INVALID_HANDLE;
    m_uSunDirVol = BGFX_INVALID_HANDLE;
    m_uSunColorEn = BGFX_INVALID_HANDLE;
    m_uCsmParams = BGFX_INVALID_HANDLE;

    for (int i = 0; i < 4; i++)
    {
        m_fbo[i] = BGFX_INVALID_HANDLE;
    }
}

R_Cascade::~R_Cascade()
{
    Shutdown();
}

void R_Cascade::Init(int res)
{
    m_resolution = res;
    m_splits = { 0.1f, 20.0f, 60.0f, 150.0f, 500.0f };
    m_matrices.assign(4, glm::mat4(1.0f));

    uint64_t rtFlags = BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_COMPARE_LESS;
    m_texArray = bgfx::createTexture2D((uint16_t)res, (uint16_t)res, false, 4, bgfx::TextureFormat::D16, rtFlags);

    for (int i = 0; i < 4; i++)
    {
        bgfx::Attachment at;
        at.init(m_texArray, bgfx::Access::Write, (uint16_t)i, 1, 0, BGFX_RESOLVE_NONE);
        m_fbo[i] = bgfx::createFrameBuffer(1, &at);
    }

    uint16_t dummyData[1] = { 0 };
    m_dummyTex = bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::D16, BGFX_TEXTURE_RT, bgfx::copy(dummyData, 2));

    m_uSunMatrices = bgfx::createUniform("u_csmMatrices", bgfx::UniformType::Mat4, 4);
    m_uSunSplitsLow = bgfx::createUniform("u_csmSplitsLow", bgfx::UniformType::Vec4);
    m_uSunDirVol = bgfx::createUniform("u_sunDirVol", bgfx::UniformType::Vec4);
    m_uSunColorEn = bgfx::createUniform("u_sunColorEn", bgfx::UniformType::Vec4);
    m_uCsmParams = bgfx::createUniform("u_csmSplit4_volSteps", bgfx::UniformType::Vec4);
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
    m_viewMatrices.clear();
    m_projMatrices.clear();
    for (size_t i = 0; i < 4; ++i)
    {
        glm::mat4 proj = glm::perspective(glm::radians(cam.GetFOV()), cam.GetAspectRatio(), m_splits[i], m_splits[i + 1]);
        auto corners = GetFrustumCornersWorldSpace(proj, cam.GetViewMatrix());

        glm::vec3 center = glm::vec3(0.0f);
        for (const auto& v : corners) 
            center += glm::vec3(v);
        center /= 8.0f;

        float radius = 0.0f;
        for (const auto& v : corners) 
            radius = std::max(radius, glm::distance(center, glm::vec3(v)));

        radius = std::ceil(radius * 16.0f) / 16.0f;

        glm::vec3 up = std::abs(sunDir.y) > 0.99f ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
        glm::mat4 lightView = glm::lookAt(center + sunDir * radius, center, up);
        glm::mat4 lightProj = glm::ortho(-radius, radius, -radius, radius, -radius * 6.0f, radius * 6.0f);

        glm::mat4 shadowMatrix = lightProj * lightView;
        glm::vec4 shadowOrigin = shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        shadowOrigin *= (float)m_resolution / 2.0f;

        glm::vec4 roundedOrigin = glm::round(shadowOrigin);
        glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
        roundOffset = roundOffset * (2.0f * radius) / (float)m_resolution;
        roundOffset.z = 0.0f;
        roundOffset.w = 0.0f;

        lightProj[3] += roundOffset;
        m_matrices.push_back(lightProj * lightView);
        m_viewMatrices.push_back(lightView);
        m_projMatrices.push_back(lightProj);
    }
}

void R_Cascade::Render(const Camera& camera, R_Shader& shadowShader, Renderer* renderer)
{
    UpdateMatrices(camera, DynamicSky::GetSettings().sunDir);

    glm::mat4 identity(1.0f);

    Frustum sunFrustum;
    sunFrustum.valid = false;

    for (uint16_t i = 0; i < 4; i++)
    {
        bgfx::ViewId shadowView = RenderView::CSM_0 + i;
        bgfx::setViewClear(shadowView, BGFX_CLEAR_DEPTH, 0, 1.0f, 0);
        bgfx::setViewRect(shadowView, 0, 0, (uint16_t)m_resolution, (uint16_t)m_resolution);
        bgfx::setViewFrameBuffer(shadowView, m_fbo[i]);
        bgfx::setViewTransform(shadowView, glm::value_ptr(m_viewMatrices[i]), glm::value_ptr(m_projMatrices[i]));

        bgfx::setTransform(glm::value_ptr(identity));

        renderer->DrawSceneDepth(shadowView, shadowShader, sunFrustum);
    }
}

void R_Cascade::Bind(R_Shader& shader, const glm::vec3& sunColor, const glm::vec3& sunDir, bool enabled, float sunVolIntensity, int sunVolSteps)
{
    bgfx::setUniform(m_uSunMatrices, glm::value_ptr(m_matrices[0]), 4);

    float splitsLow[4] = { m_splits[0], m_splits[1], m_splits[2], m_splits[3] };
    bgfx::setUniform(m_uSunSplitsLow, splitsLow);

    float dirVol[4] = { sunDir.x, sunDir.y, sunDir.z, sunVolIntensity };
    bgfx::setUniform(m_uSunDirVol, dirVol);

    float colorEn[4] = { sunColor.x, sunColor.y, sunColor.z, enabled ? 1.0f : 0.0f };
    bgfx::setUniform(m_uSunColorEn, colorEn);

    float csmParams[4] = { m_splits[4], (float)sunVolSteps, 0.0f, 0.0f };
    bgfx::setUniform(m_uCsmParams, csmParams);
}

void R_Cascade::Shutdown()
{
    for (int i = 0; i < 4; i++)
    {
        if (bgfx::isValid(m_fbo[i]))
        {
            bgfx::destroy(m_fbo[i]);
            m_fbo[i] = BGFX_INVALID_HANDLE;
        }
    }

    if (bgfx::isValid(m_texArray))
    {
        bgfx::destroy(m_texArray);
        m_texArray = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_dummyTex))
    {
        bgfx::destroy(m_dummyTex);
        m_dummyTex = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_uSunMatrices))
    {
        bgfx::destroy(m_uSunMatrices);
        bgfx::destroy(m_uSunSplitsLow);
        bgfx::destroy(m_uSunDirVol);
        bgfx::destroy(m_uSunColorEn);
        bgfx::destroy(m_uCsmParams);
        m_uSunMatrices = BGFX_INVALID_HANDLE;
    }
}