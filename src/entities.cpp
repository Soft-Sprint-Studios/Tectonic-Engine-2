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
#include "console.h"
#include "concmd.h"
#include "timing.h"
#include "physics.h"
#include "bsploader.h"
#include "filesystem.h"
#include <sstream>
#include <vector>

std::vector<std::shared_ptr<Entity>> EntityManager::s_entities;
std::vector<EntityManager::DelayedInput> EntityManager::s_delayedInputs;

void Entity::Spawn(const std::unordered_map<std::string, std::string>& keyvalues)
{
    m_keyvalues = keyvalues;
    m_targetName = GetValue("targetname");
    m_spawnflags = GetInt("spawnflags", 0);

    for (auto const& [key, val] : keyvalues)
    {
        // outputs have \x1b escape must read to not corrupt outputs
        if (val.find('\x1b') != std::string::npos)
        {
            std::stringstream ss(val);
            std::string segment;
            std::vector<std::string> parts;

            while (std::getline(ss, segment, '\x1b'))
            {
                parts.push_back(segment);
            }

            if (parts.size() >= 2)
            {
                EntityIO io;
                io.targetEntity = parts[0];
                io.input = parts[1];
                io.parameter = (parts.size() > 2) ? parts[2] : "";
                io.delay = (parts.size() > 3 && !parts[3].empty()) ? std::stof(parts[3]) : 0.0f;
                io.timesToFire = (parts.size() > 4 && !parts[4].empty()) ? std::stoi(parts[4]) : -1;

                m_outputs.push_back({ key, io });
            }
        }
    }
}

void Entity::Think(float deltaTime)
{
}

void Entity::AcceptInput(const std::string& inputName, const std::string& parameter)
{
}

void Entity::Touch(Entity* other)
{
}

void Entity::EndTouch(Entity* other)
{
}

void Entity::OnPress(Entity* activator)
{
}

bool Entity::HasSpawnFlag(int bit) const
{
    return (m_spawnflags & bit) != 0;
}

void Entity::SetSpawnFlag(int bit, bool state)
{
    if (state) 
        m_spawnflags |= bit;
    else 
        m_spawnflags &= ~bit;

    m_keyvalues["spawnflags"] = std::to_string(m_spawnflags);
}

void Entity::FireOutput(const std::string& outputName)
{
    for (auto& [name, io] : m_outputs)
    {
        bool match = Filesystem::StringEqual(name, outputName);

        if (match)
        {
            auto target = EntityManager::FindEntityByName(io.targetEntity);
            if (target)
            {
                if (io.delay <= 0.0f)
                {
                    target->AcceptInput(io.input, io.parameter);
                }
                else
                {
                    EntityManager::DelayedInput di;
                    di.target = target;
                    di.input = io.input;
                    di.parameter = io.parameter;
                    di.fireTime = (float)Time::TotalTime() + io.delay;
                    EntityManager::AddDelayedInput(di);
                }
            }
        }
    }
}

glm::vec3 Entity::GetOrigin() const
{
    return m_origin;
}

void Entity::SetOrigin(const glm::vec3& origin)
{
    m_origin = origin;
}

std::string Entity::GetClassName() const
{
    return m_className;
}

std::string Entity::GetTargetName() const
{
    return m_targetName;
}

std::string Entity::GetValue(const std::string& key, const std::string& defaultVal) const
{
    auto it = m_keyvalues.find(key);
    return (it != m_keyvalues.end()) ? it->second : defaultVal;
}

int Entity::GetInt(const std::string& key, int defaultVal) const
{
    auto it = m_keyvalues.find(key);
    if (it == m_keyvalues.end()) 
        return defaultVal;
    return std::stoi(it->second);
}

float Entity::GetFloat(const std::string& key, float defaultVal) const
{
    auto it = m_keyvalues.find(key);
    if (it == m_keyvalues.end()) 
        return defaultVal;
    return std::stof(it->second);
}

glm::vec3 Entity::GetVector(const std::string& key, const glm::vec3& defaultVal) const
{
    auto it = m_keyvalues.find(key);
    if (it == m_keyvalues.end()) 
        return defaultVal;

    glm::vec3 result;
    std::stringstream ss(it->second);
    if (ss >> result.x >> result.y >> result.z)
    {
        return result;
    }
    return defaultVal;
}

void EntityManager::Init()
{
}

void EntityManager::UpdateAll(float deltaTime)
{
    float currentTime = Time::TotalTime();
    for (auto it = s_delayedInputs.begin(); it != s_delayedInputs.end();)
    {
        if (currentTime >= it->fireTime)
        {
            if (it->target)
            {
                it->target->AcceptInput(it->input, it->parameter);
            }
            it = s_delayedInputs.erase(it);
        }
        else
        {
            it++;
        }
    }

    for (auto& ent : s_entities)
    {
        ent->Think(deltaTime);
    }
}

void EntityManager::Shutdown()
{
    s_entities.clear();
    s_delayedInputs.clear();
}

std::shared_ptr<Entity> EntityManager::SpawnEntity(const std::string& className, const BSP::EntityData& entData)
{
    // Do we have worldspawn or lights? Ignore!
    if (className == "worldspawn" || className == "light" || className == "light_spot" || className == "light_environment")
    {
        return nullptr;
    }

    auto& factory = GetFactory();
    auto it = factory.find(className);
    if (it == factory.end())
    {
        Console::Warn("Unknown entity class: " + className);
        return nullptr;
    }

    std::shared_ptr<Entity> ent = it->second();
    ent->m_className = className;

    auto originIt = entData.keyvalues.find("origin");
    if (originIt != entData.keyvalues.end())
    {
        std::stringstream ss(originIt->second);
        glm::vec3 pos;
        ss >> pos.x >> pos.y >> pos.z;
        ent->SetOrigin(pos);
    }

    // If brush entity, create physics object
    if (!entData.brushCollision.vertices.empty())
    {
        ent->m_physObject = Physics::CreateGhostObject(entData.brushCollision, ent->GetOrigin());
        if (ent->m_physObject)
        {
            ent->m_physObject->setUserPointer(ent.get());

            if (!ent->IsCollidable())
            {
                ent->m_physObject->setCollisionFlags(ent->m_physObject->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
            }
            else
            {
                ent->m_physObject->setCollisionFlags(ent->m_physObject->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
            }
        }
    }

    ent->Spawn(entData.keyvalues);
    s_entities.push_back(ent);

    return ent;
}

std::shared_ptr<Entity> EntityManager::FindEntityByClass(const std::string& className)
{
    for (auto& ent : s_entities)
    {
        if (ent->GetClassName() == className)
        {
            return ent;
        }
    }
    return nullptr;
}

std::shared_ptr<Entity> EntityManager::FindEntityByName(const std::string& name)
{
    for (auto& ent : s_entities)
    {
        if (ent->m_targetName == name)
        {
            return ent;
        }
    }
    return nullptr;
}

CON_COMMAND(ent_dump, "Lists all entities and their targetnames")
{
    Console::Log("--- Entity Dump ---");
    for (auto const& ent : EntityManager::GetEntities())
    {
        Console::Log("Class: " + ent->GetClassName() + " | Name: " + ent->GetTargetName());
    }
    Console::Log("-------------------");
}