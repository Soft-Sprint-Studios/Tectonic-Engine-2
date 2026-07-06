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
#include <bgfx/bgfx.h>

class R_UI
{
public:
    R_UI();
    ~R_UI();

    void Init(Window* window);
    void Shutdown();
    void OnWindowResize(int w, int h);

    void DrawText(const std::string& text, float x, float y, const glm::vec4& color, float scale = 1.0f);
    void DrawRect(float x, float y, float w, float h, const glm::vec4& color);
    void DrawCircle(float x, float y, float w, float h, const glm::vec4& color);
    bool IsMouseOver(float x, float y, float w, float h) const;
    void Render();

private:
    struct TextDrawCommand
    {
        std::string text;
        float x, y;
        glm::vec4 color;
        float scale;
    };

    struct CachedText
    {
        bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;
        int width = 0;
        int height = 0;
    };

    struct RectDrawCommand
    {
        float x, y, w, h;
        glm::vec4 color;
        int type;
    };

    struct UIVertex
    {
        float x, y, z;
        float u, v;
    };

    bgfx::ViewId m_viewId = 2;
    bgfx::VertexLayout m_uiLayout;
    bgfx::TextureHandle m_whiteTexture = BGFX_INVALID_HANDLE;

    bgfx::UniformHandle m_uTextColor = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_uUiParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_sTexture = BGFX_INVALID_HANDLE;

    R_Shader m_shader;
    TTF_Font* m_font = nullptr;
    glm::mat4 m_projection;

    std::vector<TextDrawCommand> m_commands;
    std::unordered_map<std::string, CachedText> m_textCache;
    std::vector<RectDrawCommand> m_rectCommands;
};