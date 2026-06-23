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
#include "dds.h"
#include "filesystem.h"
#include "console.h"
#include "cvar.h"
#include <vector>
#include <cstring>
#include <algorithm>
#include <fstream>

namespace DDS
{
    static bool InternalLoadDDS(GLuint textureID, GLenum target, const std::string& path, bool srgb, int& outWidth, int& outHeight, int& outChannels)
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
        uint32_t dataOffset = 148;
        bool compressed = true;

        if (fourCC == 0x30315844)
        {
            uint32_t dxgiFormat = *(uint32_t*)&fileData[128];
            if (dxgiFormat == 70 || dxgiFormat == 71 || dxgiFormat == 72)
            {
                format = srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                blockSize = 8;
            }
            else if (dxgiFormat == 73 || dxgiFormat == 74 || dxgiFormat == 75)
            {
                format = srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                blockSize = 16;
            }
            else if (dxgiFormat == 76 || dxgiFormat == 77 || dxgiFormat == 78)
            {
                format = srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                blockSize = 16;
            }
            else if (dxgiFormat == 98 || dxgiFormat == 99)
            {
                format = srgb ? GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM : GL_COMPRESSED_RGBA_BPTC_UNORM;
                blockSize = 16;
            }
        }
        else if (fourCC == 0x31545844)
        {
            format = srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            blockSize = 8;
            dataOffset = 128;
        }
        else if (fourCC == 0x33545844)
        {
            format = srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            blockSize = 16;
            dataOffset = 128;
        }
        else if (fourCC == 0x35545844)
        {
            format = srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            blockSize = 16;
            dataOffset = 128;
        }
        else if (fourCC == 0)
        {
            format = srgb ? GL_SRGB8 : GL_RGB8;
            compressed = false;
            dataOffset = 128;
        }

        outWidth = width;
        outHeight = height;
        outChannels = 4;

        if (mipMapCount == 0)
            mipMapCount = 1;

        int faceLayer = (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) ? int(target - GL_TEXTURE_CUBE_MAP_POSITIVE_X) : -1;

        if (faceLayer == -1 && target == GL_TEXTURE_2D)
        {
            glTextureStorage2D(textureID, mipMapCount, format, width, height);
            glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, mipMapCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(textureID, GL_TEXTURE_MAX_LEVEL, mipMapCount - 1);
        }

        uint32_t w = width;
        uint32_t h = height;

        for (uint32_t level = 0; level < mipMapCount && (w || h); ++level)
        {
            uint32_t size = compressed ? ((w + 3) / 4) * ((h + 3) / 4) * blockSize : w * h * 3;

            if (faceLayer >= 0)
            {
                if (compressed)
                    glCompressedTextureSubImage3D(textureID, level, 0, 0, faceLayer, w, h, 1, format, size, &fileData[dataOffset]);
                else
                    glTextureSubImage3D(textureID, level, 0, 0, faceLayer, w, h, 1, GL_RGB, GL_UNSIGNED_BYTE, &fileData[dataOffset]);
            }
            else
            {
                if (compressed)
                    glCompressedTextureSubImage2D(textureID, level, 0, 0, w, h, format, size, &fileData[dataOffset]);
                else
                    glTextureSubImage2D(textureID, level, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, &fileData[dataOffset]);
            }

            dataOffset += size;
            w = std::max(1u, w / 2);
            h = std::max(1u, h / 2);
        }

        if (target == GL_TEXTURE_2D)
        {
            if (mipMapCount <= 1)
            {
                glGenerateTextureMipmap(textureID);
                glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

                int maxDim = std::max(width, height);
                int mips = 0;
                while (maxDim > 0) 
                { 
                    maxDim >>= 1; 
                    mips++; 
                }
                glTextureParameteri(textureID, GL_TEXTURE_MAX_LEVEL, mips - 1);
            }
            glTextureParameterf(textureID, GL_TEXTURE_MAX_ANISOTROPY_EXT, CVar::GetFloat("r_textureAnisotropy", 16.0f));
        }

        return true;
    }

    bool Load2D(GLuint textureID, const std::string& path, bool srgb, int& outWidth, int& outHeight, int& outChannels)
    {
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
        return InternalLoadDDS(textureID, GL_TEXTURE_2D, path, srgb, outWidth, outHeight, outChannels);
    }

    bool LoadCubemapFace(GLuint textureID, GLenum target, const std::string& path, bool srgb)
    {
        int w, h, c;
        return InternalLoadDDS(textureID, target, path, srgb, w, h, c);
    }

    bool WriteUncompressedRGB(const std::string& path, int width, int height, const uint8_t* rgbData)
    {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        uint32_t header[32] = { 0 };
        header[0] = 0x20534444;
        header[1] = 124;
        header[2] = 0x100F;
        header[3] = height;
        header[4] = width;
        header[5] = width * 3;
        
        header[19] = 32;
        header[20] = 0x40;
        header[22] = 24;
        header[23] = 0x000000FF;
        header[24] = 0x0000FF00;
        header[25] = 0x00FF0000;
        
        header[27] = 0x1000;

        file.write((char*)header, 128);
        file.write((const char*)rgbData, width * height * 3);
        file.close();

        return true;
    }
}