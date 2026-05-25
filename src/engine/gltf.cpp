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
#include "gltf.h"
#include "filesystem.h"
#include "console.h"
#include <algorithm>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

namespace GLTF
{
    ModelData Load(const std::string& path)
    {
        ModelData outData;
        std::string targetPath = path;

        if (!Filesystem::Exists(targetPath))
        {
            if (targetPath == "models/error.glb")
            {
                return outData;
            }

            Console::Warn("Model not found: " + targetPath);
            targetPath = "models/error.glb";
        }

        cgltf_options options = {};
        cgltf_data* data = nullptr;

        std::string fullPath = Filesystem::GetFullPath(targetPath);
        cgltf_result result = cgltf_parse_file(&options, fullPath.c_str(), &data);

        if (result != cgltf_result_success)
        {
            Console::Error("Failed to parse GLB at: " + fullPath);
            return outData;
        }

        result = cgltf_load_buffers(&options, data, fullPath.c_str());
        if (result != cgltf_result_success)
        {
            Console::Error("Failed to load buffers for: " + fullPath);
            cgltf_free(data);
            return outData;
        }

        outData.valid = true;
        uint32_t currentVertexOffset = 0;

        for (cgltf_size i = 0; i < data->meshes_count; ++i)
        {
            const cgltf_mesh& mesh = data->meshes[i];

            for (cgltf_size j = 0; j < mesh.primitives_count; ++j)
            {
                const cgltf_primitive& prim = mesh.primitives[j];
                Primitive outPrim;
                
                uint32_t vertexCount = 0;

                for (cgltf_size k = 0; k < prim.attributes_count; ++k)
                {
                    const cgltf_attribute& attr = prim.attributes[k];
                    const cgltf_accessor* acc = attr.data;
                    
                    if (attr.type == cgltf_attribute_type_position)
                    {
                        vertexCount = (uint32_t)acc->count;
                        outPrim.positions.reserve(vertexCount);
                        
                        for (cgltf_size v = 0; v < acc->count; v++)
                        {
                            float p[3];
                            cgltf_accessor_read_float(acc, v, p, 3);
                            glm::vec3 pos(p[0], p[1], p[2]);
                            outData.localMins = glm::min(outData.localMins, pos);
                            outData.localMaxs = glm::max(outData.localMaxs, pos);
                            outPrim.positions.push_back(pos);
                            outData.physicsPositions.push_back(pos);
                        }
                    }
                    else if (attr.type == cgltf_attribute_type_texcoord)
                    {
                        outPrim.uvs.reserve(acc->count);
                        for (cgltf_size v = 0; v < acc->count; v++)
                        {
                            float p[2];
                            cgltf_accessor_read_float(acc, v, p, 2);
                            outPrim.uvs.push_back(glm::vec2(p[0], p[1]));
                        }
                    }
                    else if (attr.type == cgltf_attribute_type_normal)
                    {
                        outPrim.normals.reserve(acc->count);
                        for (cgltf_size v = 0; v < acc->count; v++)
                        {
                            float p[3];
                            cgltf_accessor_read_float(acc, v, p, 3);
                            outPrim.normals.push_back(glm::vec3(p[0], p[1], p[2]));
                        }
                    }
                    else if (attr.type == cgltf_attribute_type_tangent)
                    {
                        outPrim.tangents.reserve(acc->count);
                        for (cgltf_size v = 0; v < acc->count; v++)
                        {
                            float p[4];
                            cgltf_accessor_read_float(acc, v, p, 4);
                            outPrim.tangents.push_back(glm::vec4(p[0], p[1], p[2], p[3]));
                        }
                    }
                }

                if (prim.indices)
                {
                    const cgltf_accessor* acc = prim.indices;
                    outPrim.indices.reserve(acc->count);
                    
                    uint32_t baseIdx = (uint32_t)outData.physicsPositions.size() - vertexCount;

                    for (cgltf_size idx = 0; idx < acc->count; idx++)
                    {
                        uint32_t indexVal = (uint32_t)cgltf_accessor_read_index(acc, idx);
                        outPrim.indices.push_back(indexVal);
                        outData.physicsIndices.push_back(baseIdx + indexVal);
                    }
                }

                std::string matName = (prim.material && prim.material->name) ? prim.material->name : "";
                std::transform(matName.begin(), matName.end(), matName.begin(), ::tolower);
                outPrim.materialName = matName;

                outData.primitives.push_back(outPrim);
                currentVertexOffset += vertexCount;
            }
        }

        cgltf_free(data);
        return outData;
    }
}