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
#include "player.h"
#include "physics.h"
#include "console.h"
#include "concmd.h"
#include <glm/gtx/string_cast.hpp>

CON_COMMAND(noclip, "Toggles player noclip mode")
{
    auto ent = EntityManager::FindEntityByClass("info_player_start");
    if (ent)
    {
        auto player = std::dynamic_pointer_cast<Player>(ent);
        player->SetNoclip(!player->IsNoclip());
        Console::Log(player->IsNoclip() ? "Noclip ON" : "Noclip OFF");
    }
}

Player::Player()
{
}

Player::~Player()
{
    // Why does this cause a crash when exiting?
    /*
    if (m_character)
    {
        Physics::GetDynamicsWorld()->removeAction(m_character);
        Physics::GetDynamicsWorld()->removeCollisionObject(m_character->getGhostObject());
        delete m_character->getGhostObject()->getCollisionShape();
        delete m_character->getGhostObject();
        delete m_character;
        m_character = nullptr;
    }
    */
}

void Player::LinkCamera(Camera* cam)
{
    m_camera = cam;
}

void Player::LinkInput(Input* input)
{
    m_input = input;
}

void Player::Spawn(const std::unordered_map<std::string, std::string>& keyvalues)
{
    Console::Log("Player::Spawn at " + glm::to_string(m_origin));

    glm::vec3 physicsOrigin = { m_origin.x, m_origin.z, -m_origin.y };
    physicsOrigin *= 0.03125f;
    physicsOrigin.y += 1.0f;

    m_character = Physics::CreateCharacter(physicsOrigin);

    // Force immediate warp to the spawn position to clear penetration state
    btTransform startTrans;
    startTrans.setIdentity();
    startTrans.setOrigin(btVector3(physicsOrigin.x, physicsOrigin.y, physicsOrigin.z));
    m_character->getGhostObject()->setWorldTransform(startTrans);
    m_character->reset(Physics::GetDynamicsWorld());
}

void Player::Think(float deltaTime)
{
    if (!m_camera || !m_input || !m_character)
    {
        return;
    }

    float mouseSensitivity = 0.15f;
    m_camera->yaw += m_input->GetMouseDeltaX() * mouseSensitivity;
    m_camera->pitch -= m_input->GetMouseDeltaY() * mouseSensitivity;

    m_camera->pitch = glm::clamp(m_camera->pitch, -89.0f, 89.0f);

    glm::vec3 forward = m_camera->GetForward();
    glm::vec3 flatForward = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
    glm::vec3 right = glm::normalize(glm::cross(flatForward, glm::vec3(0, 1, 0)));

    glm::vec3 wishDir(0.0f);
    float baseSpeed = 5.0f;

    glm::vec3 moveForward = m_noclip ? forward : flatForward;

    if (m_input->GetKey(SDL_SCANCODE_W)) 
        wishDir += moveForward;
    if (m_input->GetKey(SDL_SCANCODE_S)) 
        wishDir -= moveForward;
    if (m_input->GetKey(SDL_SCANCODE_A)) 
        wishDir -= right;
    if (m_input->GetKey(SDL_SCANCODE_D)) 
        wishDir += right;

    if (glm::length(wishDir) > 0.1f)
    {
        wishDir = glm::normalize(wishDir) * baseSpeed;
    }

    // If we are in noclip then move directly
    if (m_noclip)
    {
        m_camera->position += wishDir * deltaTime;

        btTransform trans;
        trans.setIdentity();
        trans.setOrigin(btVector3(m_camera->position.x, m_camera->position.y - 1.5f, m_camera->position.z));
        m_character->getGhostObject()->setWorldTransform(trans);
        m_character->setWalkDirection(btVector3(0, 0, 0));
    }
    else
    {
        glm::vec3 displacement = wishDir * deltaTime;
        m_character->setWalkDirection(btVector3(displacement.x, displacement.y, displacement.z));

        btTransform currentTransform = m_character->getGhostObject()->getWorldTransform();
        btVector3 bulletPos = currentTransform.getOrigin();
        m_camera->position = glm::vec3(bulletPos.getX(), bulletPos.getY() + 1.5f, bulletPos.getZ());
    }
}

void Player::SetNoclip(bool state)
{
    m_noclip = state;
    if (m_character)
    {
        auto ghost = m_character->getGhostObject();
        if (state)
        {
            ghost->setCollisionFlags(ghost->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
        }
        else
        {
            ghost->setCollisionFlags(ghost->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
        }
    }
}

LINK_ENTITY_TO_CLASS("info_player_start", Player)