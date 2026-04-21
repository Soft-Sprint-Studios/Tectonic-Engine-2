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
#include "main_menu.h"
#include "renderer.h"
#include "input.h"
#include "maps.h"
#include "gamedef.h"
#include "console.h"
#include "concmd.h"

namespace MainMenu
{
    static bool s_active = true;
    static float s_menuWidth = 300.0f;

    void Init()
    {
        // Unlock mouse initially for the menu
        SDL_SetWindowRelativeMouseMode(SDL_GL_GetCurrentWindow(), false);
    }

    bool Button(Renderer* renderer, const std::string& label, float x, float y, float w, float h)
    {
        auto ui = renderer->GetUI();
        bool hovered = ui->IsMouseOver(x, y, w, h);
        
        glm::vec4 bgColor = hovered ? glm::vec4(0.4f, 0.4f, 0.4f, 0.8f) : glm::vec4(0.2f, 0.2f, 0.2f, 0.8f);
        ui->DrawRect(x, y, w, h, bgColor);
        ui->DrawText(label, x + 10.0f, y + (h * 0.5f) - 9.0f, glm::vec4(1.0f));

        return hovered && (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MASK(SDL_BUTTON_LEFT));
    }

    void Update(const Input& input, float deltaTime)
    {
        if (input.GetKeyDown(SDL_SCANCODE_ESCAPE))
        {
            SetActive(!s_active);
        }
    }

    void Draw(Renderer* renderer)
    {
        if (!s_active) return;

        int w, h;
        SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &w, &h);

        auto ui = renderer->GetUI();
        
        // Black Background
        ui->DrawRect(0, 0, (float)w, (float)h, glm::vec4(0, 0, 0, 1.0f));

        float startX = 50.0f;
        float startY = 100.0f;
        float btnH = 40.0f;
        float spacing = 10.0f;

        ui->DrawText(Gamedef::GetGameName(), startX, 50.0f, glm::vec4(1, 0.5f, 0, 1));

        if (Maps::HasMapLoaded())
        {
            if (Button(renderer, "Resume Game", startX, startY, s_menuWidth, btnH))
            {
                SetActive(false);
            }
        }
        else
        {
            ui->DrawRect(startX, startY, s_menuWidth, btnH, glm::vec4(0.1f, 0.1f, 0.1f, 0.8f));
            ui->DrawText("Resume Game", startX + 10.0f, startY + (btnH * 0.5f) - 9.0f, glm::vec4(0.4f));
        }

        if (Button(renderer, "New Game", startX, startY + (btnH + spacing), s_menuWidth, btnH))
        {
            Maps::Load(Gamedef::GetStartingMap());
            SetActive(false);
        }

        if (Button(renderer, "Quit to Desktop", startX, startY + (btnH + spacing) * 2, s_menuWidth, btnH))
        {
            Console::Execute("quit");
        }
    }

    void SetActive(bool active)
    {
        s_active = active;
        SDL_SetWindowRelativeMouseMode(SDL_GL_GetCurrentWindow(), !s_active);
    }

    bool IsActive() 
    { 
        return s_active; 
    }
}