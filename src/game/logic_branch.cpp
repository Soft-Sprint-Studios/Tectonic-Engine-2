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

class LogicBranch : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);
        m_state = GetInt("InitialValue", 0) != 0;
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(LogicBranch, m_state, FieldType::Bool));
    }

    void AcceptInput(const std::string& input, const std::string& param) override
    {
        if (input == "SetValue")
        {
            m_state = (std::stoi(param) != 0);
        }
        else if (input == "SetValueTest")
        {
            m_state = (std::stoi(param) != 0);
            Test();
        }
        else if (input == "Toggle")
        {
            m_state = !m_state;
        }
        else if (input == "ToggleTest")
        {
            m_state = !m_state;
            Test();
        }
        else if (input == "Test")
        {
            Test();
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

private:
    void Test()
    {
        if (m_state)
        {
            FireOutput("OnTrue");
        }
        else
        {
            FireOutput("OnFalse");
        }
    }

    bool m_state = false;
};

LINK_ENTITY_TO_CLASS("logic_branch", LogicBranch)