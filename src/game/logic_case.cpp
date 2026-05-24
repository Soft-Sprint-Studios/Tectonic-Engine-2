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
#include "entities.h"
#include "filesystem.h"
#include <random>

class LogicCase : public Entity
{
public:
    void Spawn(const BSP::EntityData& entData) override
    {
        Entity::Spawn(entData);

        // Load 16 possible cases
        for (int i = 1; i <= 16; ++i)
        {
            std::string key = "Case";
            if (i < 10)
            {
                key += "0";
            }
            key += std::to_string(i);

            std::string val = GetValue(key, "");
            if (!val.empty())
            {
                m_cases.push_back({ std::to_string(i), val });
            }
        }
    }

    void AcceptInput(const std::string& input, const std::string& param) override
    {
        if (input == "InValue")
        {
            bool matched = false;
            for (const auto& c : m_cases)
            {
                bool match = (c.value.length() == param.length()) && std::equal(c.value.begin(), c.value.end(), param.begin(), [](unsigned char c1, unsigned char c2)
                    {
                        return std::tolower(c1) == std::tolower(c2);
                    });

                if (match)
                {
                    std::string outputName = "OnCase";
                    if (std::stoi(c.id) < 10)
                    {
                        outputName += "0";
                    }
                    outputName += c.id;

                    FireOutput(outputName);
                    matched = true;
                }
            }

            if (!matched)
            {
                FireOutput("OnDefault");
            }
        }
        else if (input == "PickRandom")
        {
            if (m_cases.empty())
            {
                return;
            }

            static std::mt19937 gen(std::random_device{}());
            std::uniform_int_distribution<> dist(0, (int)m_cases.size() - 1);
            
            int idx = dist(gen);
            std::string outputName = "OnCase";
            if (std::stoi(m_cases[idx].id) < 10)
            {
                outputName += "0";
            }
            outputName += m_cases[idx].id;

            FireOutput(outputName);
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

private:
    struct CaseEntry
    {
        std::string id;
        std::string value;
    };

    std::vector<CaseEntry> m_cases;
};

LINK_ENTITY_TO_CLASS("logic_case", LogicCase)