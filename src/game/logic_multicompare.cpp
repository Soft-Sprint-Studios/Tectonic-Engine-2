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

class LogicMulticompare : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);
        
        m_numInputs = GetInt("numinputs", 2);
        m_targetValue = GetFloat("TargetValue", 1.0f);
        m_modeAnd = (GetInt("ShouldSatisfyAll", 1) == 1);

        for (int i = 0; i < 16; ++i)
        {
            m_currentValues[i] = 0.0f;
        }
        
        m_lastFullState = false;
    }

    void OnSave() override
    {
        Entity::OnSave();
        for (int i = 0; i < 16; ++i)
        {
             AddSaveField({ FieldType::Float, "val_" + std::to_string(i), (size_t)(&((LogicMulticompare*)0)->m_currentValues[i]), sizeof(float) });
        }
        AddSaveField(DATA_FIELD(LogicMulticompare, m_lastFullState, FieldType::Bool));
    }

    void AcceptInput(const std::string& inputName, const std::string& parameter) override
    {
        Entity::AcceptInput(inputName, parameter);

        if (inputName.find("SetInput") == 0)
        {
            std::string indexStr = inputName.substr(8);
            int index = std::stoi(indexStr) - 1;

            if (index >= 0 && index < 16)
            {
                m_currentValues[index] = std::stof(parameter);
                EvaluateLogic();
            }
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

private:
    void EvaluateLogic()
    {
        bool currentState = false;

        if (m_modeAnd)
        {
            currentState = true;
            for (int i = 0; i < m_numInputs; ++i)
            {
                if (m_currentValues[i] != m_targetValue)
                {
                    currentState = false;
                    break;
                }
            }
        }
        else
        {
            currentState = false;
            for (int i = 0; i < m_numInputs; ++i)
            {
                if (m_currentValues[i] == m_targetValue)
                {
                    currentState = true;
                    break;
                }
            }
        }

        if (currentState != m_lastFullState)
        {
            if (currentState)
            {
                FireOutput("OnTrue");
            }
            else
            {
                FireOutput("OnFalse");
            }
            m_lastFullState = currentState;
        }
    }

    int m_numInputs;
    float m_targetValue;
    bool m_modeAnd;
    bool m_lastFullState;
    float m_currentValues[16];
};

LINK_ENTITY_TO_CLASS("logic_multicompare", LogicMulticompare)