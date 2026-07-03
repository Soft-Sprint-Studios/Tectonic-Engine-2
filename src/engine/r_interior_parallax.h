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
#include <bgfx/bgfx.h>
#include <map>
#include <string>

class R_BSP;

class R_InteriorParallax
{
public:
    void Init();
    void Draw(bgfx::ViewId viewId, const Camera& camera, R_BSP* bsp);
    void Shutdown();

private:
    bgfx::TextureHandle GetCubemap(const std::string& name);
    
    R_Shader m_shader;
    std::map<std::string, bgfx::TextureHandle> m_cubemapCache;

    bgfx::UniformHandle m_sInteriorCube = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uRoomParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uViewPos = BGFX_INVALID_HANDLE;
};