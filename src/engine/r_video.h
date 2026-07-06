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
#include <string>
#include <map>
#include <memory>
#include <bgfx/bgfx.h>
#include "r_shader.h"
#include "camera.h"
#include "pl_mpeg.h"

class Video;
class R_BSP;

class R_VideoInstance
{
public:
    R_VideoInstance();
    ~R_VideoInstance();

    bool Load(const std::string& path, bool loop);
    void Update(float dt);
    void BindTextures(bgfx::UniformHandle s_texY, bgfx::UniformHandle s_texCb, bgfx::UniformHandle s_texCr);
    void Shutdown();

private:
    plm_t* m_plm = nullptr;
    bgfx::TextureHandle m_texY = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_texCb = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_texCr = BGFX_INVALID_HANDLE;
    bool m_loop = false;
    double m_time = 0.0;
    double m_nextFrameTime = 0.0;
};

class R_Video
{
public:
    void Init();
    void Draw(bgfx::ViewId viewId, const Camera& camera, R_BSP* bsp);
    void Shutdown();

private:
    R_Shader m_shader;
    std::map<Video*, std::unique_ptr<R_VideoInstance>> m_instances;

    bgfx::UniformHandle m_sTexY = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sTexCb = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sTexCr = BGFX_INVALID_HANDLE;
};