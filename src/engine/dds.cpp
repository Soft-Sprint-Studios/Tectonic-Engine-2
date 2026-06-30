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
#include <cstring>
#include <algorithm>
#include <fstream>

namespace DDS
{
    bool Load(const std::string& path, bool srgb, ImageInfo& outInfo)
    {
        std::vector<uint8_t> fileData = Filesystem::ReadBinary(path);
        if (fileData.size() < 128)
        {
            return false;
        }

        if (std::strncmp((const char*)fileData.data(), "DDS ", 4) != 0)
        {
            return false;
        }

        outInfo.height = *(uint32_t*)&fileData[12];
        outInfo.width = *(uint32_t*)&fileData[16];
        uint32_t mipMapCount = std::max(1u, *(uint32_t*)&fileData[28]);
        uint32_t fourCC = *(uint32_t*)&fileData[84];

        uint32_t dataOffset = 128;
        outInfo.compressed = true;
        uint32_t blockSize = 16;

        if (fourCC == 0x30315844)
        {
            uint32_t dxgiFormat = *(uint32_t*)&fileData[128];
            dataOffset = 148;
            if (dxgiFormat == 71)
            {
                outInfo.format = srgb ? Format::BC1_SRGB : Format::BC1;
                blockSize = 8;
            }
            else if (dxgiFormat == 74)
            {
                outInfo.format = srgb ? Format::BC2_SRGB : Format::BC2;
            }
            else if (dxgiFormat == 77)
            {
                outInfo.format = srgb ? Format::BC3_SRGB : Format::BC3;
            }
            else if (dxgiFormat == 98)
            {
                outInfo.format = srgb ? Format::BC7_SRGB : Format::BC7;
            }
        }
        else if (fourCC == 0x31545844)
        {
            outInfo.format = srgb ? Format::BC1_SRGB : Format::BC1;
            blockSize = 8;
        }
        else if (fourCC == 0x33545844)
        {
            outInfo.format = srgb ? Format::BC2_SRGB : Format::BC2;
        }
        else if (fourCC == 0x35545844)
        {
            outInfo.format = srgb ? Format::BC3_SRGB : Format::BC3;
        }
        else
        {
            outInfo.format = srgb ? Format::SRGB8 : Format::RGBA8;
            outInfo.compressed = false;
        }

        uint32_t w = outInfo.width;
        uint32_t h = outInfo.height;
        size_t currentOffset = dataOffset;

        for (uint32_t i = 0; i < mipMapCount; i++)
        {
            uint32_t size = outInfo.compressed ? ((w + 3) / 4) * ((h + 3) / 4) * blockSize : w * h * 4;

            if (currentOffset + size > fileData.size())
            {
                break;
            }

            MipLevel level;
            level.width = w;
            level.height = h;
            level.size = size;
            level.offset = currentOffset;
            outInfo.mips.push_back(level);

            currentOffset += size;
            w = std::max(1u, w / 2);
            h = std::max(1u, h / 2);
        }

        outInfo.data = std::move(fileData);
        return true;
    }

    bool WriteUncompressedRGB(const std::string& path, int width, int height, const uint8_t* rgbData)
    {
        std::ofstream file(std::filesystem::path(path), std::ios::binary);
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
        header[5] = width * 4;

        header[19] = 32;
        header[20] = 0x40;
        header[22] = 32;
        header[23] = 0x000000FF;
        header[24] = 0x0000FF00;
        header[25] = 0x00FF0000;
        header[26] = 0xFF000000;
        header[27] = 0x1000;

        file.write((char*)header, 128);
        file.write((const char*)rgbData, width * height * 4);
        return true;
    }
}