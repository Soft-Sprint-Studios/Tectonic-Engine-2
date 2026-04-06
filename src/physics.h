#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>

namespace Physics
{
    enum CollisionGroups
    {
        COL_NOTHING = 0,
        COL_WORLD = (1 << 0),
        COL_PLAYER = (1 << 1),
        COL_ALL = (COL_WORLD | COL_PLAYER)
    };

    void Init();
    void Update(float deltaTime);
    void Shutdown();

    void AddBSPCollision(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices);

    btBvhTriangleMeshShape* CreateStaticMeshShape(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices);
    void AddStaticBody(btCollisionShape* shape, const glm::mat4& transform);

    btKinematicCharacterController* CreateCharacter(const glm::vec3& position);

    btDiscreteDynamicsWorld* GetDynamicsWorld();
}