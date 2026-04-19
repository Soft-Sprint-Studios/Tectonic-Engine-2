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
#include "camera.h"
#include "r_models.h"
#include "filesystem.h"
#include "materials.h"
#include "console.h"
#include "physics.h"
#include "cubemap.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

R_Models::R_Models()
{
}

R_Models::~R_Models()
{
    Shutdown();
}

bool R_Models::Init(const BSP::MapData& mapData)
{
    Shutdown();

    std::unordered_map<std::string, std::vector<const BSP::StaticPropInstance*>> groupedProps;
    for (const auto& prop : mapData.staticProps)
    {
        groupedProps[prop.modelPath].push_back(&prop);
    }

    for (const auto& [path, props] : groupedProps)
    {
        LoadModel(path);

        auto& group = m_propGroups[path];
        group.instanceCount = (uint32_t)props.size();
        group.worldMins = glm::vec3(FLT_MAX);
        group.worldMaxs = glm::vec3(-FLT_MAX);

        if (!props.empty())
        {
            group.hasBumpedLighting = props[0]->hasBumpedLighting;
        }

        std::vector<glm::mat4> transforms;
        transforms.reserve(group.instanceCount);

        std::vector<glm::vec4> packedColors(group.instanceCount * group.totalVertices * 3, glm::vec4(1.0f));

        glm::vec3 corners[8] =
        {
            group.localMins,
            { group.localMaxs.x, group.localMins.y, group.localMins.z },
            { group.localMins.x, group.localMaxs.y, group.localMins.z },
            { group.localMins.x, group.localMins.y, group.localMaxs.z },
            { group.localMaxs.x, group.localMaxs.y, group.localMins.z },
            { group.localMins.x, group.localMaxs.y, group.localMaxs.z },
            { group.localMaxs.x, group.localMins.y, group.localMaxs.z },
            group.localMaxs
        };

        for (size_t i = 0; i < props.size(); ++i)
        {
            const auto* prop = props[i];

            glm::mat4 m_visual = glm::translate(glm::mat4(1.0f), prop->position);
            m_visual = glm::rotate(m_visual, glm::radians(prop->angles.y - 90.0f), glm::vec3(0, 1, 0));
            m_visual = glm::rotate(m_visual, glm::radians(-prop->angles.x), glm::vec3(1, 0, 0));
            m_visual = glm::rotate(m_visual, glm::radians(prop->angles.z), glm::vec3(0, 0, 1));

            glm::mat4 m_physics = m_visual;
            m_visual = glm::scale(m_visual, glm::vec3(prop->scale * BSP::MAPSCALE));

            transforms.push_back(m_visual);

            for (int c = 0; c < 8; c++)
            {
                glm::vec3 worldCorner = glm::vec3(m_visual * glm::vec4(corners[c], 1.0f));
                group.worldMins = glm::min(group.worldMins, worldCorner);
                group.worldMaxs = glm::max(group.worldMaxs, worldCorner);
            }

            if (group.physicsShape)
            {
                Physics::AddStaticBody(group.physicsShape, m_physics, glm::vec3(prop->scale));
            }

            for (int stream = 0; stream < 3; ++stream)
            {
                if (!prop->vertexColors[stream].empty())
                {
                    for (size_t v = 0; v < prop->vertexColors[stream].size() && v < group.totalVertices; ++v)
                    {
                        packedColors[(i * group.totalVertices + v) * 3 + stream] = prop->vertexColors[stream][v];
                    }
                }
            }
        }

        glGenBuffers(1, &group.transformSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, group.transformSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, transforms.size() * sizeof(glm::mat4), transforms.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &group.colorSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, group.colorSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, packedColors.size() * sizeof(glm::vec4), packedColors.data(), GL_STATIC_DRAW);
    }

    return true;
}

