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
#include "decals.h"
#include "materials.h"
#include "bsploader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

Decal::Decal(const DecalDef& def) : m_def(def)
{
}

const DecalDef& Decal::GetDef() const
{
    return m_def;
}

glm::mat4 Decal::GetModelMatrix() const
{
    glm::vec3 N = glm::normalize(m_def.normal);

    glm::vec3 up = (std::abs(N.y) < 0.95f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);

    glm::vec3 T = glm::normalize(glm::cross(up, N));
    glm::vec3 B = glm::cross(N, T);

    glm::mat4 rotation(1.0f);
    rotation[0] = glm::vec4(T, 0.0f);
    rotation[1] = glm::vec4(B, 0.0f);
    rotation[2] = glm::vec4(N, 0.0f);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), m_def.position);
    model *= rotation;
    model = glm::scale(model, glm::vec3(m_def.size));

    return model;
}

bool Decal::Update(float dt)
{
    if (m_def.lifetime < 0.0f)
    {
        return true;
    }

    m_elapsed += dt;
    return m_elapsed < m_def.lifetime;
}

namespace Decals
{
    static std::vector<std::shared_ptr<Decal>> s_decals;

    void Init()
    {
    }

    void Update(float dt)
    {
        for (auto it = s_decals.begin(); it != s_decals.end();)
        {
            if (!(*it)->Update(dt))
            {
                it = s_decals.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void Clear()
    {
        s_decals.clear();
    }

    void Shutdown()
    {
        Clear();
    }

    void CreateDecal(const DecalDef& def)
    {
        DecalDef finalizedDef = def;

        if (finalizedDef.size < 0.0f)
        {
            auto tex = Materials::GetTexture(finalizedDef.textureName);
            if (tex)
            {
                float maxDimension = static_cast<float>(std::max(tex->GetWidth(), tex->GetHeight()));
                finalizedDef.size = maxDimension * BSP::MAPSCALE;
            }
            else
            {
                finalizedDef.size = 1.0f;
            }
        }

        s_decals.push_back(std::make_shared<Decal>(finalizedDef));
    }

    const std::vector<std::shared_ptr<Decal>>& GetActiveDecals()
    {
        return s_decals;
    }
}