#pragma once
#include <string>
#include <unordered_map>
#include <SDL3/SDL.h>

class Input;

namespace Binds
{
    void Init();
    void Save();
    void Update(const Input& input);
    
    void SetBind(const std::string& keyName, const std::string& command);
    void Unbind(const std::string& keyName);
}