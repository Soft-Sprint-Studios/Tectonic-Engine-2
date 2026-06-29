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
#include "cubemap.h"
#include "r_cubemap.h"
#include "filesystem.h"
#include <sstream>
#include <glm/gtx/norm.hpp>

namespace Cubemap
{
    static std::vector<Probe> s_probes;

    void Init()
    {
    }

    void Shutdown()
    {
        Clear();
    }

    void Clear()
    {
        for (auto& p : s_probes)
        {
            R_Cubemap::Release(p.id);
        }
        s_probes.clear();
    }

    const std::vector<Probe>& GetProbes()
    {
        return s_probes;
    }

    void AddProbe(const Probe& probe)
    {
        s_probes.push_back(probe);
    }

    const Probe* FindClosest(const glm::vec3& position)
    {
        if (s_probes.empty())
        {
            return nullptr;
        }

        const Probe* closest = &s_probes[0];
        float minDistSq = glm::length2(position - closest->origin);

        for (size_t i = 1; i < s_probes.size(); ++i)
        {
            float distSq = glm::length2(position - s_probes[i].origin);
            if (distSq < minDistSq)
            {
                minDistSq = distSq;
                closest = &s_probes[i];
            }
        }
        return closest;
    }

    void LoadForMap(const std::string& mapName)
    {
        Clear();
        std::string dir = "cubemaps/" + mapName + "/";
        if (!Filesystem::Exists(dir))
        {
            return;
        }

        int index = 0;
        while (true)
        {
            std::string base = dir + "cubemap_" + std::to_string(index);
            if (!Filesystem::Exists(base + ".txt"))
            {
                break;
            }

            Probe p;
            std::string meta = Filesystem::ReadText(base + ".txt");
            std::stringstream ss(meta);
            ss >> p.origin.x >> p.origin.y >> p.origin.z;
            ss >> p.mins.x >> p.mins.y >> p.mins.z;
            ss >> p.maxs.x >> p.maxs.y >> p.maxs.z;

            p.id = R_Cubemap::CreateFromFiles(base);
            s_probes.push_back(p);
            index++;
        }
    }

    void BuildCubemaps(const std::string& mapName, Renderer* renderer)
    {
        R_Cubemap::BuildProbes(mapName, renderer);
    }
}