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
#include "saveload.h"
#include "entities.h"
#include "filesystem.h"
#include "console.h"
#include "maps.h"
#include "concmd.h"
#include <fstream>

// Save game api was heavily inspired by pathos

namespace Save
{
    void Write(const std::string& name)
    {
        Filesystem::CreateDirectory("saves/");
        std::ofstream file(Filesystem::GetFullPath("saves/" + name + ".sav"), std::ios::binary);

        if (!file.is_open())
        {
            return;
        }

        // Write map name
        std::string mapName = Maps::GetCurrentMapName();
        uint32_t nameLen = (uint32_t)mapName.length();
        file.write((char*)&nameLen, sizeof(uint32_t));
        file.write(mapName.c_str(), nameLen);

        // Write entity count
        auto& entities = EntityManager::GetEntities();
        uint32_t entCount = (uint32_t)entities.size();
        file.write((char*)&entCount, sizeof(uint32_t));

        // Serialize each entity
        for (auto& ent : entities)
        {
            // Class name
            std::string cls = ent->GetClassName();
            uint32_t clsLen = (uint32_t)cls.length();
            file.write((char*)&clsLen, sizeof(uint32_t));
            file.write(cls.c_str(), clsLen);

            // Target name
            std::string targetName = ent->GetTargetName();
            uint32_t targetLen = (uint32_t)targetName.length();
            file.write((char*)&targetLen, sizeof(uint32_t));
            file.write(targetName.c_str(), targetLen);

            // World position
            glm::vec3 origin = ent->GetOrigin();
            file.write((char*)&origin, sizeof(glm::vec3));

            ent->ClearSaveFields();
            ent->OnSave();
            auto& fields = ent->GetSaveFields();
            uint32_t fieldCount = (uint32_t)fields.size();
            file.write((char*)&fieldCount, sizeof(uint32_t));

            // Write each field
            for (auto& f : fields)
            {
                char* dataPtr = (char*)ent.get() + f.offset;
                if (f.type == FieldType::String)
                {
                    std::string* s = (std::string*)dataPtr;
                    uint32_t sLen = (uint32_t)s->length();
                    file.write((char*)&sLen, sizeof(uint32_t));
                    file.write(s->c_str(), sLen);
                }
                else
                {
                    file.write(dataPtr, f.size);
                }
            }
        }

        Console::Log("Saved game: " + name);
    }

    void Read(const std::string& name)
    {
        std::ifstream file(Filesystem::GetFullPath("saves/" + name + ".sav"), std::ios::binary);
        if (!file.is_open())
        {
            return;
        }

        // Read map name
        uint32_t nameLen;
        file.read((char*)&nameLen, sizeof(uint32_t));
        std::string mapName;
        mapName.resize(nameLen);
        file.read(&mapName[0], nameLen);

        // Load map
        Maps::Load(mapName);

        // Read entity count
        uint32_t entCount;
        file.read((char*)&entCount, sizeof(uint32_t));

        // Process each entity
        for (uint32_t i = 0; i < entCount; i++)
        {
            // Class name
            uint32_t clsLen;
            file.read((char*)&clsLen, sizeof(uint32_t));
            std::string cls;
            cls.resize(clsLen);
            file.read(&cls[0], clsLen);

            // Target name
            uint32_t tLen;
            file.read((char*)&tLen, sizeof(uint32_t));
            std::string target;
            target.resize(tLen);
            file.read(&target[0], tLen);

            // Position
            glm::vec3 origin;
            file.read((char*)&origin, sizeof(glm::vec3));

            std::shared_ptr<Entity> ent = nullptr;
            if (!target.empty())
            {
                ent = EntityManager::FindEntityByName(target);
            }
            else if (cls == "info_player_start")
            {
                // Special handle for player
                ent = EntityManager::FindEntityByClass(cls);
            }
            else
            {
                for (auto& e : EntityManager::GetEntities())
                {
                    if (e->GetClassName() == cls && glm::distance(e->GetOrigin(), origin) < 0.1f)
                    {
                        ent = e;
                        break;
                    }
                }
            }

            // Read field count
            uint32_t fieldCount;
            file.read((char*)&fieldCount, sizeof(uint32_t));

            if (ent)
            {
                // Apply base entity state
                ent->SetOrigin(origin);
                ent->ClearSaveFields();
                ent->OnSave();
                auto& fields = ent->GetSaveFields();

                for (uint32_t j = 0; j < fieldCount; j++)
                {
                    if (j >= fields.size()) 
                        break;

                    auto& f = fields[j];
                    char* dataPtr = (char*)ent.get() + f.offset;

                    if (f.type == FieldType::String)
                    {
                        uint32_t sLen;
                        file.read((char*)&sLen, sizeof(uint32_t));
                        std::string* s = (std::string*)dataPtr;
                        s->resize(sLen);
                        file.read(&(*s)[0], sLen);
                    }
                    else
                    {
                        file.read(dataPtr, f.size);
                    }
                }
            }
        }

        Console::Log("Loaded game: " + name);
    }

    CON_COMMAND(save, "Saves the game: save <name>")
    {
        if (args.size() < 2)
        {
            Console::Log("Usage: save <name>");
            return;
        }
        Save::Write(args[1]);
    }

    CON_COMMAND(load, "Loads a game: load <name>")
    {
        if (args.size() < 2)
        {
            Console::Log("Usage: load <name>");
            return;
        }
        Save::Read(args[1]);
    }
}