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
#pragma once
#include "camera.h"
#include "r_shader.h"
#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <vector>

class R_Cascade
{
public:
    R_Cascade();
    ~R_Cascade();

    void Init(int resolution = 4096);
    void Shutdown();

    void Render(const Camera& camera, R_Shader& shadowShader, class Renderer* renderer);
    void Bind(R_Shader& shader, const glm::vec3& sunColor, const glm::vec3& sunDir, bool enabled, float sunVolIntensity, int sunVolSteps);

    bgfx::TextureHandle GetTexArray() const 
    { 
        return m_texArray; 
    }

private:
    void UpdateMatrices(const Camera& cam, const glm::vec3& sunDir);
    std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);

    bgfx::FrameBufferHandle m_fbo[4];
    bgfx::TextureHandle m_texArray = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_dummyTex = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uSunMatrices = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uSunSplitsLow = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uSunDirVol = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uSunColorEn = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uCsmParams = BGFX_INVALID_HANDLE;

    int m_resolution;
    std::vector<glm::mat4> m_matrices;
    std::vector<float> m_splits;
};