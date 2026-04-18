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
#include "bsploader.h"
#include "physics.h"
#include "console.h"
#include "entities.h"
#include "cvar.h"

#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/CollisionDispatch/btCollisionDispatcher.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <LinearMath/btDefaultMotionState.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <set>
#include <algorithm>
#include <memory>

CVar g_gravity("g_gravity", "-9.81", CVAR_SAVE);

namespace Physics
{
    static std::unique_ptr<btDefaultCollisionConfiguration> s_collisionConfiguration;
    static std::unique_ptr<btCollisionDispatcher> s_dispatcher;
    static std::unique_ptr<btBroadphaseInterface> s_broadphase;
    static std::unique_ptr<btSequentialImpulseConstraintSolver> s_solver;
    static std::unique_ptr<btDiscreteDynamicsWorld> s_dynamicsWorld;
    static std::unique_ptr<btGhostPairCallback> s_ghostPairCallback;

    static std::unique_ptr<btTriangleMesh> s_bspMesh;
    static std::unique_ptr<btBvhTriangleMeshShape> s_bspShape;
    static std::unique_ptr<btRigidBody> s_bspBody;
    static std::unique_ptr<btDefaultMotionState> s_bspMotionState;

    static std::vector<std::unique_ptr<btTriangleMesh>> s_modelMeshes;
    static std::vector<std::unique_ptr<btCollisionShape>> s_shapes;
    static std::vector<std::unique_ptr<btRigidBody>> s_rigidBodies;
    static std::vector<std::unique_ptr<btCollisionObject>> s_collisionObjects;
    static std::vector<std::unique_ptr<btMotionState>> s_motionStates;
    static std::vector<std::unique_ptr<btKinematicCharacterController>> s_characters;

    static std::set<std::pair<const btCollisionObject*, const btCollisionObject*>> s_lastFramePairs;

    void Init()
    {
        s_collisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
        s_dispatcher = std::make_unique<btCollisionDispatcher>(s_collisionConfiguration.get());
        s_broadphase = std::make_unique<btDbvtBroadphase>();
        s_solver = std::make_unique<btSequentialImpulseConstraintSolver>();
        s_dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(s_dispatcher.get(), s_broadphase.get(), s_solver.get(), s_collisionConfiguration.get());

        s_dynamicsWorld->setGravity(btVector3(0, g_gravity.GetFloat(), 0));

        s_ghostPairCallback = std::make_unique<btGhostPairCallback>();
        s_dynamicsWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(s_ghostPairCallback.get());
    }

    void Update(float deltaTime)
    {
        s_dynamicsWorld->stepSimulation(deltaTime, 10);

        // Touch / EndTouch API
        std::set<std::pair<const btCollisionObject*, const btCollisionObject*>> currentFramePairs;
        int numManifolds = s_dispatcher->getNumManifolds();
        for (int i = 0; i < numManifolds; i++)
        {
            btPersistentManifold* contactManifold = s_dispatcher->getManifoldByIndexInternal(i);
            const btCollisionObject* obA = contactManifold->getBody0();
            const btCollisionObject* obB = contactManifold->getBody1();

            if (obA->getUserPointer() && obB->getUserPointer())
            {
                currentFramePairs.insert(std::minmax(obA, obB));
            }
        }

        // OnTouch API
        for (const auto& pair : currentFramePairs)
        {
            if (s_lastFramePairs.find(pair) == s_lastFramePairs.end())
            {
                Entity* entA = static_cast<Entity*>(pair.first->getUserPointer());
                Entity* entB = static_cast<Entity*>(pair.second->getUserPointer());
                entA->Touch(entB);
                entB->Touch(entA);
            }
        }

        // OnEndTouch API
        for (const auto& pair : s_lastFramePairs)
        {
            if (currentFramePairs.find(pair) == currentFramePairs.end())
            {
                Entity* entA = static_cast<Entity*>(pair.first->getUserPointer());
                Entity* entB = static_cast<Entity*>(pair.second->getUserPointer());
                entA->EndTouch(entB);
                entB->EndTouch(entA);
            }
        }
        s_lastFramePairs = currentFramePairs;
    }

