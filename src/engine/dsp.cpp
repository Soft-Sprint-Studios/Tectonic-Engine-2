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
#include "dsp.h"
#include "filesystem.h"
#include "console.h"
#include <sstream>
#include <vector>

namespace DSP
{
    static std::unordered_map<int, EFXEAXREVERBPROPERTIES> s_registry;

    void Init()
    {
        static const EFXEAXREVERBPROPERTIES defaults[] =
        {
            EFX_REVERB_PRESET_GENERIC,              // 0: Normal (Dry)
            EFX_REVERB_PRESET_GENERIC,              // 1: Generic
            EFX_REVERB_PRESET_FACTORY_SMALLROOM,    // 2: Metal
            EFX_REVERB_PRESET_CITY_SUBWAY,          // 3: Tunnel
            EFX_REVERB_PRESET_STONEROOM,            // 4: Chamber
            EFX_REVERB_PRESET_CAVE,                 // 5: Cave
            EFX_REVERB_PRESET_ROOM,                 // 6: Small Room
            EFX_REVERB_PRESET_AUDITORIUM,           // 7: Large Hall
            EFX_REVERB_PRESET_SEWERPIPE,            // 8: Sewer
            EFX_REVERB_PRESET_HANGAR,               // 9: Hangar
            EFX_REVERB_PRESET_CASTLE_SMALLROOM      // 10: Basement
        };

        for (int i = 0; i < 11; i++)
        {
            s_registry[i] = defaults[i];
        }

        std::string content = Filesystem::ReadText("dsp.def");
        if (content.empty())
        {
            return;
        }

        std::stringstream ss(content);
        std::string token;

        while (ss >> token)
        {
            if (token.front() == '\"')
            {
                int id = std::stoi(token.substr(1, token.size() - 2));
                EFXEAXREVERBPROPERTIES p = EFX_REVERB_PRESET_GENERIC;

                ss >> token; // {

                while (ss >> token && token != "}")
                {
                    if (token == "flDensity")
                    {
                        ss >> p.flDensity;
                    }
                    else if (token == "flDiffusion")
                    {
                        ss >> p.flDiffusion;
                    }
                    else if (token == "flGain")
                    {
                        ss >> p.flGain;
                    }
                    else if (token == "flGainHF")
                    {
                        ss >> p.flGainHF;
                    }
                    else if (token == "flGainLF")
                    {
                        ss >> p.flGainLF;
                    }
                    else if (token == "flDecayTime")
                    {
                        ss >> p.flDecayTime;
                    }
                    else if (token == "flDecayHFRatio")
                    {
                        ss >> p.flDecayHFRatio;
                    }
                    else if (token == "flDecayLFRatio")
                    {
                        ss >> p.flDecayLFRatio;
                    }
                    else if (token == "flReflectionsGain")
                    {
                        ss >> p.flReflectionsGain;
                    }
                    else if (token == "flReflectionsDelay")
                    {
                        ss >> p.flReflectionsDelay;
                    }
                    else if (token == "flReflectionsPan")
                    {
                        ss >> p.flReflectionsPan[0] >> p.flReflectionsPan[1] >> p.flReflectionsPan[2];
                    }
                    else if (token == "flLateReverbGain")
                    {
                        ss >> p.flLateReverbGain;
                    }
                    else if (token == "flLateReverbDelay")
                    {
                        ss >> p.flLateReverbDelay;
                    }
                    else if (token == "flLateReverbPan")
                    {
                        ss >> p.flLateReverbPan[0] >> p.flLateReverbPan[1] >> p.flLateReverbPan[2];
                    }
                    else if (token == "flEchoTime")
                    {
                        ss >> p.flEchoTime;
                    }
                    else if (token == "flEchoDepth")
                    {
                        ss >> p.flEchoDepth;
                    }
                    else if (token == "flModulationTime")
                    {
                        ss >> p.flModulationTime;
                    }
                    else if (token == "flModulationDepth")
                    {
                        ss >> p.flModulationDepth;
                    }
                    else if (token == "flAirAbsorptionGainHF")
                    {
                        ss >> p.flAirAbsorptionGainHF;
                    }
                    else if (token == "flHFReference")
                    {
                        ss >> p.flHFReference;
                    }
                    else if (token == "flLFReference")
                    {
                        ss >> p.flLFReference;
                    }
                    else if (token == "flRoomRolloffFactor")
                    {
                        ss >> p.flRoomRolloffFactor;
                    }
                    else if (token == "iDecayHFLimit")
                    {
                        ss >> p.iDecayHFLimit;
                    }
                }

                s_registry[id] = p;
            }
        }
    }

    const EFXEAXREVERBPROPERTIES& GetPreset(int id)
    {
        auto it = s_registry.find(id);
        if (it != s_registry.end())
        {
            return it->second;
        }
        return s_registry[0];
    }
}