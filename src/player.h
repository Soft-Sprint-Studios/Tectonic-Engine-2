#pragma once
#include "camera.h"
#include "input.h"
#include "entities.h"

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

private:
    Camera* m_camera = nullptr;
    Input* m_input = nullptr;
    btKinematicCharacterController* m_character = nullptr;
    bool m_noclip = false;
};