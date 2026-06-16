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
#include "ambient_zone.h"
#include "filesystem.h"
#include "console.h"
#include "sound.h"
#include "timing.h"
#include <sstream>
#include <algorithm>
#include <random>
#include <memory>

namespace AmbientZone
{
    static std::unordered_map<std::string, Def> s_definitions;
    static Def s_activeDef;
    static bool s_hasActive = false;

    static std::unique_ptr<Sound::AudioSource> s_loopChannel;
    static const int MAX_SPOTS = 4;
    static std::unique_ptr<Sound::AudioSource> s_spotChannels[MAX_SPOTS];
    static int s_spotIndex = 0;

    static float RandomFloat(float min, float max)
    {
        static std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen);
    }

    void Init()
    {
        s_loopChannel = std::make_unique<Sound::AudioSource>();
        s_loopChannel->SetSpatialized(false);

        for (int i = 0; i < MAX_SPOTS; i++)
        {
            s_spotChannels[i] = std::make_unique<Sound::AudioSource>();
            s_spotChannels[i]->SetSpatialized(true);
        }

        std::string content = Filesystem::ReadText("ambient_zones.def");

        std::stringstream ss(content);
        std::string token;

        while (ss >> token)
        {
            if (token.front() == '\"')
            {
                Def def;
                def.name = token.substr(1, token.size() - 2);
                std::transform(def.name.begin(), def.name.end(), def.name.begin(), ::tolower);

                ss >> token; // {

                while (ss >> token && token != "}")
                {
                    std::string key = token;
                    if (key.front() == '\"' && key.back() == '\"')
                    {
                        key = key.substr(1, key.size() - 2);
                    }

                    if (key == "loop")
                    {
                        ss >> token;
                        def.loopFile = token.substr(1, token.size() - 2);
                    }
                    else if (key == "loop_volume")
                    {
                        ss >> token;
                        def.loopVolume = std::stof(token.substr(1, token.size() - 2));
                    }
                    else if (key == "spot_sound")
                    {
                        SpotSoundRule spot;
                        ss >> token; // {

                        while (ss >> token && token != "}")
                        {
                            std::string subKey = token;
                            if (subKey.front() == '\"' && subKey.back() == '\"')
                            {
                                subKey = subKey.substr(1, subKey.size() - 2);
                            }

                            if (subKey == "file")
                            {
                                ss >> token;
                                spot.soundFile = token.substr(1, token.size() - 2);
                            }
                            else if (subKey == "volume")
                            {
                                std::string minStr, maxStr;
                                ss >> minStr >> maxStr;
                                spot.minVolume = std::stof(minStr.substr(1, minStr.size() - 2));
                                spot.maxVolume = std::stof(maxStr.substr(1, maxStr.size() - 2));
                            }
                            else if (subKey == "pitch")
                            {
                                std::string minStr, maxStr;
                                ss >> minStr >> maxStr;
                                spot.minPitch = std::stof(minStr.substr(1, minStr.size() - 2));
                                spot.maxPitch = std::stof(maxStr.substr(1, maxStr.size() - 2));
                            }
                            else if (subKey == "interval")
                            {
                                std::string minStr, maxStr;
                                ss >> minStr >> maxStr;
                                spot.minInterval = std::stof(minStr.substr(1, minStr.size() - 2));
                                spot.maxInterval = std::stof(maxStr.substr(1, maxStr.size() - 2));
                            }
                        }
                        def.spots.push_back(spot);
                    }
                }
                s_definitions[def.name] = def;
            }
        }
    }

    void Activate(const std::string& name)
    {
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

        if (s_hasActive && s_activeDef.name == lowerName)
        {
            return;
        }

        auto it = s_definitions.find(lowerName);
        if (it == s_definitions.end())
        {
            Console::Warn("Ambient zone definition not found: " + name);
            return;
        }

        s_activeDef = it->second;
        s_hasActive = true;

        if (!s_activeDef.loopFile.empty())
        {
            s_loopChannel->SetVolume(s_activeDef.loopVolume);
            s_loopChannel->Play(s_activeDef.loopFile, true);
        }
        else
        {
            s_loopChannel->Stop();
        }

        float curTime = (float)Time::TotalTime();
        for (auto& s : s_activeDef.spots)
        {
            s.nextPlayTime = curTime + RandomFloat(s.minInterval, s.maxInterval);
        }
    }

    void Update(float dt, const glm::vec3& playerPos)
    {
        if (!s_hasActive)
        {
            return;
        }

        float curTime = (float)Time::TotalTime();

        for (auto& s : s_activeDef.spots)
        {
            if (curTime >= s.nextPlayTime)
            {
                float angle = RandomFloat(0.0f, 3.14159265f * 2.0f);
                float dist = RandomFloat(15.0f, 30.0f);
                glm::vec3 offset = glm::vec3(cos(angle) * dist, RandomFloat(-2.0f, 5.0f), sin(angle) * dist);
                glm::vec3 spotPos = playerPos + offset;

                float vol = RandomFloat(s.minVolume, s.maxVolume);
                float pitch = RandomFloat(s.minPitch, s.maxPitch);

                s_spotChannels[s_spotIndex]->SetPosition(spotPos);
                s_spotChannels[s_spotIndex]->SetVolume(vol);
                s_spotChannels[s_spotIndex]->SetPitch(pitch);
                s_spotChannels[s_spotIndex]->Play(s.soundFile, false);

                s_spotIndex = (s_spotIndex + 1) % MAX_SPOTS;
                s.nextPlayTime = curTime + RandomFloat(s.minInterval, s.maxInterval);
            }
        }
    }

    void Clear()
    {
        if (s_loopChannel)
        {
            s_loopChannel->Stop();
        }
        for (int i = 0; i < MAX_SPOTS; i++)
        {
            if (s_spotChannels[i])
            {
                s_spotChannels[i]->Stop();
            }
        }
        s_hasActive = false;
    }

    void Shutdown()
    {
        Clear();
        s_loopChannel.reset();
        for (int i = 0; i < MAX_SPOTS; i++)
        {
            s_spotChannels[i].reset();
        }
        s_definitions.clear();
    }
}