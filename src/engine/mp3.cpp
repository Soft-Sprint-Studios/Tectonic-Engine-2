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
#include "mp3.h"
#include "filesystem.h"

#define MINIMP3_IMPLEMENTATION
#include "minimp3_ex.h"

namespace MP3
{
    bool Load(const std::string& path, AudioData& outData)
    {
        std::vector<uint8_t> fileData = Filesystem::ReadBinary(path);
        if (fileData.empty()) 
        {
            return false;
        }

        mp3dec_ex_t mp3d;
        if (mp3dec_ex_open_buf(&mp3d, fileData.data(), fileData.size(), 0))
        {
            return false;
        }

        outData.pcm.resize(mp3d.samples);
        mp3dec_ex_read(&mp3d, outData.pcm.data(), mp3d.samples);
        
        outData.channels = mp3d.info.channels;
        outData.sampleRate = mp3d.info.hz;

        mp3dec_ex_close(&mp3d);
        return true;
    }
}