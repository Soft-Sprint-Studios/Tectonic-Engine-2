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
#include "weapons.h"
#include "physics.h"
#include "console.h"
#include "cvar.h"
#include "concmd.h"
#include "dynamic_light.h"
#include "shake.h"
#include <glm/gtx/string_cast.hpp>
#include "maps.h"

CVar cl_sensitivity("cl_sensitivity", "1.0", "Mouse sensitivity multiplier.", CVAR_SAVE);
CVar cl_walk_speed("cl_walk_speed", "3.64", "Player walking speed.", CVAR_SAVE);
CVar cl_crouch_speed("cl_crouch_speed", "1.82", "Player crouching speed.", CVAR_SAVE);
CVar cl_noclip_speed("cl_noclip_speed", "4.0", "Speed while in noclip mode.", CVAR_SAVE);
CVar cl_sprint_speed("cl_sprint_speed", "5.0", "Player sprinting speed.", CVAR_SAVE);
CVar cl_jump_force("cl_jump_force", "5.0", "Initial upward velocity of a jump.", CVAR_SAVE);
CVar cl_view_height("cl_view_height", "1.5", "Standing eye level height.", CVAR_SAVE);
CVar cl_crouch_height("cl_crouch_height", "0.7", "Crouching eye level height.", CVAR_SAVE);
CVar cl_view_interp("cl_view_interp", "12.0", "Speed of view height interpolation.", CVAR_SAVE);

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
    if (m_camera)
    {
        m_camera->yaw = m_spawnYaw;
        m_camera->pitch = m_spawnPitch;
    }
}

void Player::LinkInput(Input* input)
{
    m_input = input;
}

void Player::Spawn(const BSP::EntityData& entData)
{
    Entity::Spawn(entData);

    m_character = Physics::CreateCharacter(GetOrigin(), this);
    m_currentFOV = CVar::GetFloat("cl_fov", 75.0f);
    m_targetFOV = m_currentFOV;

    m_spawnYaw = -m_vecAngles.y;
    m_spawnPitch = -m_vecAngles.x;

    // Force immediate warp to the spawn position to clear penetration state
    btTransform startTrans;
    startTrans.setIdentity();
    glm::vec3 spawnPos = GetOrigin();
    startTrans.setOrigin(btVector3(spawnPos.x, spawnPos.y, spawnPos.z));
    m_character->getGhostObject()->setWorldTransform(startTrans);
    m_character->reset(Physics::GetDynamicsWorld());

    RestoreDefaultGravity();

    // Initialize flashlight
    m_flashlight = DynamicLights::CreateSpotLight(GetOrigin(), glm::vec3(0, 0, 1), glm::vec3(1.0f, 1.0f, 0.9f), 4.0f, 15.0f, 30.0f);
    if (m_flashlight)
    {
        m_flashlight->SetActive(false);
        m_flashlight->GetDef().castsShadows = false;
    }

    Weapons::Init();
    Weapons::SelectWeapon(0);
}

