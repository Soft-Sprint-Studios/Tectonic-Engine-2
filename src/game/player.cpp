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
#include "physics.h"
#include "player.h"
#include "physics.h"
#include "console.h"
#include "cvar.h"
#include "concmd.h"
#include "dynamic_light.h"
#include <glm/gtx/string_cast.hpp>

CVar sensitivity("sensitivity", "1.0", "Mouse sensitivity multiplier.", CVAR_SAVE);
CVar g_walk_speed("g_walk_speed", "3.64", "Player walking speed.", CVAR_SAVE);
CVar g_crouch_speed("g_crouch_speed", "1.82", "Player crouching speed.", CVAR_SAVE);
CVar g_noclip_speed("g_noclip_speed", "4.0", "Speed while in noclip mode.", CVAR_SAVE);
CVar g_sprint_speed("g_sprint_speed", "5.0", "Player sprinting speed.", CVAR_SAVE);
CVar g_jump_force("g_jump_force", "5.0", "Initial upward velocity of a jump.", CVAR_SAVE);
CVar g_view_height("g_view_height", "1.5", "Standing eye level height.", CVAR_SAVE);
CVar g_crouch_height("g_crouch_height", "0.7", "Crouching eye level height.", CVAR_SAVE);
CVar g_view_interp("g_view_interp", "12.0", "Speed of view height interpolation.", CVAR_SAVE);
CVar g_viewbob("g_viewbob", "1", "Enable view bobbing movement.", CVAR_SAVE);
CVar g_viewbob_scale("g_viewbob_scale", "1.0", "Strength of the view bobbing effect.", CVAR_SAVE);

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
    if (m_character)
    {
        Physics::GetDynamicsWorld()->removeAction(m_character);
        Physics::GetDynamicsWorld()->removeCollisionObject(m_character->getGhostObject());
        m_character = nullptr;
    }
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
    m_character = Physics::CreateCharacter(m_origin, this);

    // Force immediate warp to the spawn position to clear penetration state
    btTransform startTrans;
    startTrans.setIdentity();
    startTrans.setOrigin(btVector3(m_origin.x, m_origin.y, m_origin.z));
    m_character->getGhostObject()->setWorldTransform(startTrans);
    m_character->reset(Physics::GetDynamicsWorld());

    // Initialize flashlight
    m_flashlight = DynamicLights::CreateSpotLight(m_origin, glm::vec3(0, 0, 1), glm::vec3(1.0f, 1.0f, 0.9f), 4.0f, 15.0f, 30.0f);
    if (m_flashlight)
    {
        m_flashlight->SetActive(false);
        m_flashlight->GetDef().castsShadows = false;
    }
}

