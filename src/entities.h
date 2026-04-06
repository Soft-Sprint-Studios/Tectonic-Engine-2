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
#include "bsploader.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <glm/glm.hpp>

class btCollisionObject;

struct EntityIO
{
    std::string targetEntity;
    std::string input;
    std::string parameter;
    float delay;
    int timesToFire;
};

class Entity
{
public:
    virtual ~Entity() = default;

    virtual void Spawn(const std::unordered_map<std::string, std::string>& keyvalues);
    virtual void Think(float deltaTime);
    virtual void AcceptInput(const std::string& inputName, const std::string& parameter);

    virtual void Touch(Entity* other) {}
    virtual void EndTouch(Entity* other) {}
    virtual void OnPress(Entity* activator) {}

    void FireOutput(const std::string& outputName);

    glm::vec3 GetOrigin() const;
    void SetOrigin(const glm::vec3& origin);
    std::string GetClassName() const;
    std::string GetTargetName() const;

    std::string GetValue(const std::string& key, const std::string& defaultVal = "") const;
    int GetInt(const std::string& key, int defaultVal = 0) const;
    float GetFloat(const std::string& key, float defaultVal = 0.0f) const;
    glm::vec3 GetVector(const std::string& key, const glm::vec3& defaultVal = glm::vec3(0.0f)) const;

protected:
    glm::vec3 m_origin{ 0.0f, 0.0f, 0.0f };
    std::string m_className;
    std::string m_targetName;
    btCollisionObject* m_physObject = nullptr;

    std::unordered_map<std::string, std::string> m_keyvalues;
    std::vector<std::pair<std::string, EntityIO>> m_outputs;

    friend class EntityManager;
};

class EntityManager
{
public:
    struct DelayedInput
    {
        std::shared_ptr<Entity> target;
        std::string input;
        std::string parameter;
        float fireTime;
    };

    static void Init();
    static void UpdateAll(float deltaTime);
    static void Shutdown();

    static std::shared_ptr<Entity> SpawnEntity(const std::string& className, const BSP::EntityData& entData);

    static std::shared_ptr<Entity> FindEntityByClass(const std::string& className);
    static std::shared_ptr<Entity> FindEntityByName(const std::string& name);

    static void AddDelayedInput(const DelayedInput& di)
    {
        s_delayedInputs.push_back(di);
    }

    static std::unordered_map<std::string, std::function<std::shared_ptr<Entity>()>>& GetFactory()
    {
        static std::unordered_map<std::string, std::function<std::shared_ptr<Entity>()>> s_factory;
        return s_factory;
    }

private:
    static std::vector<std::shared_ptr<Entity>> s_entities;
    static std::vector<DelayedInput> s_delayedInputs;
};

template<typename T>
class EntityRegister
{
public:
    EntityRegister(const std::string& className)
    {
        EntityManager::GetFactory()[className] = []()
            {
                return std::make_shared<T>();
            };
    }
};

#define LINK_ENTITY_TO_CLASS(mapName, className) \
    static EntityRegister<className> _reg_##className(mapName);