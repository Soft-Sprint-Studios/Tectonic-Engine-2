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
#include "weapons.h"
#include "timing.h"
#include "physics.h"
#include "sound.h"
#include "particles.h"
#include <unordered_map>

namespace Weapons
{
    static std::vector<WeaponDef> s_weaponLibrary;
    static int s_activeWeaponIdx = 0;
    static float s_nextAttackTime = 0.0f;
    static std::unique_ptr<Sound::AudioSource> s_weaponSnd;

    void Init()
    {
        s_weaponSnd = std::make_unique<Sound::AudioSource>();

        WeaponDef hands;
        hands.name = "Hands";
        hands.type = WeaponType::Melee;
        hands.damage = 5.0f;
        hands.range = 2.0f;
        hands.fireRate = 0.5f;
        hands.fireSound = "swing.mp3";
        hands.impactParticle = "";
        s_weaponLibrary.push_back(hands);
    }

    const WeaponDef& GetCurrent()
    {
        return s_weaponLibrary[s_activeWeaponIdx];
    }

    void SelectWeapon(int index)
    {
        if (index >= 0 && index < s_weaponLibrary.size())
        {
            s_activeWeaponIdx = index;
        }
    }

    void Fire(const glm::vec3& startPos, const glm::vec3& direction, Entity* owner)
    {
        if (Time::TotalTime() < s_nextAttackTime)
        {
            return;
        }

        const WeaponDef& current = GetCurrent();
        s_nextAttackTime = (float)Time::TotalTime() + current.fireRate;

        s_weaponSnd->SetPosition(startPos);
        s_weaponSnd->Play(current.fireSound);

        btVector3 rayFrom(startPos.x, startPos.y, startPos.z);
        glm::vec3 target = startPos + (direction * current.range);
        btVector3 rayTo(target.x, target.y, target.z);

        btCollisionWorld::ClosestRayResultCallback rayCallback(rayFrom, rayTo);

        rayCallback.m_collisionFilterGroup = Physics::COL_PLAYER;
        rayCallback.m_collisionFilterMask = Physics::COL_WORLD;

        Physics::GetDynamicsWorld()->rayTest(rayFrom, rayTo, rayCallback);

        if (rayCallback.hasHit())
        {
            if (rayCallback.m_collisionObject->getUserPointer())
            {
                Entity* hitEnt = static_cast<Entity*>(rayCallback.m_collisionObject->getUserPointer());
                hitEnt->TakeDamage(current.damage, owner);
            }

            if (!current.impactParticle.empty())
            {
                btVector3 p = rayCallback.m_hitPointWorld;
                Particles::CreateSystem(current.impactParticle, glm::vec3(p.x(), p.y(), p.z()));
            }
        }
    }
}