void Player::Think(float deltaTime)
{
    if (!m_camera || !m_input || !m_character)
    {
        return;
    }

    if (m_viewOverride)
    {
        m_camera->position = m_viewOverride->GetOrigin();
        glm::vec3 ang = m_viewOverride->GetAngles();
        m_camera->pitch = -ang.x;
        m_camera->yaw = -ang.y;
        return;
    }

    // Handle env_zoom
    if (std::abs(m_currentFOV - m_targetFOV) > 0.01f)
    {
        float dir = (m_targetFOV > m_currentFOV) ? 1.0f : -1.0f;
        m_currentFOV += dir * m_fovSpeed * deltaTime;

        if ((dir > 0.0f && m_currentFOV > m_targetFOV) || (dir < 0.0f && m_currentFOV < m_targetFOV))
        {
            m_currentFOV = m_targetFOV;
        }
    }
    else
    {
        if (m_targetFOV == m_currentFOV && m_fovSpeed == 0.0f)
        {
            m_currentFOV = CVar::GetFloat("cl_fov", 75.0f);
            m_targetFOV = m_currentFOV;
        }
    }

    m_camera->SetFOV(m_currentFOV);

    if (m_saveYaw != 0.0f || m_savePitch != 0.0f)
    {
        m_camera->yaw = m_saveYaw;
        m_camera->pitch = m_savePitch;
        m_saveYaw = 0.0f;
        m_savePitch = 0.0f;
    }

    float mouseSensitivity = 0.15f * cl_sensitivity.GetFloat();
    m_camera->yaw += m_input->GetMouseDeltaX() * mouseSensitivity;
    m_camera->pitch -= m_input->GetMouseDeltaY() * mouseSensitivity;

    m_camera->pitch = glm::clamp(m_camera->pitch, -89.0f, 89.0f);

    if (m_input->GetKeyDown(SDL_SCANCODE_F) && m_flashlight)
    {
        m_flashlightOn = !m_flashlightOn;
        m_flashlight->SetActive(m_flashlightOn);
        m_sndFlashlight.SetPosition(m_camera->position);
        m_sndFlashlight.SetVolume(0.3f);
        m_sndFlashlight.Play("flashlight_toggle.mp3");
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
        m_character->jump(btVector3(0, cl_jump_force.GetFloat(), 0));
        m_sndJump.SetPosition(GetOrigin());
        m_sndJump.SetVolume(0.5f);
        m_sndJump.Play("jump.mp3");
    }

    // Handle weapons
    if (m_input->GetKeyDown(SDL_SCANCODE_1)) 
        Weapons::SelectWeapon(0);

    if (!m_noclip && (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)))
    {
        Weapons::Fire(m_camera->position, m_camera->GetForward(), this);
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
    float baseSpeed = isSprinting ? cl_sprint_speed.GetFloat() : cl_walk_speed.GetFloat();

    float fovMod = isSprinting ? 5.0f : 0.0f;
    float targetBaseFOV = CVar::GetFloat("cl_fov");

    if (m_targetFOV == targetBaseFOV || m_targetFOV == targetBaseFOV + 5.0f)
    {
        m_targetFOV = targetBaseFOV + fovMod;
        m_fovSpeed = 15.0f;
    }

    baseSpeed *= m_moveMultiplier;

    if (m_isCrouching && !m_noclip)
    {
        baseSpeed = cl_crouch_speed.GetFloat();
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
        float noclipSpeed = cl_noclip_speed.GetFloat();
        if (isSprinting) 
            noclipSpeed *= 2.0f;

        m_camera->position += wishDir * noclipSpeed * deltaTime;

        btTransform trans;
        trans.setIdentity();
        trans.setOrigin(btVector3(m_camera->position.x, m_camera->position.y - 1.5f, m_camera->position.z));
        m_character->getGhostObject()->setWorldTransform(trans);
        m_character->setWalkDirection(btVector3(0, 0, 0));
    }
    else if (m_onLadder)
    {
        m_character->setGravity(btVector3(0, 0, 0));
        m_character->setVelocityForTimeInterval(btVector3(0, 0, 0), 0);

        bool isSprinting = m_input->GetKey(SDL_SCANCODE_LSHIFT);
        float climbSpeed = m_climbSpeed * (isSprinting ? 1.6f : 1.0f);
        glm::vec3 climbDir(0, 0, 0);

        if (m_input->GetKey(SDL_SCANCODE_W))
        {
            climbDir.y += climbSpeed;
        }
        if (m_input->GetKey(SDL_SCANCODE_S))
        {
            climbDir.y -= climbSpeed;
        }

        if (m_input->GetKey(SDL_SCANCODE_A))
        {
            climbDir -= right * 1.5f;
        }
        if (m_input->GetKey(SDL_SCANCODE_D))
        {
            climbDir += right * 1.5f;
        }

        m_character->setWalkDirection(btVector3(climbDir.x * deltaTime, climbDir.y * deltaTime, climbDir.z * deltaTime));

        btTransform currentTransform = m_character->getGhostObject()->getWorldTransform();
        btVector3 bulletPos = currentTransform.getOrigin();
        m_camera->position = glm::vec3(bulletPos.getX(), bulletPos.getY() + m_viewHeight, bulletPos.getZ());
        glm::vec3 worldPos = glm::vec3(bulletPos.x(), bulletPos.y(), bulletPos.z());
        if (m_parent)
            m_vecOrigin = worldPos - m_parent->GetOrigin();
        else
            m_vecOrigin = worldPos;
    }
    else
    {
        float physicsStep = 1.0f / 60.0f;
        glm::vec3 walkDisplacement = wishDir * physicsStep;
        glm::vec3 pushDisplacement = m_pushVelocity * physicsStep;

        m_character->setWalkDirection(btVector3(walkDisplacement.x + pushDisplacement.x, 0.0f, walkDisplacement.z + pushDisplacement.z));

        m_pushVelocity = glm::vec3(0.0f);

        float targetHeight = m_isCrouching ? cl_crouch_height.GetFloat() : cl_view_height.GetFloat();
        float interpFactor = 1.0f - exp(-cl_view_interp.GetFloat() * deltaTime);
        m_viewHeight = glm::mix(m_viewHeight, targetHeight, interpFactor);

        btTransform currentTransform = m_character->getGhostObject()->getWorldTransform();
        btVector3 bulletPos = currentTransform.getOrigin();
        m_camera->position = glm::vec3(bulletPos.getX(), bulletPos.getY() + m_viewHeight, bulletPos.getZ());

        if (m_character->onGround() && glm::length(wishDir) > 0.1f)
        {
            float stepInterval = m_isCrouching ? 0.6f : (isSprinting ? 0.3f : 0.45f);
            m_stepTimer += deltaTime;

            if (m_stepTimer >= stepInterval)
            {
                m_sndStep.SetPosition(GetOrigin());
                m_sndStep.SetVolume(m_isCrouching ? 0.2f : (isSprinting ? 0.6f : 0.4f));
                m_sndStep.Play("footstep.mp3");
                m_stepTimer = 0.0f;
            }
        }
        else
        {
            m_stepTimer = 0.0f;
        }

        // Apply env_shake
        m_camera->position += Shake::GetPositionOffset();
        glm::vec3 angShake = Shake::GetAngleOffset();
        m_camera->pitch += angShake.x;
        m_camera->yaw += angShake.y;

        glm::vec3 worldPos = glm::vec3(bulletPos.x(), bulletPos.y(), bulletPos.z());
        if (m_parent)
            m_vecOrigin = worldPos - m_parent->GetOrigin();
        else
            m_vecOrigin = worldPos;
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
    if (glm::distance(GetOrigin(), origin) > 0.1f)
    {
        Entity::SetOrigin(origin);

        if (m_character)
        {
            btTransform trans;
            trans.setIdentity();
            trans.setOrigin(btVector3(origin.x, origin.y, origin.z));
            m_character->getGhostObject()->setWorldTransform(trans);
            m_character->reset(Physics::GetDynamicsWorld());
        }
    }
    else
    {
        Entity::SetOrigin(origin);
    }
}

void Player::SetGravity(float newGravityY)
{
    if (m_character)
    {
        m_character->setGravity(btVector3(0, newGravityY, 0));
    }
}

void Player::RestoreDefaultGravity()
{
    if (m_character)
    {
        float defaultGravity = CVar::GetFloat("sv_gravity");
        m_character->setGravity(btVector3(0, defaultGravity, 0));
    }
}

glm::vec3 Player::GetViewForward() const
{
    return m_camera ? m_camera->GetForward() : glm::vec3(0.0f, 0.0f, 1.0f);
}

void Player::SetViewOverride(Entity* ent)
{
    m_viewOverride = ent;
}

Entity* Player::GetViewOverride() const
{
    return m_viewOverride;
}

void Player::TakeDamage(float amount)
{
    if (m_noclip || !IsEnabled())
        return;

    m_health -= amount;
    if (m_health <= 0)
    {
        m_health = 0;
        Console::Log("Player Died!.");
        // todo actually reload map without crashing
        return;
    }
}

float Player::GetHealth() const
{
    return m_health; 
}

void Player::SetFOV(float targetFov, float duration)
{
    m_targetFOV = targetFov;
    if (duration <= 0.0f)
    {
        m_currentFOV = targetFov;
        m_fovSpeed = 0.0f;
    }
    else
    {
        m_fovSpeed = std::abs(m_targetFOV - m_currentFOV) / duration;
    }
}

void Player::SetMoveMultiplier(float mul)
{
    m_moveMultiplier = mul;
}

void Player::SetOnLadder(bool state)
{
    m_onLadder = state;
}

void Player::ApplyPushVelocity(const glm::vec3& vel)
{
    m_pushVelocity += vel;
}

void Player::SetClimbSpeed(float speed)
{
    m_climbSpeed = speed;
}

bool Player::IsOnLadder() const
{
    return m_onLadder;
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
    AddSaveField(DATA_FIELD(Player, m_health, FieldType::Float));
}

LINK_ENTITY_TO_CLASS("info_player_start", Player)