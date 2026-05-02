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
#include "postprocess.h"

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
        m_negative = GetFloat("negative", 0.0f);
        m_sepia = GetFloat("sepia", 0.0f);
        m_sharpen = GetFloat("sharpen", 0.0f);
        m_lensDirtStrength = GetFloat("lens_dirt_strength", 0.0f);
        m_lensDirtTexture = GetValue("lens_dirt_texture", "");
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

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(PostProcessController, m_enabled, FieldType::Bool));
        AddSaveField(DATA_FIELD(PostProcessController, m_vignette, FieldType::Float));
        AddSaveField(DATA_FIELD(PostProcessController, m_chroma, FieldType::Float));
        AddSaveField(DATA_FIELD(PostProcessController, m_grain, FieldType::Float));
        AddSaveField(DATA_FIELD(PostProcessController, m_bw, FieldType::Float));
        AddSaveField(DATA_FIELD(PostProcessController, m_negative, FieldType::Float));
        AddSaveField(DATA_FIELD(PostProcessController, m_sepia, FieldType::Float));
        AddSaveField(DATA_FIELD(PostProcessController, m_sharpen, FieldType::Float));
        AddSaveField(DATA_FIELD(PostProcessController, m_lensDirtStrength, FieldType::Float));
        AddSaveField(DATA_FIELD(PostProcessController, m_lensDirtTexture, FieldType::String));
        AddSaveField(DATA_FIELD(PostProcessController, m_fogColor, FieldType::Vec3));
        AddSaveField(DATA_FIELD(PostProcessController, m_fogStart, FieldType::Float));
        AddSaveField(DATA_FIELD(PostProcessController, m_fogEnd, FieldType::Float));
        AddSaveField(DATA_FIELD(PostProcessController, m_fogAffectsSky, FieldType::Bool));
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
            PostProcess::SetSharpen(0.0f);
            PostProcess::SetLensDirt(0.0f, "");
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
        else if (inputName == "SetNegative")
        {
            m_negative = std::stof(parameter);
            if (m_enabled) 
                PostProcess::SetNegative(m_negative);
        }
        else if (inputName == "SetSepia")
        {
            m_sepia = std::stof(parameter);
            if (m_enabled) 
                PostProcess::SetSepia(m_sepia);
        }
        else if (inputName == "SetSharpen")
        {
            m_sharpen = std::stof(parameter);
            if (m_enabled)
                PostProcess::SetSharpen(m_sharpen);
        }
        else if (inputName == "SetLensDirt")
        {
            m_lensDirtStrength = std::stof(parameter);
            if (m_enabled)
                PostProcess::SetLensDirt(m_lensDirtStrength, m_lensDirtTexture);
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
        PostProcess::SetNegative(m_negative);
        PostProcess::SetSepia(m_sepia);
        PostProcess::SetSharpen(m_sharpen);
        PostProcess::SetLensDirt(m_lensDirtStrength, m_lensDirtTexture);
        PostProcess::SetFog(true, m_fogColor, m_fogStart, m_fogEnd, m_fogAffectsSky);
    }

    float m_vignette = 0.0f;
    float m_chroma = 0.0f;
    float m_grain = 0.0f;
    float m_bw = 0.0f;
    float m_negative = 0.0f;
    float m_sepia = 0.0f;
    float m_sharpen = 0.0f;
    float m_lensDirtStrength = 0.0f;
    std::string m_lensDirtTexture;
    glm::vec3 m_fogColor;
    float m_fogStart, m_fogEnd;
    bool m_fogAffectsSky;
    bool m_enabled = false;
};

LINK_ENTITY_TO_CLASS("postprocess_controller", PostProcessController)