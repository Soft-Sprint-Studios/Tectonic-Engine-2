#include "entities.h"
#include "console.h"
#include "timing.h"

class FuncButton : public Entity
{
public:
    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override
    {
        Entity::Spawn(keyvalues);

        m_delay = GetFloat("wait", 1.0f);

        m_nextUseTime = 0.0f;

        Console::Log("func_button '" + m_targetName + "' spawned with a " + std::to_string(m_delay) + "s delay.");
    }

    void OnPress(Entity* activator) override
    {
        if (Time::TotalTime() < m_nextUseTime)
        {
            return;
        }

        Console::Log("Button '" + m_targetName + "' was pressed by player.");
        FireOutput("OnPressed");

        m_nextUseTime = Time::TotalTime() + m_delay;
    }

private:
    float m_delay = 1.0f;
    float m_nextUseTime = 0.0f;
};

LINK_ENTITY_TO_CLASS("func_button", FuncButton)