#include "engine_api.h"
#include "material_manager.h"
#include "resource_manager.h"
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
#include "map_system.h"
#include "networking.h"

ENGINE_API int Engine_Main(int argc, char** argv)
{
    // Core Systems Init
    Filesystem::Init();
    Console::Init();
    Networking::Init();
    CVar::Init();

    Window window;
    if (!window.Init("Tectonic Engine 2", 1280, 720)) {
        return -1;
    }

    // 3. Engine Systems Init
    Input input;
    Renderer renderer;
    Camera camera;

    Physics::Init();
    EntityManager::Init();

    MaterialManager::Init();
    MaterialManager::LoadDefinitions("materials.def");

    Binds::Init();

    MapSystem::Init(&renderer, &camera, &input);

    if (!renderer.Init(window)) 
    {
        Console::Error("Renderer Init Failed!");
        return -1;
    }

    MapSystem::Load("test");

    bool running = true;

    while (running)
    {
        // Update Time and Console
        Time::Update();
        Console::Update();
        Networking::Update();
        Physics::Update(Time::DeltaTime());

        float dt = Time::DeltaTime();

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
    ResourceManager::Clear();
    EntityManager::Shutdown();
    Physics::Shutdown();
    CVar::Save();
    Console::Shutdown();
    Networking::Shutdown();
    window.Shutdown();

    return 0;
}