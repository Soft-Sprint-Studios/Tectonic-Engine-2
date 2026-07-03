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
#include "r_shader.h"
#include "camera.h"
#include "beams.h"
#include <bgfx/bgfx.h>
#include <memory>
#include <vector>

class R_Beams
{
public:
    void Init();
    void Draw(bgfx::ViewId viewId, const Camera& camera, const std::vector<std::shared_ptr<Beam>>& beams);
    void Shutdown();

private:
    R_Shader m_shader;
    bgfx::VertexBufferHandle m_vbo = BGFX_INVALID_HANDLE;
    bgfx::VertexLayout m_layout;

    bgfx::UniformHandle m_uBeamParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uStartPosLocal = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uEndPosLocal = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uViewPosLocal = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uBeamColor = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uBeamTime = BGFX_INVALID_HANDLE;
};