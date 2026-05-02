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
#include "timing.h"
#include <SDL3/SDL.h>
#include <algorithm>

namespace Time
{
    static uint64_t s_startTicks = 0;
    static uint64_t s_lastTicks = 0;
    static float s_deltaTime = 0.0f;
    static float s_totalTime = 0.0f;
    static unsigned int s_fps = 0;
    static int s_frameCount = 0;
    static float s_fpsTimer = 0.0f;

    void Update()
    {
        if (s_startTicks == 0) 
        {
            s_startTicks = SDL_GetTicksNS();
            s_lastTicks = s_startTicks;
        }

        uint64_t currentTicks = SDL_GetTicksNS();
        uint64_t deltaNS = currentTicks - s_lastTicks;
        
        s_lastTicks = currentTicks;

        // Convert nanoseconds to float seconds
        float rawDelta = (float)((double)deltaNS / 1000000000.0);
        s_deltaTime = std::clamp(rawDelta, 0.0001f, 0.066f);
        s_totalTime = (float)((double)(currentTicks - s_startTicks) / 1000000000.0);

        // FPS Calculation
        s_frameCount++;
        s_fpsTimer += s_deltaTime;
        if (s_fpsTimer >= 1.0f) 
        {
            s_fps = s_frameCount;
            s_frameCount = 0;
            s_fpsTimer -= 1.0f;
        }
    }

    float DeltaTime() 
    {
        return s_deltaTime; 
    }

    float TotalTime() 
    {
        return s_totalTime; 
    }

    unsigned int FPS() 
    {
        return s_fps; 
    }
}