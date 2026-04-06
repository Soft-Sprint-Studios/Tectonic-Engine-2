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
#include "map_system.h"
#include "renderer.h"
#include "physics.h"
#include "entities.h"
#include "player.h"
#include "concmd.h"
#include "console.h"
#include "filesystem.h"

namespace MapSystem
{
    static Renderer* s_renderer = nullptr;
    static Camera*   s_camera = nullptr;
    static Input*    s_input = nullptr;

    void Init(Renderer* renderer, Camera* camera, Input* input)
    {
        s_renderer = renderer;
        s_camera = camera;
        s_input = input;
    }

    void Load(const std::string& mapName)
    {
        if (!s_renderer) 
            return;

        Console::Log("Changing map to: " + mapName);

        EntityManager::Shutdown();
        Physics::Shutdown();

        Physics::Init();
        EntityManager::Init();

        std::string path = "maps/" + mapName + ".bsp";
        if (s_renderer->LoadMap(path))
        {
            // Link the new player entity
            auto ent = EntityManager::FindEntityByClass("info_player_start");
            if (ent)
            {
                auto player = std::dynamic_pointer_cast<Player>(ent);
                player->LinkCamera(s_camera);
                player->LinkInput(s_input);
            }
        }
    }

    CON_COMMAND(map, "Loads a map: map <mapname>")
    {
        if (args.size() < 2)
        {
            Console::Log("Usage: map <map>");
            return;
        }
        Load(args[1]);
    }

    CON_COMMAND(maps, "Lists all available maps in the maps/ folder")
    {
        auto maps = Filesystem::ListFiles("maps/", ".bsp");
        Console::Log("Available maps:");
        for (const auto& m : maps)
        {
            Console::Log(" - " + m);
        }
    }
}