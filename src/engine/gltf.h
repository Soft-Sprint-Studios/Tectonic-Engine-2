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
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace GLTF
{
    struct Node
    {
        std::string name;
        int parent = -1;
        std::vector<int> children;
        glm::vec3 translation{ 0.0f };
        glm::quat rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
        glm::vec3 scale{ 1.0f };
        glm::mat4 globalMatrix{ 1.0f };

        glm::mat4 GetLocalMatrix() const 
        {
            return glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
        }
    };

    struct Skin
    {
        std::string name;
        std::vector<int> joints;
        std::vector<glm::mat4> inverseBindMatrices;
    };

    struct AnimationSampler
    {
        std::vector<float> timestamps;
        std::vector<glm::vec4> keyframes;
    };

    struct AnimationChannel
    {
        enum class Path 
        { 
            Translation, 
            Rotation, 
            Scale 
        };

        Path path;
        int nodeIndex;
        int samplerIndex;
    };

    struct AnimationClip
    {
        std::string name;
        std::vector<AnimationSampler> samplers;
        std::vector<AnimationChannel> channels;
        float duration = 0.0f;
    };

    struct Primitive
    {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec4> tangents;
        std::vector<glm::uvec4> joints;
        std::vector<glm::vec4> weights;
        std::vector<uint32_t> indices;
        std::string materialName;
    };

    struct ModelData
    {
        std::vector<Primitive> primitives;

        std::vector<Node> nodes;
        std::vector<AnimationClip> animations;
        Skin skin;
        bool isAnimated = false;

        std::vector<glm::vec3> physicsPositions;
        std::vector<uint32_t> physicsIndices;
        glm::vec3 localMins{ 1e10f };
        glm::vec3 localMaxs{ -1e10f };
        bool valid = false;
    };

    ModelData Load(const std::string& path);
}