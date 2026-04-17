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
#include "r_ui.h"
#include "filesystem.h"
#include "console.h"
#include <glm/gtc/matrix_transform.hpp>

R_UI::R_UI() 
{
}

R_UI::~R_UI() 
{ 
    Shutdown(); 
}

void R_UI::Init(Window* window)
{
    if (!TTF_Init())
    {
        Console::Error("Failed to initialize SDL_ttf: " + std::string(SDL_GetError()));
        return;
    }

    std::string fontPath = Filesystem::GetFullPath("fonts/LiberationSans-Regular.ttf");
    m_font = TTF_OpenFont(fontPath.c_str(), 18);
    if (!m_font)
    {
        Console::Error("Failed to load font: " + fontPath);
        return;
    }

    m_shader.Load("shaders/ui.vert", "shaders/ui.frag");
    int w, h;
    SDL_GetWindowSize(window->Get(), &w, &h);
    OnWindowResize(w, h);

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void R_UI::Shutdown()
{
    for (auto const& [text, cached] : m_textCache)
    {
        glDeleteTextures(1, &cached.textureID);
    }
    m_textCache.clear();
    m_commands.clear();

    if (m_font) 
        TTF_CloseFont(m_font);
    m_font = nullptr;

    if (m_vao) 
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) 
        glDeleteBuffers(1, &m_vbo);
    m_vao = m_vbo = 0;

    if (TTF_WasInit())
    {
        TTF_Quit();
    }
}

void R_UI::OnWindowResize(int w, int h)
{
    m_projection = glm::ortho(0.0f, (float)w, (float)h, 0.0f);
}

void R_UI::DrawText(const std::string& text, float x, float y, const glm::vec4& color)
{
    m_commands.push_back({text, x, y, color});
}

void R_UI::Render()
{
    if (!m_font || m_commands.empty())
    {
        m_commands.clear();
        return;
    }

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    m_shader.Bind();
    m_shader.SetMat4("projection", m_projection);
    m_shader.SetMat4("model", glm::mat4(1.0f));
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_vao);

    for (const auto& cmd : m_commands)
    {
        // Check for cache, else prase and convert it
        if (m_textCache.find(cmd.text) == m_textCache.end())
        {
            SDL_Color sdlColor = {255, 255, 255, 255};
            SDL_Surface* surface = TTF_RenderText_Blended(m_font, cmd.text.c_str(), 0, sdlColor);
            if (!surface) 
                continue;

            SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
            SDL_DestroySurface(surface);
            if (!converted) 
                continue;

            CachedText newCache;
            newCache.width = converted->w;
            newCache.height = converted->h;

            glGenTextures(1, &newCache.textureID);
            glBindTexture(GL_TEXTURE_2D, newCache.textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, converted->w, converted->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, converted->pixels);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            m_textCache[cmd.text] = newCache;
            SDL_DestroySurface(converted);
        }

        CachedText& cached = m_textCache[cmd.text];
        m_shader.SetVec4("textColor", cmd.color);
        glBindTexture(GL_TEXTURE_2D, cached.textureID);

        float xpos = cmd.x;
        float ypos = cmd.y;
        float w = (float)cached.width;
        float h = (float)cached.height;

        // Vertices for the text
        float vertices[6][4] = 
        {
            { xpos,     ypos + h,   0.0f, 1.0f },        
            { xpos,     ypos,       0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 0.0f },
            { xpos,     ypos + h,   0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 0.0f },
            { xpos + w, ypos + h,   1.0f, 1.0f }
        };

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    
    m_commands.clear();
}