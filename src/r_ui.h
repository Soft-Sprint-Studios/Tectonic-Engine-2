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
#include "window.h"
#include "r_shader.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <SDL3_ttf/SDL_ttf.h>
#include <glad/glad.h>

class R_UI
{
public:
    R_UI();
    ~R_UI();

    void Init(Window* window);
    void Shutdown();
    void OnWindowResize(int w, int h);
    
    void DrawText(const std::string& text, float x, float y, const glm::vec4& color);
    void Render();

private:
    struct TextDrawCommand
    {
        std::string text;
        float x, y;
        glm::vec4 color;
    };

    struct CachedText
    {
        GLuint textureID;
        int width, height;
    };

    R_Shader m_shader;
    GLuint m_vao = 0, m_vbo = 0;
    TTF_Font* m_font = nullptr;
    glm::mat4 m_projection;
    std::vector<TextDrawCommand> m_commands;
    std::unordered_map<std::string, CachedText> m_textCache;
};