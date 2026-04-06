#pragma once
#include <string>

class Renderer;
class Camera;
class Input;

namespace MapSystem
{
    void Init(Renderer* renderer, Camera* camera, Input* input);
    void Load(const std::string& mapName);
}