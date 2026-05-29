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
#include "npc_base.h"
#include "physics.h"
#include "timing.h"
#include "console.h"
#include <btBulletDynamicsCommon.h>

NPCBase::NPCBase() : m_state(NPCState::Idle) 
{
}

NPCBase::~NPCBase() 
{
    if (m_character) 
    {
        Physics::GetDynamicsWorld()->removeAction(m_character);
        Physics::GetDynamicsWorld()->removeCollisionObject(m_character->getGhostObject());
        m_character = nullptr;
    }
}

void NPCBase::Spawn(const BSP::EntityData& entData) 
{
    Entity::Spawn(entData);

    m_health = GetFloat("health", 100.0f);
    m_moveSpeed = GetFloat("speed", 150.0f) * BSP::MAPSCALE;

    m_character = Physics::CreateCharacter(GetOrigin(), this);
    
    UpdatePhysicsState();
}

void NPCBase::Think(float deltaTime) 
{
    if (m_state == NPCState::Dead || !IsEnabled()) 
    {
        return;
    }

    UpdateMovement(deltaTime);
}

void NPCBase::MoveToPosition(const glm::vec3& targetPos) 
{
    auto graph = Nodegraph::Get();
    m_currentPath = graph->FindPath(GetOrigin(), targetPos, m_hullType);

    if (m_currentPath.empty()) 
    {
        OnPathFailed();
        m_pathIndex = 0;
    } 
    else 
    {
        m_pathIndex = 0;
    }
}

void NPCBase::StopMoving() 
{
    m_currentPath.clear();
    m_pathIndex = 0;
    if (m_character) 
    {
        m_character->setWalkDirection(btVector3(0, 0, 0));
    }
}

void NPCBase::UpdateMovement(float dt) 
{
    if (m_currentPath.empty() || m_pathIndex >= m_currentPath.size()) 
    {
        return;
    }

    glm::vec3 currentPos = GetOrigin();
    glm::vec3 targetNode = m_currentPath[m_pathIndex];

    float distToNode = glm::distance(glm::vec2(currentPos.x, currentPos.z), glm::vec2(targetNode.x, targetNode.z));
    
    if (distToNode < 0.5f) 
    {
        m_pathIndex++;
        if (m_pathIndex >= m_currentPath.size()) 
        {
            StopMoving();
            OnReachDestination();
            return;
        }
        targetNode = m_currentPath[m_pathIndex];
    }

    glm::vec3 moveDir = glm::normalize(targetNode - currentPos);
    moveDir.y = 0; 

    FacePosition(targetNode, dt);

    float physicsStep = 1.0f / 60.0f;
    glm::vec3 walkDisplacement = moveDir * m_moveSpeed * physicsStep;
    m_character->setWalkDirection(btVector3(walkDisplacement.x, 0, walkDisplacement.z));

    btTransform trans = m_character->getGhostObject()->getWorldTransform();
    btVector3 bulletPos = trans.getOrigin();
    m_vecOrigin = glm::vec3(bulletPos.x(), bulletPos.y(), bulletPos.z());
}

void NPCBase::FacePosition(const glm::vec3& target, float dt) 
{
    glm::vec3 dir = glm::normalize(target - GetOrigin());
    float targetYaw = glm::degrees(atan2(dir.x, -dir.z)); 
    
    glm::vec3 angles = GetAngles();
    float newYaw = glm::mix(angles.y, targetYaw, m_yawSpeed * dt);
    SetAngles(glm::vec3(angles.x, newYaw, angles.z));
}

bool NPCBase::CanSeeEntity(Entity* target) 
{
    if (!target) 
    {
        return false;
    }

    glm::vec3 eyePos = GetOrigin() + glm::vec3(0, 1.5f, 0); 
    glm::vec3 targetPos = target->GetOrigin() + glm::vec3(0, 1.0f, 0);

    float dist = glm::distance(eyePos, targetPos);
    if (dist > m_viewDistance) 
    {
        return false;
    }

    glm::vec3 toTarget = glm::normalize(targetPos - eyePos);
    glm::vec3 forward;
    float yawRad = glm::radians(GetAngles().y + 90.0f);
    forward.x = cos(yawRad);
    forward.y = 0;
    forward.z = -sin(yawRad);

    float dot = glm::dot(forward, toTarget);
    if (dot < cos(glm::radians(m_fov * 0.5f))) 
    {
        return false;
    }

    btVector3 rayFrom(eyePos.x, eyePos.y, eyePos.z);
    btVector3 rayTo(targetPos.x, targetPos.y, targetPos.z);

    btCollisionWorld::ClosestRayResultCallback rayCallback(rayFrom, rayTo);
    rayCallback.m_collisionFilterGroup = Physics::COL_PLAYER;
    rayCallback.m_collisionFilterMask = Physics::COL_WORLD;

    Physics::GetDynamicsWorld()->rayTest(rayFrom, rayTo, rayCallback);

    if (rayCallback.hasHit())
    {
        return false;
    }

    return true; 
}

void NPCBase::TakeDamage(float damage, Entity* attacker) 
{
    if (m_state == NPCState::Dead) 
    {
        return;
    }

    m_health -= damage;
    if (m_health <= 0) 
    {
        m_health = 0;
        m_state = NPCState::Dead;
        FireOutput("OnDeath");

        m_character->getGhostObject()->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
        SetEnabled(false); 
    }
}

void NPCBase::AcceptInput(const std::string& input, const std::string& param) 
{
    Entity::AcceptInput(input, param);

    if (input == "SetSpeed") 
    {
        m_moveSpeed = std::stof(param) * BSP::MAPSCALE;
    }
}

void NPCBase::OnSave() 
{
    Entity::OnSave();
    AddSaveField(DATA_FIELD(NPCBase, m_health, FieldType::Float));
    AddSaveField(DATA_FIELD(NPCBase, m_state, FieldType::Int32));
}