    void Shutdown()
    {
        if (s_dynamicsWorld)
        {
            if (s_bspBody)
            {
                s_dynamicsWorld->removeRigidBody(s_bspBody.get());
                s_bspBody.reset();
            }

            for (auto& character : s_characters)
            {
                s_dynamicsWorld->removeAction(character.get());
            }

            for (auto& body : s_rigidBodies)
            {
                s_dynamicsWorld->removeRigidBody(body.get());
            }

            for (auto& obj : s_collisionObjects)
            {
                s_dynamicsWorld->removeCollisionObject(obj.get());
            }
        }

        s_characters.clear();
        s_rigidBodies.clear();
        s_collisionObjects.clear();
        s_motionStates.clear();
        s_shapes.clear();
        s_modelMeshes.clear();
        s_bspShape.reset();
        s_bspMesh.reset();
        s_bspMotionState.reset();

        s_lastFramePairs.clear();
        s_dynamicsWorld.reset();
        s_solver.reset();
        s_broadphase.reset();
        s_dispatcher.reset();
        s_collisionConfiguration.reset();
        s_ghostPairCallback.reset();
    }

    void AddBSPCollision(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices)
    {
        if (!s_dynamicsWorld || indices.empty() || vertices.empty())
            return;

        if (s_bspBody)
        {
            s_dynamicsWorld->removeRigidBody(s_bspBody.get());
            s_bspBody.reset();
            s_bspShape.reset();
            s_bspMesh.reset();
            s_bspMotionState.reset();
        }

        s_bspMesh = std::make_unique<btTriangleMesh>();
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            btVector3 v0(vertices[indices[i]].x, vertices[indices[i]].y, vertices[indices[i]].z);
            btVector3 v1(vertices[indices[i + 1]].x, vertices[indices[i + 1]].y, vertices[indices[i + 1]].z);
            btVector3 v2(vertices[indices[i + 2]].x, vertices[indices[i + 2]].y, vertices[indices[i + 2]].z);
            s_bspMesh->addTriangle(v0, v1, v2);
        }

        s_bspShape = std::make_unique<btBvhTriangleMeshShape>(s_bspMesh.get(), true);
        s_bspShape->setMargin(0.04f);

