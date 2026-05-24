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

void Entity::Spawn(const BSP::EntityData& entData)
{
    m_keyvalues = entData.keyvalues;
    m_targetName = GetValue("targetname");
    m_parentName = GetValue("parentname");
    m_vecAngles = GetVector("angles", { 0, 0, 0 });
    m_spawnflags = GetInt("spawnflags", 0);

    // Start disabled flag
    if (HasSpawnFlag(1))
    {
        m_enabled = false;
    }

    for (auto const& [key, val] : entData.rawKeyValues)
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

// Handle the base entity enable/disable
void Entity::AcceptInput(const std::string& inputName, const std::string& parameter)
{
    if (inputName == "Enable")
    {
        SetEnabled(true);
    }
    else if (inputName == "Disable")
    {
        SetEnabled(false);
    }
    else if (inputName == "Toggle")
    {
        SetEnabled(!m_enabled);
    }
    else if (inputName == "SetParent")
    {
        auto parent = EntityManager::FindEntityByName(parameter);
        if (parent) 
            SetParent(parent.get());
    }
    else if (inputName == "ClearParent")
    {
        SetParent(nullptr);
    }
}

void Entity::Touch(Entity* other)
{
}

void Entity::EndTouch(Entity* other)
{
}

void Entity::Stay(Entity* other)
{
}

void Entity::OnPress(Entity* activator)
{
}

void Entity::TakeDamage(float damage, Entity* attacker)
{
}

void Entity::OnSave()
{
    AddSaveField(DATA_FIELD(Entity, m_enabled, FieldType::Bool));
    AddSaveField(DATA_FIELD(Entity, m_parentName, FieldType::String));
    AddSaveField(DATA_FIELD(Entity, m_vecOrigin, FieldType::Vec3));
    AddSaveField(DATA_FIELD(Entity, m_vecAngles, FieldType::Vec3));
    AddSaveField(DATA_FIELD(Entity, m_bmodelIndex, FieldType::Int32));
}

int Entity::GetBModelIndex() const
{
    return m_bmodelIndex;
}

bool Entity::IsEnabled() const
{
    return m_enabled;
}

void Entity::SetEnabled(bool state)
{
    m_enabled = state;
    UpdatePhysicsState();
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
        bool match = (name.length() == outputName.length()) && std::equal(name.begin(), name.end(), outputName.begin(), [](unsigned char c1, unsigned char c2)
            {
                return std::tolower(c1) == std::tolower(c2);
            });

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

Entity* Entity::GetParent() const 
{ 
    return m_parent; 
}

void Entity::SetParent(Entity* parent)
{
    glm::vec3 worldPos = GetOrigin();
    glm::vec3 worldAng = GetAngles();

    m_parent = parent;

    if (m_parent)
    {
        m_vecOrigin = worldPos - m_parent->GetOrigin();
        m_vecAngles = worldAng - m_parent->GetAngles();
    }
    else
    {
        m_vecOrigin = worldPos;
        m_vecAngles = worldAng;
    }
}

glm::vec3 Entity::GetOrigin() const
{
    if (m_parent)
    {
        return m_parent->GetOrigin() + m_vecOrigin;
    }
    return m_vecOrigin;
}

void Entity::SetOrigin(const glm::vec3& origin)
{
    if (m_parent)
        m_vecOrigin = origin - m_parent->GetOrigin();
    else
        m_vecOrigin = origin;

    UpdatePhysicsState();
}

glm::vec3 Entity::GetAngles() const
{
    if (m_parent)
        return m_parent->GetAngles() + m_vecAngles;
    return m_vecAngles;
}

void Entity::SetAngles(const glm::vec3& angles)
{
    if (m_parent)
        m_vecAngles = angles - m_parent->GetAngles();
    else
        m_vecAngles = angles;
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

glm::vec4 Entity::GetVector4(const std::string& key, const glm::vec4& defaultVal) const
{
    auto it = m_keyvalues.find(key);
    if (it == m_keyvalues.end())
        return defaultVal;

    glm::vec4 result;
    std::stringstream ss(it->second);
    if (ss >> result.x >> result.y >> result.z >> result.w)
    {
        return result;
    }
    return defaultVal;
}

void Entity::UpdatePhysicsState()
{
    if (m_physObject)
    {
        if (IsCollidable())
        {
            m_physObject->setCollisionFlags(m_physObject->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
        }
        else
        {
            m_physObject->setCollisionFlags(m_physObject->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
        }
    }
}

void Entity::UpdatePhysicsTransform()
{
    if (m_physObject)
    {
        btTransform trans;
        trans.setIdentity();
        glm::vec3 worldPos = GetOrigin();
        trans.setOrigin({ worldPos.x, worldPos.y, worldPos.z });

        glm::vec3 ang = GetAngles();
        btQuaternion rot;
        rot.setEuler(glm::radians(ang.y), glm::radians(ang.x), glm::radians(ang.z));
        trans.setRotation(rot);

        m_physObject->setWorldTransform(trans);
    }
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
        if (ent->IsEnabled())
        {
            ent->Think(deltaTime);

            // Handle parenting
            if (ent->GetParent() && ent->GetPhysObject())
            {
                btTransform trans;
                trans.setIdentity();
                glm::vec3 origin = ent->GetOrigin();
                trans.setOrigin({ origin.x, origin.y, origin.z });

                glm::vec3 ang = ent->GetAngles();
                btQuaternion rot;
                rot.setEuler(glm::radians(ang.y), glm::radians(ang.x), glm::radians(ang.z));
                trans.setRotation(rot);

                ent->GetPhysObject()->setWorldTransform(trans);
            }
        }
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
    auto modelIt = entData.keyvalues.find("model");
    if (modelIt != entData.keyvalues.end() && modelIt->second[0] == '*')
    {
        ent->m_bmodelIndex = entData.modelIndex;
    }

    auto originIt = entData.keyvalues.find("origin");
    if (originIt != entData.keyvalues.end())
    {
        std::stringstream ss(originIt->second);
        glm::vec3 rawPos;
        if (ss >> rawPos.x >> rawPos.y >> rawPos.z)
        {
            ent->m_vecOrigin = BSP::ToEngineSpace(rawPos);
        }
    }

    // If brush entity, create physics object
    if (!entData.brushCollision.vertices.empty())
    {
        ent->m_physObject = Physics::CreateGhostObject(entData.brushCollision, ent->GetOrigin());
        if (ent->m_physObject)
        {
            ent->m_physObject->setUserPointer(ent.get());

            ent->UpdatePhysicsState();
        }
    }

    ent->Spawn(entData);
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

void EntityManager::RelinkAllParents(bool recalculateOffsets)
{
    for (auto& ent : s_entities)
    {
        if (!ent->m_parentName.empty())
        {
            auto parent = FindEntityByName(ent->m_parentName);
            if (parent)
            {
                if (recalculateOffsets)
                {
                    ent->SetParent(parent.get());
                }
                else
                {
                    ent->m_parent = parent.get();
                }
            }
        }
        else
        {
            ent->m_parent = nullptr;
        }
    }
}