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
#include "sys.h"
#include "engine_api.h"
#include "platform.h"
#include "build_date.h"
#include "gamedef.h"
#include "cmdargs.h"
#include "materials.h"
#include "resources.h"
#include "filesystem.h"
#include "waters.h"
#include "sprite.h"
#include "timing.h"
#include "console.h"
#include "cvar.h"
#include "binds.h"
#include "window.h"
#include "input.h"
#include "localization.h"
#include "concmd.h"
#include "renderer.h"
#include "camera.h"
#include "cubemap.h"
#include "particles.h"
#include "dynamic_light.h"
#include "lightstyles.h"
#include "beams.h"
#include "physics.h"
#include "entities.h"
#include "player.h"
#include "maps.h"
#include "main_menu.h"
#include "networking.h"
#include "sound.h"
#include "discord.h"
#include "shake.h"
#include "fade.h"
#include "video.h"
#include "screen_text.h"

namespace Sys
{
    static bool s_running = true;
    
    // Engine Objects
    static Window   s_window;
    static Input    s_input;
    static Renderer s_renderer;
    static Camera   s_camera;

    bool Init(int argc, char** argv)
    {
        Filesystem::Init();
        Console::Init();
        Gamedef::Init();
        Discord::Init();
        Cubemap::Init();
        Sound::Init();
        Networking::Init();
        CVar::Init();
        CommandLine::Init(argc, argv);

        int width = std::stoi(CommandLine::GetValue("-w", "1280"));
        int height = std::stoi(CommandLine::GetValue("-h", "720"));

        if (CommandLine::HasParm("-window"))
        {
            CVar::Set("r_fullscreen", "0");
        }

        if (CommandLine::HasParm("-fullscreen"))
        {
            CVar::Set("r_fullscreen", "1");
        }

        if (!s_window.Init(Gamedef::GetGameName().c_str(), width, height))
        {
            return false;
        }

        Localization::Init();
        Physics::Init();
        EntityManager::Init();

        // Load Definitions
        Materials::Init();
        Materials::LoadDefinitions("materials.def");

        Waters::Init();
        Waters::LoadDefinitions("water.def");

        Particles::Init();
        Particles::LoadDefinitions("particles.def");

        DynamicLights::Init();
        LightStyles::Init();
        Sprites::Init();

        Binds::Init();
        MainMenu::Init();

        Maps::Init(&s_renderer, &s_camera, &s_input);

        if (!s_renderer.Init(s_window)) 
        {
            Console::Error("Renderer Init Failed!");
            return false;
        }

        CommandLine::ExecuteInitialCommands();

        if (!Maps::HasMapLoaded())
        {
            Maps::Load(Gamedef::GetStartingMap());
        }

        int ww, wh;
        SDL_GetWindowSize(s_window.Get(), &ww, &wh);
        s_renderer.OnWindowResize(ww, wh);

        return true;
    }

    void RunFrame()
    {
        // Update Time and Console
        uint64_t frameStart = SDL_GetTicksNS();
        Time::Update();
        Console::Update();
        Discord::Update();
        Sound::Update(s_camera.position, s_camera.GetForward());
        Networking::Update();

        float dt = Time::DeltaTime();

        MainMenu::Update(s_input, dt);

        if (!MainMenu::IsActive())
        {
            Particles::Update(dt);
            DynamicLights::Update();
            Videos::Update(dt);
            Shake::Update(dt);
            Fade::Update(dt);
            ScreenText::Update(dt);
            LightStyles::Update(Time::TotalTime());
            Sprites::Update();
            Beams::Update();
            Cables::Update();
            Physics::Update(dt);
            EntityManager::UpdateAll(dt);
        }

        // Input
        s_input.BeginFrame();
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT)
            {
                s_running = false;
                continue;
            }

