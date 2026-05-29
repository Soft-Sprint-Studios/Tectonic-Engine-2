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
#include "entities.h"
#include "nodegraph.h"
#include "sound.h"
#include <vector>

enum class NPCState 
{
    Idle,
    Patrol,
    Chasing,
    Combat,
    Dead
};

class NPCBase : public Entity 
{
public:
    NPCBase();
    virtual ~NPCBase();

    virtual void Spawn(const BSP::EntityData& entData) override;
    virtual void Think(float deltaTime) override;
    virtual void AcceptInput(const std::string& inputName, const std::string& parameter) override;
    virtual void TakeDamage(float damage, Entity* attacker) override;
    virtual void OnSave() override;

    void MoveToPosition(const glm::vec3& targetPos);
    void StopMoving();
    bool CanSeeEntity(Entity* target);
    
    bool IsAtDestination() const 
    {
        return m_currentPath.empty();
    }

    float GetHealth() const 
    { 
        return m_health; 
    }

    NPCState GetState() const 
    { 
        return m_state; 
    }

    void SetState(NPCState newState) 
    { 
        m_state = newState; 
    }

protected:
    virtual void OnReachDestination() 
    {
    }

    virtual void OnPathFailed() 
    {
    }

    virtual void OnStateChanged(NPCState oldState, NPCState newState) 
    {
    }

    float m_health = 100.0f;
    float m_moveSpeed = 3.0f;
    float m_yawSpeed = 8.0f;
    float m_viewDistance = 1200.0f * BSP::MAPSCALE;
    float m_fov = 90.0f;

private:
    void UpdateMovement(float dt);
    void FacePosition(const glm::vec3& target, float dt);

    NPCState m_state = NPCState::Idle;

    std::vector<glm::vec3> m_currentPath;
    int m_pathIndex = 0;

    class btKinematicCharacterController* m_character = nullptr;
    NavHull m_hullType = NavHull::Human;

    Sound::AudioSource m_sndStep;
};