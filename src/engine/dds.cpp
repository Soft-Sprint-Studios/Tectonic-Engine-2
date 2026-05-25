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
    static bool InternalLoadDDS(GLenum target, const std::string& path, bool srgb, int& outWidth, int& outHeight, int& outChannels)
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

        uint32_t height      = *(uint32_t*)&fileData[12];
        uint32_t width       = *(uint32_t*)&fileData[16];
        uint32_t mipMapCount = *(uint32_t*)&fileData[28];
        uint32_t fourCC      = *(uint32_t*)&fileData[84];

        uint32_t format = 0;
        uint32_t blockSize = 0;
        uint32_t dataOffset = 128;
        bool compressed = true;

        if (fourCC == 0x30315844) 
        {
            if (fileData.size() < 148)
            {
                Console::Error("Failed to load DDS (missing DX10 header): " + fullPath);
                return false;
            }
            
            uint32_t dxgiFormat = *(uint32_t*)&fileData[128];
            dataOffset = 148;

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
            else 
            {
                Console::Error("Failed to load DDS");
                return false;
            }
        }
        else if (fourCC == 0x31545844) 
        { 
            format = srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            blockSize = 8;
        } 
        else if (fourCC == 0x33545844) 
        { 
            format = srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            blockSize = 16;
        } 
        else if (fourCC == 0x35545844) 
        { 
            format = srgb ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            blockSize = 16;
        } 
        else if (fourCC == 0)
        {
            format = srgb ? GL_SRGB8 : GL_RGB8;
            compressed = false;
        }
        else 
        {
            Console::Error("Failed to load DDS");
            return false; 
        }

        outWidth = width;
        outHeight = height;
        outChannels = 4;
        
        if (mipMapCount == 0) 
        {
            mipMapCount = 1;
        }

        if (target == GL_TEXTURE_2D)
        {
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, mipMapCount > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, mipMapCount - 1);
        }

        uint32_t w = width;
        uint32_t h = height;

        for (uint32_t level = 0; level < mipMapCount && (w || h); ++level) 
        {
            if (w == 0) 
                w = 1;
            if (h == 0) 
                h = 1;
            
            uint32_t size = 0;
            if (compressed)
            {
                size = ((w + 3) / 4) * ((h + 3) / 4) * blockSize;
            }
            else
            {
                size = w * h * 3;
            }
            
            if (dataOffset + size > fileData.size()) 
            {
                if (target == GL_TEXTURE_2D)
                {
                    glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, level > 0 ? level - 1 : 0);
                }
                break; 
            }

            if (compressed)
            {
                glCompressedTexImage2D(target, level, format, w, h, 0, size, &fileData[dataOffset]);
            }
            else
            {
                glTexImage2D(target, level, format, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, &fileData[dataOffset]);
            }

            dataOffset += size;
            w /= 2;
            h /= 2;
        }

        if (target == GL_TEXTURE_2D)
        {
            if (mipMapCount <= 1) 
            {
                glGenerateMipmap(target);
                glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                
                int maxDim = std::max(width, height);
                int mips = 0;

                while (maxDim > 0) 
                { 
                    maxDim >>= 1; 
                    mips++; 
                }

                glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, mips - 1);
            }

            glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, CVar::GetFloat("r_textureAnisotropy", 16.0f));
        }

        return true;
    }

    bool Load2D(GLuint textureID, const std::string& path, bool srgb, int& outWidth, int& outHeight, int& outChannels)
    {
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        bool success = InternalLoadDDS(GL_TEXTURE_2D, path, srgb, outWidth, outHeight, outChannels);
        glBindTexture(GL_TEXTURE_2D, 0);
        return success;
    }

    bool LoadCubemapFace(GLenum target, const std::string& path, bool srgb)
    {
        int w, h, c;
        return InternalLoadDDS(target, path, srgb, w, h, c);
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