void R_Models::LoadModel(const std::string& path)
{
    std::string targetPath = path;

    if (!Filesystem::Exists(targetPath))
    {
        if (targetPath == "models/error.glb")
        {
            return;
        }

        Console::Warn("Model not found: " + targetPath);
        targetPath = "models/error.glb";

        if (m_propGroups.find(targetPath) != m_propGroups.end())
        {
            m_propGroups[path] = m_propGroups[targetPath];
            m_propGroups[path].instanceCount = 0;
            return;
        }
    }

    cgltf_options options = {};
    cgltf_data* data = nullptr;

    std::string fullPath = Filesystem::GetFullPath(targetPath);
    cgltf_result result = cgltf_parse_file(&options, fullPath.c_str(), &data);

    if (result != cgltf_result_success)
    {
        Console::Error("Failed to parse GLB at: " + fullPath);
        if (targetPath != "models/error.glb")
        {
            LoadModel("models/error.glb");
            m_propGroups[path] = m_propGroups["models/error.glb"];
            m_propGroups[path].instanceCount = 0;
        }
        return;
    }

    result = cgltf_load_buffers(&options, data, fullPath.c_str());
    if (result != cgltf_result_success)
    {
        Console::Error("Failed to load buffers for: " + fullPath);
        cgltf_free(data);
        return;
    }

    PropGroup group;
    uint32_t currentVertexOffset = 0;

    std::vector<glm::vec3> physicsPositions;
    std::vector<uint32_t> physicsIndices;

    for (cgltf_size i = 0; i < data->meshes_count; ++i)
    {
        const cgltf_mesh& mesh = data->meshes[i];

        for (cgltf_size j = 0; j < mesh.primitives_count; ++j)
        {
            const cgltf_primitive& prim = mesh.primitives[j];
            ModelMesh m = {};
            m.vertexOffset = currentVertexOffset;
            uint32_t vertexCount = 0;

            glGenVertexArrays(1, &m.vao);
            glBindVertexArray(m.vao);

            for (cgltf_size k = 0; k < prim.attributes_count; ++k)
            {
                const cgltf_attribute& attr = prim.attributes[k];
                const cgltf_accessor* acc = attr.data;
                const cgltf_buffer_view* view = acc->buffer_view;

                uint8_t* bufferPtr = (uint8_t*)view->buffer->data + view->offset + acc->offset;

                GLuint vbo;
                glGenBuffers(1, &vbo);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)view->size, bufferPtr, GL_STATIC_DRAW);
                m.vbos.push_back(vbo);

                GLsizei stride = acc->stride ? (GLsizei)acc->stride : 0;

                if (attr.type == cgltf_attribute_type_position)
                {
                    vertexCount = (uint32_t)acc->count;
                    glEnableVertexAttribArray(0);
                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);

                    for (cgltf_size v = 0; v < acc->count; v++)
                    {
                        float p[3];
                        cgltf_accessor_read_float(acc, v, p, 3);
                        glm::vec3 pos(p[0], p[1], p[2]);
                        group.localMins = glm::min(group.localMins, pos);
                        group.localMaxs = glm::max(group.localMaxs, pos);
                        physicsPositions.push_back(pos * BSP::MAPSCALE);
                    }
                }
                else if (attr.type == cgltf_attribute_type_texcoord)
                {
                    glEnableVertexAttribArray(1);
                    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, 0);
                }
                else if (attr.type == cgltf_attribute_type_normal)
                {
                    glEnableVertexAttribArray(9);
                    glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, stride, 0);
                }
                else if (attr.type == cgltf_attribute_type_tangent)
                {
                    glEnableVertexAttribArray(10);
                    glVertexAttribPointer(10, 3, GL_FLOAT, GL_FALSE, stride, 0);
                }
            }

            if (prim.indices)
            {
                const cgltf_accessor* acc = prim.indices;
                const cgltf_buffer_view* view = acc->buffer_view;
                uint8_t* bufferPtr = (uint8_t*)view->buffer->data + view->offset + acc->offset;

                glGenBuffers(1, &m.ebo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)view->size, bufferPtr, GL_STATIC_DRAW);

                m.indexCount = (uint32_t)acc->count;
                m.indexType = (acc->component_type == cgltf_component_type_r_16u) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

                uint32_t baseIdx = (uint32_t)physicsPositions.size() - vertexCount;
                for (cgltf_size idx = 0; idx < prim.indices->count; idx++)
                {
                    physicsIndices.push_back(baseIdx + (uint32_t)cgltf_accessor_read_index(prim.indices, idx));
                }
            }

            std::string matName = (prim.material && prim.material->name) ? prim.material->name : "";
            m.texture = Materials::GetTexture(matName);
            m.normalMap = Materials::GetNormalMap(matName);
            m.specularMap = Materials::GetSpecularMap(matName);

            m.vertexCount = vertexCount;
            currentVertexOffset += vertexCount;
            group.meshes.push_back(m);
        }
    }

    group.totalVertices = currentVertexOffset;
    group.physicsShape = Physics::CreateStaticMeshShape(physicsPositions, physicsIndices);

    m_propGroups[path] = group;
    cgltf_free(data);
}

