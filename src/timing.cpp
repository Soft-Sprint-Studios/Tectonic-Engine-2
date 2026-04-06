#include "timing.h"
#include <SDL3/SDL.h>

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
        s_deltaTime = (float)((double)deltaNS / 1000000000.0);
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