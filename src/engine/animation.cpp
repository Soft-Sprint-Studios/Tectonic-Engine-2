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
#include "animation.h"
#include <glm/gtx/quaternion.hpp>

namespace Animation
{
    void UpdateHierarchy(const GLTF::ModelData& model, std::vector<NodeState>& states, int clipIndex, float time)
    {
        if (states.empty())
            return;

        for (int i = 0; i < (int)model.nodes.size(); ++i) 
        {
            states[i].translation = model.nodes[i].translation;
            states[i].rotation = model.nodes[i].rotation;
            states[i].scale = model.nodes[i].scale;
        }

        if (clipIndex >= 0 && clipIndex < (int)model.animations.size())
        {
            const auto& anim = model.animations[clipIndex];
            float modTime = fmod(time, anim.duration);

            for (const auto& channel : anim.channels)
            {
                const auto& sampler = anim.samplers[channel.samplerIndex];
                auto it = std::upper_bound(sampler.timestamps.begin(), sampler.timestamps.end(), modTime);
                int next = int(std::distance(sampler.timestamps.begin(), it));

                if (next >= int(sampler.timestamps.size()))
                {
                    next = int(sampler.timestamps.size()) - 1;
                }

                int prev = (next == 0) ? 0 : next - 1;
                float factor = (next == prev) ? 0.0f : (modTime - sampler.timestamps[prev]) / (sampler.timestamps[next] - sampler.timestamps[prev]);

                auto& state = states[channel.nodeIndex];
                if (channel.path == GLTF::AnimationChannel::Path::Translation)
                {
                    state.translation = glm::mix(glm::vec3(sampler.keyframes[prev]), glm::vec3(sampler.keyframes[next]), factor);
                }
                else if (channel.path == GLTF::AnimationChannel::Path::Rotation)
                {
                    state.rotation = glm::slerp(glm::quat(sampler.keyframes[prev].w, sampler.keyframes[prev].x, sampler.keyframes[prev].y, sampler.keyframes[prev].z), glm::quat(sampler.keyframes[next].w, sampler.keyframes[next].x, sampler.keyframes[next].y, sampler.keyframes[next].z), factor);
                }
                else if (channel.path == GLTF::AnimationChannel::Path::Scale)
                {
                    state.scale = glm::mix(glm::vec3(sampler.keyframes[prev]), glm::vec3(sampler.keyframes[next]), factor);
                }
            }
        }

        for (int i = 0; i < (int)model.nodes.size(); ++i)
        {
            const auto& node = model.nodes[i];
            auto& state = states[i];
            glm::mat4 local = glm::translate(glm::mat4(1.0f), state.translation) * glm::toMat4(state.rotation);
            local = glm::scale(local, state.scale);

            if (node.parent == -1)
            {
                state.globalMatrix = local;
            }
            else
            {
                state.globalMatrix = states[node.parent].globalMatrix * local;
            }
        }
    }

    void GetSkinMatrices(const GLTF::ModelData& model, const std::vector<NodeState>& states, std::vector<glm::mat4>& outMatrices)
    {
        outMatrices.resize(model.skin.joints.size());
        for (size_t i = 0; i < model.skin.joints.size(); ++i)
        {
            outMatrices[i] = states[model.skin.joints[i]].globalMatrix * model.skin.inverseBindMatrices[i];
        }
    }
}