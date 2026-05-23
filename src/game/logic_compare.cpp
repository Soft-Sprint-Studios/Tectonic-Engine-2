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

class LogicCompare : public Entity
{
public:
    void Spawn(const BSP::EntityData& entData) override
    {
        Entity::Spawn(entData);
        m_value = GetFloat("InitialValue", 0.0f);
        m_compareValue = GetFloat("CompareValue", 0.0f);
    }

    void OnSave() override
    {
        Entity::OnSave();
        AddSaveField(DATA_FIELD(LogicCompare, m_value, FieldType::Float));
        AddSaveField(DATA_FIELD(LogicCompare, m_compareValue, FieldType::Float));
    }

    void AcceptInput(const std::string& input, const std::string& param) override
    {
        if (input == "SetValue")
        {
            m_value = std::stof(param);
        }
        else if (input == "SetCompareValue")
        {
            m_compareValue = std::stof(param);
        }
        else if (input == "Compare")
        {
            DoCompare();
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

private:
    void DoCompare()
    {
        if (m_value == m_compareValue)
        {
            FireOutput("OnEqualTo");
        }
        else
        {
            FireOutput("OnNotEqualTo");
        }

        if (m_value > m_compareValue)
        {
            FireOutput("OnGreaterThan");
        }

        if (m_value < m_compareValue)
        {
            FireOutput("OnLessThan");
        }

        if (m_value >= m_compareValue)
        {
            FireOutput("OnGreaterOrEqual");
        }

        if (m_value <= m_compareValue)
        {
            FireOutput("OnLessOrEqual");
        }
    }

    float m_value = 0.0f;
    float m_compareValue = 0.0f;
};

LINK_ENTITY_TO_CLASS("logic_compare", LogicCompare)