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
#include "camera.h"
#include "input.h"
#include "entities.h"
#include "dynamic_light.h"

#include <BulletDynamics/Character/btKinematicCharacterController.h>

class Player : public Entity
{
public:
    Player();
    ~Player();

    void Spawn(const std::unordered_map<std::string, std::string>& keyvalues) override;
    void Think(float deltaTime) override;

    void LinkCamera(Camera* cam);
    void LinkInput(Input* input);

    void SetNoclip(bool state);
    bool IsNoclip() const 
    { 
        return m_noclip; 
    }

    bool IsPlayer() const override 
    { 
        return true; 
    }

    std::shared_ptr<DynamicLight> m_flashlight;
    bool m_flashlightOn = false;

    float m_viewHeight = 1.5f;
    bool m_isCrouching = false;

private:
    Camera* m_camera = nullptr;
    Input* m_input = nullptr;
    btKinematicCharacterController* m_character = nullptr;
    bool m_noclip = false;
    glm::vec3 m_smoothedFlashlightDir;
};