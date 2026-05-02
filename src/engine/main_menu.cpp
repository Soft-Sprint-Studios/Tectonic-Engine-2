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
#include "cvar.h"
#include "localization.h"
#include <algorithm>

namespace MainMenu
{
    static bool s_active = true;
    static float s_menuWidth = 300.0f;
    static MenuPage s_currentPage = MenuPage::Main;
    static bool s_mousePressedLast = false;

    static bool IsMouseJustPressed() 
    {
        bool down = SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MASK(SDL_BUTTON_LEFT);
        bool pressed = down && !s_mousePressedLast;
        s_mousePressedLast = down;
        return pressed;
    }

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

        return hovered && IsMouseJustPressed();
    }

    bool Checkbox(Renderer* renderer, const std::string& label, bool& value, float x, float y) 
    {
        auto ui = renderer->GetUI();
        float size = 20.0f;
        bool hovered = ui->IsMouseOver(x, y, size, size);

        ui->DrawRect(x, y, size, size, glm::vec4(0.15f, 0.15f, 0.15f, 1.0f));
        if (value) 
            ui->DrawRect(x + 4, y + 4, size - 8, size - 8, glm::vec4(1, 0.5f, 0, 1));

        ui->DrawText(label, x + size + 10.0f, y + 2.0f, glm::vec4(1.0f));

        if (hovered && IsMouseJustPressed()) 
        {
            value = !value;
            return true;
        }
        return false;
    }

    void Slider(Renderer* renderer, const std::string& label, float& value, float min, float max, float x, float y) 
    {
        auto ui = renderer->GetUI();
        float w = s_menuWidth;
        float h = 8.0f;

        ui->DrawText(label + ": " + std::to_string(value).substr(0, 4), x, y - 22.0f, glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
        ui->DrawRect(x, y, w, h, glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));

        float progress = (value - min) / (max - min);
        ui->DrawRect(x + (progress * w) - 4.0f, y - 6.0f, 8.0f, 20.0f, glm::vec4(1, 0.5f, 0, 1));

        if (ui->IsMouseOver(x, y - 10.0f, w, 30.0f) && (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MASK(SDL_BUTTON_LEFT))) 
        {
            float mx;
            SDL_GetMouseState(&mx, NULL);
            float pct = std::clamp((mx - x) / w, 0.0f, 1.0f);
            value = min + (max - min) * pct;
        }
    }

    void Update(const Input& input, float deltaTime) 
    {
        if (input.GetKeyDown(SDL_SCANCODE_ESCAPE)) 
            SetActive(!s_active);
    }

    void Draw(Renderer* renderer)
    {
        if (!s_active)
            return;

        // Reset click tracking if mouse released
        if (!(SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)))
            s_mousePressedLast = false;

        int w, h;
        SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &w, &h);
        auto ui = renderer->GetUI();

        // Black Background
        ui->DrawRect(0, 0, (float)w, (float)h, glm::vec4(0, 0, 0, 1.0f));

        float startX = 50.0f;
        float startY = 150.0f;
        float btnH = 40.0f;
        float spacing = 12.0f;

        if (s_currentPage == MenuPage::Main)
        {
            ui->DrawText(Gamedef::GetGameName(), startX, 60.0f, glm::vec4(1, 0.5f, 0, 1));

            std::string startLabel = Maps::HasMapLoaded() ? Localization::Get("Menu_ResumeGame") : Localization::Get("Menu_NewGame");
            if (Button(renderer, startLabel, startX, startY, s_menuWidth, btnH))
            {
                if (!Maps::HasMapLoaded())
                    Maps::Load(Gamedef::GetStartingMap());
                SetActive(false);
            }

            if (Button(renderer, Localization::Get("Menu_Options"), startX, startY + (btnH + spacing), s_menuWidth, btnH))
                s_currentPage = MenuPage::Options;

            if (Button(renderer, Localization::Get("Menu_Quit"), startX, startY + (btnH + spacing) * 2, s_menuWidth, btnH))
                Console::Execute("quit");
        }
        else if (s_currentPage == MenuPage::Options)
        {
            ui->DrawText(Localization::Get("Menu_Options"), startX, 60.0f, glm::vec4(1, 0.5f, 0, 1));

            bool bloom = CVar::Find("r_bloom")->GetInt() > 0;
            if (Checkbox(renderer, Localization::Get("Opt_Bloom"), bloom, startX, startY))
                CVar::Set("r_bloom", bloom ? "1" : "0");

            bool ssao = CVar::Find("r_ssao")->GetInt() > 0;
            if (Checkbox(renderer, Localization::Get("Opt_SSAO"), ssao, startX, startY + 35.0f))
                CVar::Set("r_ssao", ssao ? "1" : "0");

            bool volumetrics = CVar::Find("r_volumetrics")->GetInt() > 0;
            if (Checkbox(renderer, Localization::Get("Opt_Volumetrics"), volumetrics, startX, startY + 70.0f))
                CVar::Set("r_volumetrics", volumetrics ? "1" : "0");

            bool shadows = CVar::Find("r_shadows")->GetInt() > 0;
            if (Checkbox(renderer, Localization::Get("Opt_Shadows"), shadows, startX, startY + 105.0f))
                CVar::Set("r_shadows", shadows ? "1" : "0");

            bool tonemap = CVar::Find("r_tonemap")->GetInt() > 0;
            if (Checkbox(renderer, Localization::Get("Opt_Tonemap"), tonemap, startX, startY + 140.0f))
                CVar::Set("r_tonemap", tonemap ? "1" : "0");

            float vol = CVar::Find("s_volume")->GetFloat();
            Slider(renderer, Localization::Get("Opt_MasterVol"), vol, 0.0f, 1.0f, startX, startY + 210.0f);
            CVar::Set("s_volume", std::to_string(vol));

            float fov = CVar::Find("fov")->GetFloat();
            Slider(renderer, Localization::Get("Opt_FOV"), fov, 60.0f, 110.0f, startX, startY + 270.0f);
            CVar::Set("fov", std::to_string(fov));

            float sens = CVar::Find("sensitivity")->GetFloat();
            Slider(renderer, Localization::Get("Opt_Sens"), sens, 0.1f, 5.0f, startX, startY + 330.0f);
            CVar::Set("sensitivity", std::to_string(sens));

            float gamma = CVar::Find("r_gamma")->GetFloat();
            Slider(renderer, Localization::Get("Opt_Gamma"), gamma, 1.0f, 3.0f, startX, startY + 390.0f);
            CVar::Set("r_gamma", std::to_string(gamma));

            if (Button(renderer, Localization::Get("Menu_Back"), startX, (float)h - 100.0f, s_menuWidth, btnH))
            {
                s_currentPage = MenuPage::Main;
                CVar::Save();
            }
        }
    }

    void SetActive(bool active) 
    {
        s_active = active;
        if (!s_active) 
            s_currentPage = MenuPage::Main;
        SDL_SetWindowRelativeMouseMode(SDL_GL_GetCurrentWindow(), !s_active);
    }

    bool IsActive() 
    {
        return s_active;
    }
}