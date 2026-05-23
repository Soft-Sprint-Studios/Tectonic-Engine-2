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
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>

// NavHull idea inspired by pathos ai_nodegraph
enum class NavHull
{
    Small,
    Human,
    Large,
    Fly
};

struct AINode
{
    int index;
    glm::vec3 origin;
    std::vector<int> links;
};

class AINodeGraph
{
public:
    void Init();
    void Clear();

    int AddNode(const glm::vec3& pos);
    void BuildLinks();

    int GetNearestNode(const glm::vec3& pos, NavHull hull);
    std::vector<glm::vec3> FindPath(const glm::vec3& startPos, const glm::vec3& endPos, NavHull hull);

    const std::vector<AINode>& GetNodes() const
    {
        return m_nodes;
    }

private:
    struct PathNode
    {
        int nodeIdx;
        float gScore;
        float fScore;
        int parentIdx = -1;
    };

    bool IsPathValid(const glm::vec3& start, const glm::vec3& end, NavHull hull);
    std::vector<AINode> m_nodes;
};

namespace Nodegraph
{
    AINodeGraph* Get();
}