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
#include "engine_api.h"
#include "materials.h"
#include "resources.h"
#include "filesystem.h"
#include "timing.h"
#include "console.h"
#include "cvar.h"
#include "binds.h"
#include "window.h"
#include "input.h"
#include "renderer.h"
#include "camera.h"
#include "physics.h"
#include "entities.h"
#include "player.h"
#include "maps.h"
#include "networking.h"
#include "sound.h"
#include "discord.h"
#include "sentry.h"
#include "cubemap.h"
#include "particles.h"
#include "dynamic_light.h"
#include "lightstyles.h"
#include "build_date.h"
#include "gamedef.h"
#include "sprite.h"
#include "waters.h"
#include "concmd.h"
#include "main_menu.h"

CVar r_max_fps("r_max_fps", "0", CVAR_SAVE);
CVar r_show_fps("r_show_fps", "0", CVAR_SAVE);

bool running = true;

ENGINE_API int Engine_Main(int argc, char** argv)
{
    // Core Systems Init
    Filesystem::Init();
    Console::Init();
    Gamedef::Init();
    Sentry::Init();
    Discord::Init();
    Cubemap::Init();
    Sound::Init();
    Networking::Init();
    CVar::Init();

    Window window;
    window.Init(Gamedef::GetGameName().c_str(), 1280, 720);

    // Engine Systems Init
    Input input;
    Renderer renderer;
    Camera camera;

    Physics::Init();
    EntityManager::Init();

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

    Console::Log("Tectonic Engine 2 Initialized");
    std::string fullBuildStr = std::string(Build::GetCompileDate()) + " " + std::string(Build::GetCompileTime());
    Console::Log("Build Date: " + fullBuildStr);
    Console::Log("Build Number: " + std::to_string(Build::GetBuildNumber()));

    Maps::Init(&renderer, &camera, &input);

    if (!renderer.Init(window)) 
    {
        Console::Error("Renderer Init Failed!");
        return -1;
    }

    Maps::Load(Gamedef::GetStartingMap());

    while (running)
    {
        // Update Time and Console
        uint64_t frameStart = SDL_GetTicksNS();
        Time::Update();
        Console::Update();
        Discord::Update();
        Sound::Update(camera.position, camera.GetForward());
        Networking::Update();

        float dt = Time::DeltaTime();

        MainMenu::Update(input, dt);

        if (!MainMenu::IsActive())
        {
            Particles::Update(dt);
            DynamicLights::Update();
            LightStyles::Update(Time::TotalTime());
            Sprites::Update();
            Physics::Update(dt);
            EntityManager::UpdateAll(dt);
        }

        // Input Handling
        input.BeginFrame();
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            input.ProcessEvent(e);
            if (e.type == SDL_EVENT_QUIT)
                running = false;

            if (e.type == SDL_EVENT_WINDOW_RESIZED)
            {
                renderer.OnWindowResize(e.window.data1, e.window.data2);
            }
        }

        Binds::Update(input);

        if (r_show_fps.GetInt() > 0 && renderer.GetUI())
        {
            std::string fpsText = "FPS: " + std::to_string(Time::FPS());
            renderer.GetUI()->DrawText(fpsText, 10.0f, 10.0f, { 1.0f, 1.0f, 0.0f, 1.0f });
        }

        // Rendering
        renderer.Render(camera);

        if (renderer.GetUI())
        {
            MainMenu::Draw(&renderer);
        }

        if (r_max_fps.GetInt() > 0) 
        {
            uint64_t targetNS = 1000000000 / r_max_fps.GetInt();
            uint64_t elapsed = SDL_GetTicksNS() - frameStart;
            if (elapsed < targetNS) 
                SDL_DelayNS(targetNS - elapsed);
        }
    }

    // Cleanup
    renderer.Shutdown();
    Sound::Shutdown();
    Cubemap::Shutdown();
    DynamicLights::Shutdown();
    Resources::Clear();
    EntityManager::Shutdown();
    Physics::Shutdown();
    CVar::Save();
    Console::Shutdown();
    Sentry::Shutdown();
    Discord::Shutdown();
    Networking::Shutdown();
    window.Shutdown();

    return 0;
}

CON_COMMAND(quit, "Quits the engine")
{
    running = false;
}

CON_COMMAND(exit, "Quits the engine")
{
    running = false;
}