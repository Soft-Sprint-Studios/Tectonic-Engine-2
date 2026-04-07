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
#include "discord.h"
#include "console.h"
#include "cvar.h"
#include "timing.h"
#include <discord_rpc.h>
#include <cstring>

namespace Discord
{
    CVar discord_enabled("discord_enabled", "1", CVAR_SAVE);
    CVar discord_appid("discord_appid", "1491042222966308987", CVAR_SAVE);

    static bool s_initialized = false;
    static int64_t s_startTime = 0;

    void Init()
    {
        if (discord_enabled.GetInt() == 0) 
            return;

        DiscordEventHandlers handlers;
        std::memset(&handlers, 0, sizeof(handlers));

        Discord_Initialize(discord_appid.GetString().c_str(), &handlers, 1, nullptr);
        
        s_initialized = true;
        s_startTime = (int64_t)Time::TotalTime();
        
        // Initial state
        UpdatePresence("Loading Engine...", "");
    }

    void Update()
    {
        if (!s_initialized || discord_enabled.GetInt() == 0) 
            return;

        Discord_RunCallbacks();
    }

    void Shutdown()
    {
        if (!s_initialized) 
            return;

        Discord_Shutdown();
        s_initialized = false;
    }

    void UpdatePresence(const std::string& details, const std::string& state)
    {
        if (!s_initialized || discord_enabled.GetInt() == 0) 
            return;

        DiscordRichPresence discordPresence;
        std::memset(&discordPresence, 0, sizeof(discordPresence));
        
        discordPresence.details = details.c_str();
        discordPresence.state = state.c_str();
        discordPresence.startTimestamp = s_startTime;
        discordPresence.largeImageText = "Tectonic Engine 2";
        
        Discord_UpdatePresence(&discordPresence);
    }
}