void Player::Think(float deltaTime)
{
    if (!m_camera || !m_input || !m_character)
    {
        return;
    }

    if (m_saveYaw != 0.0f || m_savePitch != 0.0f)
    {
        m_camera->yaw = m_saveYaw;
        m_camera->pitch = m_savePitch;
        m_saveYaw = 0.0f;
        m_savePitch = 0.0f;
    }

    float mouseSensitivity = 0.15f * sensitivity.GetFloat();
    m_camera->yaw += m_input->GetMouseDeltaX() * mouseSensitivity;
    m_camera->pitch -= m_input->GetMouseDeltaY() * mouseSensitivity;

    m_camera->pitch = glm::clamp(m_camera->pitch, -89.0f, 89.0f);

    if (m_input->GetKeyDown(SDL_SCANCODE_F) && m_flashlight)
    {
        m_flashlightOn = !m_flashlightOn;
        m_flashlight->SetActive(m_flashlightOn);
    }

    if (m_flashlightOn && m_flashlight) 
    {
        m_flashlight->SetPosition(m_camera->position);

        float lerpSpeed = 15.0f;
        m_smoothedFlashlightDir = glm::mix(m_smoothedFlashlightDir, m_camera->GetForward(), deltaTime * lerpSpeed);

        m_flashlight->SetDirection(normalize(m_smoothedFlashlightDir));
    }

    if (m_input->GetKeyDown(SDL_SCANCODE_SPACE) && m_character->onGround() && !m_noclip)
    {
        m_character->jump(btVector3(0, g_jump_force.GetFloat(), 0));
    }

    bool wantCrouch = m_input->GetKey(SDL_SCANCODE_LCTRL);
    if (wantCrouch && !m_isCrouching)
    {
        m_isCrouching = true;
        m_character->getGhostObject()->getCollisionShape()->setLocalScaling(btVector3(1, 0.5f, 1));
    }
    else if (!wantCrouch && m_isCrouching)
    {
        m_isCrouching = false;
        m_character->getGhostObject()->getCollisionShape()->setLocalScaling(btVector3(1, 1.0f, 1));
    }

    // OnPress api
    if (m_input->GetKeyDown(SDL_SCANCODE_E))
    {
        btVector3 rayFrom(m_camera->position.x, m_camera->position.y, m_camera->position.z);
        glm::vec3 forward = m_camera->GetForward() * 2.0f;
        btVector3 rayTo(rayFrom.x() + forward.x, rayFrom.y() + forward.y, rayFrom.z() + forward.z);

        btCollisionWorld::ClosestRayResultCallback rayCallback(rayFrom, rayTo);
        rayCallback.m_collisionFilterGroup = Physics::CollisionGroups::COL_PLAYER;
        rayCallback.m_collisionFilterMask = Physics::CollisionGroups::COL_WORLD;
        Physics::GetDynamicsWorld()->rayTest(rayFrom, rayTo, rayCallback);

        if (rayCallback.hasHit())
        {
            if (rayCallback.m_collisionObject->getUserPointer())
            {
                Entity* hitEnt = static_cast<Entity*>(rayCallback.m_collisionObject->getUserPointer());
                hitEnt->OnPress(this);
            }
        }
    }

    glm::vec3 forward = m_camera->GetForward();
    glm::vec3 flatForward = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
    glm::vec3 right = glm::normalize(glm::cross(flatForward, glm::vec3(0, 1, 0)));

    glm::vec3 wishDir(0.0f);
    bool isSprinting = m_input->GetKey(SDL_SCANCODE_LSHIFT) && !m_isCrouching;
    float baseSpeed = isSprinting ? g_sprint_speed.GetFloat() : g_walk_speed.GetFloat();

    if (m_isCrouching && !m_noclip)
    {
        baseSpeed = g_crouch_speed.GetFloat();
    }

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
        float noclipSpeed = g_noclip_speed.GetFloat();
        if (isSprinting) 
            noclipSpeed *= 2.0f;

        m_camera->position += wishDir * noclipSpeed * deltaTime;

        btTransform trans;
        trans.setIdentity();
        trans.setOrigin(btVector3(m_camera->position.x, m_camera->position.y - 1.5f, m_camera->position.z));
        m_character->getGhostObject()->setWorldTransform(trans);
        m_character->setWalkDirection(btVector3(0, 0, 0));
    }
    else
    {
        float physicsStep = 1.0f / 60.0f;
        glm::vec3 walkDisplacement = wishDir * physicsStep;
        m_character->setWalkDirection(btVector3(walkDisplacement.x, 0.0f, walkDisplacement.z));

        float targetHeight = m_isCrouching ? g_crouch_height.GetFloat() : g_view_height.GetFloat();
        float interpFactor = 1.0f - exp(-g_view_interp.GetFloat() * deltaTime);
        m_viewHeight = glm::mix(m_viewHeight, targetHeight, interpFactor);

        btTransform currentTransform = m_character->getGhostObject()->getWorldTransform();
        btVector3 bulletPos = currentTransform.getOrigin();
        m_camera->position = glm::vec3(bulletPos.getX(), bulletPos.getY() + m_viewHeight, bulletPos.getZ());

        // Apply View Bobbing
        if (g_viewbob.GetInt() > 0 && m_character->onGround() && !m_noclip)
        {
            float horizontalSpeed = glm::length(glm::vec2(wishDir.x, wishDir.z));
            if (horizontalSpeed > 0.1f)
            {
                float speedFactor = isSprinting ? 1.4f : 1.0f;
                m_bobTimer += deltaTime * 10.0f * speedFactor;

                float amount = 0.035f * g_viewbob_scale.GetFloat();

                m_camera->position.y += sin(m_bobTimer) * amount;

                glm::vec3 forward = m_camera->GetForward();
                glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
                m_camera->position += right * (cos(m_bobTimer * 0.5f) * (amount * 0.4f));
            }
            else
            {
                m_bobTimer = glm::mix(m_bobTimer, 0.0f, deltaTime * 5.0f);
            }
        }

        m_origin = glm::vec3(bulletPos.x(), bulletPos.y(), bulletPos.z());
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

void Player::SetOrigin(const glm::vec3& origin)
{
    m_origin = origin;

    if (m_character)
    {
        btTransform trans;
        trans.setIdentity();
        trans.setOrigin(btVector3(origin.x, origin.y, origin.z));
        m_character->getGhostObject()->setWorldTransform(trans);

        m_character->reset(Physics::GetDynamicsWorld());
    }
}

void Player::OnSave()
{
    Entity::OnSave();

    if (m_camera)
    {
        m_saveYaw = m_camera->yaw;
        m_savePitch = m_camera->pitch;
    }

    AddSaveField(DATA_FIELD(Player, m_flashlightOn, FieldType::Bool));
    AddSaveField(DATA_FIELD(Player, m_viewHeight, FieldType::Float));
    AddSaveField(DATA_FIELD(Player, m_isCrouching, FieldType::Bool));
    AddSaveField(DATA_FIELD(Player, m_noclip, FieldType::Bool));
    AddSaveField(DATA_FIELD(Player, m_saveYaw, FieldType::Float));
    AddSaveField(DATA_FIELD(Player, m_savePitch, FieldType::Float));
}

LINK_ENTITY_TO_CLASS("info_player_start", Player)