void R_Models::Draw(const R_Shader& shader, const Frustum& frustum, bool depthOnly)
{
    shader.SetInt("u_isInstanced", 1);

    if (!depthOnly)
    {
        shader.SetInt("u_isModel", 1);
    }

    for (auto& [path, group] : m_propGroups)
    {
        if (group.instanceCount == 0)
        {
            continue;
        }

        if (frustum.valid && !frustum.IsBoxVisible(group.worldMins, group.worldMaxs))
        {
            continue;
        }

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, group.transformSSBO);

        if (!depthOnly)
        {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, group.colorSSBO);
            shader.SetInt("u_totalVertices", group.totalVertices);
        }

        for (auto& mesh : group.meshes)
        {
            if (mesh.indexCount == 0)
            {
                continue;
            }

            glBindVertexArray(mesh.vao);

            if (depthOnly)
            {
                for (auto& mesh : group.meshes)
                {
                    glBindVertexArray(mesh.vao);
                    glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount, mesh.indexType, 0, group.instanceCount);
                }
            }
            else
            {
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, group.colorSSBO);
                shader.SetInt("u_totalVertices", group.totalVertices);

                for (auto& mesh : group.meshes)
                {
                    if (mesh.indexCount == 0) 
                        continue;

                    glBindVertexArray(mesh.vao);
                    shader.SetInt("u_vertexOffset", mesh.vertexOffset);
                    shader.SetInt("u_useBump", group.hasBumpedLighting ? 1 : 0);

                    (mesh.texture ? mesh.texture : Materials::GetTexture(""))->Bind(0);
                    (mesh.normalMap ? mesh.normalMap : Materials::GetFlatNormal())->Bind(2);
                    (mesh.specularMap ? mesh.specularMap : Materials::GetWhiteTexture())->Bind(3);

                    glVertexAttrib3f(11, 0.0f, 0.0f, 0.0f);
                    glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount, mesh.indexType, 0, group.instanceCount);
                }
            }
        }
    }

    glBindVertexArray(0);
    shader.SetInt("u_isInstanced", 0);

    if (!depthOnly)
    {
        shader.SetInt("u_isModel", 0);
    }
}

void R_Models::Shutdown()
{
    for (auto& [path, group] : m_propGroups)
    {
        for (auto& mesh : group.meshes)
        {
            if (mesh.vao != 0)
                glDeleteVertexArrays(1, &mesh.vao);
            if (!mesh.vbos.empty())
                glDeleteBuffers((GLsizei)mesh.vbos.size(), mesh.vbos.data());
            if (mesh.ebo != 0)
                glDeleteBuffers(1, &mesh.ebo);
        }

        if (group.transformSSBO != 0)
            glDeleteBuffers(1, &group.transformSSBO);

        if (group.colorSSBO != 0)
            glDeleteBuffers(1, &group.colorSSBO);

        group.physicsShape = nullptr;
    }
    m_propGroups.clear();
}