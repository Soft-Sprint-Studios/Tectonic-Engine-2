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
#include "prop_animation.h"
#include "timing.h"

void PropAnimation::Spawn(const BSP::EntityData& entData)
{
    Entity::Spawn(entData);
    m_modelPath = GetValue("model");
    m_animName = GetValue("defaultanim");
    m_scale = GetFloat("uniformscale", 1.0f);
    m_vecAngles.y += 90.0f;

    m_playing = !m_animName.empty() && !HasSpawnFlag(1);
    m_looping = true;

    if (!m_modelPath.empty())
    {
        size_t dot = m_modelPath.find_last_of(".");
        if (dot != std::string::npos)
        {
            m_modelPath = m_modelPath.substr(0, dot) + ".glb";
        }
    }
}

void PropAnimation::Think(float dt)
{
    if (m_playing)
    {
        m_animTime += dt;
    }
}

void PropAnimation::AcceptInput(const std::string& input, const std::string& param)
{
    Entity::AcceptInput(input, param);
    if (input == "PlayAnimationLoop")
    {
        m_animName = param;
        m_playing = true;
        m_looping = true;
        m_animTime = 0.0f;
    }
    else if (input == "PlayAnimation")
    {
        m_animName = param;
        m_playing = true;
        m_looping = false;
        m_animTime = 0.0f;
    }
    else if (input == "StopAnimation")
    {
        m_playing = false;
        m_animName = "";
    }
}

bool PropAnimation::IsRenderable() const
{
    return true;
}

LINK_ENTITY_TO_CLASS("prop_animation", PropAnimation)