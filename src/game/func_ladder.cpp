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
#include "player.h"

class FuncLadder : public Entity
{
public:
    void Spawn(const BSP::EntityData& entData)
    {
        Entity::Spawn(entData);
        m_speed = GetFloat("speed", 3.0f);
    }

    void OnPress(Entity* activator) override
    {
        if (activator && activator->IsPlayer())
        {
            Player* p = static_cast<Player*>(activator);
            bool newState = !p->IsOnLadder();
            p->SetOnLadder(newState);

            if (!newState)
            {
                p->RestoreDefaultGravity();
            }
        }
    }

    void Touch(Entity* other)
    {
        if (other && other->IsPlayer())
        {
            static_cast<Player*>(other)->SetClimbSpeed(m_speed);
        }
    }

    void EndTouch(Entity* other)
    {
        if (other && other->IsPlayer())
        {
            Player* p = static_cast<Player*>(other);
            p->SetOnLadder(false);
            p->RestoreDefaultGravity();
        }
    }

    bool IsCollidable() const override
    {
        return false;
    }

private:
    float m_speed = 3.0f;
};

LINK_ENTITY_TO_CLASS("func_ladder", FuncLadder)