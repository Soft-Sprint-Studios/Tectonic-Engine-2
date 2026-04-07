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
#include "bsploader.h"
#include "entities.h"
#include "sound.h"
#include "timing.h"
#include "console.h"
#include "postprocess.h"

// ==========================================
// trigger_multiple
// ==========================================
class TriggerMultiple : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);
        m_wait = GetFloat("wait", 0.0f);
        m_nextFireTime = 0.0f;
    }

    void Touch(Entity* other) override
    {
        if (m_disabled) 
            return;

        if (other && other->IsPlayer())
        {
            if (Time::TotalTime() >= m_nextFireTime)
            {
                FireOutput("OnTrigger");

                // If wait is -1 it behaves like a trigger_once
                if (m_wait < 0)
                {
                    m_disabled = true;
                }
                else
                {
                    m_nextFireTime = (float)Time::TotalTime() + m_wait;
                }
            }
        }
    }

    void AcceptInput(const std::string& input, const std::string& param) override
    {
        if (input == "Enable") 
            m_disabled = false;
        if (input == "Disable") 
            m_disabled = true;
        if (input == "Toggle") 
            m_disabled = !m_disabled;
    }

    bool IsVisible() const override 
    { 
        return false; 
    }
    bool IsCollidable() const override 
    { 
        return false; 
    }

protected:
    float m_wait = 0.0f;
    float m_nextFireTime = 0.0f;
    bool m_disabled = false;
};

LINK_ENTITY_TO_CLASS("trigger_multiple", TriggerMultiple)

// ==========================================
// trigger_once
// ==========================================
class TriggerOnce : public TriggerMultiple
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        TriggerMultiple::Spawn(keyvalues);
        m_wait = -1.0f;
    }
};

LINK_ENTITY_TO_CLASS("trigger_once", TriggerOnce)

// ==========================================
// func_button
// ==========================================
class FuncButton : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);

        m_delay = GetFloat("wait", 0.0f);
    }

    void OnPress(Entity* activator) override
    {
        if (Time::TotalTime() < m_nextUseTime) 
            return;

        FireOutput("OnPressed");
        m_nextUseTime = (float)Time::TotalTime() + m_delay;
    }

    bool IsVisible() const override
    {
        return true;
    }
    bool IsCollidable() const override
    {
        return true;
    }

private:
    float m_delay = 0.0f;
    float m_nextUseTime = 0.0f;
};

LINK_ENTITY_TO_CLASS("func_button", FuncButton)

// ==========================================
// env_cubemap_box
// ==========================================
class EnvCubemapBox : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);
    }

    bool IsVisible() const override 
    { 
        return false; 
    }
    bool IsCollidable() const override 
    { 
        return false; 
    }
};
LINK_ENTITY_TO_CLASS("env_cubemap_box", EnvCubemapBox)

// ==========================================
// sound
// ==========================================
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
        }
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
};

LINK_ENTITY_TO_CLASS("sound", SoundEntity)

// ==========================================
// postprocess_controller
// ==========================================
class PostProcessController : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);
        m_vignette = GetFloat("vignette", 0.0f);
        m_chroma = GetFloat("chroma", 0.0f);
        m_grain = GetFloat("grain", 0.0f);
        m_bw = GetFloat("bw", 0.0f);

        if (!HasSpawnFlag(1))
        {
            ApplySettings();
            m_enabled = true;
        }
    }

    void AcceptInput(const std::string& inputName, const std::string& parameter) override
    {
        if (inputName == "Enable")
        {
            ApplySettings();
            m_enabled = true;
        }
        else if (inputName == "Disable")
        {
            PostProcess::SetVignette(0.0f);
            PostProcess::SetChroma(0.0f);
            PostProcess::SetGrain(0.0f);
            PostProcess::SetBW(0.0f);
            m_enabled = false;
        }
        else if (inputName == "Toggle")
        {
            if (m_enabled) 
                AcceptInput("Disable", "");
            else 
                AcceptInput("Enable", "");
        }
        else if (inputName == "SetVignette")
        {
            m_vignette = std::stof(parameter);
            if (m_enabled) 
                PostProcess::SetVignette(m_vignette);
        }
        else if (inputName == "SetChroma")
        {
            m_chroma = std::stof(parameter);
            if (m_enabled) 
                PostProcess::SetChroma(m_chroma);
        }
        else if (inputName == "SetGrain")
        {
            m_grain = std::stof(parameter);
            if (m_enabled) 
                PostProcess::SetGrain(m_grain);
        }
        else if (inputName == "SetBW")
        {
            m_bw = std::stof(parameter);
            if (m_enabled) 
                PostProcess::SetBW(m_bw);
        }
    }

private:
    void ApplySettings()
    {
        PostProcess::SetVignette(m_vignette);
        PostProcess::SetChroma(m_chroma);
        PostProcess::SetGrain(m_grain);
        PostProcess::SetBW(m_bw);
    }

    float m_vignette = 0.0f;
    float m_chroma = 0.0f;
    float m_grain = 0.0f;
    float m_bw = 0.0f;
    bool m_enabled = false;
};

LINK_ENTITY_TO_CLASS("postprocess_controller", PostProcessController)