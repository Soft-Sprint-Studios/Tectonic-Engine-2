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
#include <memory>
#include <vector>

struct SpriteDef
{
    std::string textureName;
    glm::vec2 scale{ 1.0f, 1.0f };
    glm::vec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
    bool cylindrical = false;
};

class Sprite
{
public:
    Sprite(const SpriteDef& def, const glm::vec3& position);

    void SetActive(bool active);
    void SetPosition(const glm::vec3& position);
    void SetColor(const glm::vec4& color);
    void SetScale(const glm::vec2& scale);

    bool IsActive() const;
    SpriteDef& GetDef();
    const glm::vec3& GetPosition() const;

private:
    SpriteDef m_def;
    glm::vec3 m_position;
    bool m_active;
};

namespace Sprites
{
    void Init();
    void Update();
    void Shutdown();
    void Clear();

    std::shared_ptr<Sprite> CreateSprite(const glm::vec3& position, const std::string& texture);
    const std::vector<std::shared_ptr<Sprite>>& GetActiveSprites();
}