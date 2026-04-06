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
#include "console.h"

#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/CollisionDispatch/btCollisionDispatcher.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <LinearMath/btDefaultMotionState.h>

namespace Physics
{
    static btDefaultCollisionConfiguration* s_collisionConfiguration;
    static btCollisionDispatcher* s_dispatcher;
    static btBroadphaseInterface* s_broadphase;
    static btSequentialImpulseConstraintSolver* s_solver;
    static btDiscreteDynamicsWorld* s_dynamicsWorld;

    static btTriangleMesh* s_bspMesh;
    static btBvhTriangleMeshShape* s_bspShape;
    static btRigidBody* s_bspBody;
    static std::vector<btTriangleMesh*> s_modelMeshes;

    void Init()
    {
        s_collisionConfiguration = new btDefaultCollisionConfiguration();
        s_dispatcher = new btCollisionDispatcher(s_collisionConfiguration);
        s_broadphase = new btDbvtBroadphase();
        s_solver = new btSequentialImpulseConstraintSolver();
        s_dynamicsWorld = new btDiscreteDynamicsWorld(s_dispatcher, s_broadphase, s_solver, s_collisionConfiguration);

        s_dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
        s_dynamicsWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
    }

    void Update(float deltaTime)
    {
        s_dynamicsWorld->stepSimulation(deltaTime, 10);
    }

    void Shutdown()
    {
        if (s_dynamicsWorld)
        {
            if (s_bspBody) {
                s_dynamicsWorld->removeRigidBody(s_bspBody);
                if (s_bspBody->getMotionState()) 
                    delete s_bspBody->getMotionState();
                delete s_bspBody;
                s_bspBody = nullptr;
            }

            for (int i = s_dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
            {
                btCollisionObject* obj = s_dynamicsWorld->getCollisionObjectArray()[i];
                btRigidBody* body = btRigidBody::upcast(obj);
                if (body && body->getMotionState()) 
                    delete body->getMotionState();
                s_dynamicsWorld->removeCollisionObject(obj);
                delete obj;
            }

            for (auto m : s_modelMeshes) 
                delete m;
            s_modelMeshes.clear();
        }

        delete s_dynamicsWorld; 
        s_dynamicsWorld = nullptr;
        delete s_solver; 
        s_solver = nullptr;
        delete s_broadphase; 
        s_broadphase = nullptr;
        delete s_dispatcher; 
        s_dispatcher = nullptr;
        delete s_collisionConfiguration; 
        s_collisionConfiguration = nullptr;

        delete s_bspShape; 
        s_bspShape = nullptr;
        delete s_bspMesh; 
        s_bspMesh = nullptr;
    }

    void AddBSPCollision(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices)
    {
        if (!s_dynamicsWorld) 
            return;

        if (indices.empty() || vertices.empty()) 
            return;

        if (s_bspBody)
        {
            s_dynamicsWorld->removeRigidBody(s_bspBody);
            delete s_bspBody;
            s_bspBody = nullptr;
            delete s_bspShape;
            s_bspShape = nullptr;
            delete s_bspMesh;
            s_bspMesh = nullptr;
        }

        s_bspMesh = new btTriangleMesh();
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            btVector3 v0(vertices[indices[i]].x, vertices[indices[i]].y, vertices[indices[i]].z);
            btVector3 v1(vertices[indices[i + 1]].x, vertices[indices[i + 1]].y, vertices[indices[i + 1]].z);
            btVector3 v2(vertices[indices[i + 2]].x, vertices[indices[i + 2]].y, vertices[indices[i + 2]].z);
            s_bspMesh->addTriangle(v0, v1, v2);
        }

        s_bspShape = new btBvhTriangleMeshShape(s_bspMesh, true);
        s_bspShape->setMargin(0.005f);

        btTransform groundTransform;
        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(0, 0, 0));

        btDefaultMotionState* motionState = new btDefaultMotionState(groundTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(0, motionState, s_bspShape);
        s_bspBody = new btRigidBody(rbInfo);

        s_bspBody->setFriction(0.5f);
        s_bspBody->setRollingFriction(0.5f);

        s_dynamicsWorld->addRigidBody(s_bspBody, COL_WORLD, COL_ALL);
    }

    btBvhTriangleMeshShape* CreateStaticMeshShape(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices)
    {
        if (vertices.empty() || indices.empty()) 
            return nullptr;

        btTriangleMesh* meshInterface = new btTriangleMesh();
        s_modelMeshes.push_back(meshInterface);

        for (size_t i = 0; i < indices.size(); i += 3)
        {
            btVector3 v0(vertices[indices[i]].x, vertices[indices[i]].y, vertices[indices[i]].z);
            btVector3 v1(vertices[indices[i + 1]].x, vertices[indices[i + 1]].y, vertices[indices[i + 1]].z);
            btVector3 v2(vertices[indices[i + 2]].x, vertices[indices[i + 2]].y, vertices[indices[i + 2]].z);
            meshInterface->addTriangle(v0, v1, v2);
        }

        btBvhTriangleMeshShape* shape = new btBvhTriangleMeshShape(meshInterface, true);
        shape->setMargin(0.005f);
        return shape;
    }

    void AddStaticBody(btCollisionShape* shape, const glm::mat4& transform)
    {
        if (!shape) 
            return;

        btTransform btTrans;
        btTrans.setFromOpenGLMatrix(&transform[0][0]);

        btRigidBody* body = new btRigidBody(0, new btDefaultMotionState(btTrans), shape);
        body->setFriction(0.5f);
        s_dynamicsWorld->addRigidBody(body, COL_WORLD, COL_ALL);
    }

    btKinematicCharacterController* CreateCharacter(const glm::vec3& position)
    {
        btScalar characterHeight = 1.75f;
        btScalar characterRadius = 0.4f;

        btCapsuleShape* capsule = new btCapsuleShape(characterRadius, characterHeight - 2 * characterRadius);

        btTransform startTransform;
        startTransform.setIdentity();
        startTransform.setOrigin(btVector3(position.x, position.y, position.z));

        btPairCachingGhostObject* ghostObject = new btPairCachingGhostObject();
        ghostObject->setWorldTransform(startTransform);
        ghostObject->setCollisionShape(capsule);
        ghostObject->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);

        s_dynamicsWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
        s_dynamicsWorld->addCollisionObject(ghostObject, COL_PLAYER, COL_WORLD);

        btKinematicCharacterController* character = new btKinematicCharacterController(ghostObject, capsule, 0.35f);

        character->setGravity(s_dynamicsWorld->getGravity());
        character->setMaxSlope(btRadians(50.0f));

        s_dynamicsWorld->addAction(character);

        return character;
    }

    btDiscreteDynamicsWorld* GetDynamicsWorld()
    {
        return s_dynamicsWorld;
    }
}