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
#include "particles.h"
#include "filesystem.h"
#include "console.h"
#include <sstream>
#include <random>
#include <glm/gtc/matrix_transform.hpp>

namespace Particles
{
    static std::unordered_map<std::string, ParticleDef> s_defs;
    static std::vector<std::shared_ptr<ParticleSystem>> s_systems;

    static float RandF(float min, float max)
    {
        static std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen);
    }

    void Init()
    {
    }

    void LoadDefinitions(const std::string& path)
    {
        std::string content = Filesystem::ReadText(path);
        if (content.empty()) 
            return;

        std::stringstream ss(content);
        std::string token;
        while (ss >> token)
        {
            if (token.front() == '\"')
            {
                ParticleDef d;
                d.name = token.substr(1, token.size() - 2);
                ss >> token; // {

                while (ss >> token && token != "}")
                {
                    if (token == "texture")
                    {
                        ss >> token;
                        d.textureName = token.substr(1, token.size() - 2);
                    }
                    else if (token == "lifetime")
                    {
                        ss >> d.lifetime;
                    }
                    else if (token == "size")
                    {
                        ss >> d.startSize >> d.endSize;
                    }
                    else if (token == "gravity")
                    {
                        ss >> d.gravity;
                    }
                    else if (token == "rate")
                    {
                        ss >> d.rate;
                    }
                    else if (token == "additive")
                    {
                        ss >> d.additive;
                    }
                    else if (token == "speed") 
                    { 
                        ss >> d.minSpeed >> d.maxSpeed; 
                    }
                    else if (token == "spread") 
                    { 
                        ss >> d.spread; 
                    }
                    else if (token == "color")
                    {
                        ss >> d.startColor.r >> d.startColor.g >> d.startColor.b >> d.startColor.a
                            >> d.endColor.r >> d.endColor.g >> d.endColor.b >> d.endColor.a;
                    }
                }
                s_defs[d.name] = d;
            }
        }
    }

    void Update(float dt)
    {
        for (auto it = s_systems.begin(); it != s_systems.end();)
        {
            (*it)->Update(dt);
            if ((*it)->IsDead()) 
                it = s_systems.erase(it);
            else 
                ++it;
        }
    }

    std::shared_ptr<ParticleSystem> CreateSystem(const std::string& name, const glm::vec3& origin)
    {
        if (s_defs.find(name) == s_defs.end()) 
            return nullptr;
        auto s = std::make_shared<ParticleSystem>(s_defs[name], origin);
        s_systems.push_back(s);
        return s;
    }

    const std::vector<std::shared_ptr<ParticleSystem>>& GetActiveSystems()
    {
        return s_systems;
    }

    void Shutdown()
    {
        s_systems.clear();
    }
}

ParticleSystem::ParticleSystem(const ParticleDef& def, const glm::vec3& origin)
    : m_def(def), m_origin(origin), m_spawnAccum(0.0f), m_active(true)
{
}

void ParticleSystem::Update(float dt)
{
    if (m_active)
    {
        m_spawnAccum += dt * m_def.rate;
        while (m_spawnAccum >= 1.0f)
        {
            ParticleInstance p;
            p.pos = m_origin;

            glm::vec3 localDir = glm::vec3(0.0f, 0.0f, -1.0f);

            //Apply random spread
            if (m_def.spread > 0.0f)
            {
                float s = glm::radians(m_def.spread);
                localDir.x += Particles::RandF(-s, s);
                localDir.y += Particles::RandF(-s, s);
                localDir = glm::normalize(localDir);
            }

            glm::vec3 worldDir = m_rotationMatrix * localDir;

            float speed = Particles::RandF(m_def.minSpeed, m_def.maxSpeed);
            p.vel = worldDir * speed;
            p.life = 0.0f;
            p.maxLife = m_def.lifetime;
            p.col = m_def.startColor;
            p.size = m_def.startSize;
            m_particles.push_back(p);
            m_spawnAccum -= 1.0f;
        }
    }

    for (size_t i = 0; i < m_particles.size();)
    {
        ParticleInstance& p = m_particles[i];
        p.life += dt / p.maxLife;

        if (p.life >= 1.0f)
        {
            m_particles[i] = m_particles.back();
            m_particles.pop_back();
            continue;
        }

        p.vel.y -= 9.81f * m_def.gravity * dt;
        p.pos += p.vel * dt;
        p.col = glm::mix(m_def.startColor, m_def.endColor, p.life);
        p.size = glm::mix(m_def.startSize, m_def.endSize, p.life);
        i++;
    }
}

bool ParticleSystem::IsDead() const
{
    return !m_active && m_particles.empty();
}

void ParticleSystem::SetActive(bool active)
{
    m_active = active;
}

void ParticleSystem::SetOrigin(const glm::vec3& pos)
{
    m_origin = pos;
}

void ParticleSystem::SetAngles(const glm::vec3& angles)
{
    glm::mat4 rot = glm::mat4(1.0f);

    rot = glm::rotate(rot, glm::radians(angles.y - 90.0f), glm::vec3(0, 1, 0));
    rot = glm::rotate(rot, glm::radians(angles.x), glm::vec3(1, 0, 0));
    rot = glm::rotate(rot, glm::radians(angles.z), glm::vec3(0, 0, 1));

    m_rotationMatrix = glm::mat3(rot);
}

const ParticleDef& ParticleSystem::GetDef() const
{
    return m_def;
}

const std::vector<ParticleInstance>& ParticleSystem::GetParticles() const
{
    return m_particles;
}