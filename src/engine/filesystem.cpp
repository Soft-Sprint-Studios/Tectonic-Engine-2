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
#include "filesystem.h"
#include "console.h"
#include <SDL3/SDL.h>
#include <fstream>
#include <filesystem>
#include <set>
#include <cstring>
#include "miniz.h"

namespace Filesystem
{
    static std::string s_basePath;
    static std::vector<mz_zip_archive*> s_archives;

    void Init()
    {
        if (const char* base = SDL_GetBasePath())
        {
            s_basePath = base;
            SDL_free(const_cast<char*>(base));
        }

        // Scan base directory for any .zip files
        std::filesystem::path rootPath = std::filesystem::u8path(s_basePath);

        if (std::filesystem::exists(rootPath))
        {
            for (const auto& entry : std::filesystem::directory_iterator(rootPath))
            {
                std::string ext = entry.path().extension().string();
                if (ext == ".zip")
                {
                    mz_zip_archive* zip = new mz_zip_archive();
                    std::memset(zip, 0, sizeof(mz_zip_archive));

                    if (mz_zip_reader_init_file(zip, entry.path().string().c_str(), 0))
                    {
                        s_archives.push_back(zip);
                        Console::Log("Filesystem: Mounted archive " + entry.path().filename().string());
                    }
                    else
                    {
                        Console::Error("Filesystem: Failed to mount archive " + entry.path().filename().string());
                        delete zip;
                    }
                }
            }
        }
    }

    void Shutdown()
    {
        for (auto* zip : s_archives)
        {
            mz_zip_reader_end(zip);
            delete zip;
        }
        s_archives.clear();
    }

    std::string GetFullPath(const std::string& relativePath)
    {
        return s_basePath + relativePath;
    }

    std::vector<uint8_t> ReadBinary(const std::string& path)
    {
        // Try to extract from loaded ZIP archives first
        for (auto* zip : s_archives)
        {
            int fileIndex = mz_zip_reader_locate_file(zip, path.c_str(), nullptr, 0);
            if (fileIndex >= 0)
            {
                size_t size = 0;
                void* p = mz_zip_reader_extract_file_to_heap(zip, path.c_str(), &size, 0);
                if (p)
                {
                    std::vector<uint8_t> buffer((uint8_t*)p, (uint8_t*)p + size);
                    mz_free(p);
                    return buffer;
                }
            }
        }

        // Fallback to loose files on disk
        std::string fullPath = GetFullPath(path);
        std::ifstream file(std::filesystem::u8path(fullPath), std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            Console::Error("Filesystem: Failed to find file: " + path);
            return {};
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<uint8_t> buffer(fileSize);
        file.seekg(0);
        file.read((char*)buffer.data(), fileSize);

        return buffer;
    }

    std::string ReadText(const std::string& path)
    {
        std::vector<uint8_t> data = ReadBinary(path);
        if (data.empty())
        {
            return "";
        }
        return std::string(data.begin(), data.end());
    }

    bool Exists(const std::string& relativePath)
    {
        for (auto* zip : s_archives)
        {
            if (mz_zip_reader_locate_file(zip, relativePath.c_str(), nullptr, 0) >= 0)
            {
                return true;
            }
        }

        return std::filesystem::exists(std::filesystem::u8path(GetFullPath(relativePath)));
    }

    void CreateDirectory(const std::string& relativePath)
    {
        std::filesystem::create_directories(GetFullPath(relativePath));
    }

    std::vector<std::string> ListFiles(const std::string& relativePath, const std::string& extension)
    {
        std::set<std::string> uniqueResults;

        // Search mounted archives
        for (auto* zip : s_archives)
        {
            int numFiles = mz_zip_reader_get_num_files(zip);
            for (int i = 0; i < numFiles; i++)
            {
                mz_zip_archive_file_stat file_stat;
                if (mz_zip_reader_file_stat(zip, i, &file_stat))
                {
                    std::string fname = file_stat.m_filename;

                    if (fname.find(relativePath) == 0)
                    {
                        if (fname.size() >= extension.size() && fname.compare(fname.size() - extension.size(), extension.size(), extension) == 0)
                        {
                            std::filesystem::path p = std::filesystem::u8path(fname);
                            uniqueResults.insert(p.stem().string());
                        }
                    }
                }
            }
        }

        // Search loose files on disk
        std::filesystem::path fullPath = std::filesystem::u8path(GetFullPath(relativePath));
        if (std::filesystem::exists(fullPath))
        {
            for (const auto& entry : std::filesystem::directory_iterator(fullPath))
            {
                if (entry.path().extension() == extension)
                {
                    uniqueResults.insert(entry.path().stem().string());
                }
            }
        }

        return std::vector<std::string>(uniqueResults.begin(), uniqueResults.end());
    }
}