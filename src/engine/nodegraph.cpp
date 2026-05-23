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
#include "nodegraph.h"
#include "physics.h"
#include <algorithm>
#include <queue>
#include <map>
#include <console.h>

void AINodeGraph::Init()
{
    Clear();
}

void AINodeGraph::Clear()
{
    m_nodes.clear();
}

int AINodeGraph::AddNode(const glm::vec3& pos)
{
    AINode node;
    node.index = (int)m_nodes.size();
    node.origin = pos;
    m_nodes.push_back(node);
    return node.index;
}

bool AINodeGraph::IsPathValid(const glm::vec3& start, const glm::vec3& end, NavHull hull)
{
    btVector3 halfExtents(0.3f, 0.85f, 0.3f);

    if (hull == NavHull::Small)
    {
        halfExtents.setY(0.4f);
    }
    else if (hull == NavHull::Large)
    {
        halfExtents *= 2.0f;
    }

    float groundOffset = halfExtents.y() + 0.1f;

    btVector3 bStart(start.x, start.y + groundOffset, start.z);
    btVector3 bEnd(end.x, end.y + groundOffset, end.z);

    btBoxShape shape(halfExtents);

    btTransform tStart, tEnd;
    tStart.setIdentity();
    tStart.setOrigin(bStart);
    tEnd.setIdentity();
    tEnd.setOrigin(bEnd);

    struct NavCallback : public btCollisionWorld::ConvexResultCallback
    {
        bool hit = false;
        btScalar addSingleResult(btCollisionWorld::LocalConvexResult& res, bool norm) override
        {
            hit = true;
            return 1.0;
        }
    } cb;

    Physics::GetDynamicsWorld()->convexSweepTest(&shape, tStart, tEnd, cb, 0.05f);

    return !cb.hit;
}

void AINodeGraph::BuildLinks()
{
    for (auto& node : m_nodes)
    {
        node.links.clear();
        for (auto& target : m_nodes)
        {
            if (node.index == target.index)
            {
                continue;
            }

            float dist = glm::distance(node.origin, target.origin);

            if (dist < 30.0f)
            {
                if (IsPathValid(node.origin, target.origin, NavHull::Human))
                {
                    node.links.push_back(target.index);
                }
            }
        }
    }
}

int AINodeGraph::GetNearestNode(const glm::vec3& pos, NavHull hull)
{
    int best = -1;
    float bestDist = 1e10f;

    for (const auto& node : m_nodes)
    {
        float d = glm::distance(pos, node.origin);
        if (d < bestDist)
        {
            if (IsPathValid(pos, node.origin, hull))
            {
                bestDist = d;
                best = node.index;
            }
        }
    }
    return best;
}

std::vector<glm::vec3> AINodeGraph::FindPath(const glm::vec3& startPos, const glm::vec3& endPos, NavHull hull)
{
    int startIdx = GetNearestNode(startPos, hull);
    int endIdx = GetNearestNode(endPos, hull);

    if (startIdx == -1 || endIdx == -1)
    {
        return {};
    }

    std::vector<PathNode> open;
    std::map<int, PathNode> closed;

    PathNode start;
    start.nodeIdx = startIdx;
    start.gScore = 0;
    start.fScore = glm::distance(m_nodes[startIdx].origin, m_nodes[endIdx].origin);
    open.push_back(start);

    while (!open.empty())
    {
        auto it = std::min_element(open.begin(), open.end(), [](const PathNode& a, const PathNode& b)
            {
                return a.fScore < b.fScore;
            });

        PathNode current = *it;
        if (current.nodeIdx == endIdx)
        {
            std::vector<glm::vec3> path;
            path.push_back(endPos);
            while (current.nodeIdx != startIdx)
            {
                path.push_back(m_nodes[current.nodeIdx].origin);
                current = closed[current.parentIdx];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        open.erase(it);
        closed[current.nodeIdx] = current;

        for (int neighborIdx : m_nodes[current.nodeIdx].links)
        {
            if (closed.count(neighborIdx))
            {
                continue;
            }

            float g = current.gScore + glm::distance(m_nodes[current.nodeIdx].origin, m_nodes[neighborIdx].origin);

            auto openIt = std::find_if(open.begin(), open.end(), [neighborIdx](const PathNode& n)
                {
                    return n.nodeIdx == neighborIdx;
                });

            if (openIt == open.end() || g < openIt->gScore)
            {
                PathNode neighbor;
                neighbor.nodeIdx = neighborIdx;
                neighbor.gScore = g;
                neighbor.fScore = g + glm::distance(m_nodes[neighborIdx].origin, m_nodes[endIdx].origin);
                neighbor.parentIdx = current.nodeIdx;

                if (openIt != open.end())
                {
                    *openIt = neighbor;
                }
                else
                {
                    open.push_back(neighbor);
                }
            }
        }
    }

    return {};
}

static AINodeGraph s_graph;
AINodeGraph* Nodegraph::Get()
{
    return &s_graph;
}