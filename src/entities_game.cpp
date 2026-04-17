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
#include "particles.h"
#include "dynamic_light.h"
#include "lightstyles.h"
#include "sprite.h"
#include "r_sky.h"
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>

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
        m_fogColor = GetVector("fog_color", glm::vec3(128.0f)) / 255.0f;
        m_fogStart = GetFloat("fog_start", 50.0f);
        m_fogEnd = GetFloat("fog_end", 200.0f);
        m_fogAffectsSky = GetInt("fog_affects_sky", 1) != 0;

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
        else if (inputName == "EnableFog")
        {
            if (m_enabled) 
                PostProcess::SetFog(true, m_fogColor, m_fogStart, m_fogEnd, m_fogAffectsSky);
        }
        else if (inputName == "DisableFog")
        {
            if (m_enabled) 
                PostProcess::SetFog(true, m_fogColor, m_fogStart, m_fogEnd, m_fogAffectsSky);
        }
    }

private:
    void ApplySettings()
    {
        PostProcess::SetVignette(m_vignette);
        PostProcess::SetChroma(m_chroma);
        PostProcess::SetGrain(m_grain);
        PostProcess::SetBW(m_bw);
        PostProcess::SetFog(true, m_fogColor, m_fogStart, m_fogEnd, m_fogAffectsSky);
    }

    float m_vignette = 0.0f;
    float m_chroma = 0.0f;
    float m_grain = 0.0f;
    float m_bw = 0.0f;
    glm::vec3 m_fogColor;
    float m_fogStart, m_fogEnd;
    bool m_fogAffectsSky;
    bool m_enabled = false;
};

LINK_ENTITY_TO_CLASS("postprocess_controller", PostProcessController)

class ParticleEntity : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);
        m_effect = GetValue("effect_name");
        m_sys = Particles::CreateSystem(m_effect, m_origin);
        if (m_sys) 
        {
            glm::vec3 angles = GetVector("angles");
            m_sys->SetAngles(angles);
            m_sys->SetActive(!HasSpawnFlag(1));
        }
    }

    void AcceptInput(const std::string& input, const std::string& param) override
    {
        if (!m_sys) 
            return;
        if (input == "Start") 
            m_sys->SetActive(true);
        if (input == "Stop") 
            m_sys->SetActive(false);
    }

private:
    std::string m_effect;
    std::shared_ptr<ParticleSystem> m_sys;
};

LINK_ENTITY_TO_CLASS("particle_system", ParticleEntity)

