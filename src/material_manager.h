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
#include "texture.h"
#include <unordered_map>
#include <string>
#include <memory>

class MaterialManager
{
public:
    static void Init();
    static void LoadDefinitions(const std::string& path);
    static std::shared_ptr<Texture> GetTexture(const std::string& name);
    static std::shared_ptr<Texture> GetNormalMap(const std::string& name);
    static std::shared_ptr<Texture> GetSpecularMap(const std::string& name);

private:
    static void CreateFallbackTexture();
    static std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
    static std::unordered_map<std::string, std::shared_ptr<Texture>> m_normals;
    static std::unordered_map<std::string, std::shared_ptr<Texture>> m_speculars;
    static std::shared_ptr<Texture> m_fallback;
};