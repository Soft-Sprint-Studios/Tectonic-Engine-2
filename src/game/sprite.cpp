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
#include "entities.h"
#include "sprite.h"

class SpriteEmitter : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);

        m_texturePath = GetValue("texture");
        m_sprite = Sprites::CreateSprite(m_origin, m_texturePath);

        if (m_sprite)
        {
            auto& def = m_sprite->GetDef();
            def.cylindrical = (GetInt("rendermode") == 1);
            def.scale = glm::vec2(GetFloat("scale", 1.0f));

            glm::vec3 color = GetVector("rendercolor", { 255, 255, 255 });
            float alpha = GetFloat("renderamt", 255.0f);
            def.color = glm::vec4(color / 255.0f, alpha / 255.0f);

            m_isVisible = !HasSpawnFlag(1);
            m_sprite->SetActive(m_isVisible);
        }
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(SpriteEmitter, m_texturePath, FieldType::String));
        m_isVisible = m_sprite->IsActive();
        AddSaveField(DATA_FIELD(SpriteEmitter, m_isVisible, FieldType::Bool));
    }

    void AcceptInput(const std::string& input, const std::string& param) override
    {
        if (!m_sprite) 
            return;
        if (input == "ShowSprite") 
            m_sprite->SetActive(true);
        if (input == "HideSprite") 
            m_sprite->SetActive(false);
        if (input == "ToggleSprite") 
            m_sprite->SetActive(!m_sprite->IsActive());
    }

private:
    std::shared_ptr<Sprite> m_sprite;
    std::string m_texturePath;
    bool m_isVisible = true;
};

LINK_ENTITY_TO_CLASS("sprite", SpriteEmitter)