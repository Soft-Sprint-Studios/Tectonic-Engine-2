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
#include "bsploader.h"
#include "shader.h"
#include "texture.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <btBulletDynamicsCommon.h>

struct ModelMesh
{
    uint32_t vao;
    std::vector<uint32_t> vbos;
    uint32_t ebo;
    uint32_t indexCount;
    uint32_t indexType;
    std::shared_ptr<Texture> texture;
    std::shared_ptr<Texture> normalMap;
    std::shared_ptr<Texture> specularMap;
    uint32_t vertexOffset;
};

struct PropInstanceData
{
    glm::mat4 transform;
    uint32_t colorVbo[3] = { 0, 0, 0 };
    bool isBumped = false;
};

class R_Models
{
public:
    R_Models();
    ~R_Models();

    bool Init(const BSP::MapData& mapData);
    void Draw(const Shader& shader);
    void Shutdown();

private:
    struct PropGroup
    {
        std::vector<ModelMesh> meshes;
        std::vector<PropInstanceData> instances;
        btCollisionShape* physicsShape = nullptr;
    };

    std::unordered_map<std::string, PropGroup> m_propGroups;

    void LoadModel(const std::string& path);
};