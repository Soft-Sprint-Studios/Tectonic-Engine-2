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
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

struct DecalDef
{
    glm::vec3 position;
    glm::vec3 normal;
    float size = 0.2f;
    std::string textureName;
    float lifetime = -1.0f;
};

class Decal
{
public:
    Decal(const DecalDef& def);
    const DecalDef& GetDef() const;
    glm::mat4 GetModelMatrix() const;
    bool Update(float dt);

private:
    DecalDef m_def;
    float m_elapsed = 0.0f;
};

namespace Decals
{
    void Init();
    void Update(float dt);
    void Clear();
    void Shutdown();

    void CreateDecal(const DecalDef& def);
    const std::vector<std::shared_ptr<Decal>>& GetActiveDecals();
}