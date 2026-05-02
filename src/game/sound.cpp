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

class SoundEntity : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);

        m_soundName = GetValue("message");
        m_volume = GetFloat("health", 10.0f) / 10.0f;
        m_pitch = GetFloat("pitch", 100.0f) / 100.0f;
        float radius = GetFloat("radius", 1250.0f) * BSP::MAPSCALE;

        m_source.SetPosition(m_origin * BSP::MAPSCALE);
        m_source.SetVolume(m_volume);
        m_source.SetPitch(m_pitch);
        m_source.SetRadius(radius * 0.1f, radius);

        m_isLooping = HasSpawnFlag(2);
        m_source.SetLooping(m_isLooping);

        if (!HasSpawnFlag(1))
        {
            m_source.Play(m_soundName, m_isLooping);
            m_isPlaying = true;
        }
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(SoundEntity, m_soundName, FieldType::String));
        AddSaveField(DATA_FIELD(SoundEntity, m_isLooping, FieldType::Bool));
        AddSaveField(DATA_FIELD(SoundEntity, m_volume, FieldType::Float));
        AddSaveField(DATA_FIELD(SoundEntity, m_pitch, FieldType::Float));
        m_isPlaying = m_source.IsPlaying();
        AddSaveField(DATA_FIELD(SoundEntity, m_isPlaying, FieldType::Bool));
    }

    void AcceptInput(const std::string& inputName, const std::string& parameter) override
    {
        if (inputName == "PlaySound")
        {
            m_source.Play(m_soundName, m_isLooping);
        }
        else if (inputName == "StopSound")
        {
            m_source.Stop();
        }
        else if (inputName == "ToggleSound")
        {
            if (m_source.IsPlaying()) 
                m_source.Stop();
            else 
                m_source.Play(m_soundName, m_isLooping);
        }
        else if (inputName == "EnableLoop")
        {
            m_isLooping = true;
            m_source.SetLooping(true);
        }
        else if (inputName == "DisableLoop")
        {
            m_isLooping = false;
            m_source.SetLooping(false);
        }
        else if (inputName == "ToggleLoop")
        {
            m_isLooping = !m_isLooping;
            m_source.SetLooping(m_isLooping);
        }
    }

private:
    Sound::AudioSource m_source;
    std::string m_soundName;
    float m_volume;
    float m_pitch;
    bool m_isLooping = false;
    bool m_isPlaying = false;
};

LINK_ENTITY_TO_CLASS("sound", SoundEntity)