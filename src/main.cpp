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
    if (!window.Init("Tectonic Engine 2", 1280, 720)) 
    {
        return -1;
    }

    // Engine Systems Init
    Input input;
    Renderer renderer;
    Camera camera;

    Physics::Init();
    EntityManager::Init();

    Materials::Init();
    Materials::LoadDefinitions("materials.def");

    Particles::Init();
    Particles::LoadDefinitions("particles.def");

    DynamicLights::Init();
    LightStyles::Init();
    Sprites::Init();

    Binds::Init();

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

    bool running = true;

    while (running)
    {
        // Update Time and Console
        Time::Update();
        Console::Update();
        Discord::Update();
        Sound::Update(camera.position, camera.GetForward());
        Networking::Update();

        float dt = Time::DeltaTime();

        Particles::Update(dt);
        DynamicLights::Update();
        LightStyles::Update(Time::TotalTime());
        Sprites::Update();
        Physics::Update(dt);

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

        if (input.GetKey(SDL_SCANCODE_ESCAPE))
            running = false;

        EntityManager::UpdateAll(dt);

        Binds::Update(input);

        // Rendering
        renderer.Render(camera);
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