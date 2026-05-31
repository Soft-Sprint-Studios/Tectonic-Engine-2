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
#include "decals.h"
#include "physics.h"
#include <btBulletDynamicsCommon.h>
#include <algorithm>

class InfoDecal : public Entity
{
public:
    void Spawn(const BSP::EntityData& entData) override
    {
        Entity::Spawn(entData);

        std::string textureName = GetValue("texture");
        if (textureName.empty())
        {
            return;
        }

        if (textureName.rfind("decals/", 0) == 0)
        {
            textureName = textureName.substr(7);
        }

        std::transform(textureName.begin(), textureName.end(), textureName.begin(), ::tolower);

        glm::vec3 origin = GetOrigin();
        float traceDist = 2.0f * BSP::MAPSCALE;

        glm::vec3 offsets[6] = 
        {
            glm::vec3(0.0f, 0.0f, traceDist),
            glm::vec3(0.0f, traceDist, 0.0f),
            glm::vec3(traceDist, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, -traceDist),
            glm::vec3(0.0f, -traceDist, 0.0f),
            glm::vec3(-traceDist, 0.0f, 0.0f)
        };

        bool foundSurface = false;
        btVector3 hitPoint, hitNormal;

        for (int i = 0; i < 3; ++i)
        {
            glm::vec3 start = origin + offsets[i];
            glm::vec3 end = origin + offsets[i + 3];

            btVector3 rayFrom(start.x, start.y, start.z);
            btVector3 rayTo(end.x, end.y, end.z);

            btCollisionWorld::ClosestRayResultCallback rayCallback(rayFrom, rayTo);
            rayCallback.m_collisionFilterGroup = Physics::COL_PLAYER;
            rayCallback.m_collisionFilterMask = Physics::COL_WORLD;

            Physics::GetDynamicsWorld()->rayTest(rayFrom, rayTo, rayCallback);

            if (rayCallback.hasHit())
            {
                hitPoint = rayCallback.m_hitPointWorld;
                hitNormal = rayCallback.m_hitNormalWorld;
                foundSurface = true;
                break;
            }
        }

        if (!foundSurface)
        {
            for (int i = 0; i < 3; ++i)
            {
                glm::vec3 start = origin + offsets[i + 3];
                glm::vec3 end = origin + offsets[i];

                btVector3 rayFrom(start.x, start.y, start.z);
                btVector3 rayTo(end.x, end.y, end.z);

                btCollisionWorld::ClosestRayResultCallback rayCallback(rayFrom, rayTo);
                rayCallback.m_collisionFilterGroup = Physics::COL_PLAYER;
                rayCallback.m_collisionFilterMask = Physics::COL_WORLD;

                Physics::GetDynamicsWorld()->rayTest(rayFrom, rayTo, rayCallback);

                if (rayCallback.hasHit())
                {
                    hitPoint = rayCallback.m_hitPointWorld;
                    hitNormal = rayCallback.m_hitNormalWorld;
                    foundSurface = true;
                    break;
                }
            }
        }

        if (foundSurface)
        {
            DecalDef dDef;
            dDef.position = glm::vec3(hitPoint.x(), hitPoint.y(), hitPoint.z());
            dDef.normal = glm::vec3(hitNormal.x(), hitNormal.y(), hitNormal.z());
            dDef.size = -1.0f;
            dDef.textureName = textureName;
            dDef.lifetime = -1.0f;
            dDef.blendMode = 0;
            Decals::CreateDecal(dDef);
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }
};

LINK_ENTITY_TO_CLASS("infodecal", InfoDecal)