// ==========================================
// light_dynamic_point
// ==========================================
class LightDynamicPoint : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);

        glm::vec4 colorData = GetVector4("_light", glm::vec4(255.0f, 255.0f, 255.0f, 200.0f));
        m_baseColor = glm::vec3(colorData.x, colorData.y, colorData.z) / 255.0f;
        m_baseColor *= (colorData.w / 255.0f) * 2.0f;

        float radius = GetFloat("distance", 500.0f) * BSP::MAPSCALE;

        m_light = DynamicLights::CreatePointLight(m_origin, m_baseColor, radius);
        if (m_light)
        {
            m_light->SetActive(!HasSpawnFlag(1));
            auto& def = const_cast<DynamicLightDef&>(m_light->GetDef());
            def.castsShadows = HasSpawnFlag(2);
            def.isStaticShadow = HasSpawnFlag(4);
            def.shadowRes = GetInt("shadow_res", 512);
            def.volumetricIntensity = GetFloat("volumetric_intensity", 0.0f);
            def.volumetricSteps = GetInt("volumetric_steps", 32);
        }
    }

    void Think(float deltaTime) override
    {
        Entity::Think(deltaTime);
        if (m_light)
        {
            m_light->SetPosition(m_origin);

            int styleIndex = GetInt("style", 0);
            float mod = LightStyles::GetModifier(styleIndex);

            m_light->GetDef().color = m_baseColor * mod;
        }
    }

    void AcceptInput(const std::string& input, const std::string& param) override
    {
        if (!m_light)
        {
            return;
        }

        if (input == "Enable")
        {
            m_light->SetActive(true);
        }
        else if (input == "Disable")
        {
            m_light->SetActive(false);
        }
        else if (input == "Toggle")
        {
            m_light->SetActive(!m_light->IsActive());
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

protected:
    glm::vec4 GetVector4(const std::string& key, const glm::vec4& defaultVal = glm::vec4(0.0f)) const
    {
        auto it = m_keyvalues.find(key);
        if (it == m_keyvalues.end())
        {
            return defaultVal;
        }
        glm::vec4 result;
        std::stringstream ss(it->second);
        if (ss >> result.x >> result.y >> result.z >> result.w)
        {
            return result;
        }
        return defaultVal;
    }

    std::shared_ptr<DynamicLight> m_light;
    glm::vec3 m_baseColor;
};

LINK_ENTITY_TO_CLASS("light_dynamic_point", LightDynamicPoint)

// ==========================================
// light_dynamic_spot
// ==========================================
class LightDynamicSpot : public LightDynamicPoint
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);

        glm::vec4 colorData = GetVector4("_light", glm::vec4(255.0f, 255.0f, 255.0f, 200.0f));
        m_baseColor = glm::vec3(colorData.x, colorData.y, colorData.z) / 255.0f;
        m_baseColor *= (colorData.w / 255.0f) * 2.0f;

        float radius = GetFloat("distance", 1000.0f) * BSP::MAPSCALE;
        float inner = GetFloat("_inner_cone", 30.0f);
        float outer = GetFloat("_cone", 45.0f);

        glm::vec3 angles = GetVector("angles");
        glm::mat4 rot = glm::mat4(1.0f);

        rot = glm::rotate(rot, glm::radians(angles.y - 90.0f), glm::vec3(0, 1, 0));
        rot = glm::rotate(rot, glm::radians(angles.x), glm::vec3(1, 0, 0));
        rot = glm::rotate(rot, glm::radians(angles.z), glm::vec3(0, 0, 1));

        m_direction = glm::normalize(glm::vec3(rot * glm::vec4(0, 0, -1, 0.0f)));

        m_light = DynamicLights::CreateSpotLight(m_origin, m_direction, m_baseColor, radius, inner, outer);
        if (m_light)
        {
            m_light->SetActive(!HasSpawnFlag(1));
            auto& def = const_cast<DynamicLightDef&>(m_light->GetDef());
            def.castsShadows = HasSpawnFlag(2);
            def.isStaticShadow = HasSpawnFlag(4);
            def.shadowRes = GetInt("shadow_res", 512);
            def.volumetricIntensity = GetFloat("volumetric_intensity", 0.0f);
            def.volumetricSteps = GetInt("volumetric_steps", 32);
        }
    }

    void Think(float deltaTime) override
    {
        Entity::Think(deltaTime);
        if (m_light)
        {
            m_light->SetPosition(m_origin);

            int styleIndex = GetInt("style", 0);
            float mod = LightStyles::GetModifier(styleIndex);

            m_light->GetDef().color = m_baseColor * mod;
        }
    }

private:
    glm::vec3 m_direction;
    glm::vec3 m_baseColor;
};

LINK_ENTITY_TO_CLASS("light_dynamic_spot", LightDynamicSpot)

class SpriteEmitter : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);

        std::string texturePath = GetValue("texture");
        m_sprite = Sprites::CreateSprite(m_origin, texturePath);

        if (m_sprite)
        {
            auto& def = m_sprite->GetDef();
            def.cylindrical = (GetInt("rendermode") == 1);
            def.scale = glm::vec2(GetFloat("scale", 1.0f));

            glm::vec3 color = GetVector("rendercolor", { 255, 255, 255 });
            float alpha = GetFloat("renderamt", 255.0f);
            def.color = glm::vec4(color / 255.0f, alpha / 255.0f);

            m_sprite->SetActive(!HasSpawnFlag(1));
        }
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
};

LINK_ENTITY_TO_CLASS("sprite", SpriteEmitter)

// ==========================================
// dynamic_sky
// ==========================================
class DynamicSky : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);

        R_Sky::s_useDynamic = true;
        glm::vec3 angles = GetVector("angles", { 45, 0, 0 });

        glm::mat4 rot = glm::mat4(1.0f);
        rot = glm::rotate(rot, glm::radians(angles.y), glm::vec3(0, 1, 0));
        rot = glm::rotate(rot, glm::radians(angles.x), glm::vec3(1, 0, 0));

        R_Sky::s_sunDir = glm::normalize(glm::vec3(rot * glm::vec4(0, 0, 1, 0)));
    }

    void AcceptInput(const std::string& input, const std::string& param) override
    {
        if (input == "Enable")
        {
            R_Sky::s_useDynamic = true;
        }

        if (input == "Disable")
        {
            R_Sky::s_useDynamic = false;
        }
    }
};

LINK_ENTITY_TO_CLASS("dynamic_sky", DynamicSky)