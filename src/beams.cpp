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
#include "beams.h"
#include "entities.h"

namespace Beams
{
    static std::vector<std::shared_ptr<Beam>> s_beams;

    void Init() 
    {
    }

    void Update()
    {
        for (auto it = s_beams.begin(); it != s_beams.end();)
        {
            if (it->use_count() <= 1)
            {
                it = s_beams.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void Clear()
    {
        s_beams.clear();
    }

    std::shared_ptr<Beam> CreateBeam(const BeamDef& def)
    {
        auto b = std::make_shared<Beam>(def);
        s_beams.push_back(b);
        return b;
    }

    const std::vector<std::shared_ptr<Beam>>& GetActiveBeams()
    {
        return s_beams;
    }
}