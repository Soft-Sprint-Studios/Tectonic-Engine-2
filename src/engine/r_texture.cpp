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
#include "r_texture.h"
#include "filesystem.h"
#include "dds.h"

R_Texture::R_Texture()
{
}

R_Texture::~R_Texture()
{
    if (bgfx::isValid(m_handle))
    {
        bgfx::destroy(m_handle);
    }
}

void R_Texture::Create(int width, int height, unsigned char* data, bool srgb)
{
    if (bgfx::isValid(m_handle))
    {
        bgfx::destroy(m_handle);
    }

    m_width = width;
    m_height = height;

    uint64_t flags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC;

    if (srgb)
    {
        flags |= BGFX_TEXTURE_SRGB;
    }

    const bgfx::Memory* mem = bgfx::copy(data, width * height * 4);

    m_handle = bgfx::createTexture2D(uint16_t(width), uint16_t(height), false, 1, bgfx::TextureFormat::RGBA8, flags, mem);
}

bool R_Texture::Load(const std::string& path, bool srgb)
{
    DDS::ImageInfo info;
    if (!DDS::Load(path, srgb, info))
    {
        return false;
    }

    m_width = info.width;
    m_height = info.height;

    const bgfx::Memory* mem = bgfx::copy(info.data.data(), (uint32_t)info.data.size());
    m_handle = bgfx::createTexture(mem);

    return bgfx::isValid(m_handle);
}