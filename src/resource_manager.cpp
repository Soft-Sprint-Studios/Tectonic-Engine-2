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
#include "resource_manager.h"
#include "console.h"

std::unordered_map<std::string, std::shared_ptr<Texture>> ResourceManager::s_textures;

std::shared_ptr<Texture> ResourceManager::LoadTexture(const std::string& path)
{
    auto it = s_textures.find(path);
    if (it != s_textures.end())
    {
        return it->second;
    }

    auto tex = std::make_shared<Texture>();
    if (tex->Load(path))
    {
        s_textures[path] = tex;
        return tex;
    }

    return nullptr;
}

void ResourceManager::UnloadUnused()
{
    for (auto it = s_textures.begin(); it != s_textures.end();)
    {
        if (it->second.use_count() <= 1)
        {
            it = s_textures.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void ResourceManager::Clear()
{
    s_textures.clear();
}