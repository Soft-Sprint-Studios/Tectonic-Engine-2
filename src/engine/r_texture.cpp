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
#include "cvar.h"
#include "r_texture.h"
#include "filesystem.h"
#include "console.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

CVar r_textureAnisotropy("r_textureAnisotropy", "16.0", "Maximum anisotropic filtering level.", CVAR_SAVE);

R_Texture::R_Texture()
{
    m_id = 0;
    m_width = 0;
    m_height = 0;
    m_channels = 0;
}

R_Texture::~R_Texture()
{
    Release();
}

bool R_Texture::Load(const std::string& path, bool srgb)
{
    if (path.length() >= 4)
    {
        std::string ext = path.substr(path.length() - 4);
        for (char& c : ext) 
            c = std::tolower(c);

        if (ext == ".dds")
        {
            return LoadDDS(path, srgb);
        }
    }

    std::string fullPath = Filesystem::GetFullPath(path);
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(fullPath.c_str(), &m_width, &m_height, &m_channels, 4);

    if (!data)
    {
        Console::Error("Failed to load texture: " + fullPath);
        return false;
    }

    Create(m_width, m_height, data, srgb);
    stbi_image_free(data);

    return true;
}

bool R_Texture::LoadDDS(const std::string& path, bool srgb)
{
    std::string fullPath = Filesystem::GetFullPath(path);
    std::vector<uint8_t> fileData = Filesystem::ReadBinary(path);
    if (fileData.size() < 128)
    {
        Console::Error("Failed to load DDS (too small): " + fullPath);
        return false;
    }

    if (std::strncmp((const char*)fileData.data(), "DDS ", 4) != 0)
    {
        Console::Error("Failed to load DDS (invalid magic): " + fullPath);
        return false;
    }

    uint32_t height = *(uint32_t*)&fileData[12];
    uint32_t width = *(uint32_t*)&fileData[16];
    uint32_t mipMapCount = *(uint32_t*)&fileData[28];
    uint32_t fourCC = *(uint32_t*)&fileData[84];

    uint32_t format = 0;
    uint32_t blockSize = 0;
    uint32_t dataOffset = 128;

    if (fourCC == 0x30315844) // 'DX10'
    {
        if (fileData.size() < 148)
        {
            Console::Error("Failed to load DDS (missing DX10 header): " + fullPath);
            return false;
        }

        uint32_t dxgiFormat = *(uint32_t*)&fileData[128];
        dataOffset = 148;

        if (dxgiFormat == 70 || dxgiFormat == 71 || dxgiFormat == 72) // DXT1
        {
            format = srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            blockSize = 8;
        }
        else if (dxgiFormat == 73 || dxgiFormat == 74 || dxgiFormat == 75) // DXT3
        {
            format = srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            blockSize = 16;
        }
        else if (dxgiFormat == 76 || dxgiFormat == 77 || dxgiFormat == 78) // DXT5
        {
            format = srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            blockSize = 16;
        }
        else
        {
            Console::Error("Failed to load DDS");
            return false;
        }
    }
    else if (fourCC == 0x31545844) // DXT1
    {
        format = srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        blockSize = 8;
    }
    else if (fourCC == 0x33545844) // DXT3
    {
        format = srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        blockSize = 16;
    }
    else if (fourCC == 0x35545844) // DXT5
    {
        format = srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        blockSize = 16;
    }
    else
    {
        Console::Error("Failed to load DDS");
        return false;
    }

    m_width = width;
    m_height = height;
    m_channels = 4;

    if (m_id != 0)
    {
        glDeleteTextures(1, &m_id);
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (mipMapCount == 0)
    {
        mipMapCount = 1;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipMapCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapCount - 1);

    uint32_t w = width;
    uint32_t h = height;

    for (uint32_t level = 0; level < mipMapCount && (w || h); ++level)
    {
        if (w == 0)
        {
            w = 1;
        }

        if (h == 0)
        {
            h = 1;
        }

        uint32_t size = ((w + 3) / 4) * ((h + 3) / 4) * blockSize;

        if (dataOffset + size > fileData.size())
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, level > 0 ? level - 1 : 0);
            break;
        }

        glCompressedTexImage2D(GL_TEXTURE_2D, level, format, w, h, 0, size, &fileData[dataOffset]);
        dataOffset += size;
        w /= 2;
        h /= 2;
    }

    if (mipMapCount <= 1)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        int maxDim = std::max(width, height);
        int mips = 0;

        while (maxDim > 0)
        {
            maxDim >>= 1;
            mips++;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mips - 1);
    }

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_textureAnisotropy.GetFloat());
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void R_Texture::Create(int width, int height, unsigned char* data, bool srgb)
{
    m_width = width;
    m_height = height;

    if (m_id != 0)
    {
        glDeleteTextures(1, &m_id);
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLint internalFormat = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_textureAnisotropy.GetFloat());

    glBindTexture(GL_TEXTURE_2D, 0);
}

void R_Texture::Bind(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void R_Texture::Release()
{
    if (m_id != 0)
    {
        glDeleteTextures(1, &m_id);
        m_id = 0;
    }
}