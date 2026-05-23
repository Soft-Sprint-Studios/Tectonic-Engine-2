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
#include "sound.h"

class FuncBreakable : public Entity
{
public:
    void Spawn(const BSP::EntityData& entData) override
    {
        Entity::Spawn(entData);
        m_health = GetFloat("health", 1.0f);
        m_breakSound = GetValue("break_sound", "glass_break.mp3");
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(FuncBreakable, m_health, FieldType::Float));
    }

    void TakeDamage(float damage, Entity* attacker) override
    {
        if (!IsEnabled()) 
        {
            return;
        }

        m_health -= damage;

        if (m_health <= 0)
        {
            Break();
        }
        else
        {
            FireOutput("OnDamaged");
        }
    }

    void AcceptInput(const std::string& input, const std::string& param) override
    {
        Entity::AcceptInput(input, param);
        if (input == "Break")
        {
            Break();
        }
    }

    bool IsRenderable() const override
    {
        return IsEnabled();
    }

    bool IsCollidable() const override
    {
        return IsEnabled();
    }

private:
    void Break()
    {
        FireOutput("OnBreak");

        Sound::AudioSource* snd = new Sound::AudioSource();
        snd->SetPosition(GetOrigin());
        snd->Play(m_breakSound);

        SetEnabled(false);
    }

    float m_health = 1.0f;
    std::string m_breakSound;
};

LINK_ENTITY_TO_CLASS("func_breakable", FuncBreakable)