        btTransform groundTransform;
        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(0, 0, 0));

        s_bspMotionState = std::make_unique<btDefaultMotionState>(groundTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(0, s_bspMotionState.get(), s_bspShape.get());
        s_bspBody = std::make_unique<btRigidBody>(rbInfo);

        s_bspBody->setFriction(0.5f);
        s_bspBody->setRollingFriction(0.5f);

        s_dynamicsWorld->addRigidBody(s_bspBody.get(), COL_WORLD, COL_ALL);
    }

    btBvhTriangleMeshShape* CreateStaticMeshShape(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices)
    {
        if (vertices.empty() || indices.empty())
            return nullptr;

        auto meshInterface = std::make_unique<btTriangleMesh>();
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            btVector3 v0(vertices[indices[i]].x, vertices[indices[i]].y, vertices[indices[i]].z);
            btVector3 v1(vertices[indices[i + 1]].x, vertices[indices[i + 1]].y, vertices[indices[i + 1]].z);
            btVector3 v2(vertices[indices[i + 2]].x, vertices[indices[i + 2]].y, vertices[indices[i + 2]].z);
            meshInterface->addTriangle(v0, v1, v2);
        }

        auto shape = std::make_unique<btBvhTriangleMeshShape>(meshInterface.get(), true);
        shape->setMargin(0.005f);

        btBvhTriangleMeshShape* rawPtr = shape.get();
        s_modelMeshes.push_back(std::move(meshInterface));
        s_shapes.push_back(std::move(shape));

        return rawPtr;
    }

    void AddStaticBody(btCollisionShape* shape, const glm::mat4& transform, const glm::vec3& scale)
    {
        if (!shape)
            return;

        shape->setLocalScaling(btVector3(scale.x, scale.y, scale.z));

        btTransform btTrans;
        btTrans.setFromOpenGLMatrix(&transform[0][0]);

        auto motionState = std::make_unique<btDefaultMotionState>(btTrans);
        auto body = std::make_unique<btRigidBody>(0, motionState.get(), shape);
        body->setFriction(0.5f);

        s_dynamicsWorld->addRigidBody(body.get(), COL_WORLD, COL_ALL);

        s_motionStates.push_back(std::move(motionState));
        s_rigidBodies.push_back(std::move(body));
    }

    btCollisionObject* CreateGhostObject(const BSP::CollisionData& collisionData, const glm::vec3& origin)
    {
        if (collisionData.vertices.empty())
            return nullptr;

        auto mesh = std::make_unique<btTriangleMesh>();
        for (size_t i = 0; i < collisionData.indices.size(); i += 3)
        {
            const auto& v0_glm = collisionData.vertices[collisionData.indices[i]];
            const auto& v1_glm = collisionData.vertices[collisionData.indices[i + 1]];
            const auto& v2_glm = collisionData.vertices[collisionData.indices[i + 2]];
            mesh->addTriangle(btVector3(v0_glm.x, v0_glm.y, v0_glm.z), btVector3(v1_glm.x, v1_glm.y, v1_glm.z), btVector3(v2_glm.x, v2_glm.y, v2_glm.z));
        }

        auto shape = std::make_unique<btBvhTriangleMeshShape>(mesh.get(), true);
        auto ghost = std::make_unique<btPairCachingGhostObject>();

        ghost->setCollisionShape(shape.get());
        ghost->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);

        btTransform trans;
        trans.setIdentity();
        trans.setOrigin({ origin.x, origin.y, origin.z });
        ghost->setWorldTransform(trans);

        s_dynamicsWorld->addCollisionObject(ghost.get(), COL_WORLD, COL_PLAYER);

        btCollisionObject* rawPtr = ghost.get();
        s_modelMeshes.push_back(std::move(mesh));
        s_shapes.push_back(std::move(shape));
        s_collisionObjects.push_back(std::move(ghost));

        return rawPtr;
    }

    btKinematicCharacterController* CreateCharacter(const glm::vec3& position, void* userPtr)
    {
        btScalar characterHeight = 1.75f;
        btScalar characterRadius = 0.4f;

        auto capsule = std::make_unique<btCapsuleShape>(characterRadius, characterHeight - 2 * characterRadius);

        btTransform startTransform;
        startTransform.setIdentity();
        startTransform.setOrigin(btVector3(position.x, position.y, position.z));

        auto ghostObject = std::make_unique<btPairCachingGhostObject>();
        ghostObject->setWorldTransform(startTransform);
        ghostObject->setCollisionShape(capsule.get());
        ghostObject->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
        ghostObject->setUserPointer(userPtr);

        s_dynamicsWorld->addCollisionObject(ghostObject.get(), COL_PLAYER, COL_WORLD);

        auto character = std::make_unique<btKinematicCharacterController>(ghostObject.get(), capsule.get(), 0.35f);
        character->setGravity(s_dynamicsWorld->getGravity());
        character->setMaxSlope(btRadians(50.0f));

        s_dynamicsWorld->addAction(character.get());

        btKinematicCharacterController* rawPtr = character.get();
        s_shapes.push_back(std::move(capsule));
        s_collisionObjects.push_back(std::move(ghostObject));
        s_characters.push_back(std::move(character));

        return rawPtr;
    }

    btDiscreteDynamicsWorld* GetDynamicsWorld()
    {
        return s_dynamicsWorld.get();
    }
}