            if (e.type == SDL_EVENT_WINDOW_RESIZED)
            {
                s_renderer.OnWindowResize(e.window.data1, e.window.data2);
                continue;
            }

            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_GRAVE && !e.key.repeat)
            {
                Console::Toggle();
                if (Console::IsOpen())
                {
                    s_input.ClearStates();
                }
                continue;
            }

            if (Console::IsOpen())
            {
                if (Console::HandleEvent(e)) 
                {
                    continue;
                }
            }

            s_input.ProcessEvent(e);

            if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST)
            {
                SDL_SetWindowRelativeMouseMode(s_window.Get(), false);
            }

            if (e.type == SDL_EVENT_WINDOW_FOCUS_GAINED)
            {
                if (!MainMenu::IsActive() && Maps::HasMapLoaded())
                {
                    SDL_SetWindowRelativeMouseMode(s_window.Get(), true);
                }
            }
        }

        Binds::Update(s_input);

        // HUD Overlays
        if (CVar::GetInt("cl_showfps") > 0 && s_renderer.GetUI())
        {
            std::string fpsText = "FPS: " + std::to_string(Time::FPS());
            s_renderer.GetUI()->DrawText(fpsText, 10.0f, 10.0f, { 1.0f, 1.0f, 0.0f, 1.0f });
        }

        if (CVar::GetInt("cl_showpos") > 0 && s_renderer.GetUI())
        {
            char buf[128];
            snprintf(buf, sizeof(buf), "pos: %.2f %.2f %.2f  ang: %.2f %.2f", s_camera.position.x, s_camera.position.y, s_camera.position.z, s_camera.pitch, s_camera.yaw);
            s_renderer.GetUI()->DrawText(buf, 10.0f, 30.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
        }

        if (!MainMenu::IsActive() && Maps::HasMapLoaded() && s_renderer.GetUI())
        {
            if (CVar::GetInt("cl_crosshair") > 0)
            {
                int w, h;
                SDL_GetWindowSize(s_window.Get(), &w, &h);
                float cx = (float)w * 0.5f;
                float cy = (float)h * 0.5f;
                s_renderer.GetUI()->DrawCircle(cx - 5, cy - 5, 10, 10, { 1, 1, 1, 0.5f });
            }

            auto ent = EntityManager::FindEntityByClass("info_player_start");
            if (ent)
            {
                auto player = std::dynamic_pointer_cast<Player>(ent);
                std::string healthStr = "Health: " + std::to_string((int)player->GetHealth());
                int w, h;
                SDL_GetWindowSize(s_window.Get(), &w, &h);
                s_renderer.GetUI()->DrawText(healthStr, 20.0f, (float)h - 40.0f, { 1.0f, 0.2f, 0.2f, 1.0f }, 1.5f);
            }
        }

        // Rendering
        s_renderer.Render(s_camera);

        if (s_renderer.GetUI())
        {
            MainMenu::Draw(&s_renderer);
            Console::Draw(&s_renderer);
            ScreenText::Draw(&s_renderer);
        }

        // Framerate Limiter
        int maxFps = CVar::GetInt("r_max_fps");
        if (maxFps > 0) 
        {
            uint64_t targetNS = 1000000000 / maxFps;
            uint64_t elapsed = SDL_GetTicksNS() - frameStart;
            if (elapsed < targetNS) 
            {
                SDL_DelayNS(targetNS - elapsed);
            }
        }
    }

    void Shutdown()
    {
        s_renderer.Shutdown();
        Sound::Shutdown();
        Cubemap::Shutdown();
        DynamicLights::Shutdown();
        Resources::Clear();
        EntityManager::Shutdown();
        Physics::Shutdown();
        CVar::Save();
        Console::Shutdown();
        Discord::Shutdown();
        Networking::Shutdown();
        s_window.Shutdown();
    }

    bool ShouldExit()
    {
        return !s_running;
    }

    void Quit()
    {
        s_running = false;
    }
}

CON_COMMAND(quit, "Quits the engine")
{
    Sys::Quit();
}