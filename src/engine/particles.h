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
#include <vector>
#include <memory>
#include <unordered_map>

struct ParticleDef
{
    std::string name;
    std::string textureName;
    float lifetime;
    float startSize, endSize;
    glm::vec4 startColor, endColor;
    float minSpeed = 1.0f, maxSpeed = 2.0f;
    float spread = 0.0f;
    float gravity;
    float rate;
    bool additive;
};

struct ParticleInstance
{
    glm::vec3 pos;
    glm::vec3 vel;
    glm::vec4 col;
    float size;
    float life;
    float maxLife;
};

class ParticleSystem
{
public:
    ParticleSystem(const ParticleDef& def, const glm::vec3& origin);
    void Update(float dt);
    
    bool IsDead() const;
    void SetActive(bool active);
    void SetOrigin(const glm::vec3& pos);
    void SetAngles(const glm::vec3& angles);

    const ParticleDef& GetDef() const;
    const std::vector<ParticleInstance>& GetParticles() const;

private:
    ParticleDef m_def;
    glm::vec3 m_origin;
    glm::mat3 m_rotationMatrix = glm::mat3(1.0f);
    std::vector<ParticleInstance> m_particles;
    float m_spawnAccum;
    bool m_active;
};

namespace Particles
{
    void Init();
    void LoadDefinitions(const std::string& path);
    void Update(float dt);
    void Shutdown();

    std::shared_ptr<ParticleSystem> CreateSystem(const std::string& name, const glm::vec3& origin);
    const std::vector<std::shared_ptr<ParticleSystem>>& GetActiveSystems();
}